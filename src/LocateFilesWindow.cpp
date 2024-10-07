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
	    label->setText( QObject::tr( "Limited to %L1 results" ).arg( results ) );
	else if ( results == 1 )
	    label->setText( QObject::tr( "1 result" ) );
	else
	    label->setText( QObject::tr( "%L1 results" ).arg( results ) );
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
	app()->dirTreeModel()->setTreeIconSize( tree );

	QTreeWidgetItem * headerItem = tree->headerItem();
	headerItem->setText( LL_SizeCol,  QObject::tr( "Total Size" ) );
	headerItem->setText( LL_MTimeCol, QObject::tr( "Last Modified" ) );
	headerItem->setText( LL_PathCol,  QObject::tr( "Path" ) );
	headerItem->setTextAlignment( LL_PathCol, Qt::AlignLeft | Qt::AlignVCenter);

	QHeaderView * header = tree->header();
	header->setDefaultAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
	header->setSectionResizeMode( QHeaderView::ResizeToContents );

	// The tree will be sorted each time populateSharedInstance() is called
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

    connect( _ui->treeWidget,    &QTreeWidget::customContextMenuRequested,
             this,               &LocateFilesWindow::itemContextMenu );

    connect( _ui->treeWidget, &QTreeWidget::currentItemChanged, &locateInMainWindow );
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

    // Set the heading and sort order for each new populate command
    instance->_ui->treeWidget->sortByColumn( sortCol, sortOrder );
    instance->_ui->heading->setStatusTip( headingText );
    instance->populate( fileInfo );

    // Show now so the BusyPopup is not obscured
    instance->show();
    instance->raise();
}


void LocateFilesWindow::populate( FileInfo * fileInfo )
{
    // logDebug() << "populating with " << fileInfo << Qt::endl;

    _ui->treeWidget->clear();

    _subtree = fileInfo;
    _treeWalker->prepare( _subtree() );

    populateRecursive( fileInfo );
    showResultsCount( _ui->treeWidget->topLevelItemCount(), _treeWalker->overflow(), _ui->resultsLabel );

    // Force a redraw of the header from the status tip
    resizeEvent( nullptr );

    // Select the first row after a delay so it (and its signals) doesn't slow down the list showing
    QTimer::singleShot( 50, this, [ this ]()
	{ _ui->treeWidget->setCurrentItem( _ui->treeWidget->topLevelItem( 0 ) ); } );
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
    const QString heading = [ this ]()
    {
	const QString statusTip = _ui->heading->statusTip();
	if ( statusTip.isEmpty() )
	    return QString{};

	const FileInfo * fileInfo = _subtree();
	return statusTip.arg( fileInfo ? fileInfo->url() : QString{} );
    }();

    // Calculate a width from the dialog less margins, less a bit more
    elideLabel( _ui->heading, heading, size().width() - 24 );
}




LocateListItem::LocateListItem( FileInfo * item ):
    QTreeWidgetItem{ UserType },
    _size{ item->totalSize() },
    _mtime{ item->mtime() },
    _path{ item->url() }
{
    /**
     * Helper function to set the text and text alignment for a column.
     **/
    const auto set = [ this ]( int col, Qt::Alignment alignment, const QString & text )
    {
	setText( col, text );
	setTextAlignment( col, alignment | Qt::AlignVCenter );
    };

    set( LL_SizeCol,  Qt::AlignRight,   formatSize( _size ) );
    set( LL_MTimeCol, Qt::AlignHCenter, formatTime( _mtime ) );
    set( LL_PathCol,  Qt::AlignLeft,    _path );

    setIcon( LL_PathCol, app()->dirTreeModel()->itemTypeIcon( item ) );
}


QVariant LocateListItem::data( int column, int role ) const
{
    // This is just for the tooltip on columns that are likely to be long and elided
    if ( role != Qt::ToolTipRole || column != LL_PathCol )
	return QTreeWidgetItem::data( column, role );

    return tooltipForElided( this, LL_PathCol, 0 );
}


bool LocateListItem::operator<( const QTreeWidgetItem & rawOther ) const
{
    if ( !treeWidget() )
	return QTreeWidgetItem::operator<( rawOther );

    // Since this is a reference, the dynamic_cast will throw a std::bad_cast
    // exception if it fails. Not catching this here since this is a genuine
    // error which should not be silently ignored.
    const LocateListItem & other = dynamic_cast<const LocateListItem &>( rawOther );

    switch ( treeWidget()->sortColumn() )
    {
	case LL_SizeCol:  return _size  < other.size();
	case LL_MTimeCol: return _mtime < other.mtime();
	default:                 return QTreeWidgetItem::operator<( rawOther );
    }


}

