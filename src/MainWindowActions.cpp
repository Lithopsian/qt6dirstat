/*
 *   File name: MainWindowActions.cpp
 *   Summary:   Connecting menu actions in the QDirStat main window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QMouseEvent>

#include "MainWindow.h"
#include "DirTree.h"
#include "DiscoverActions.h"
#include "FileInfo.h"
#include "HistoryButtons.h"
#include "Logger.h"
#include "QDirStatApp.h"
#include "SelectionModel.h"
#include "Version.h"


using namespace QDirStat;


void MainWindow::connectMenuActions()
{
    _ui->actionWhatsNew->setStatusTip( RELEASE_URL ); // defined in Version.h

    // Invisible, not on any menu or toolbar
    addAction( _ui->actionExpandTreeLevel6 );    // Ctrl-6
    addAction( _ui->actionExpandTreeLevel7 );    // Ctrl-7
    addAction( _ui->actionExpandTreeLevel8 );    // Ctrl-8
    addAction( _ui->actionExpandTreeLevel9 );    // Ctrl-9
    addAction( _ui->actionVerboseSelection );    // Shift-F7
    addAction( _ui->actionDumpSelection );       // F7
    connectAction( _ui->actionDumpSelection,      &MainWindow::dumpSelectedItems );

    // CleanupCollection::add() handles the cleanup actions

    // File menu
    connectAction( _ui->actionOpenDir,            &MainWindow::askOpenDir );
    connectAction( _ui->actionOpenPkg,            &MainWindow::askOpenPkg );
    connectAction( _ui->actionOpenUnpkg,          &MainWindow::askOpenUnpkg );
    connectAction( _ui->actionRefreshAll,         &MainWindow::refreshAll );
    connectAction( _ui->actionRefreshSelected,    &MainWindow::refreshSelected );
    connectAction( _ui->actionReadExcluded,       &MainWindow::refreshSelected );
    connectAction( _ui->actionContinueReading,    &MainWindow::refreshSelected );
    connectAction( _ui->actionStopReading,        &MainWindow::stopReading );
    connectAction( _ui->actionAskReadCache,       &MainWindow::askReadCache );
    connectAction( _ui->actionAskWriteCache,      &MainWindow::askWriteCache );
    connectAction( _ui->actionQuit,               &MainWindow::quit );

    // Edit menu
    connectAction( _ui->actionCopyPath,           &MainWindow::copyCurrentPathToClipboard );
    connectAction( _ui->actionMoveToTrash,        &MainWindow::moveToTrash );
    connectAction( _ui->actionFindFiles,          &MainWindow::askFindFiles );
    connectAction( _ui->actionConfigure,          &MainWindow::openConfigDialog );

    // View menu
    connectAction( _ui->actionLayout1,            &MainWindow::changeLayoutSlot );
    connectAction( _ui->actionLayout2,            &MainWindow::changeLayoutSlot );
    connectAction( _ui->actionLayout3,            &MainWindow::changeLayoutSlot );

    // Go menu
    connectAction( _ui->actionGoUp,               &MainWindow::navigateUp );
    connectAction( _ui->actionGoToToplevel,       &MainWindow::navigateToToplevel );

    // Discover menu
    connectAction( _ui->actionFileSizeStats,      &MainWindow::showFileSizeStats );
    connectAction( _ui->actionFileTypeStats,      &MainWindow::showFileTypeStats );
    connectAction( _ui->actionFileAgeStats,       &MainWindow::showFileAgeStats );
    connectAction( _ui->actionShowFilesystems,    &MainWindow::showFilesystems );

    // Help menu
    connectAction( _ui->actionHelp,               &MainWindow::openActionUrl );
    connectAction( _ui->actionAbout,              &MainWindow::showAboutDialog );
    connectAction( _ui->actionAboutQt,            &MainWindow::showAboutQtDialog );
    connectAction( _ui->actionTreemapHelp,        &MainWindow::openActionUrl );
    connectAction( _ui->actionPkgViewHelp,        &MainWindow::openActionUrl );
    connectAction( _ui->actionUnpkgViewHelp,      &MainWindow::openActionUrl );
    connectAction( _ui->actionFileAgeStatsHelp,   &MainWindow::openActionUrl );
    connectAction( _ui->actionWhatsNew,           &MainWindow::openActionUrl );
    connectAction( _ui->actionCantMoveDirToTrash, &MainWindow::openActionUrl );
    connectAction( _ui->actionBtrfsSizeReporting, &MainWindow::openActionUrl );
    connectAction( _ui->actionShadowedByMount,    &MainWindow::openActionUrl );
    connectAction( _ui->actionHeadlessServers,    &MainWindow::openActionUrl );
    connectAction( _ui->actionDonate,             &MainWindow::showDonateDialog );

    // Toggle actions
    connectToggleAction( _ui->actionShowBreadcrumbs,    &MainWindow::updateLayoutBreadcrumbs );
    connectToggleAction( _ui->actionShowDetailsPanel,   &MainWindow::updateLayoutDetailsPanel );
    connectToggleAction( _ui->actionShowDirTree,        &MainWindow::updateLayoutDirTree );
    connectToggleAction( _ui->actionShowTreemap,        &MainWindow::updateLayoutTreemap );
    connectToggleAction( _ui->actionTreemapOnSide,      &MainWindow::treemapAsSidePanel );
    connectToggleAction( _ui->actionDetailsWithTreemap, &MainWindow::detailsWithTreemap );
    connectToggleAction( _ui->actionVerboseSelection,   &MainWindow::toggleVerboseSelection );

    // Treemap actions
    connectTreemapAction( _ui->actionTreemapZoomTo,    &TreemapView::zoomTo );
    connectTreemapAction( _ui->actionTreemapZoomIn,    &TreemapView::zoomIn );
    connectTreemapAction( _ui->actionTreemapZoomOut,   &TreemapView::zoomOut );
    connectTreemapAction( _ui->actionResetTreemapZoom, &TreemapView::resetZoom );

    // Expand tree to level actions
    mapTreeExpandAction( _ui->actionCloseAllTreeLevels, 0 );
    mapTreeExpandAction( _ui->actionExpandTreeLevel1, 1 );
    mapTreeExpandAction( _ui->actionExpandTreeLevel2, 2 );
    mapTreeExpandAction( _ui->actionExpandTreeLevel3, 3 );
    mapTreeExpandAction( _ui->actionExpandTreeLevel4, 4 );
    mapTreeExpandAction( _ui->actionExpandTreeLevel5, 5 );
    mapTreeExpandAction( _ui->actionExpandTreeLevel6, 6 );
    mapTreeExpandAction( _ui->actionExpandTreeLevel7, 7 );
    mapTreeExpandAction( _ui->actionExpandTreeLevel8, 8 );
    mapTreeExpandAction( _ui->actionExpandTreeLevel9, 9 );

    // Connect the (non-class) DiscoverActions functions
    connectFunctorAction( _ui->actionDiscoverLargestFiles,    &DiscoverActions::discoverLargestFiles );
    connectFunctorAction( _ui->actionDiscoverNewestFiles,     &DiscoverActions::discoverNewestFiles );
    connectFunctorAction( _ui->actionDiscoverOldestFiles,     &DiscoverActions::discoverOldestFiles );
    connectFunctorAction( _ui->actionDiscoverHardLinkedFiles, &DiscoverActions::discoverHardLinkedFiles );
    connectFunctorAction( _ui->actionDiscoverBrokenSymLinks,  &DiscoverActions::discoverBrokenSymLinks );
    connectFunctorAction( _ui->actionDiscoverSparseFiles,     &DiscoverActions::discoverSparseFiles );
}


void MainWindow::connectAction( QAction * action, void( MainWindow::*actee )( void ) )
{
    connect( action, &QAction::triggered, this, actee );
}


void MainWindow::connectToggleAction( QAction * action, void( MainWindow::*actee )( bool ) )
{
    connect( action, &QAction::toggled, this, actee );
}


void MainWindow::mapTreeExpandAction( QAction * action, int level )
{
    connect( action, &QAction::triggered,
             this,   [ this, level ]() { expandTreeToLevel( level ); } );
}


void MainWindow::connectTreemapAction( QAction * action, void( TreemapView::*actee )( void ) )
{
    connect( action, &QAction::triggered, _ui->treemapView, actee );
}


void MainWindow::connectFunctorAction( QAction * action, void( *actee )( void ) )
{
    connect( action, &QAction::triggered, actee );
}


void MainWindow::updateActions()
{
    const bool       reading       = app()->dirTree()->isBusy();
    const FileInfo * firstToplevel = app()->firstToplevel();
    const bool       isTree        = firstToplevel && !reading;
    const bool       pkgView       = firstToplevel && firstToplevel->isPkgInfo();

    _ui->actionStopReading->setEnabled  ( reading );
    _ui->actionRefreshAll->setEnabled   ( isTree );
    _ui->actionAskReadCache->setEnabled ( !reading );
    _ui->actionAskWriteCache->setEnabled( isTree && !pkgView );

    const FileInfoSet selectedItems     = app()->selectionModel()->selectedItems();
    const bool        sel               = selectedItems.size() > 0;
    const FileInfo  * first             = sel ? selectedItems.first() : nullptr;
    const bool        selSizeOne        = !reading && selectedItems.size() == 1 && !pkgView;

    _ui->actionRefreshSelected->setEnabled( selSizeOne && !first->isMountPoint() && !first->isExcluded() );
    _ui->actionContinueReading->setEnabled( selSizeOne && first->isMountPoint() );
    _ui->actionReadExcluded->setEnabled   ( selSizeOne && first->isExcluded()   );

    const FileInfo * currentItem       = app()->currentItem();
    const bool       pseudoDirSelected = selectedItems.containsPseudoDir();

    _ui->actionCopyPath->setEnabled   ( isTree && currentItem );
    _ui->actionFindFiles->setEnabled  ( firstToplevel );
    _ui->actionMoveToTrash->setEnabled( !reading && sel && !pseudoDirSelected && !pkgView );

    _ui->actionGoUp->setEnabled        ( currentItem && currentItem->treeLevel() > 1 );
    _ui->actionGoToToplevel->setEnabled( firstToplevel );

    _ui->actionCloseAllTreeLevels->setEnabled( firstToplevel );
    _ui->menuExpandTreeToLevel->setEnabled   ( firstToplevel );

    const bool showingTreemap = _ui->treemapView->isVisible();
//    _ui->actionTreemapAsSidePanel->setEnabled( showingTreemap );
    _ui->actionTreemapZoomTo->setEnabled     ( showingTreemap && _ui->treemapView->canZoomIn() );
    _ui->actionTreemapZoomIn->setEnabled     ( showingTreemap && _ui->treemapView->canZoomIn() );
    _ui->actionTreemapZoomOut->setEnabled    ( showingTreemap && _ui->treemapView->canZoomOut() );
    _ui->actionResetTreemapZoom->setEnabled  ( showingTreemap && _ui->treemapView->canZoomOut() );

    for ( QAction * action : _ui->menuDiscover->actions() )
    {
	if ( action != _ui->actionShowFilesystems )
	    action->setEnabled( isTree );
    }

    _historyButtons->updateActions();
}


void MainWindow::mousePressEvent( QMouseEvent * event )
{
    if ( event )
    {
	// Handle the back / forward buttons on the mouse to act like the
	// history back / forward buttons in the tool bar
	switch ( event->button() )
	{
	    case Qt::BackButton:
		if ( _ui->actionGoBack->isEnabled() )
		    _ui->actionGoBack->trigger();
		break;

	    case Qt::ForwardButton:
		if ( _ui->actionGoForward->isEnabled() )
		    _ui->actionGoForward->trigger();
		break;

	    default:
		QMainWindow::mousePressEvent( event );
		break;
	}
    }
}


// For more MainWindow:: methods, see also:
//
//   - MainWindow.cpp
//   - MainWindowLayout.cpp
//   - MainWindowUnpkg.cpp
