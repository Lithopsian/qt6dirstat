/*
 *   File name: PkgManager.cpp
 *   Summary:   Simple package manager support for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "PkgQuery.h"
#include "DpkgPkgManager.h"
#include "Logger.h"
#include "PacManPkgManager.h"
#include "PkgFileListCache.h"
#include "PkgManager.h"
#include "RpmPkgManager.h"


#define CACHE_SIZE 1000
#define CACHE_COST    1

#define VERBOSE_PKG_QUERY 0


using namespace QDirStat;


namespace
{
    /**
     * Initiate an external process to see if the PkgManager object is a
     * primary package manager.  If the externel process program exists and is
     * executable, then 'pkgManager' is added to the list of package managers.
     * Otherwise it is deleted.
     *
     * The check must be started externally because it calls virtual functions
     * from the base class.
     **/
    void startPkgManagerCheck( PkgManagerList & pkgManagers, PkgManager * pkgManager )
    {
        if ( pkgManager->check() )
            pkgManagers << pkgManager;
        else
            delete pkgManager;
    }


    /**
     * Wait until all the package managers still in the list have finished
     * their checks, or until waitForFinished() times out after 30 seconds.
     **/
    void waitForPkgManagers( const PkgManagerList & pkgManagers )
    {
        for ( PkgManager * pkgManager : pkgManagers )
        {
            if ( !pkgManager->waitForFinished() )
                logWarning() << pkgManager->name() << " check timed out" << Qt::endl;
        }
    }

} // namespace


PkgQuery * PkgQuery::instance()
{
    static PkgQuery _instance;

    return &_instance;
}


PkgQuery::PkgQuery()
{
    _cache.setMaxCost( CACHE_SIZE );

    logInfo() << "Checking available supported package managers..." << Qt::endl;
    startPkgManagerCheck( _pkgManagers, new DpkgPkgManager );
    startPkgManagerCheck( _pkgManagers, new PacManPkgManager );
    startPkgManagerCheck( _pkgManagers, new RpmPkgManager );
}


PkgQuery::~PkgQuery()
{
    qDeleteAll( _pkgManagers );
}


const PkgManager * PkgQuery::getPrimary() const
{
    if ( !_primaryPkgManager )
        waitForPkgManagers( _pkgManagers );

    return _primaryPkgManager;
}


QString PkgQuery::getOwningPackage( const QString & path )
{
    const QString * cachePkg = _cache[ path ];
    if ( cachePkg )
    {
#if VERBOSE_PKG_QUERY
        logDebug() << "Cache: package " << cachePkg << " owns " << path << Qt::endl;
#endif
        return *cachePkg;
    }

    const QString pkg = [ &path ]( const PkgManagerList & pkgManagers )
    {
        for ( const PkgManager * pkgManager : pkgManagers )
        {
            const QString pkg = pkgManager->owningPkg( path );
            if ( !pkg.isEmpty() )
            {
#if VERBOSE_PKG_QUERY
                logDebug() << pkgManager->name() << ": package " << pkg << " owns " << path << Qt::endl;
#endif
                return pkg;
            }
        }

#if VERBOSE_PKG_QUERY
        logDebug() << "No package owns " << path << Qt::endl;
#endif
        return QString{};
    }( _pkgManagers );

    // Insert package name (even if empty) into the cache
    _cache.insert( path, new QString{ pkg }, CACHE_COST );

    return pkg;
}


PkgInfoList PkgQuery::getInstalledPkg() const
{
    PkgInfoList pkgList;

    for ( const PkgManager * pkgManager : _pkgManagers )
        pkgList.append( pkgManager->installedPkg() );

    return pkgList;
}


GlobalFileListCache * PkgQuery::getFileList() const
{
    GlobalFileListCache * fileList = new GlobalFileListCache{};

    for ( const PkgManager * pkgManager : _pkgManagers )
    {
        const PkgFileListCache * pkgFileListCache = pkgManager->createFileListCache();
        fileList->add( *pkgFileListCache );
        delete pkgFileListCache;
    }

    return fileList;
}
