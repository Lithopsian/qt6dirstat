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
     **/
    void startPkgManagerCheck( PkgManagerList & pkgManagers, PkgManager * pkgManager )
    {
        if ( pkgManager->check() )
            pkgManagers << pkgManager;
        else
            delete pkgManager;
    }


    /**
     * Wait for all the package managers still in the checking list have
     * finished their checks.  The list will then be emptied; further calls
     * to this function will return immediately.
     **/
    void waitForPkgManagers( const PkgManagerList & pkgManagers )
    {
        for ( PkgManager * pkgManager : pkgManagers )
        {
            if ( !pkgManager->waitForFinished() )
                logWarning() << pkgManager->name() << " check timed out" << Qt::endl;
        }

#if VERBOSE_PKG_QUERY
        QStringList available;

        for ( const PkgManager * pkgManager : pkgManagers )
            available << pkgManager->name();

        logDebug() << "Found " << available.join( ", "_L1 )  << Qt::endl;
#endif
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
    if ( _cache.contains( path ) )
    {
        const QString * pkg = _cache[ path ];

#if VERBOSE_PKG_QUERY
        logDebug() << "Cache: package " << *pkg << " owns " << path << Qt::endl;
#endif

        return *pkg;
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

    for ( const PkgManager * pkgManager : asConst( _pkgManagers ) )
        pkgList.append( pkgManager->installedPkg() );

    return pkgList;
}


QStringList PkgQuery::getFileList( const PkgInfo * pkg ) const
{
    for ( const PkgManager * pkgManager : asConst( _pkgManagers ) )
    {
        const QStringList fileList = pkgManager->fileList( pkg );
        if ( !fileList.isEmpty() )
            return fileList;
    }

    return QStringList{};
}
