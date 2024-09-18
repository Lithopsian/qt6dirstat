/*
 *   File name: LocateFilesWindow.cpp
 *   Summary:   QDirStat "locate files" window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QPointer>
#include <QResizeEvent>

#include "LocateFilesWindow.h"
#include "ActionManager.h"
#include "DirTreeModel.h"       // itemTypeIcon()
#include "Exception.h"
#include "FileInfoIterator.h"
#include "FormatUtil.h"
#include "HeaderTweaker.h"
#include "MainWindow.h"
#include "QDirStatApp.h"        // SelectionModel, DirTreeModel, mainWindow()
#include "SelectionModel.h"
#include "Settings.h"
#include "TreeWalker.h"


using namespace QDirStat;


namespace
{
    /**
     * Format the number of results and display in 'label'.
     **/
    void showResultsCount( int results, bool overflow, QLabel * label )
    {
	if ( overflow )
	    label->setText( QObject::tr( "Limited to %1 results" ).arg( results ) );
	else if ( results == 1 )
	    label->setText( QObject::tr( "1 result" ) );
	else
	    label->setText( QObject::tr( "%1 results" ).arg( results ) );
    }


    /**
     * Locate one of the items in this list results in the main window's
     * tree and treemap widgets via their SelectionModel.
     **/
    void locateInMainWindow( QTreeWidgetItem * item )
    {
	if ( !item )
	    return;

	LocateListItem * searchResult = dynamic_cast<LocateListItem *>( item );
	CHECK_DYNAMIC_CAST( searchResult, "LocateListItem" );

	// logDebug() << "Locating " << searchResult->path() << " in tree" << Qt::endl;
	app()->selectionModel()->setCurrentItemPath( searchResult->path() );
    }


    /**
     * Add the hotkeys (shortcuts) of the cleanup actions to this window.
     **/
    void addCleanupHotkeys( LocateFilesWindow * window )
    {
	ActionManager::addActions( window, { "actionMoveToTrash",
	                                     "actionFindFiles",
	                                     ActionManager::cleanups() } );
    }


    /**
     * One-time initialization of the tree widget.
     **/
    void initTree( QTreeWidget * tree )
    {
	app()->dirTreeModel()->setTreeWidgetSizes( tree );

	tree->setHeaderLabels( { QObject::tr( "Size" ),
	                         QObject::tr( "Last Modified" ),
	                         QObject::tr( "Path" ) } );
	tree->header()->setDefaultAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
	tree->headerItem()->setTextAlignment( LocateListPathCol, Qt::AlignLeft | Qt::AlignVCenter);
	tree->header()->setSectionResizeMode( QHeaderView::ResizeToContents );
    }

} // namespace


LocateFilesWindow::LocateFilesWindow( TreeWalker * treeWalker,
                                      QWidget    * parent ):
    QDialog{ parent },
    _ui{ new Ui::LocateFilesWindow },
    _treeWalker{ treeWalker }
{
    // logDebug() << "init" << Qt::endl;

    setAttribute( Qt::WA_DeleteOnClose );

    _ui->setupUi( this );

    initTree( _ui->treeWidget );
    addCleanupHotkeys( this );
    _ui->resultsLabel->setText( QString{} );

    Settings::readWindowSettings( this, "LocateFilesWindow" );

    connect( _ui->refreshButton, &QPushButton::clicked,
             this,               &LocateFilesWindow::refresh );

    connect( _ui->treeWidget,    &QTreeWidget::currentItemChanged,
             this,               &locateInMainWindow );

    connect( _ui->treeWidget,    &QTreeWidget::customContextMenuRequested,
             this,               &LocateFilesWindow::itemContextMenu );
}


LocateFilesWindow::~LocateFilesWindow()
{
    //logDebug() << "destroying" << Qt::endl;

    Settings::writeWindowSettings( this, "LocateFilesWindow" );
}


LocateFilesWindow * LocateFilesWindow::sharedInstance( TreeWalker * treeWalker )
{
    static QPointer<LocateFilesWindow> _sharedInstance;

    if ( _sharedInstance )
	_sharedInstance->_treeWalker.reset( treeWalker );
    else
	_sharedInstance = new LocateFilesWindow{ treeWalker, app()->mainWindow() };

    return _sharedInstance;
}


void LocateFilesWindow::refresh()
{
    populate( _subtree() );
    selectFirstItem();
}


void LocateFilesWindow::populateSharedInstance( TreeWalker    * treeWalker,
                                                FileInfo      * fileInfo,
                                                const QString & headingText,
                                                int             sortCol,
                                                Qt::SortOrder   sortOrder )
{
    if ( !treeWalker || !fileInfo )
        return;

    // Get the shared instance, creating it if necessary
    LocateFilesWindow * instance = sharedInstance( treeWalker );

    instance->_ui->heading->setStatusTip( headingText );
    instance->populate( fileInfo );
    instance->_ui->treeWidget->sortByColumn( sortCol, sortOrder );
    instance->show();

    // Select the first row after a delay so it doesn't slow down displaying the list
    QTimer::singleShot( 25, instance, &LocateFilesWindow::selectFirstItem );
}


void LocateFilesWindow::populate( FileInfo * fileInfo )
{
    // logDebug() << "populating with " << fileInfo << Qt::endl;

    _subtree = fileInfo;
    _treeWalker->prepare( _subtree() );

    // For better Performance: Disable sorting while inserting many items
    _ui->treeWidget->setSortingEnabled( false );
    _ui->treeWidget->clear();

    populateRecursive( fileInfo ? fileInfo : _subtree() );
    showResultsCount( _ui->treeWidget->topLevelItemCount(), _treeWalker->overflow(), _ui->resultsLabel );

    _ui->treeWidget->setSortingEnabled( true );

    // Force a redraw of the header from the status tip
    resizeEvent( nullptr );
}


void LocateFilesWindow::populateRecursive( FileInfo * dir )
{
    if ( !dir )
	return;

    for ( DotEntryIterator it{ dir }; *it; ++it )
    {
	if ( _treeWalker->check( *it ) )
	    _ui->treeWidget->addTopLevelItem( new LocateListItem{ *it } );

	if ( it->hasChildren() )
	    populateRecursive( *it );
    }
}


void LocateFilesWindow::itemContextMenu( const QPoint & pos )
{
    // See if the right click was actually on an item
    if ( !_ui->treeWidget->itemAt( pos ) )
	return;

    QMenu * menu = ActionManager::createMenu( { "actionCopyPath", "actionMoveToTrash" },
                                              { ActionManager::separator(), ActionManager::cleanups() } );
    menu->exec( _ui->treeWidget->mapToGlobal( pos ) );
}


void LocateFilesWindow::resizeEvent( QResizeEvent * )
{
    // Format the heading with the current url, which may be a fallback
    const FileInfo * fileInfo = _subtree();
    const QString heading = _ui->heading->statusTip().arg( fileInfo ? fileInfo->url() : QString{} );

    // Calculate a width from the dialog less margins, less a bit more
    elideLabel( _ui->heading, heading, size().width() - 24 );
}




LocateListItem::LocateListItem( FileInfo * item ):
    QTreeWidgetItem{ QTreeWidgetItem::UserType }
{
    CHECK_PTR( item );

    _path  = item->url();
    _size  = item->totalSize();
    _mtime = item->mtime();

    set( LocateListSizeCol,  formatSize( _size ),  Qt::AlignRight   );
    set( LocateListMTimeCol, formatTime( _mtime ), Qt::AlignHCenter );
    set( LocateListPathCol,  _path,                Qt::AlignLeft    );

    setIcon( LocateListPathCol, app()->dirTreeModel()->itemTypeIcon( item ) );
}


bool LocateListItem::operator<( const QTreeWidgetItem & rawOther ) const
{
    if ( !treeWidget() )
	return QTreeWidgetItem::operator<( rawOther );

    // Since this is a reference, the dynamic_cast will throw a std::bad_cast
    // exception if it fails. Not catching this here since this is a genuine
    // error which should not be silently ignored.
    const LocateListItem & other = dynamic_cast<const LocateListItem &>( rawOther );

    switch ( static_cast<LocateListColumns>( treeWidget()->sortColumn() ) )
    {
	case LocateListSizeCol:  return _size  < other.size();
	case LocateListMTimeCol: return _mtime < other.mtime();

	case LocateListPathCol:
	case LocateListColumnCount:
	    break;
    }

    return QTreeWidgetItem::operator<( rawOther );
}

