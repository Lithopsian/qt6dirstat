/*
 *   File name: MainWindowLayout.cpp
 *   Summary:	Unpackaged files view functions in the QDirStat main window
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */

#include "MainWindow.h"
#include "QDirStatApp.h"
#include "OpenUnpkgDialog.h"
#include "DirTree.h"
#include "DirTreeModel.h"
#include "DirTreePatternFilter.h"
#include "DirTreePkgFilter.h"
#include "PkgManager.h"
#include "PkgQuery.h"
#include "ExcludeRules.h"
#include "BusyPopup.h"
#include "Exception.h"
#include "Logger.h"

using namespace QDirStat;


void MainWindow::askOpenUnpkg()
{
    const PkgManager * pkgManager = PkgQuery::primaryPkgManager();

    if ( ! pkgManager )
    {
	logError() << "No supported primary package manager" << Qt::endl;
	return;
    }

    OpenUnpkgDialog dialog( this );

    if ( dialog.exec() == QDialog::Accepted )
	showUnpkgFiles( dialog.values() );
}


void MainWindow::showUnpkgFiles( const QString & url )
{
    showUnpkgFiles( UnpkgSettings( url ) );
}


void MainWindow::showUnpkgFiles( const UnpkgSettings & unpkgSettings )
{
    unpkgSettings.dump();

    _enableDirPermissionsWarning = true;

    const PkgManager * pkgManager = PkgQuery::primaryPkgManager();
    if ( ! pkgManager )
    {
	logError() << "No supported primary package manager" << Qt::endl;
	return;
    }

    pkgQuerySetup();
    BusyPopup msg( tr( "Reading package database..." ), this );

    setUnpkgExcludeRules( unpkgSettings );
    setUnpkgFilters( unpkgSettings, pkgManager );
    setUnpkgCrossFilesystems( unpkgSettings );

    // Start reading the directory
    try
    {
        const QString dir = parseUnpkgStartingDir( unpkgSettings );
        _futureSelection.setUrl( dir );
	app()->dirTreeModel()->openUrl( dir );
	updateWindowTitle( app()->dirTree()->url() );
    }
    catch ( const SysCallFailedException & ex )
    {
	CAUGHT( ex );
        showOpenDirErrorPopup( ex );
    }

    updateActions();
}


void MainWindow::setUnpkgExcludeRules( const UnpkgSettings & unpkgSettings )
{
    // Set up the exclude rules for directories that should be ignored
    ExcludeRules * excludeRules = new ExcludeRules( unpkgSettings.excludeDirs(),
                                                    ExcludeRule::Wildcard,
                                                    true,    // case-sensitive
                                                    true,    // useFullPath
                                                    false ); // checkAnyFileChild
    CHECK_NEW( excludeRules );

    app()->dirTree()->setTmpExcludeRules( excludeRules );
}


void MainWindow::setUnpkgFilters( const UnpkgSettings & unpkgSettings,
                                  const PkgManager    * pkgManager )
{
    // Filter for ignoring all files from all installed packages
    DirTreeFilter * filter = new DirTreePkgFilter( pkgManager );
    CHECK_NEW( filter );

    app()->dirTree()->clearFilters();
    app()->dirTree()->addFilter( filter );

    // Add the filters for each file pattern the user explicitly requested to ignore
    for ( const QString & pattern : unpkgSettings.ignorePatterns() )
	app()->dirTree()->addFilter( DirTreePatternFilter::create( pattern ) );
}


void MainWindow::setUnpkgCrossFilesystems( const UnpkgSettings & unpkgSettings )
{
    app()->dirTree()->setCrossFilesystems( unpkgSettings.crossFilesystems() );
}


QString MainWindow::parseUnpkgStartingDir( const UnpkgSettings & unpkgSettings )
{
    QString dir = unpkgSettings.startingDir();
    dir.replace( QRegularExpression( "^unpkg:/+", QRegularExpression::CaseInsensitiveOption ), "/" );

    if ( dir != unpkgSettings.startingDir() )
	logInfo() << "Parsed starting dir: " << dir << Qt::endl;

    return dir;
}
