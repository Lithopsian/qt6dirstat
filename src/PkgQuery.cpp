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
#include "Settings.h"


#define VERBOSE_PKG_QUERY 0


using namespace QDirStat;


namespace
{
    /**
     * Read the configuration settings for the package managers.
     *
     * This is a singleton object which is not destroyed during the Qt main
     * exec loop, so default values are written immediately to the settings
     * file.  The only way to change the settings is to edit the config file.
     **/
    void readSettings( int & cacheSize, int & pkgListWarningSecs, int & owningPkgTimeoutSecs)
    {
        Settings settings;
        settings.beginGroup( "Pkg" );

        cacheSize            = settings.value( "OwningPkgCacheSize", 1000 ).toInt();
        pkgListWarningSecs   = settings.value( "PkgListWarningSecs",   10 ).toInt();
        owningPkgTimeoutSecs = settings.value( "OwningPkgTimeoutSecs",  5 ).toInt();

        settings.setDefaultValue( "OwningPkgCacheSize",   cacheSize           );
        settings.setDefaultValue( "PkgListWarningSecs",   pkgListWarningSecs   );
        settings.setDefaultValue( "OwningPkgTimeoutSecs", owningPkgTimeoutSecs );

        settings.endGroup();
    }


    /**
     * Initiate an external process to see if the PkgManager object is a
     * primary package manager.  If the external process is started
     * successfully, then 'pkgManager' is added to the list of package
     * managers.  Otherwise it is deleted.
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
            if ( !pkgManager->waitForPrimary() )
                logWarning() << pkgManager->name() << " check timed out or error" << Qt::endl;
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
    int cacheSize;
    readSettings( cacheSize, _pkgListWarningSecs, _owningPkgTimeoutSecs );
    _cache.setMaxCost( cacheSize );

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


const QString * PkgQuery::getOwningPkg( const QString & path )
{
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

    QString * newPkg = new QString{ pkg };
    // Insert package name (even if empty) into the cache
    _cache.insert( path, newPkg );

    return newPkg;
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
        if ( pkgFileListCache )
        {
            fileList->add( *pkgFileListCache );
            delete pkgFileListCache;
        }
    }

    return fileList;
}
