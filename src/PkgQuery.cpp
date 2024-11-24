/*
 *   File name: PkgManager.cpp
 *   Summary:   Simple package manager support for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "PkgQuery.h"
#include "PkgManager.h"
#include "DpkgPkgManager.h"
#include "RpmPkgManager.h"
#include "PacManPkgManager.h"
#include "Logger.h"


#define CACHE_SIZE 1000
#define CACHE_COST    1

#define VERBOSE_PKG_QUERY 0


using namespace QDirStat;


namespace
{
    /**
     * Check if 'pkgManager' is available; add it to the 'pkgManagers'
     * list if it is, or delete it if not.
     **/
    void checkPkgManager( PkgManagerList & pkgManagers, const PkgManager * pkgManager )
    {
        if ( pkgManager->isPrimaryPkgManager() )
        {
            // Primaries at the start of the list
            logInfo() << "Found primary package manager " << pkgManager->name() << Qt::endl;
            pkgManagers.prepend( pkgManager );
        }
        else if ( pkgManager->isAvailable() )
        {
            // Secondaries at the end of the list
            logInfo() << "Found secondary package manager " << pkgManager->name() << Qt::endl;
            pkgManagers.append( pkgManager) ;
        }
        else
        {
            // Not in the list at all
            delete pkgManager;
        }
    }


    /**
     * Check which supported package managers are available and add them to
     * the internal list.
     **/
    void checkPkgManagers( PkgManagerList & pkgManagers )
    {
        logInfo() << "Checking available supported package managers..." << Qt::endl;

        checkPkgManager( pkgManagers, new DpkgPkgManager   );
        checkPkgManager( pkgManagers, new RpmPkgManager    );
        checkPkgManager( pkgManagers, new PacManPkgManager );

        // The following is just for logging
        if ( pkgManagers.isEmpty() )
        {
            logInfo() << "No supported package manager found." << Qt::endl;
        }
#if VERBOSE_PKG_QUERY
        else
        {
            QStringList available;

            for ( const PkgManager * pkgManager : asConst( pkgManagers ) )
                available << pkgManager->name();

            logInfo() << "Found " << available.join( ", "_L1 )  << Qt::endl;
        }
#endif
    }

}


PkgQuery * PkgQuery::instance()
{
    static PkgQuery _instance;

    return &_instance;
}


PkgQuery::PkgQuery()
{
    _cache.setMaxCost( CACHE_SIZE );
    checkPkgManagers( _pkgManagers );
}


PkgQuery::~PkgQuery()
{
    qDeleteAll( _pkgManagers );
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

    QString pkg;

    for ( const PkgManager * pkgManager : asConst( _pkgManagers ) )
    {
        pkg = pkgManager->owningPkg( path );
        if ( !pkg.isEmpty() )
        {
#if VERBOSE_PKG_QUERY
            logDebug() << pkgManager->name() << ": package " << pkg << " owns " << path << Qt::endl;
#endif
            break;
        }
    }

#if VERBOSE_PKG_QUERY
    if ( pkg.isEmpty() )
        logDebug() << "No package owns " << path << Qt::endl;
#endif

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


bool PkgQuery::checkGetInstalledPkgSupport() const
{
    for ( const PkgManager * pkgManager : asConst( _pkgManagers ) )
    {
        if ( pkgManager->supportsGetInstalledPkg() )
            return true;
    }

    return false;
}


bool PkgQuery::checkFileListSupport() const
{
    for ( const PkgManager * pkgManager : asConst( _pkgManagers ) )
    {
        if ( pkgManager->supportsFileList() )
            return true;
    }

    return false;
}
