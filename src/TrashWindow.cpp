/*
 *   File name: TrashWindow.cpp
 *   Summary:   QDirStat file type statistics window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <dirent.h> // opendir(), etc
#include <stdio.h> // rename()
#include <sys/stat.h> // struct stat, mkdir(), S_ISDIR, etc.

#include <QAbstractButton>
#include <QDateTime>
#include <QHeaderView>
#include <QMessageBox>
#include <QPointer>
#include <QScrollBar>
#include <QThread>

#include "TrashWindow.h"
#include "ActionManager.h"
#include "BusyPopup.h"
#include "CleanupCollection.h"
#include "DirTreeModel.h"
#include "FormatUtil.h"
#include "Logger.h"
#include "MainWindow.h"
#include "MountPoints.h"
#include "QDirStatApp.h"
#include "ProcessStarter.h"
#include "SelectionModel.h"
#include "Settings.h"
#include "SignalBlocker.h"
#include "SysUtil.h"
#include "Trash.h"


using namespace QDirStat;


namespace
{
    /**
     * Returns an icon to represent the type of item.  These closely match the
     * icons used in the main tree, but are derived here directly from the
     * st_mode.
     **/
    const QIcon & itemTypeIcon( mode_t mode )
    {
	if ( S_ISDIR(  mode ) ) return app()->dirTreeModel()->dirIcon();
	if ( S_ISREG(  mode ) ) return app()->dirTreeModel()->fileIcon();
	if ( S_ISLNK(  mode ) ) return app()->dirTreeModel()->symlinkIcon();
	if ( S_ISBLK(  mode ) ) return app()->dirTreeModel()->blockDeviceIcon();
	if ( S_ISCHR(  mode ) ) return app()->dirTreeModel()->charDeviceIcon();

	return app()->dirTreeModel()->specialIcon();
    }


    /**
     * Returns whether the three lines form a valid .trashinfo file.
     **/
    bool validTrashinfo( const QString & tagLine, const QString & pathLine, const QString & mTimeLine )
    {
	if ( tagLine != TrashDir::trashInfoTag() )
	    return false;

	if ( pathLine.size() <= TrashDir::trashInfoPathTag().size() )
	    return false;

	if ( !pathLine.startsWith( TrashDir::trashInfoPathTag() ) )
	    return false;

	if ( mTimeLine.size() <= TrashDir::trashInfoDateTag().size() )
	    return false;

	if ( !mTimeLine.startsWith( TrashDir::trashInfoDateTag() ) )
	    return false;

	return true;
    }


    /**
     * Return 'mTime' converted to the number of seconds since 1970.  'mTime'
     * is expected to be a string in ISO date format.
     **/
    time_t stringToMTime( const QString & mTime )
    {
#if QT_VERSION < QT_VERSION_CHECK( 5, 8, 0 )
	return QDateTime::fromString( mTime, Qt::ISODate ).toTime_t();
#else
	return QDateTime::fromString( mTime, Qt::ISODate ).toSecsSinceEpoch();
#endif
    }


    /**
     * Return whether 'entryName' is "." or "..".
     **/
    bool isDotOrDotDot( const char * entryName )
    {
	return strcmp( entryName, "." ) == 0 || strcmp( entryName, ".." ) == 0;
    }


    /**
     * One-time initialization of the widgets in this window.
     **/
    void initTree( QTreeWidget * tree )
    {
	QTreeWidgetItem * headerItem = tree->headerItem();
	const auto set = [ headerItem ]( TrashCols col, Qt::Alignment alignment, const QString & title )
	{
	    headerItem->setText( col, title );
	    headerItem->setTextAlignment( col, alignment | Qt::AlignVCenter );
	};

	app()->dirTreeModel()->setTreeIconSize( tree );

	set( TW_NameCol,    Qt::AlignLeft,    QObject::tr( "Name" ) );
	set( TW_SizeCol,    Qt::AlignHCenter, QObject::tr( "Size" ) );
	set( TW_DeletedCol, Qt::AlignHCenter, QObject::tr( "Date Deleted" ) );
	set( TW_DirCol,     Qt::AlignLeft,    QObject::tr( "Original Directory" ) );

	tree->sortByColumn( TW_NameCol, Qt::AscendingOrder );
	tree->setFocus();
    }


    /**
     * Sets the current item in 'treeWidget' to the item at position
     * 'itemIndex'.
     **/
    void setCurrentItem( QTreeWidget * treeWidget, int itemIndex )
    {
	const int topLevelItemCount = treeWidget->topLevelItemCount();
	if ( topLevelItemCount > 0 )
	{
	    const int adjustedItemIndex = qMin( topLevelItemCount - 1, itemIndex );
	    treeWidget->setCurrentItem( treeWidget->topLevelItem( adjustedItemIndex ) );
	}
    }


    /**
     * Return the index of the current widget item in 'treeWidget'.
     **/
    int currentIndex( QTreeWidget * treeWidget )
    {
	const auto selectedItems = treeWidget->selectedItems();
	const auto item = selectedItems.isEmpty() ? treeWidget->currentItem() : selectedItems.first();
	return treeWidget->indexOfTopLevelItem( item );
    }


    /**
     * Ensure that a directory "qexpunged" in 'trashRootPath' exists.  This
     * will be used to quickly move files to be deleted from the "files" and
     * "info" directories.  The "qexpunged" directory can then be deleted
     * later.
     **/
    void ensureExpunged( const char * expungedDirPath )
    {
	if ( mkdir( expungedDirPath, 0700 ) != 0 && errno != EEXIST )
	    logWarning() << "Failed to create 'qexpunged' directory" << ": " << formatErrno() << Qt::endl;
    }


    /**
     * Spawn an external process in the background to recursively remove 'path'
     * and any children.
     **/
    void rmPath( const QString & path )
    {
	QProcess::startDetached( "rm", { "-rf", path } );
    }


    /**
     * Delete 'path', which may be a directory or file.
     **/
    bool deletePath( const QString & path )
    {
	QFileInfo fileInfo{ path };
	if ( fileInfo.isDir() )
	    return QDir{ path }.removeRecursively();
	else if ( fileInfo.exists() )
	    return QFile{ path }.remove();
	else
	    return true;
    }


    /**
     * Delete the 'expungedDirPath' directory.  Qt doesn't always manage to do
     * this, for example with filenames in an unexpected locale.  So spawn a
     * background process to run 'rm -rf' in those cases, which should be more
     * reliable.
     *
     * Note: if 'expungedDirPath' doesn't exist, this operation "succeeds"
     * immediately.
     **/
    void deleteExpunged( const QString & expungedDirPath )
    {
	if ( !deletePath( expungedDirPath ) )
	{
	    logWarning() << "Qt failed to delete 'qexpunged', try to spawn 'rm -rf' process" << Qt::endl;
	    rmPath( expungedDirPath );
	}
    }


    /**
     * Move 'entryName' from 'oldDirPath' to 'expungedDirPath'.  It is expected
     * that 'oldDirPath' and 'expungedDirPath' will be on the same filesystem:
     * normally they will have the same parent. In rare cases where the move
     * fails, try to directly delete 'entryName', which may be a file or
     * directory.
     **/
    bool moveToExpunged( const char * oldDirPath, const char * expungedDirPath, const char * entryName )
    {
	const QByteArray delimiter{ "/" };
	if ( rename( oldDirPath + delimiter + entryName, expungedDirPath + delimiter + entryName ) != 0 )
	{
	    const QString oldFilePath = oldDirPath % QChar{ u'/' } % entryName;
	    logWarning() << "Failed to move " << oldFilePath << " to 'qexpunged': "
	                 << formatErrno() << ". Attempting to delete in place." << Qt::endl;

	    return deletePath( oldFilePath );
	}

	return true;
    }


    /**
     * Move all the files and directories from 'oldDirPath' to
     * 'expungedDirPath'. Both directories are expected to be on the same
     * filesystem, indeed to have the same parent directory.  If there are
     * items to be moved and 'expungedDirPath' does not exist, it is created.
     *
     **/
    void moveAllToExpunged( const QString & oldDirPath, const QString & expungedDirPath )
    {
	const QByteArray oldDirStr = oldDirPath.toUtf8();
	DIR * oldDir = opendir( oldDirStr );
	if ( !oldDir )
	    return;

	const auto readdirEntry = [ oldDir ]()
	{
	    struct dirent * entry = readdir( oldDir );
	    while ( entry && isDotOrDotDot( entry->d_name ) )
		entry = readdir( oldDir );

	    return entry;
	};

	struct dirent * entry = readdirEntry();
	if ( entry )
	{
	    const QByteArray expungedDirStr = expungedDirPath.toUtf8();
	    ensureExpunged( expungedDirStr );

	    do
	    {
		moveToExpunged( oldDirStr, expungedDirStr, entry->d_name );
		entry = readdirEntry();
	    } while ( entry );
	}

	closedir( oldDir );
    }


    /**
     * Returns whether the trash directory, as well as the "files" and "info"
     * directories, and their contents, can be read and modified.
     **/
    bool isTrashAccessible( const QString & trashPath )
    {
	if ( !SysUtil::canAccess( trashPath ) )
	    return false;

	if ( !SysUtil::canAccess( Trash::filesDirPath( trashPath ) ) )
	    return false;

	if ( !SysUtil::canAccess( Trash::infoDirPath( trashPath ) ) )
	    return false;

	return true;
    }


    /**
     * Return a list of all the trash directories found.  This may include the
     * one in the users's home directory and any at the top level of mounted
     * filesystems.  Only valid trash directories in which both the files and
     * info directories exist and are accessible will be returned.
     **/
    QStringList trashRoots()
    {
	QStringList trashRoots;

	const QString homeTrashPath = Trash::homeTrash( QDir::homePath() );
	if ( isTrashAccessible( homeTrashPath ) )
	    trashRoots << homeTrashPath;

	MountPoints::reload();

	for ( MountPointIterator it{ false, true }; *it; ++it )
	{
	    const QString trashRoot = Trash::trashRoot( it->path() == u'/' ? QString{} : it->path() );

	    if ( Trash::isValidMainTrash( trashRoot ) )
	    {
		const QString mainTrashPath = Trash::mainTrashPath( trashRoot );
		if ( isTrashAccessible( mainTrashPath ) )
		    trashRoots << mainTrashPath;
	    }

	    const QString userTrashPath = Trash::userTrashPath( trashRoot );
	    if ( isTrashAccessible( userTrashPath ) )
		trashRoots << userTrashPath;
	}

	return trashRoots;
    }


    /**
     * Add widget items for all entries found in the "files" directory of the
     * trash directory 'dir'.  If 'dir' does not exist or cannot be accessed,
     * this function will silently do nothing.
     **/
    void populateTrashDir( QTreeWidget    * treeWidget,
                           const QString  & trashRoot,
                           ProcessStarter * processStarter )
    {
	if ( trashRoot.isEmpty() )
	    return;

	DIR * diskDir = opendir( Trash::filesDirPath( trashRoot ).toUtf8() );
	if ( !diskDir )
	    return;

	const int filesDirFd = dirfd( diskDir );

	QEventLoop eventLoop;
	int count = 0;

	struct dirent * entry;
	while ( ( entry = readdir( diskDir ) ) )
	{
	    // Give other activity, such as a directory read, chance to make visible progress
	    if ( ++count > 100 )
	    {
		count = 0;
		eventLoop.processEvents( QEventLoop::ExcludeUserInputEvents );
	    }

	    if ( !isDotOrDotDot( entry->d_name ) )
	    {
		TrashItem * item = new TrashItem{ processStarter, trashRoot, filesDirFd, entry->d_name };
		treeWidget->addTopLevelItem( item );
	    }
	}
    }


    /**
     * Try to remove a .trashinfo file.  If the remove failed using Qt, throw
     * off an external process to see if the OS can do any better.  The trash
     * entry has already been processed, so either the .trashinfo file will
     * get removed or it won't.
     **/
    void removeTrashInfoFile( const QString & trashRoot, const QString & entryName)
    {
	const QString trashInfoPath = Trash::trashInfoPath ( trashRoot, entryName );
	if ( !QFile{ trashInfoPath }.remove() )
	{
	    logWarning() << "Qt failed to delete " << trashInfoPath << " - try 'rm -rf'" << Qt::endl;
	    rmPath( trashInfoPath );
	}
    }

} // namespace


TrashWindow::TrashWindow( QWidget * parent ):
    QDialog{ parent },
    _ui{ new Ui::TrashWindow }
{
    // logDebug() << "init" << Qt::endl;

    setAttribute( Qt::WA_DeleteOnClose );

    _ui->setupUi( this );

    initTree( _ui->treeWidget );
    Settings::readWindowSettings( this, "TrashWindow" );
    ActionManager::actionHotkeys( this, "TrashWindow" );

    connect( _ui->treeWidget,    &QTreeWidget::itemSelectionChanged,
             this,               &TrashWindow::enableActions );

    connect( _ui->refreshButton, &QAbstractButton::clicked,
             this,               &TrashWindow::refresh );

    connect( _ui->deleteButton,  &QAbstractButton::clicked,
             this,               &TrashWindow::deleteSelected );

    connect( _ui->restoreButton, &QAbstractButton::clicked,
             this,               &TrashWindow::restoreSelected );

    connect( _ui->emptyButton,   &QAbstractButton::clicked,
             this,               &TrashWindow::empty );

    connect( _ui->treeWidget,    &QTreeWidget::customContextMenuRequested,
             this,               &TrashWindow::contextMenu);

    connect( ActionManager::cleanupCollection(), &CleanupCollection::trashFinished,
             this,                               &TrashWindow::refresh );
}


TrashWindow::~TrashWindow()
{
    // logDebug() << "destroying" << Qt::endl;

    Settings::writeWindowSettings( this, "TrashWindow" );
}


TrashWindow * TrashWindow::sharedInstance()
{
    static QPointer<TrashWindow> _sharedInstance;

    if ( !_sharedInstance )
	_sharedInstance = new TrashWindow{ app()->mainWindow() };

    return _sharedInstance;
}


void TrashWindow::populateTree()
{
    //logDebug() << "Locating Trash items ..." << Qt::endl;

    _ui->treeWidget->setSortingEnabled( false );
    _ui->treeWidget->clear();

    // Use ProcessStarter to limit the number of 'du' processes spawned at once
    ProcessStarter * processStarter = new ProcessStarter{ QThread::idealThreadCount() };
    QObject::connect( processStarter, &ProcessStarter::destroyed,
                      this,           &TrashWindow::calculateTotalSize );

    const QStringList trashRootPaths = trashRoots();
    for ( const QString & trashRootPath : trashRootPaths )
	populateTrashDir( _ui->treeWidget, trashRootPath, processStarter );

    // Tell the ProcessStarter it is allowed to die now
    processStarter->noMoreProcesses();

    _ui->heading->setText( QObject::tr( "Calculating Trash total size..." ) );
    _ui->treeWidget->setSortingEnabled( true );
}


void TrashWindow::calculateTotalSize()
{
    const QString headingText = [ this ]()
    {
	FileSize totalSize = 0;
	for ( QTreeWidgetItemIterator it{ _ui->treeWidget }; *it; ++it )
	    totalSize += static_cast<const TrashItem *>(*it)->totalSize();

	const FileCount items = _ui->treeWidget->topLevelItemCount();
	if ( items == 0 )
	    return QObject::tr( "Trash is empty" );

	const QString itemsCount =
	    items == 1 ? QObject::tr( "1 item: " ) : QObject::tr( "%L1 items: " ).arg( items );

	return QString{ itemsCount % formatSize( totalSize ) };
    }();
    _ui->heading->setText( headingText );

    // Increase the column width if necessary to fit the new contents, but don't shrink it
    QHeaderView * headerView = _ui->treeWidget->header();
    const int originalSectionWidth = headerView->sectionSize( TW_SizeCol );
    headerView->setSectionResizeMode( TW_SizeCol, QHeaderView::ResizeToContents );
    const int newSectionWidth = headerView->sectionSize( TW_SizeCol );
    headerView->setSectionResizeMode( TW_SizeCol, QHeaderView::Interactive );
    headerView->resizeSection( TW_SizeCol, qMax( originalSectionWidth, newSectionWidth ) );
}


void TrashWindow::refresh()
{
    // Do a full populate if the list is currently empty; it will resize the columns and select item 0
    if ( _ui->treeWidget->topLevelItemCount() == 0 )
    {
	populate();
	return;
    }

    // Make a list of the selected trash entry names
    const QList<QTreeWidgetItem *> selectedItems = _ui->treeWidget->selectedItems();
    QSet<TrashEntry> selectedItemEntries;
    selectedItemEntries.reserve( selectedItems.size() );
    for ( const QTreeWidgetItem * item : selectedItems )
	selectedItemEntries << static_cast<const TrashItem *>( item )->trashEntry();

    // Remember the scrollbar position to make refreshes as seemless as possible
    const int scrollbarPosition = _ui->treeWidget->verticalScrollBar()->value();

    populateTree();

    // Block signals so enableActions() isn't called thousands of times
    SignalBlocker( _ui->treeWidget );

    // Optimisation so this isn't very slow when most items of a long list are selected
    const bool selectAll = selectedItemEntries.size() > _ui->treeWidget->topLevelItemCount() / 2;
    if ( selectAll )
	_ui->treeWidget->selectAll();

    // Recreate the previous selection as near as possible
    for ( QTreeWidgetItemIterator it{ _ui->treeWidget }; *it; ++it )
    {
	const bool selected =
	    selectedItemEntries.contains( static_cast<const TrashItem *>(*it)->trashEntry() );
	if ( ( selectAll && !selected ) || ( !selectAll && selected ) )
	    (*it)->setSelected( selected );
    }

    _ui->treeWidget->verticalScrollBar()->setValue( scrollbarPosition );

    enableActions();
}


void TrashWindow::deleteSelected()
{
    BusyPopup msg{ tr( "Deleting selected items..." ), this };

    // Remember the current item position to reset after this operation removes the selected items
    const int oldCurrentIndex = currentIndex( _ui->treeWidget );

    const QList<QTreeWidgetItem *> selectedItems = _ui->treeWidget->selectedItems();
    for ( QTreeWidgetItem * item : selectedItems )
	static_cast<TrashItem *>( item )->deleteItem();

    const QStringList trashRootPaths = trashRoots();
    for ( const QString & trashRootPath : trashRootPaths )
	deleteExpunged( TrashWindow::expungedDirPath( trashRootPath ) );

    // If everything was restored (and the items deleted), then select the closest neighbour
    if ( _ui->treeWidget->selectedItems().isEmpty() )
	setCurrentItem( _ui->treeWidget, oldCurrentIndex );

    calculateTotalSize();
    enableActions();
}


void TrashWindow::restoreSelected()
{
    BusyPopup msg{ tr( "Restoring selected items..." ), this };

    // Remember the current item position to reset after this operation removes the selected items
    const int oldCurrentIndex = currentIndex( _ui->treeWidget );

    int buttonResponse = QMessageBox::NoButton;

    const QList<QTreeWidgetItem *> selectedItems = _ui->treeWidget->selectedItems();
    const bool singleItem = selectedItems.size() == 1;
    for ( QTreeWidgetItem * item : selectedItems )
    {
	const int button = static_cast<TrashItem *>( item )->restoreItem( singleItem, buttonResponse );
	if ( button == QMessageBox::Abort )
	    break;

	// Remember message box responses yes-to-all and no-to-all
	if ( button == QMessageBox::YesToAll || button == QMessageBox::NoToAll )
	    buttonResponse = button;
    }

    // If everything was restored (and the items deleted), then select the closest neighbour
    if ( _ui->treeWidget->selectedItems().isEmpty() )
	setCurrentItem( _ui->treeWidget, oldCurrentIndex );

    calculateTotalSize();
    enableActions();
}


void TrashWindow::empty()
{
    BusyPopup msg{ QObject::tr( "Emptying Trash..." ), this };

    // Loop through all accessible trash directories
    const QStringList trashRootPaths = trashRoots();
    for ( const QString & trashRootPath : trashRootPaths )
    {
	const QString expungedDirPath = TrashWindow::expungedDirPath( trashRootPath );

	moveAllToExpunged( Trash::filesDirPath( trashRootPath ), expungedDirPath );
	moveAllToExpunged( Trash::infoDirPath( trashRootPath ), expungedDirPath );

	deleteExpunged( expungedDirPath );
    }

    populate();
}


void TrashWindow::populate()
{
    populateTree();

    // Show after populating, or it hides the BusyPopup
    show();

    // Make sure something is selected, even if this window is not the active one
    setCurrentItem( _ui->treeWidget, 0 );

    resizeTreeColumns( _ui->treeWidget );

    enableActions();
}


void TrashWindow::enableActions()
{
    _ui->emptyButton->setEnabled( _ui->treeWidget->topLevelItemCount() );

    const QList<QTreeWidgetItem *> selectedItems = _ui->treeWidget->selectedItems();
    const bool itemSelected = !selectedItems.isEmpty();
    _ui->deleteButton->setEnabled( itemSelected );
    _ui->restoreButton->setEnabled( itemSelected );

    // Can't restore known "broken" trash items
    for ( const QTreeWidgetItem * item : selectedItems )
    {
	if ( item->foreground( TW_NameCol ) == app()->dirTreeModel()->dirReadErrColor() )
	{
	    _ui->restoreButton->setEnabled( false );
	    return;
	}
    }
}


void TrashWindow::contextMenu( const QPoint & pos )
{
    QMenu menu;
    menu.addAction( _ui->actionRefresh );

    if ( _ui->emptyButton->isEnabled() )
	menu.addAction( _ui->actionSelectAll );

    const bool deleteEnabled  = _ui->deleteButton->isEnabled();
    const bool restoreEnabled = _ui->restoreButton->isEnabled();
    if ( deleteEnabled || restoreEnabled )
    {
	menu.addSeparator();

	if ( deleteEnabled )
	    menu.addAction( _ui->actionDelete );

	if ( restoreEnabled )
	    menu.addAction( _ui->actionRestore );
    }

    if ( _ui->emptyButton->isEnabled() )
    {
	menu.addSeparator();
	menu.addAction( _ui->actionEmpty );
    }

    menu.exec( _ui->treeWidget->mapToGlobal( pos ) );
}


void TrashWindow::keyPressEvent( QKeyEvent * event )
{
    // Let return/enter trigger itemActivated instead of buttons that don't have focus
    if ( event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter )
	return;

    QDialog::keyPressEvent( event );
}


void TrashWindow::changeEvent( QEvent * event )
{
    const auto type = event->type();
    if ( type == QEvent::PaletteChange )
	refresh();

    QDialog::changeEvent( event );
}




TrashItem::TrashItem( ProcessStarter * processStarter,
                      const QString  & trashRoot,
                      int              filesDirFd,
                      const char     * entryName ):
    QTreeWidgetItem{ UserType },
    _trashRoot { trashRoot },
    _entryName { entryName }
{
    const auto set = [ this ]( TrashCols col, Qt::Alignment alignment, const QString & text )
    {
	setText( col, text );
	setTextAlignment( col, alignment | Qt::AlignVCenter );
    };

    const auto error = [ this, &set ]( const QString & msg )
    {
	setIcon( TW_NameCol, app()->dirTreeModel()->unreadableDirIcon() );

	const QColor errorTextColor = app()->dirTreeModel()->dirReadErrColor();
	set( TW_NameCol, Qt::AlignLeft, replaceCrLf( _entryName ) );
	setForeground( TW_NameCol, errorTextColor );
	set( TW_DirCol,  Qt::AlignLeft, msg );
	setForeground( TW_DirCol, errorTextColor );
    };

    struct stat statInfo;
    if ( SysUtil::stat( filesDirFd, entryName, statInfo ) != 0 )
    {
	logWarning() << "Can't access " << entryName << ": " << formatErrno() << Qt::endl;
	error( tr( "Can't access Trash entry" ) );
	return;
    }

    const bool isDir = S_ISDIR( statInfo.st_mode );
    _totalSize = statInfo.st_size;
    set( TW_SizeCol, Qt::AlignRight, isDir ? "..." : formatSize( _totalSize ) );

    if ( isDir )
    {
	// The process will be killed if the window is closed, although it will spam the log about it
	QProcess * process = new QProcess{ this };
	process->setProgram( "du" );
	process->setArguments( { "-bs", Trash::trashEntryPath( trashRoot, _entryName ) } );
	connect( process, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ),
                 this,    &TrashItem::processFinished );
	processStarter->add( process );
    }

    const QString trashInfoPath = Trash::trashInfoPath( trashRoot, _entryName );
    QFile infoFile{ trashInfoPath };
    if ( !infoFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
	logWarning() << "Can't open " << trashInfoPath << ": " << infoFile.errorString() << Qt::endl;
	error( tr( "Can't read .trashinfo file" ) );
	return;
    }

    QTextStream in{ &infoFile };
    const QString tagLine   = in.readLine();
    const QString pathLine  = in.readLine();
    const QString mTimeLine = in.readLine();
    if ( !validTrashinfo( tagLine, pathLine, mTimeLine ) )
    {
	logWarning() << trashInfoPath << " format invalid" << Qt::endl;
	error( tr( "Invalid .trashinfo file format" ) );
	return;
    }

    setIcon( TW_NameCol, itemTypeIcon( statInfo.st_mode ) );

    _deletedMTime = stringToMTime( mTimeLine.mid( TrashDir::trashInfoDateTag().size() ) );
    set( TW_DeletedCol, Qt::AlignRight, formatTime( _deletedMTime ) );

    const QString path = pathLine.mid( TrashDir::trashInfoPathTag().size() );
    QString name;
    QString originalDir;
    SysUtil::splitPath( QUrl::fromPercentEncoding( path.toLatin1() ), originalDir, name );
    set( TW_NameCol, Qt::AlignLeft, replaceCrLf( name ) );
    set( TW_DirCol,  Qt::AlignLeft, replaceCrLf( originalDir ) );

    if ( text( TW_NameCol ) != name )
	setToolTip( TW_NameCol, name );
    if ( text( TW_DirCol ) != originalDir )
	setToolTip( TW_DirCol, originalDir );
}


void TrashItem::processFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
    QProcess * senderProcess = qobject_cast<QProcess *>( sender() );
    if ( !senderProcess )
	return;

    senderProcess->deleteLater();

    if ( exitStatus == QProcess::CrashExit )
    {
	// No useful output, just leave the directory own size
	logError() << "'du' process crashed for " << _entryName << " in " << _trashRoot << Qt::endl;
    }
    else
    {
	if ( exitCode != 0 )
	{
	    // du returns 1 for all errors, most likely permissions warnings, but may still return a size
	    logWarning() << "'du' process exit code " << exitCode
	                  << " for " << _entryName << " in " << _trashRoot << Qt::endl;
	}

	const QString output = QString::fromUtf8( senderProcess->readAllStandardOutput() );
	const QRegularExpression notDigit{ "[^0-9]" };
	const FileSize totalSize = output.left( output.indexOf( notDigit ) ).toLongLong();

	// Ignore 0, which indicates the command failed in some way
	if ( totalSize > 0 )
	    _totalSize = totalSize;
    }

    setText( TW_SizeCol, formatSize( _totalSize ) );
}


void TrashItem::deleteItem()
{
    const QByteArray expungedDirStr = TrashWindow::expungedDirPath( _trashRoot ).toUtf8();
    ensureExpunged( expungedDirStr );

    if ( moveToExpunged( Trash::filesDirPath( _trashRoot ).toUtf8(), expungedDirStr, _entryName.toUtf8() ) )
    {
	// If the "files" entry was moved, try to move its corresponding .trashinfo file
	const QString infoName = _entryName % Trash::trashInfoSuffix();
	moveToExpunged( Trash::infoDirPath( _trashRoot ).toUtf8(), expungedDirStr, infoName.toUtf8() );

	// Even if the .trashinfo file is still there, it won't show up in the tree any more
	delete this;
    }
}


int TrashItem::restoreItem( bool singleItem, int buttonResponse )
{
    const QString restoreDirPath = text( TW_DirCol );
    const QString restoreFileName = text( TW_NameCol );
    if ( !SysUtil::exists( restoreDirPath ) )
    {
	logInfo() << restoreDirPath << " no longer exists - attempt to recreate" << Qt::endl;
	QDir{ restoreDirPath }.mkpath( "." );
    }

    const QString restorePath{ restoreDirPath % '/' % restoreFileName };
    if ( SysUtil::exists( restorePath ) )
    {
	if ( buttonResponse == QMessageBox::NoToAll )
	    return buttonResponse;

	if ( buttonResponse != QMessageBox::YesToAll )
	{
	    const QString title = tr( "Cannot restore " ) % restoreFileName;
	    const QString msg = tr( "'%1' already exists." ).arg( restorePath );
	    QMessageBox::StandardButtons buttons{ QMessageBox::Yes | QMessageBox::No };
	    if ( !singleItem )
		buttons |= { QMessageBox::YesToAll | QMessageBox::NoToAll | QMessageBox::Abort };
	    QMessageBox box{ QMessageBox::Question, title, pad( msg, 50 ), buttons, trashWindow() };
	    box.setInformativeText( tr( "Replace? Existing item will be permanently deleted." ) );
	    if ( !singleItem )
		box.button( QMessageBox::Abort )->setToolTip( tr( "Stop restoring items" ) );

	    const int button = box.exec();
	    if ( button == QMessageBox::No || button == QMessageBox::NoToAll || button == QMessageBox::Abort )
		return button;

	    if ( button == QMessageBox::YesToAll )
		buttonResponse = button;
	}

	deletePath( restorePath );
    }

    QFile trashEntry{ Trash::trashEntryPath( _trashRoot, _entryName ) };
    if ( !trashEntry.rename( restorePath ) )
    {
	const QString title = tr( "Restore failed" );
	const QString msg = tr( "Cannot move '%1' to '%2':" ).arg( restoreFileName, restoreDirPath );
	const QMessageBox::StandardButtons buttons = singleItem ? QMessageBox::Ok : QMessageBox::Abort;
	QMessageBox box{ QMessageBox::Warning, title, pad( msg, 50 ), buttons, trashWindow() };
	box.setInformativeText( trashEntry.errorString() );
	if ( !singleItem )
	{
	    QPushButton * button = box.addButton( tr( "&Continue" ), QMessageBox::AcceptRole );
	    button->setToolTip( tr( "Skip this item and continue to restore other selected items" ) );
	    box.button( QMessageBox::Abort )->setToolTip( tr( "Stop restoring items" ) );
	}

	const int button = box.exec();
	if ( button == QMessageBox::Abort )
	    return button;

	return buttonResponse;
    }

    removeTrashInfoFile( _trashRoot, _entryName );

    delete this;

    return buttonResponse;
}


QVariant TrashItem::data( int column, int role ) const
{
    // This is just for the tooltip on columns that are elided and don't otherwise have a tooltip
    if ( role != Qt::ToolTipRole )
	return QTreeWidgetItem::data( column, role );

    const QString tooltipText = QTreeWidgetItem::data( column, Qt::ToolTipRole ).toString();
    return tooltipText.isEmpty() ? tooltipForElided( this, column, 1 ) : tooltipText;
}


bool TrashItem::operator<( const QTreeWidgetItem & rawOther ) const
{
    if ( !treeWidget() )
	return QTreeWidgetItem::operator<( rawOther );

    // Since this is a reference, the dynamic_cast will throw a std::bad_cast
    // exception if it fails. Not catching this here since this is a genuine
    // error which should not be silently ignored.
    const TrashItem & other = dynamic_cast<const TrashItem &>( rawOther );

    switch ( treeWidget()->sortColumn() )
    {
	case TW_SizeCol:    return _totalSize    < other._totalSize;
	case TW_DeletedCol: return _deletedMTime < other._deletedMTime;
	default:            return QTreeWidgetItem::operator<( rawOther );
    }
}
