/*
 *   File name: MainWindow.cpp
 *   Summary:	QDirStat main window
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
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
#include "DiscoverActions.h"
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

#define UPDATE_MILLISEC		200

#define USE_CUSTOM_OPEN_DIR_DIALOG 1

using namespace QDirStat;


MainWindow::MainWindow( bool slowUpdate ):
    QMainWindow(),
    _ui { new Ui::MainWindow },
    _sortCol { QDirStat::DataColumns::toViewCol( QDirStat::SizeCol ) },
    _sortOrder { Qt::DescendingOrder }
{
    CHECK_NEW( _ui );
    _ui->setupUi( this );
    _ui->menubar->setCornerWidget( new QLabel( MENUBAR_VERSION ) );

    _historyButtons = new HistoryButtons( _ui->actionGoBack, _ui->actionGoForward );
    CHECK_NEW( _historyButtons );

    _updateTimer.setInterval( UPDATE_MILLISEC );

    // The first call to app() creates the QDirStatApp and with it
    // - the DirTreeModel
    // - the DirTree (owned and managed by the DirTreeModel)
    // - the SelectionModel

    if ( slowUpdate )
        app()->dirTreeModel()->setSlowUpdate();

    _ui->dirTreeView->setModel( app()->dirTreeModel() );
    _ui->dirTreeView->setSelectionModel( app()->selectionModel() );

    _ui->treemapView->setDirTree( app()->dirTree() );
    _ui->treemapView->setSelectionModel( app()->selectionModel() );

    _futureSelection.setTree( app()->dirTree() );
    _futureSelection.setUseRootFallback( false );
    _futureSelection.setUseParentFallback( true );

    ActionManager::setActions( this, app()->selectionModel(), _ui->toolBar, _ui->menuCleanup );

    connectSignals();
    connectMenuActions();               // see MainWindowActions.cpp
    readSettings();

    app()->dirTreeModel()->setBaseFont( _ui->dirTreeView->font() );

    _ui->treemapView->hideTreemap();

#ifdef Q_OS_MACX
    // This makes the application to look more "native" on macOS
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

    // Relying on the QObject hierarchy to properly clean this up resulted in a
    //	segfault; there was probably a problem in the deletion order.

    delete _ui->dirTreeView;
    delete _ui;
    delete _historyButtons;

    // logDebug() << "Main window destroyed" << Qt::endl;
}


void MainWindow::checkPkgManagerSupport()
{
    if ( !PkgQuery::haveGetInstalledPkgSupport() ||
	 !PkgQuery::haveFileListSupport()	    )
    {
	logInfo() << "No package manager support "
		  << "for getting installed packages or file lists"
		  << Qt::endl;

	_ui->actionOpenPkg->setEnabled( false );
    }

    const PkgManager * pkgManager = PkgQuery::primaryPkgManager();
    if ( !pkgManager || !pkgManager->supportsFileListCache() )
    {
	logInfo() << "No package manager support "
		  << "for getting a file lists cache"
		  << Qt::endl;

	_ui->actionOpenUnpkg->setEnabled( false );
    }
}


void MainWindow::connectSignals()
{
    const DirTree           * dirTree           = app()->dirTree();
    const DirTreeModel      * dirTreeModel      = app()->dirTreeModel();
    const SelectionModel    * selectionModel    = app()->selectionModel();
    const CleanupCollection * cleanupCollection = ActionManager::cleanupCollection();

    connect( dirTree,		       &DirTree::startingReading,
	     this,		       &MainWindow::startingReading );

    connect( dirTree,		       &DirTree::startingRefresh,
	     this,		       &MainWindow::busyDisplay );

    connect( dirTree,		       &DirTree::finished,
	     this,		       &MainWindow::readingFinished );

    connect( dirTree,		       &DirTree::aborted,
	     this,		       &MainWindow::readingAborted );

    connect( dirTreeModel,             &DirTreeModel::layoutChanged,
	     this,		       &MainWindow::layoutChanged );

    connect( selectionModel,	       &SelectionModel::currentItemChanged,
	     _historyButtons,	       &HistoryButtons::addToHistory );

    connect( _historyButtons,	       &HistoryButtons::navigateToUrl,
	     this,		       &MainWindow::navigateToUrl );

    connect( selectionModel,	       &SelectionModel::currentItemChanged,
	     _ui->breadcrumbNavigator, &BreadcrumbNavigator::setPath );

    connect( _ui->breadcrumbNavigator, &BreadcrumbNavigator::pathClicked,
	     selectionModel,	       qOverload<const QString &>( &SelectionModel::setCurrentItem ) );

    connect( selectionModel,	       qOverload<>( &SelectionModel::selectionChanged ),
	     this,		       &MainWindow::selectionChanged );

    connect( selectionModel,	       &SelectionModel::currentItemChanged,
	     this,		       &MainWindow::currentItemChanged );

    connect( cleanupCollection,        &CleanupCollection::startingCleanup,
	     this,		       &MainWindow::startingCleanup );

    connect( cleanupCollection,        &CleanupCollection::cleanupFinished,
	     this,		       &MainWindow::cleanupFinished );

    connect( cleanupCollection,	       &CleanupCollection::assumedDeleted,
             _ui->treemapView,	       &TreemapView::enable );

    connect( _ui->treemapView,	       &TreemapView::treemapChanged,
	     this,		       &MainWindow::updateActions );

    connect( _ui->treemapView,	       &TreemapView::hoverEnter,
	     this,		       &MainWindow::showCurrent );

    connect( _ui->treemapView,	       &TreemapView::hoverLeave,
	     this,		       &MainWindow::showSummary );

    connect( &_updateTimer,	       &QTimer::timeout,
	     this,		       &MainWindow::showElapsedTime );

    // Here because DirTreeView doesn't have a selectionModel when it is first constructed
    connect( selectionModel,	       &SelectionModel::currentBranchChanged,
	     _ui->dirTreeView,	       &DirTreeView::closeAllExcept );

}


void MainWindow::updateSettings( bool urlInWindowTitle,
                                 bool useTreemapHover,
                                 int statusBarTimeout,
                                 int longStatusBarTimeout )
{
    _urlInWindowTitle = urlInWindowTitle;
    updateWindowTitle( app()->dirTree()->url() );

    _ui->treemapView->setUseTreemapHover( useTreemapHover );

    _statusBarTimeout = statusBarTimeout;
    _longStatusBarTimeout = longStatusBarTimeout;
}


void MainWindow::readSettings()
{
    QDirStat::Settings settings;
    settings.beginGroup( "MainWindow" );

    restoreState(settings.value("State").toByteArray());

    _statusBarTimeout		= settings.value( "StatusBarTimeoutMillisec",  3000 ).toInt();
    _longStatusBarTimeout	= settings.value( "LongStatusBarTimeout",     30000 ).toInt();

    const bool showTreemap	= settings.value( "ShowTreemap",	      true  ).toBool();
    const bool treemapOnSide	= settings.value( "TreemapOnSide",	      false ).toBool();

    _verboseSelection		= settings.value( "VerboseSelection",	      false ).toBool();
    _urlInWindowTitle		= settings.value( "UrlInWindowTitle",	      false ).toBool();
    const bool useTreemapHover  = settings.value( "UseTreemapHover",	      false ).toBool();

    const QString layoutName	= settings.value( "Layout",		      "L2"  ).toString();
    const bool showMenuBar	= settings.value( "ShowMenuBar",	      true  ).toBool();
    const bool showStatusBar	= settings.value( "ShowStatusBar",	      true  ).toBool();

    _ui->fileDetailsView->setLabelLimit( settings.value( "FileDetailsLabelLimit", 0 ).toInt() );

    settings.endGroup();

    settings.beginGroup( "MainWindow-Subwindows" );
    const QByteArray mainSplitterState = settings.value( "MainSplitter" , QByteArray() ).toByteArray();
    const QByteArray topSplitterState	 = settings.value( "TopSplitter"  , QByteArray() ).toByteArray();
    settings.endGroup();

    _ui->actionShowMenuBar->setChecked  ( showMenuBar   );
    _ui->actionShowStatusBar->setChecked( showStatusBar );
    showBars();

    _ui->treemapView->setUseTreemapHover( useTreemapHover );
    _ui->actionShowTreemap->setChecked( showTreemap );
    _ui->actionTreemapAsSidePanel->setChecked( treemapOnSide );
    treemapAsSidePanel( treemapOnSide );

    _ui->actionVerboseSelection->setChecked( _verboseSelection );
    toggleVerboseSelection( _verboseSelection );

    readWindowSettings( this, "MainWindow" );

    if ( !mainSplitterState.isNull() )
	_ui->mainWinSplitter->restoreState( mainSplitterState );

    if ( topSplitterState.isNull() )
    {
	// No configuration settings for the details panel size
	// The window geometry isn't set yet, so just put in something vaguely workable
	_ui->topViewsSplitter->setStretchFactor( 0, 1 );
	_ui->topViewsSplitter->setStretchFactor( 1, 4 );
//	_ui->fileDetailsPanel->resize( QSize( 300, 300 ) );
    }
    else
    {
	_ui->topViewsSplitter->restoreState( topSplitterState );
    }

    initLayouts( layoutName );
}


void MainWindow::writeSettings()
{
    QDirStat::Settings settings;
    settings.beginGroup( "MainWindow" );

    settings.setValue( "ShowTreemap",      _ui->actionShowTreemap->isChecked() );
    settings.setValue( "TreemapOnSide",    _ui->actionTreemapAsSidePanel->isChecked() );
    settings.setValue( "VerboseSelection", _verboseSelection );

    settings.setValue( "Layout",           currentLayoutName()                   );
    settings.setValue( "ShowMenuBar",      _ui->actionShowMenuBar->isChecked()   );
    settings.setValue( "ShowStatusBar",    _ui->actionShowStatusBar->isChecked() );

    settings.setValue( "StatusBarTimeoutMillisec", _statusBarTimeout );
    settings.setValue( "LongStatusBarTimeout",     _longStatusBarTimeout );
    settings.setValue( "UrlInWindowTitle",         _urlInWindowTitle );
    settings.setValue( "UseTreemapHover",          _ui->treemapView->useTreemapHover() );

    settings.setValue( "FileDetailsLabelLimit",    _ui->fileDetailsView->labelLimit() );

    settings.setValue( "State", saveState() );

    settings.endGroup();

    writeWindowSettings( this, "MainWindow" );

    settings.beginGroup( "MainWindow-Subwindows" );
    settings.setValue( "MainSplitter", _ui->mainWinSplitter->saveState()  );
    settings.setValue( "TopSplitter",  _ui->topViewsSplitter->saveState() );
    settings.endGroup();

    writeLayoutSettings();  // see MainWindowLayout.cpp
}


void MainWindow::showTreemapView( bool show )
{
    if ( !show )
	_ui->treemapView->hideTreemap();
    else if ( !_updateTimer.isActive() ) // don't show from F9 during a read
	_ui->treemapView->showTreemap();
}


void MainWindow::treemapAsSidePanel( bool asSidePanel )
{
    if ( asSidePanel )
	_ui->mainWinSplitter->setOrientation( Qt::Horizontal );
    else
	_ui->mainWinSplitter->setOrientation( Qt::Vertical );
}


void MainWindow::busyDisplay()
{
    //logInfo() << Qt::endl;

    _ui->treemapView->disable();
    updateActions();
    ActionManager::swapActions( _ui->toolBar, _ui->actionRefreshAll, _ui->actionStopReading );

    // If it is open, close the window that lists unreadable directories:
    // With the next directory read, things might have changed; the user may
    // have fixed permissions or ownership of those directories.
    UnreadableDirsWindow::closeSharedInstance();

    if ( _dirPermissionsWarning )
        _dirPermissionsWarning->deleteLater();

    _updateTimer.start();

    // It would be nice to sort by read jobs during reading, but this confuses
    // the hell out of the Qt side of the data model; so let's sort by name
    // instead.  Seems OK in 5.15.
    SignalBlocker signalBlocker( app()->dirTreeModel() );
    const int sortCol = QDirStat::DataColumns::toViewCol( QDirStat::ReadJobsCol );
    _ui->dirTreeView->sortByColumn( sortCol, Qt::DescendingOrder );
}


void MainWindow::idleDisplay()
{
    logInfo() << Qt::endl;

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
//    else if ( app()->selectionModel()->currentBranch() )
//    {
//	logDebug() << "Keep current branch " << app()->selectionModel()->currentBranch() << Qt::endl;
//    }
    else
    {
	logDebug() << "No future selection - expanding tree to level 1" << Qt::endl;
	expandTreeToLevel( 1 );
    }

    updateFileDetailsView();
}


void MainWindow::updateFileDetailsView()
{
    if ( !_ui->fileDetailsView->isVisible() )
	return;

    const FileInfoSet sel = app()->selectionModel()->selectedItems();
    if ( sel.isEmpty() )
	_ui->fileDetailsView->showDetails( app()->currentItem() );
    else if ( sel.count() == 1 )
	_ui->fileDetailsView->showDetails( sel.first() );
    else
	_ui->fileDetailsView->showDetails( sel );
}


void MainWindow::setBreadcrumbsVisible( bool breadcrumbsVisible )
{
    updateLayoutBreadcrumbs( breadcrumbsVisible );
}


void MainWindow::setDetailsPanelVisible( bool detailsPanelVisible )
{
    updateLayoutDetailsPanel( detailsPanelVisible );

    updateFileDetailsView();
}


void MainWindow::startingReading()
{
    _stopWatch.start();
    busyDisplay();

    // Open this here, so it doesn't get done for refresh selected
    // And don't do it for package reads because so many toplevel packages slow things down
    if ( !PkgInfo::isPkgUrl( app()->dirTree()->url() ) )
	// After a short delay so there is a tree to open
	QTimer::singleShot( 0, [this]() { expandTreeToLevel( 1 ); } );
}


namespace
{
    // Verbose logging functions, all callers might be commented out
    [[gnu::unused]] QStringList modelTreeAncestors( const QModelIndex & index )
    {
	QStringList parents;

	for ( QModelIndex parent = index; parent.isValid(); parent = index.model()->parent( parent ) )
	{
	    const QVariant data = index.model()->data( parent, 0 );
	    if ( data.isValid() )
		parents.prepend( data.toString() );
	}

	return parents;
    }


    [[gnu::unused]] void dumpModelTree( const QAbstractItemModel * model,
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


void MainWindow::readingFinished()
{
    idleDisplay();

    const QString elapsedTime = formatMillisec( _stopWatch.elapsed() );
    _ui->statusBar->showMessage( tr( "Finished. Elapsed time: " ) + elapsedTime, _longStatusBarTimeout );
    logInfo() << "Reading finished after " << elapsedTime << Qt::endl;

    if ( app()->root() && app()->root()->errSubDirCount() > 0 )
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


void MainWindow::layoutChanged( const QList<QPersistentModelIndex> &, QAbstractItemModel::LayoutChangeHint changeHint )
{
    if ( changeHint == QAbstractItemModel::VerticalSortHint )
    {
	// Remember this order to restore after the next tree read
	_sortCol = app()->dirTreeModel()->sortColumn();
	_sortOrder = app()->dirTreeModel()->sortOrder();
    }
}


void MainWindow::openUrl( const QString & url )
{
    _historyButtons->clearHistory();

    if ( PkgInfo::isPkgUrl( url ) )
	readPkg( url );
    else if ( isUnpkgUrl( url ) )
	showUnpkgFiles( url );
    else
	openDir( url );
}


void MainWindow::openDir( const QString & url )
{
    _enableDirPermissionsWarning = true;
    try
    {
	app()->dirTreeModel()->openUrl( url );
	const QString & dirTreeUrl = app()->dirTree()->url();
	updateWindowTitle( dirTreeUrl );
        _futureSelection.setUrl( dirTreeUrl );
    }
    catch ( const SysCallFailedException & ex )
    {
	CAUGHT( ex );
        showOpenDirErrorPopup( ex );
	askOpenDir();
    }

    updateActions();
//    expandTreeToLevel( 1 );
}


void MainWindow::showOpenDirErrorPopup( const SysCallFailedException & ex )
{
    updateWindowTitle( "" );
    app()->dirTree()->sendFinished();

    const QString msg = pad( tr( "Could not open directory " ) + ex.resourceName(), 50 );
    QMessageBox errorPopup( QMessageBox::Warning, tr( "Error" ), msg );
    errorPopup.setDetailedText( ex.what() );
    errorPopup.exec();
}


void MainWindow::askOpenDir()
{
    DirTree * tree = app()->dirTree();

    bool crossFilesystems = tree->crossFilesystems();

#if USE_CUSTOM_OPEN_DIR_DIALOG
    const QString path = QDirStat::OpenDirDialog::askOpenDir( this, &crossFilesystems );
#else
    const QString path = QFileDialog::getExistingDirectory( this, tr("Select directory to scan") );
#endif

    if ( !path.isEmpty() )
    {
	tree->reset();
	tree->setCrossFilesystems( crossFilesystems );
	openDir( path );
    }
}


void MainWindow::askOpenPkg()
{
    bool canceled;
    PkgFilter pkgFilter = OpenPkgDialog::askPkgFilter( &canceled );

    if ( !canceled )
    {
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
    BusyPopup msg( tr( "Reading package database..." ), this );

    app()->dirTreeModel()->readPkg( pkgFilter );
    app()->selectionModel()->setCurrentItem( app()->root() );
}


void MainWindow::pkgQuerySetup()
{
    delete _dirPermissionsWarning;
    _ui->breadcrumbNavigator->clear();
    _ui->fileDetailsView->clear();
    app()->dirTreeModel()->clear();
    ActionManager::swapActions( _ui->toolBar, _ui->actionRefreshAll, _ui->actionStopReading );
}


void MainWindow::askFindFiles()
{
    bool canceled;
    const FileSearchFilter filter = FindFilesDialog::askFindFiles( &canceled );

    if ( !canceled )
	DiscoverActions::findFiles( filter );
}


void MainWindow::setFutureSelection()
{
    const FileInfoSet sel = app()->selectionModel()->selectedItems();
    _futureSelection.set( sel.isEmpty() ? app()->selectionModel()->currentItem() : sel.first() );
}


void MainWindow::refreshAll()
{
    _enableDirPermissionsWarning = true;
    setFutureSelection();
    _ui->treemapView->saveTreemapRoot();

    const QString url = app()->dirTree()->url();
    if ( url.isEmpty() )
    {
	askOpenDir();
    }
    else
    {
	//logDebug() << "Refreshing " << url << Qt::endl;

	if ( PkgInfo::isPkgUrl( url ) )
	    readPkg( url );
	else
	    app()->dirTreeModel()->openUrl( url );

        // No need to check if the URL is an unpkg:/ URL:
        //
        // In that case, the previous filters are still set, and just reading
        // the dir tree again from disk with openUrl() will filter out the
        // unwanted packaged files, ignored extensions and excluded directories
        // again.

	updateActions();
    }
}


void MainWindow::refreshSelected()
{
    // logDebug() << "Setting future selection: " << _futureSelection.subtree() << Qt::endl;
    setFutureSelection();
    _ui->treemapView->saveTreemapRoot();
    busyDisplay();

    FileInfo * sel = app()->selectionModel()->selectedItems().first();
    while ( sel && ( !sel->isDir() || sel->isPseudoDir() ) && sel->parent() )
	sel = sel->parent();

    if ( sel && sel->isDirInfo() )
    {
	// logDebug() << "Refreshing " << sel << Qt::endl;
	app()->dirTreeModel()->busyDisplay();

	FileInfoSet refreshSet;
	refreshSet << sel;
	app()->selectionModel()->prepareRefresh( refreshSet );

	app()->dirTree()->refresh( sel->toDirInfo() );
    }
    else
    {
	logWarning() << "NOT refreshing " << sel << Qt::endl;
    }

    updateActions();
}


void MainWindow::applyFutureSelection()
{
    FileInfo * sel = _futureSelection.subtree();
//    DirInfo  * branch = _futureSelection.dir();
    _futureSelection.clear();

    if ( sel )
    {
	//logDebug() << "Using future selection: " << sel << Qt::endl;

	// The previous branch was actually the same, but it is gone now, so force it to have changed
//        app()->selectionModel()->setCurrentBranch( nullptr, sel );
/*
        if ( branch )
            app()->selectionModel()->setCurrentBranch( nullptr, branch );
*/
        app()->selectionModel()->setCurrentItem( sel,
                                                 true);  // select

        if ( sel->isMountPoint() || sel->isDirInfo() ) // || app()->dirTree()->isToplevel( sel ) )
            _ui->dirTreeView->setExpanded( sel, true );

	_ui->dirTreeView->scrollToCurrent(); // center the selected item
    }
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
    app()->dirTreeModel()->clear();
    _historyButtons->clearHistory();

    if ( cacheFileName.isEmpty() )
	return;

    if ( !app()->dirTree()->readCache( cacheFileName ) )
    {
	idleDisplay();
	const QString msg = pad( tr( "Can't read cache file " ) + cacheFileName, 50 );
	QMessageBox::warning( this, tr( "Error" ), msg );
    }
}


void MainWindow::askReadCache()
{
    const QString fileName = QFileDialog::getOpenFileName( this, // parent
							   tr( "Select QDirStat cache file" ),
							   DEFAULT_CACHE_NAME );
    if ( !fileName.isEmpty() )
	readCache( fileName );

    updateActions();
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
    {
	showProgress( tr( "Directory tree written to file " ) + fileName );
    }
    else
    {
	QMessageBox::warning( this,
			      tr( "Error" ), // Title
			      tr( "ERROR writing cache file " ) + fileName );
    }
}


void MainWindow::updateWindowTitle( const QString & url )
{
    QString windowTitle = "Qt6DirStat";

    if ( SysUtil::runningAsRoot() )
	windowTitle += tr( " [root]" );

    if ( _urlInWindowTitle )
	windowTitle += " " + url;

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
    FileInfoSet sel = app()->selectionModel()->selectedItems();
    const int count = sel.size();

    if ( count <= 1 )
    {
	showCurrent( app()->currentItem() );
    }
    else
    {
	sel = sel.normalized();

	_ui->statusBar->showMessage( tr( "%1 items selected (%2 total)" )
				     .arg( count )
				     .arg( formatSize( sel.totalSize() ) ) );
    }
}


void MainWindow::startingCleanup( const QString & cleanupName )
{
    // Notice that this is not called for actions that are not owned by the
    // CleanupCollection such as _ui->actionMoveToTrash().

    setFutureSelection();

    showProgress( tr( "Starting cleanup action " ) + cleanupName );
}


void MainWindow::cleanupFinished( int errorCount )
{
    // Notice that this is not called for actions that are not owned by the
    // CleanupCollection such as _ui->actionMoveToTrash().

    //logDebug() << "Error count: " << errorCount << Qt::endl;

    if ( errorCount == 0 )
	showProgress( tr( "Cleanup action finished successfully" ) );
    else
	showProgress( tr( "Cleanup action finished with %1 errors" ).arg( errorCount ) );
}


void MainWindow::copyCurrentPathToClipboard()
{
    const FileInfo * currentItem = app()->currentItem();
    if ( currentItem )
    {
	QClipboard * clipboard = QApplication::clipboard();
	const QString path = currentItem->path();
	clipboard->setText( path );
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
        {
            // Close and re-open the parent to enforce a screen update:
            // Sometimes the bold font is not taken into account when moving
            // upwards, and so every column is cut off (probably a Qt bug)
            _ui->dirTreeView->setExpanded( parent, false );

            app()->selectionModel()->setCurrentItem( parent,
                                                     true ); // select

            // Re-open the parent
            _ui->dirTreeView->setExpanded( parent, true );
        }
    }
}


void MainWindow::navigateToToplevel()
{
    FileInfo * toplevel = app()->root();
    if ( toplevel )
    {
	expandTreeToLevel( 1 );
	app()->selectionModel()->setCurrentItem( toplevel,
                                                 true ); // select
    }
}


void MainWindow::navigateToUrl( const QString & url )
{
    // logDebug() << "Navigating to " << url << Qt::endl;

    if ( url.isEmpty() )
	return;

    FileInfo * sel = app()->dirTree()->locate( url,
					       true ); // findPseudoDirs
    if ( sel )
    {
	app()->selectionModel()->setCurrentItem( sel,
						 true ); // select
	_ui->dirTreeView->setExpanded( sel, true );
    }
}


void MainWindow::moveToTrash()
{
    // Note that _ui->actionMoveToTrash() is not actually a subclass of Cleanup

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
    FileTypeStatsWindow::populateSharedInstance( this, app()->selectedDirInfoOrRoot(), app()->selectionModel() );
}


void MainWindow::showFileSizeStats()
{
    FileSizeStatsWindow::populateSharedInstance( this, app()->selectedDirInfoOrRoot() );
}


void MainWindow::showFileAgeStats()
{
    FileAgeStatsWindow::populateSharedInstance( this, app()->selectedDirInfoOrRoot(), app()->selectionModel() );
}


void MainWindow::showFilesystems()
{
    FilesystemsWindow::populateSharedInstance( this );
}


void MainWindow::readFilesystem( const QString & path )
{
     app()->dirTree()->reset();
     openDir( path );
}


void MainWindow::showDirPermissionsWarning()
{
//    if ( _dirPermissionsWarning || !_enableDirPermissionsWarning )
	return; // never display, I know already

    _dirPermissionsWarning = PanelMessage::showPermissionsMsg( this, _ui->vBox );

    connect( _dirPermissionsWarning->detailsLinkLabel(), &QLabel::linkActivated,
	     this,                                       &MainWindow::showUnreadableDirs );

    _enableDirPermissionsWarning = false;
}


void MainWindow::showUnreadableDirs()
{
    UnreadableDirsWindow::populateSharedInstance( app()->root() );
}

#if 0
void MainWindow::itemClicked( const QModelIndex & index )
{
    if ( !_verboseSelection )
	return;

    if ( index.isValid() )
    {
	const FileInfo * item = static_cast<const FileInfo *>( index.internalPointer() );

	logDebug() << "Clicked row " << index.row()
		   << " col " << index.column()
		   << " (" << QDirStat::DataColumns::fromViewCol( index.column() ) << ")"
		   << "\t" << item
		   << Qt::endl;
	// << " data(0): " << index.model()->data( index, 0 ).toString()
	logDebug() << "Ancestors: " << modelTreeAncestors( index ).join( " -> " ) << Qt::endl;
    }
    else
    {
	logDebug() << "Invalid model index" << Qt::endl;
    }

    Debug::dumpPersistentIndexList( app()->dirTreeModel()->persistentIndexList() );
}
#endif

void MainWindow::selectionChanged()
{
    showSummary();
    updateFileDetailsView();

    if ( _verboseSelection )
    {
	logNewline();
	app()->selectionModel()->dumpSelectedItems();
    }

    updateActions();
}


void MainWindow::currentItemChanged( FileInfo * newCurrent, const FileInfo * oldCurrent )
{
    showSummary();

    if ( !oldCurrent )
	updateFileDetailsView();

    if ( _verboseSelection )
    {
	logDebug() << "new current: " << newCurrent << Qt::endl;
	logDebug() << "old current: " << oldCurrent << Qt::endl;
	app()->selectionModel()->dumpSelectedItems();
    }

    _ui->dirTreeView->setFocus();

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
    QCoreApplication::quit();
}


//---------------------------------------------------------------------------
//			       Debugging Helpers
//---------------------------------------------------------------------------


void MainWindow::toggleVerboseSelection( bool verboseSelection)
{
    // Verbose selection is toggled with Shift-F7
    _verboseSelection = verboseSelection;

    if ( app()->selectionModel() )
	app()->selectionModel()->setVerbose( _verboseSelection );

    logInfo() << "Verbose selection is now " << ( _verboseSelection ? "on" : "off" )
	      << ". Change this with Shift-F7." << Qt::endl;
}


// For more MainWindow:: methods, See also:
//
//   - MainWindowActions.cpp
//   - MainWindowHelp.cpp
//   - MainWindowLayout.cpp
//   - MainWindowUnpkg.cpp

