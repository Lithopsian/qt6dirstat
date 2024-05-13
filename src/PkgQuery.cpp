/*
 *   File name: PkgManager.cpp
 *   Summary:   Simple package manager support for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#define LOG_COMMANDS	true	// for SysUtil.h
#define LOG_OUTPUT	false	// for SysUtil.h

#include "PkgQuery.h"
#include "PkgManager.h"
#include "DpkgPkgManager.h"
#include "RpmPkgManager.h"
#include "PacManPkgManager.h"
#include "Logger.h"
#include "Exception.h"

#include "SysUtil.h"


#define CACHE_SIZE		5000
#define CACHE_COST		1

#define VERBOSE_PKG_QUERY	0


using namespace QDirStat;


PkgQuery * PkgQuery::instance()
{
    static PkgQuery _instance;

    return &_instance;
}


PkgQuery::PkgQuery()
{
    _cache.setMaxCost( CACHE_SIZE );
    checkPkgManagers();
}


PkgQuery::~PkgQuery()
{
    qDeleteAll( _pkgManagers );
}


void PkgQuery::checkPkgManagers()
{
    logInfo() << "Checking available supported package managers..." << Qt::endl;

    checkPkgManager( new DpkgPkgManager()   );
    checkPkgManager( new RpmPkgManager()    );
    checkPkgManager( new PacManPkgManager() );

    // The following is just for logging
    if ( _pkgManagers.isEmpty() )
        logInfo() << "No supported package manager found." << Qt::endl;
#if VERBOSE_PKG_QUERY
    else
    {
        QStringList available;

        for ( const PkgManager * pkgManager : _pkgManagers )
            available << pkgManager->name();

        logInfo() << "Found " << available.join( QLatin1String( ", " ) )  << Qt::endl;
    }
#endif
}


void PkgQuery::checkPkgManager( const PkgManager * pkgManager )
{
    CHECK_PTR( pkgManager );

    if ( pkgManager->isPrimaryPkgManager() )
    {
	// Primaries at the start of the list
	logInfo() << "Found primary package manager " << pkgManager->name() << Qt::endl;
	_pkgManagers.prepend( pkgManager );
    }
    else if ( pkgManager->isAvailable() )
    {
	// Secondaries at the end of the list
	logInfo() << "Found secondary package manager " << pkgManager->name() << Qt::endl;
	_pkgManagers.append( pkgManager) ;
    }
    else
    {
	// Not in the list at all
	delete pkgManager;
    }
}


QString PkgQuery::getOwningPackage( const QString & path )
{
    QString pkg = "";
    QString foundBy;

    if ( _cache.contains( path ) )
    {
	foundBy = "Cache";
	pkg     = *( _cache[ path ] );
    }


    if ( foundBy.isEmpty() )
    {
	for ( const PkgManager * pkgManager : _pkgManagers )
	{
	    pkg = pkgManager->owningPkg( path );
	    if ( !pkg.isEmpty() )
	    {
		foundBy = pkgManager->name();
		break;
	    }
	}

	if ( foundBy.isEmpty() )
	    foundBy = "all";

	// Insert package name (even if empty) into the cache
	_cache.insert( path, new QString( pkg ), CACHE_COST );
    }

#if VERBOSE_PKG_QUERY
    if ( pkg.isEmpty() )
	logDebug() << foundBy << ": No package owns " << path << Qt::endl;
    else
	logDebug() << foundBy << ": Package " << pkg << " owns " << path << Qt::endl;
#endif

    return pkg;
}


PkgInfoList PkgQuery::getInstalledPkg() const
{
    PkgInfoList pkgList;

    for ( const PkgManager * pkgManager : _pkgManagers )
        pkgList.append( pkgManager->installedPkg() );

    return pkgList;
}


QStringList PkgQuery::getFileList( const PkgInfo * pkg ) const
{
    for ( const PkgManager * pkgManager : _pkgManagers )
    {
        const QStringList fileList = pkgManager->fileList( pkg );
        if ( ! fileList.isEmpty() )
            return fileList;
    }

    return QStringList();
}


bool PkgQuery::checkGetInstalledPkgSupport() const
{
    for ( const PkgManager * pkgManager : _pkgManagers )
    {
        if ( pkgManager->supportsGetInstalledPkg() )
            return true;
    }

    return false;
}


bool PkgQuery::checkFileListSupport() const
{
    for ( const PkgManager * pkgManager : _pkgManagers )
    {
        if ( pkgManager->supportsFileList() )
            return true;
    }

    return false;
}
