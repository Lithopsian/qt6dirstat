/*
 *   File name: MainWindowLayout.cpp
 *   Summary:   Unpackaged files view functions in the QDirStat main window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QStringBuilder>

#include "MainWindow.h"
#include "OpenUnpkgDialog.h"
#include "BusyPopup.h"
#include "DirTree.h"
#include "DirTreeModel.h"
#include "DirTreeFilter.h"
#include "Exception.h"
#include "ExcludeRules.h"
#include "HistoryButtons.h"
#include "PkgManager.h"
#include "PkgQuery.h"
#include "QDirStatApp.h"


using namespace QDirStat;


void MainWindow::askOpenUnpkg()
{
    const PkgManager * pkgManager = PkgQuery::primaryPkgManager();

    if ( !pkgManager )
    {
	logError() << "No supported primary package manager" << Qt::endl;
	return;
    }

    OpenUnpkgDialog dialog{ this };

    if ( dialog.exec() == QDialog::Accepted )
    {
	_historyButtons->clear();
	showUnpkgFiles( dialog.values() );
    }
}


void MainWindow::showUnpkgFiles( const QString & url )
{
    showUnpkgFiles( UnpkgSettings( url ) );
}


void MainWindow::showUnpkgFiles( const UnpkgSettings & unpkgSettings )
{
    unpkgSettings.dump();

    const PkgManager * pkgManager = PkgQuery::primaryPkgManager();
    if ( !pkgManager )
    {
	logWarning() << "No supported primary package manager" << Qt::endl;
	return;
    }

    pkgQuerySetup();
    BusyPopup msg{ tr( "Reading package database..." ) };

    setUnpkgExcludeRules( unpkgSettings );
    setUnpkgFilters( unpkgSettings, pkgManager );
    setUnpkgCrossFilesystems( unpkgSettings );

    // Start reading the directory
    const QString url = parseUnpkgStartingDir( unpkgSettings );
    _futureSelection.setUrl( url );

    try
    {
	DirTree * tree = app()->dirTree();
	tree->startReading( url );
	updateWindowTitle( tree->url() );
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
    app()->dirTree()->setTmpExcludeRules( new ExcludeRules{ unpkgSettings.excludeDirs(),
                                                            ExcludeRule::Wildcard,
                                                            true,    // case-sensitive
                                                            true,    // useFullPath
                                                            false } ); // checkAnyFileChild
}


void MainWindow::setUnpkgFilters( const UnpkgSettings & unpkgSettings,
                                  const PkgManager    * pkgManager )
{
    // Filter for ignoring all files from all installed packages
    DirTree * tree = app()->dirTree();
    tree->clearFilters();
    tree->addFilter( new DirTreePkgFilter{ pkgManager } );

    // Add the filters for each file pattern the user explicitly requested to ignore
    for ( const QString & pattern : unpkgSettings.ignorePatterns() )
	tree->addFilter( DirTreePatternFilter::create( pattern ) );
}


void MainWindow::setUnpkgCrossFilesystems( const UnpkgSettings & unpkgSettings )
{
    app()->dirTree()->setCrossFilesystems( unpkgSettings.crossFilesystems() );
}


QString MainWindow::parseUnpkgStartingDir( const UnpkgSettings & unpkgSettings )
{
    // Remove any scheme prefix to leave a standard Unix absolute path
    QString dir = unpkgSettings.startingDir();
    const QString unpkgPattern = '^' % unpkgScheme() % "/*"_L1;
    dir.replace( QRegularExpression{ unpkgPattern, QRegularExpression::CaseInsensitiveOption }, "/" );

    return dir;
}
