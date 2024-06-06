/*
 *   File name: MainWindow.cpp
 *   Summary:   QDirStat main window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>

#include "MainWindow.h"
#include "ActionManager.h"
#include "BusyPopup.h"
#include "CleanupCollection.h"
#include "ConfigDialog.h"
#include "DataColumns.h"
#include "DirTree.h"
#include "DirTreeCache.h"
#include "DirTreeModel.h"
#include "Exception.h"
#include "FormatUtil.h"
#include "FileAgeStatsWindow.h"
#include "FileDetailsView.h"
#include "FileSearchFilter.h"
#include "FileSizeStatsWindow.h"
#include "FilesystemsWindow.h"
#include "FileTypeStatsWindow.h"
#include "FindFilesDialog.h"
#include "HistoryButtons.h"
#include "Logger.h"
#include "OpenDirDialog.h"
#include "OpenPkgDialog.h"
#include "PanelMessage.h"
#include "PkgManager.h"
#include "PkgQuery.h"
#include "QDirStatApp.h"
#include "SelectionModel.h"
#include "Settings.h"
#include "SettingsHelpers.h"
#include "SignalBlocker.h"
#include "SysUtil.h"
#include "UnreadableDirsWindow.h"
#include "Version.h"


#define UPDATE_MILLISEC 200

#define USE_CUSTOM_OPEN_DIR_DIALOG 1


using namespace QDirStat;


MainWindow::MainWindow( bool slowUpdate ):
    QMainWindow(),
    _ui { new Ui::MainWindow },
    _sortCol { QDirStat::DataColumns::toViewCol( QDirStat::SizeCol ) },
    _sortOrder { Qt::DescendingOrder }
{
    _ui->setupUi( this );
    _ui->menubar->setCornerWidget( new QLabel( MENUBAR_VERSION ) );

    _historyButtons = new HistoryButtons( _ui->actionGoBack, _ui->actionGoForward, this );

    _updateTimer.setInterval( UPDATE_MILLISEC );

    // The first call to app() creates the QDirStatApp and with it
    // - DirTreeModel
    // - DirTree (owned and managed by DirTreeModel)
    // - SelectionModel
    if ( slowUpdate )
        app()->dirTreeModel()->setSlowUpdate();

    _ui->dirTreeView->setModel( app()->dirTreeModel() );
    _ui->dirTreeView->setSelectionModel( app()->selectionModel() );

    _ui->treemapView->setDirTree( app()->dirTree() );
    _ui->treemapView->setSelectionModel( app()->selectionModel() );

    _futureSelection.setTree( app()->dirTree() );
    _futureSelection.setUseParentFallback( true );

    ActionManager::setActions( this, app()->selectionModel(), _ui->toolBar, _ui->menuCleanup );

    connectSignals();
    connectMenuActions(); // see MainWindowActions.cpp
    readSettings();

    app()->dirTreeModel()->setBaseFont( _ui->dirTreeView->font() );

#ifdef Q_OS_MACX
    // This makes the application look more "native" on macOS
    setUnifiedTitleAndToolBarOnMac( true );
    _ui->toolBar->setMovable( false );
#endif

    checkPkgManagerSupport();

    updateActions();
}


MainWindow::~MainWindow()
{
    // logDebug() << "Destroying main window" << Qt::endl;

    writeSettings();

    // logDebug() << "Main window destroyed" << Qt::endl;
}


void MainWindow::checkPkgManagerSupport()
{
    if ( !PkgQuery::haveGetInstalledPkgSupport() || !PkgQuery::haveFileListSupport() )
    {
	logInfo() << "No package manager support for getting installed packages or file lists" << Qt::endl;

	_ui->actionOpenPkg->setEnabled( false );
    }

    const PkgManager * pkgManager = PkgQuery::primaryPkgManager();
    if ( !pkgManager || !pkgManager->supportsFileListCache() )
    {
	logInfo() << "No package manager support for getting a file lists cache" << Qt::endl;

	_ui->actionOpenUnpkg->setEnabled( false );
    }
}


void MainWindow::connectSignals()
{
    const DirTree           * dirTree           = app()->dirTree();
    const DirTreeModel      * dirTreeModel      = app()->dirTreeModel();
    const SelectionModel    * selectionModel    = app()->selectionModel();
    const CleanupCollection * cleanupCollection = ActionManager::cleanupCollection();

    connect( dirTree,                  &DirTree::startingReading,
	     this,                     &MainWindow::startingReading );

    connect( dirTree,                  &DirTree::startingRefresh,
	     this,                     &MainWindow::busyDisplay );

    connect( dirTree,                  &DirTree::finished,
	     this,                     &MainWindow::readingFinished );

    connect( dirTree,                  &DirTree::aborted,
	     this,                     &MainWindow::readingAborted );

    connect( dirTreeModel,             &DirTreeModel::layoutChanged,
	     this,                     &MainWindow::layoutChanged );

    connect( selectionModel,           &SelectionModel::currentBranchChanged,
	     _ui->dirTreeView,         &DirTreeView::closeAllExcept );

    connect( selectionModel,           &SelectionModel::currentItemChanged,
	     _historyButtons,          &HistoryButtons::addToHistory );

    connect( _historyButtons,          &HistoryButtons::navigateToUrl,
	     this,                     &MainWindow::navigateToUrl );

    connect( selectionModel,           &SelectionModel::currentItemChanged,
	     _ui->breadcrumbNavigator, &BreadcrumbNavigator::setPath );

    connect( _ui->breadcrumbNavigator, &BreadcrumbNavigator::pathClicked,
	     selectionModel,           &SelectionModel::setCurrentItemPath );

    connect( selectionModel,           &SelectionModel::selectionChanged,
	     this,                     &MainWindow::selectionChanged );

    connect( selectionModel,           &SelectionModel::currentItemChanged,
	     this,                     &MainWindow::currentItemChanged );

    connect( cleanupCollection,        &CleanupCollection::startingCleanup,
	     this,                     &MainWindow::startingCleanup );

    connect( cleanupCollection,        &CleanupCollection::cleanupFinished,
	     this,                     &MainWindow::cleanupFinished );

    connect( cleanupCollection,        &CleanupCollection::assumedDeleted,
             this,                     &MainWindow::assumedDeleted );

    connect( _ui->treemapView,         &TreemapView::treemapChanged,
	     this,                     &MainWindow::updateActions );

    connect( _ui->treemapView,         &TreemapView::hoverEnter,
	     this,                     &MainWindow::showCurrent );

    connect( _ui->treemapView,         &TreemapView::hoverLeave,
	     this,                     &MainWindow::showSummary );

    connect( &_updateTimer,            &QTimer::timeout,
	     this,                     &MainWindow::showElapsedTime );
}


void MainWindow::readSettings()
{
    QDirStat::Settings settings;

    settings.beginGroup( "MainWindow" );

    restoreState( settings.value("State").toByteArray());

    _statusBarTimeout           = settings.value( "StatusBarTimeoutMillisec",  3000 ).toInt();
    _longStatusBarTimeout       = settings.value( "LongStatusBarTimeout",     30000 ).toInt();
    _urlInWindowTitle           = settings.value( "UrlInWindowTitle",         false ).toBool();
    const QString layoutName    = settings.value( "Layout",                   "L2"  ).toString();
    _showDirPermissionsMsg      = settings.value( "ShowDirPermissionsMsg",    true  ).toBool();

    _ui->actionVerboseSelection->setChecked  ( settings.value( "VerboseSelection",      false ).toBool() );
    _ui->treemapView->setUseTreemapHover     ( settings.value( "UseTreemapHover",       false ).toBool() );
    _ui->actionShowMenuBar->setChecked       ( settings.value( "ShowMenuBar",           true  ).toBool() );
    _ui->actionShowStatusBar->setChecked     ( settings.value( "ShowStatusBar",         true  ).toBool() );
    _ui->actionDetailsWithTreemap->setChecked( settings.value( "DetailsWithTreemap",    false ).toBool() );
    _ui->fileDetailsView->setLabelLimit      ( settings.value( "FileDetailsLabelLimit", 0     ).toInt()  );

    settings.endGroup();

    settings.beginGroup( "MainWindow-Subwindows" );
    const QByteArray mainSplitterState    = settings.value( "MainSplitter"    , QByteArray() ).toByteArray();
    const QByteArray detailsSplitterState = settings.value( "DetailsSplitter" , QByteArray() ).toByteArray();
    settings.endGroup();

    showBars();

    readWindowSettings( this, "MainWindow" );

    if ( !mainSplitterState.isEmpty() )
	_ui->mainWinSplitter->restoreState( mainSplitterState );
    _ui->actionTreemapOnSide->setChecked( _ui->mainWinSplitter->orientation() == Qt::Horizontal );

    if ( detailsSplitterState.isEmpty() )
    {
	// The window geometry isn't set yet, so just put in something vaguely workable
	_ui->topViewsSplitter->setStretchFactor( 0, 1 );
	_ui->topViewsSplitter->setStretchFactor( 1, 4 );
    }
    else
    {
	_ui->topViewsSplitter->restoreState( detailsSplitterState );
	_ui->bottomViewsSplitter->restoreState( detailsSplitterState );
    }

    initLayouts( layoutName );
}


void MainWindow::writeSettings()
{
    QDirStat::Settings settings;

    settings.beginGroup( "MainWindow" );
    settings.setValue( "VerboseSelection",         verboseSelection()                         );
    settings.setValue( "Layout",                   currentLayoutName()                        );
    settings.setValue( "DetailsWithTreemap",       _ui->actionDetailsWithTreemap->isChecked() );
    settings.setValue( "ShowMenuBar",              _ui->actionShowMenuBar->isChecked()        );
    settings.setValue( "ShowStatusBar",            _ui->actionShowStatusBar->isChecked()      );
    settings.setValue( "ShowDirPermissionsMsg",    _showDirPermissionsMsg                     );
    settings.setValue( "StatusBarTimeoutMillisec", _statusBarTimeout                          );
    settings.setValue( "LongStatusBarTimeout",     _longStatusBarTimeout                      );
    settings.setValue( "UrlInWindowTitle",         _urlInWindowTitle                          );
    settings.setValue( "UseTreemapHover",          _ui->treemapView->useTreemapHover()        );
    settings.setValue( "FileDetailsLabelLimit",    _ui->fileDetailsView->labelLimit()         );
    settings.setValue( "State",                    saveState()                                );
    settings.endGroup();

    writeWindowSettings( this, "MainWindow" );

    const QSplitter * visibleSplitter =
	_ui->actionDetailsWithTreemap->isChecked() ? _ui->bottomViewsSplitter : _ui->topViewsSplitter;
    settings.beginGroup( "MainWindow-Subwindows" );
    settings.setValue( "MainSplitter",    _ui->mainWinSplitter->saveState()  );
    settings.setValue( "DetailsSplitter", visibleSplitter->saveState() );
    settings.endGroup();

    writeLayoutSettings();  // see MainWindowLayout.cpp
}


void MainWindow::setUrlInWindowTitle( bool newValue )
{
    _urlInWindowTitle = newValue;
    updateWindowTitle( app()->dirTree()->url() );
}


void MainWindow::showTreemapView( bool show )
{
    if ( !show )
	_ui->treemapView->hideTreemap();
    else if ( !_updateTimer.isActive() )
	// don't show from F9 during a read, it will appear when the read is complete
	_ui->treemapView->showTreemap();
}


void MainWindow::treemapAsSidePanel( bool asSidePanel )
{
    _ui->mainWinSplitter->setOrientation ( asSidePanel ? Qt::Horizontal : Qt::Vertical );
    _ui->topViewsSplitter->setOrientation( asSidePanel ? Qt::Vertical : Qt::Horizontal );
    _ui->bottomViewsSplitter->setOrientation( asSidePanel ? Qt::Vertical : Qt::Horizontal );
}


void MainWindow::detailsWithTreemap( bool withTreemap )
{
    QScrollArea * oldParent = withTreemap ? _ui->topFileDetailsPanel : _ui->bottomFileDetailsPanel;
    QScrollArea * newParent = withTreemap ? _ui->bottomFileDetailsPanel : _ui->topFileDetailsPanel;

    newParent->setVisible( _ui->actionShowDetailsPanel->isChecked() );
    newParent->setWidget( oldParent->takeWidget() );
    oldParent->hide();
}


void MainWindow::busyDisplay()
{
    _historyButtons->lock();
    _ui->treemapView->disable();
    updateActions();
    ActionManager::swapActions( _ui->toolBar, _ui->actionRefreshAll, _ui->actionStopReading );

    // If it is open, close the window that lists unreadable directories:
    // With the next directory read, things might have changed; the user may
    // have fixed permissions or ownership of those directories.
    closeChildren();

    _updateTimer.start();

    // Sort by ReadJobsCol (aka PercentBarCol) during the read
    SignalBlocker signalBlocker( app()->dirTreeModel() );
    const int sortCol = QDirStat::DataColumns::toViewCol( QDirStat::ReadJobsCol );
    _ui->dirTreeView->sortByColumn( sortCol, Qt::DescendingOrder );
}


void MainWindow::idleDisplay()
{
    // Safe for the treemap to start work now
    _updateTimer.stop();
    showTreemapView( _ui->actionShowTreemap->isChecked() );

    updateActions();
    ActionManager::swapActions( _ui->toolBar, _ui->actionStopReading, _ui->actionRefreshAll );

    // Go back to the sort order before the read
    _ui->dirTreeView->sortByColumn( _sortCol, _sortOrder );

    if ( _futureSelection.subtree() )
    {
	//logDebug() << "Using future selection " << _futureSelection.subtree() << Qt::endl;
        applyFutureSelection();
    }
    else
    {
	//logDebug() << "No future selection - expanding tree to level 1" << Qt::endl;
	expandTreeToLevel( 1 );
    }

    updateFileDetailsView();
    _historyButtons->unlock( app()->selectionModel()->currentItem() );
}


void MainWindow::updateFileDetailsView()
{
    if ( !_ui->fileDetailsView->isVisible() )
	return;

    const FileInfoSet sel = app()->selectionModel()->selectedItems();
    if ( sel.count() > 1 )
	_ui->fileDetailsView->showDetails( sel );
    else if ( !sel.isEmpty() )
	_ui->fileDetailsView->showDetails( sel.first() );
    else
	_ui->fileDetailsView->showDetails( app()->currentItem() );
}


void MainWindow::startingReading()
{
    busyDisplay();

    // Open this here, so it doesn't get done for refresh selected
    // And don't do it for package reads because so many toplevel packages slow things down
    if ( !PkgInfo::isPkgUrl( app()->dirTree()->url() ) )
	// After a short delay so there is a tree to open
	QTimer::singleShot( 0, [ this ]() { expandTreeToLevel( 1 ); } );
}


namespace
{
    [[gnu::unused]] void dumpModelTree(	const QAbstractItemModel * model,
					const QModelIndex        & index,
					const QString            & indent )
    {
	const int rowCount = model->rowCount( index );
	const QVariant data = model->data( index, Qt::DisplayRole );

	if ( !data.isValid() )
	    logDebug() << "<No data> " << rowCount << " rows" << Qt::endl;
	else if ( rowCount > 0 )
	    logDebug() << indent << data.toString() << ": " << rowCount << " rows" << Qt::endl;
	else
	    logDebug() << indent << data.toString() << Qt::endl;

	for ( int row=0; row < rowCount; row++ )
	{
	    const QModelIndex childIndex = model->index( row, 0, index );
	    dumpModelTree( model, childIndex, indent + QString( 4, ' ' ) );
	}
    }

} // namespace


void MainWindow::dumpSelectedItems()
{
    logDebug() << "Current item: " << app()->selectionModel()->currentItem() << Qt::endl;

    const FileInfoSet items = app()->selectionModel()->selectedItems();
    logDebug() << items.size() << " items selected" << Qt::endl;

    for ( const FileInfo * item : items )
	logDebug() << "	 Selected: " << item << Qt::endl;

    logNewline();
}


void MainWindow::readingFinished()
{
    idleDisplay();

    const QString elapsedTime = formatMillisec( _stopWatch.elapsed() );
    _ui->statusBar->showMessage( tr( "Finished. Elapsed time: " ) + elapsedTime, _longStatusBarTimeout );
    logInfo() << "Reading finished after " << elapsedTime << Qt::endl;

    if ( app()->firstToplevel() && app()->firstToplevel()->errSubDirCount() > 0 )
	showDirPermissionsWarning();

    //dumpModelTree( app()->dirTreeModel(), QModelIndex(), "" );
}


void MainWindow::readingAborted()
{
    idleDisplay();

    const QString elapsedTime = formatMillisec( _stopWatch.elapsed() );
    _ui->statusBar->showMessage( tr( "Aborted. Elapsed time: " ) + elapsedTime, _longStatusBarTimeout );
    logInfo() << "Reading aborted after " << elapsedTime << Qt::endl;
}


void MainWindow::layoutChanged( const QList<QPersistentModelIndex> &,
                                QAbstractItemModel::LayoutChangeHint changeHint )
{
    if ( changeHint == QAbstractItemModel::VerticalSortHint )
    {
	_ui->dirTreeView->scrollToCurrent();

	// Remember this order to restore after the next tree read ends
	_sortCol = app()->dirTreeModel()->sortColumn();
	_sortOrder = app()->dirTreeModel()->sortOrder();
    }
}


void MainWindow::openUrl( const QString & url )
{
    _historyButtons->clear();

    if ( PkgInfo::isPkgUrl( url ) )
	readPkg( url );
    else if ( isUnpkgUrl( url ) )
	showUnpkgFiles( url );
    else
	openDir( url );
}


void MainWindow::openDir( const QString & url )
{
    enableDirPermissionsMsg();
    _stopWatch.start();

    try
    {
	app()->dirTree()->clear();
	app()->dirTreeModel()->openUrl( url );
	const QString & dirTreeUrl = app()->dirTree()->url(); // canonical version of url
	updateWindowTitle( dirTreeUrl );
	_futureSelection.setUrl( dirTreeUrl );
    }
    catch ( const SysCallFailedException & ex )
    {
	CAUGHT( ex );
	showOpenDirErrorPopup( ex );
//	askOpenDir();
    }

    updateActions();
}


void MainWindow::showOpenDirErrorPopup( const SysCallFailedException & ex )
{
    _historyButtons->clear();
    app()->selectionModel()->clear();
    _ui->breadcrumbNavigator->clear();
    updateWindowTitle( "" );
    app()->dirTree()->sendFinished();

    QMessageBox errorPopup( QMessageBox::Warning, tr( "Error" ),
                            pad( tr( "Could not open directory " ) + ex.resourceName(), 50 ) );
    errorPopup.setDetailedText( ex.what() );
    errorPopup.exec();
}


void MainWindow::askOpenDir()
{
    DirTree * tree = app()->dirTree();

    bool crossFilesystems = tree->crossFilesystems();

#if USE_CUSTOM_OPEN_DIR_DIALOG
    const QString path = QDirStat::OpenDirDialog::askOpenDir( this, crossFilesystems );
#else
    const QString path = QFileDialog::getExistingDirectory( this, tr("Select directory to scan") );
#endif

    if ( !path.isEmpty() )
    {
	_historyButtons->clear();
	tree->reset();
	tree->setCrossFilesystems( crossFilesystems );
	openDir( path );
    }
}


void MainWindow::askOpenPkg()
{
    bool cancelled;
    PkgFilter pkgFilter = OpenPkgDialog::askPkgFilter( &cancelled );

    if ( !cancelled )
    {
	_historyButtons->clear();
	app()->dirTree()->reset();
	readPkg( pkgFilter );
    }
}


void MainWindow::readPkg( const PkgFilter & pkgFilter )
{
    // logInfo() << "URL: " << pkgFilter.url() << Qt::endl;

    _futureSelection.setUrl( PkgInfo::pkgSummaryUrl() );
    updateWindowTitle( pkgFilter.url() );
    pkgQuerySetup();
    BusyPopup msg( tr( "Reading package database..." ) );

    app()->dirTreeModel()->readPkg( pkgFilter );
    app()->selectionModel()->setCurrentItem( app()->firstToplevel() );
}


void MainWindow::pkgQuerySetup()
{
    closeChildren();
    _stopWatch.start();
    _updateTimer.stop();
    _ui->statusBar->clearMessage();
    _ui->breadcrumbNavigator->clear();
    _ui->fileDetailsView->clear();
    app()->dirTree()->clear();
    ActionManager::swapActions( _ui->toolBar, _ui->actionRefreshAll, _ui->actionStopReading );
    enableDirPermissionsMsg();
}


void MainWindow::askFindFiles()
{
    FindFilesDialog::askFindFiles( this );
}


void MainWindow::setFutureSelection()
{
    const FileInfoSet sel = app()->selectionModel()->selectedItems();
    _futureSelection.set( sel.isEmpty() ? app()->currentItem() : sel.first() );
}


void MainWindow::refreshAll()
{
    const QString url = app()->dirTree()->url(); // don't take reference since it is about to be cleared
    if ( url.isEmpty() )
	// Refresh shouldn't be enabled with no tree, but can't read an empty string
	return;

    enableDirPermissionsMsg();
    setFutureSelection();
    _ui->treemapView->saveTreemapRoot();

    //logDebug() << "Refreshing " << url << Qt::endl;

    if ( PkgInfo::isPkgUrl( url ) )
    {
	readPkg( url );
    }
    else
    {
	_stopWatch.start();

	// This will throw if the url no longer exists or is inaccessible
	try
	{
	    app()->dirTree()->clear();
	    app()->dirTreeModel()->openUrl( url );
	}
	catch ( const SysCallFailedException & ex )
	{
	    CAUGHT( ex );
	    showOpenDirErrorPopup( ex );
//	    askOpenDir();
	}
    }

    // No need to check if the URL is an unpkg:/ URL:
    //
    // In that case, the previous filters are still set, and just reading
    // the dir tree again from disk with openUrl() will filter out the
    // unwanted packaged files, ignored extensions and excluded directories
    // again.

    updateActions();
}


void MainWindow::refreshSelected()
{
    FileInfo * sel = app()->selectionModel()->selectedItems().first();
    while ( sel && !sel->isDir() && sel->parent() != sel->tree()->root() )
	sel = sel->parent();

    // Should always be a selected item if this action is enabled, but ...
    if ( sel )
    {
	// logDebug() << "Refreshing " << sel << Qt::endl;

	enableDirPermissionsMsg();
	setFutureSelection();
	_ui->treemapView->saveTreemapRoot();

	_stopWatch.start();
	FileInfoSet refreshSet;
	refreshSet << sel;

	try
	{
	    sel->tree()->refresh( refreshSet );
	}
	catch ( const SysCallFailedException & ex )
	{
	    CAUGHT( ex );
	    showOpenDirErrorPopup( ex );
	}
    }

    updateActions();
}


void MainWindow::applyFutureSelection()
{
    FileInfo * sel = _futureSelection.subtree();
    if ( sel )
    {
	//logDebug() << "Using future selection: " << sel << Qt::endl;

        app()->selectionModel()->setCurrentItem( sel,
                                                 true);  // select

	// A bit annoying to open every refreshed directory, so just the top level
        if ( sel->parent() == app()->dirTree()->root() )
            _ui->dirTreeView->setExpanded( sel, true );

	_ui->dirTreeView->scrollToCurrent(); // center the selected item
    }

    _futureSelection.clear();
}


void MainWindow::stopReading()
{
    if ( app()->dirTree()->isBusy() )
    {
	app()->dirTree()->abortReading();
	_ui->statusBar->showMessage( tr( "Reading aborted." ), _longStatusBarTimeout );
    }
}


void MainWindow::readCache( const QString & cacheFileName )
{
    if ( cacheFileName.isEmpty() )
	return;

    _stopWatch.start();

    if ( !app()->dirTree()->readCache( cacheFileName ) )
    {
	idleDisplay();
	QMessageBox::warning( this, tr( "Error" ), tr( "Can't read cache file " ) + cacheFileName );
    }

    updateActions();
}


void MainWindow::askReadCache()
{
    const QString fileName = QFileDialog::getOpenFileName( this, // parent
							   tr( "Select QDirStat cache file" ),
							   DEFAULT_CACHE_NAME );
    if ( fileName.isEmpty() )
	return;

    app()->dirTree()->clear();
    app()->dirTree()->reset();
    _historyButtons->clear();
    _ui->breadcrumbNavigator->clear();
    readCache( fileName );
}


void MainWindow::askWriteCache()
{
    const QString fileName = QFileDialog::getSaveFileName( this, // parent
							   tr( "Enter name for QDirStat cache file"),
							   DEFAULT_CACHE_NAME );
    if ( fileName.isEmpty() )
	return;

    const bool ok = app()->dirTree()->writeCache( fileName );
    if ( ok )
	showProgress( tr( "Directory tree written to file " ) + fileName );
    else
	QMessageBox::warning( this, tr( "Error" ), tr( "ERROR writing cache file " ) + fileName );
}


void MainWindow::updateWindowTitle( const QString & url )
{
    QString windowTitle = "Qt6DirStat";

    if ( SysUtil::runningAsRoot() )
	windowTitle += " [root]";

    if ( _urlInWindowTitle )
	windowTitle += ' ' + url;

    setWindowTitle( windowTitle );
}


void MainWindow::showProgress( const QString & text )
{
    _ui->statusBar->showMessage( text, _statusBarTimeout );
}


void MainWindow::showElapsedTime()
{
    showProgress( tr( "Reading... " ) + formatMillisec( _stopWatch.elapsed() ) );
}


void MainWindow::showCurrent( FileInfo * item )
{
    if ( item )
    {
	QString msg = QString( "%1  (%2%3)" )
			.arg( item->debugUrl() )
			.arg( item->sizePrefix() )
			.arg( formatSize( item->totalSize() ) );

	if ( item->readState() == DirPermissionDenied || item->readState() == DirError )
	    msg += "  " + _ui->fileDetailsView->readStateMsg( item->readState() );

	_ui->statusBar->showMessage( msg );
    }
    else
    {
	_ui->statusBar->clearMessage();
    }
}


void MainWindow::showSummary()
{
    const FileInfoSet sel = app()->selectionModel()->selectedItems();
    const int count = sel.size();

    if ( count > 1 )
    {
	_ui->statusBar->showMessage( tr( "%1 items selected (%2 total)" )
	                             .arg( count )
				     .arg( formatSize( sel.normalized().totalSize() ) ) );
    }
    else
    {
	showCurrent( app()->currentItem() );
    }
}


void MainWindow::startingCleanup( const QString & cleanupName )
{
    // Note that this is not called for actions that are not owned by the
    // CleanupCollection such as _ui->actionMoveToTrash().

    _historyButtons->lock();
    setFutureSelection();

    showProgress( tr( "Starting cleanup action " ) + cleanupName );
}


void MainWindow::cleanupFinished( int errorCount )
{
    // Note that this is not called for actions that are not owned by the
    // CleanupCollection such as _ui->actionMoveToTrash().

    //logDebug() << "Error count: " << errorCount << Qt::endl;

    const QString msg = [ errorCount ]()
    {
	if ( errorCount == 0 )
	    return tr( "Cleanup action finished successfully" );

	if ( errorCount == 1 )
	    return tr( "Cleanup action finished with 1 error" );

	return tr( "Cleanup action finished with %1 errors" ).arg( errorCount );
    }();

    showProgress( msg );
}


void MainWindow::assumedDeleted()
{
    if ( app()->firstToplevel() )
    {
	// There won't be a selected item now, so select the current item and bring it into view
	FileInfo * currentItem = app()->currentItem();
	app()->selectionModel()->setCurrentItem( currentItem ? currentItem : app()->firstToplevel(), true );
	_ui->dirTreeView->scrollToCurrent();
	_historyButtons->unlock( currentItem );
    }
    else
    {
	// Special case of the whole tree being deleted, no selection to trigger updateActions
	_historyButtons->clear();
	updateActions();
    }

    if ( !app()->dirTree()->isBusy() )
	_ui->treemapView->enable();
}


void MainWindow::copyCurrentPathToClipboard()
{
    const FileInfo * currentItem = app()->currentItem();
    if ( currentItem )
    {
	const QString path = currentItem->path();
	QApplication::clipboard()->setText( path );
	showProgress( tr( "Copied to system clipboard: " ) + path );
    }
}


void MainWindow::expandTreeToLevel( int level )
{
    //logDebug() << "Expanding tree to level " << level << Qt::endl;

    if ( level < 1 )
	_ui->dirTreeView->collapseAll();
    else
	_ui->dirTreeView->expandToDepth( level - 1 );
}


void MainWindow::navigateUp()
{
    const FileInfo * currentItem = app()->currentItem();
    if ( currentItem )
    {
	FileInfo * parent = currentItem->parent();
	if ( parent && parent != app()->dirTree()->root() )
	    app()->selectionModel()->setCurrentItem( parent, true ); // select
    }
}


void MainWindow::navigateToToplevel()
{
    FileInfo * toplevel = app()->firstToplevel();
    if ( toplevel )
    {
	expandTreeToLevel( 1 );
	app()->selectionModel()->setCurrentItem( toplevel, true ); // select
    }
}


void MainWindow::navigateToUrl( const QString & url )
{
    // logDebug() << "Navigating to " << url << Qt::endl;

    if ( url.isEmpty() )
	return;

    FileInfo * sel = app()->dirTree()->locate( url, true ); // findPseudoDirs
    if ( sel )
    {
	app()->selectionModel()->setCurrentItem( sel, true ); // select
	_ui->dirTreeView->setExpanded( sel, true );
    }
}


void MainWindow::moveToTrash()
{
    // Note that moveToTrash() is not a subclass of Cleanup

    _historyButtons->lock();

    // Save the selection - at least the first selected item
    setFutureSelection();

    ActionManager::moveToTrash();
}


void MainWindow::openConfigDialog()
{
    ConfigDialog::showSharedInstance( this );
}


void MainWindow::showFileTypeStats()
{
    FileTypeStatsWindow::populateSharedInstance( this, app()->currentDirInfo() );
}


void MainWindow::showFileSizeStats()
{
    FileSizeStatsWindow::populateSharedInstance( this, app()->currentDirInfo() );
}


void MainWindow::showFileAgeStats()
{
    FileAgeStatsWindow::populateSharedInstance( this, app()->currentDirInfo() );
}


void MainWindow::showFilesystems()
{
    FilesystemsWindow::populateSharedInstance( this );
}


void MainWindow::readFilesystem( const QString & path )
{
    _historyButtons->clear();
     app()->dirTree()->reset();
     openDir( path );
}


void MainWindow::showDirPermissionsWarning()
{
    if ( !_enableDirPermissionsMsg )
	return;

    PanelMessage::showPermissionsMsg( this, _ui->vBox );

    // Don't display again until there is a manual refresh or new read
    disableDirPermissionsMsg();
}


void MainWindow::showUnreadableDirs()
{
    UnreadableDirsWindow::populateSharedInstance( app()->firstToplevel() );
}


void MainWindow::closeChildren()
{
    UnreadableDirsWindow * unreadableDirsWindow = findChild<UnreadableDirsWindow *>();
    if ( unreadableDirsWindow )
	unreadableDirsWindow->close();

    PanelMessage::deletePermissionsMsg( this );
}


void MainWindow::selectionChanged()
{
    showSummary();
    updateFileDetailsView();

    if ( verboseSelection() )
    {
	logNewline();
	dumpSelectedItems();
    }

    updateActions();
}


void MainWindow::currentItemChanged( FileInfo * newCurrent, const FileInfo * oldCurrent )
{
    showSummary();

    if ( !oldCurrent || !newCurrent )
	updateFileDetailsView();

    if ( verboseSelection() )
    {
	logDebug() << "new current: " << newCurrent << Qt::endl;
	logDebug() << "old current: " << oldCurrent << Qt::endl;
	dumpSelectedItems();
    }

    _ui->dirTreeView->setFocus(); // no point leaving focus on the treemap

    updateActions();
}


void MainWindow::changeEvent( QEvent * event )
{
    if ( event->type() == QEvent::PaletteChange )
	updateFileDetailsView();

    QWidget::changeEvent( event );
}


void MainWindow::quit()
{
    // Stop in-progress reads cleanly
    stopReading();

    QCoreApplication::quit();
}


//---------------------------------------------------------------------------
//			       Debugging Helpers
//---------------------------------------------------------------------------


void MainWindow::toggleVerboseSelection( bool verboseSelection)
{
    // Verbose selection is toggled with Shift-F7
    if ( app()->selectionModel() )
	app()->selectionModel()->setVerbose( verboseSelection );

    logInfo() << "Verbose selection is now " << ( verboseSelection ? "on" : "off" )
	      << ". Change this with Shift-F7." << Qt::endl;
}


// For more MainWindow:: methods, See also:
//
//   - MainWindowActions.cpp
//   - MainWindowHelp.cpp
//   - MainWindowLayout.cpp
//   - MainWindowUnpkg.cpp

