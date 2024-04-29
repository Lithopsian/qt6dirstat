/*
 *   File name: LocateFilesWindow.cpp
 *   Summary:   QDirStat "locate files" window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QMenu>
#include <QResizeEvent>

#include "LocateFilesWindow.h"
#include "ActionManager.h"
#include "DirTreeModel.h"       // itemTypeIcon()
#include "FileInfo.h"
#include "FileInfoIterator.h"
#include "FormatUtil.h"
#include "HeaderTweaker.h"
#include "QDirStatApp.h"        // SelectionModel, DirTreeModel, findMainWindow()
#include "SelectionModel.h"
#include "SettingsHelpers.h"
#include "TreeWalker.h"
#include "Logger.h"
#include "Exception.h"


using namespace QDirStat;


LocateFilesWindow::LocateFilesWindow( TreeWalker * treeWalker,
                                      QWidget    * parent ):
    QDialog ( parent ),
    _ui { new Ui::LocateFilesWindow },
    _treeWalker { treeWalker }
{
    // logDebug() << "init" << Qt::endl;

    setAttribute( Qt::WA_DeleteOnClose );

    CHECK_NEW( _ui );
    _ui->setupUi( this );

    CHECK_PTR( _treeWalker );

    initWidgets();
    readWindowSettings( this, "LocateFilesWindow" );

    connect( _ui->refreshButton, &QPushButton::clicked,
	     this,               &LocateFilesWindow::refresh );

    connect( _ui->treeWidget,    &QTreeWidget::currentItemChanged,
	     this,               &LocateFilesWindow::locateInMainWindow );

    connect( _ui->treeWidget,    &QTreeWidget::customContextMenuRequested,
             this,               &LocateFilesWindow::itemContextMenu );
}


LocateFilesWindow::~LocateFilesWindow()
{
    //logDebug() << "destroying" << Qt::endl;

    writeWindowSettings( this, "LocateFilesWindow" );
    delete _treeWalker;
    delete _ui;
}


LocateFilesWindow * LocateFilesWindow::sharedInstance( TreeWalker * treeWalker )
{
    static QPointer<LocateFilesWindow> _sharedInstance = nullptr;

    if ( _sharedInstance )
    {
	_sharedInstance->setTreeWalker( treeWalker );
    }
    else
    {
	_sharedInstance = new LocateFilesWindow( treeWalker, app()->findMainWindow() );
	CHECK_NEW( _sharedInstance );
    }

    return _sharedInstance;
}


void LocateFilesWindow::clear()
{
    _ui->treeWidget->clear();
}


void LocateFilesWindow::setTreeWalker( TreeWalker * newTreeWalker )
{
    CHECK_PTR( newTreeWalker );

    delete _treeWalker;
    _treeWalker = newTreeWalker;
}


void LocateFilesWindow::refresh()
{
    populate( _subtree() );
    selectFirstItem();
}


void LocateFilesWindow::initWidgets()
{
    app()->setWidgetFontSize( _ui->treeWidget );

    _ui->treeWidget->setColumnCount( LocateListColumnCount );
    _ui->treeWidget->setHeaderLabels( { tr( "Size" ), tr( "Last Modified" ), tr( "Path" ) } );
    _ui->treeWidget->header()->setDefaultAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
    _ui->treeWidget->headerItem()->setTextAlignment( LocateListPathCol, Qt::AlignLeft | Qt::AlignVCenter);

    _ui->treeWidget->setIconSize( app()->dirTreeModel()->dirTreeIconSize() );
    _ui->resultsLabel->setText( "" );

    HeaderTweaker::resizeToContents( _ui->treeWidget->header() );

    addCleanupHotkeys();
}


void LocateFilesWindow::populateSharedInstance( TreeWalker    * treeWalker,
						FileInfo      * subtree,
						const QString & headingText,
						int             sortCol,
						Qt::SortOrder   sortOrder )
{
    if ( !treeWalker || !subtree )
        return;

    // Get the shared instance, creating it if necessary
    LocateFilesWindow * instance = sharedInstance( treeWalker );

    instance->_ui->heading->setStatusTip( headingText );
    instance->populate( subtree );
    instance->_ui->treeWidget->sortByColumn( sortCol, sortOrder );
    instance->show();
    instance->selectFirstItem();
}


void LocateFilesWindow::populate( FileInfo * newSubtree )
{
    // logDebug() << "populating with " << newSubtree << Qt::endl;

    clear();

    _subtree = newSubtree;
    _treeWalker->prepare( _subtree() );

    // For better Performance: Disable sorting while inserting many items
    _ui->treeWidget->setSortingEnabled( false );

    populateRecursive( newSubtree ? newSubtree : _subtree() );
    showResultsCount();

    _ui->treeWidget->setSortingEnabled( true );
}


void LocateFilesWindow::populateRecursive( FileInfo * dir )
{
    if ( !dir )
	return;

    for ( FileInfoIterator it( dir ); *it; ++it )
    {
	FileInfo * item = *it;
	if ( _treeWalker->check( item ) )
	{
	    LocateListItem * locateListItem = new LocateListItem( item );
	    CHECK_NEW( locateListItem );

	    _ui->treeWidget->addTopLevelItem( locateListItem );
	}

	if ( item->hasChildren() )
	    populateRecursive( item );
    }
}


void LocateFilesWindow::showResultsCount() const
{
    const int results = _ui->treeWidget->topLevelItemCount();
    if ( _treeWalker->overflow() )
	_ui->resultsLabel->setText( tr( "Limited to %1 results" ).arg( results ) );
    else if ( results == 1 )
	_ui->resultsLabel->setText( tr( "1 result" ) );
    else
        _ui->resultsLabel->setText( tr( "%1 results" ).arg( results ) );
}

/*
void LocateFilesWindow::selectFirstItem()
{
    QTreeWidgetItem * firstItem = _ui->treeWidget->topLevelItem( 0 );
    if ( firstItem )
        _ui->treeWidget->setCurrentItem( firstItem );
}
*/

void LocateFilesWindow::locateInMainWindow( QTreeWidgetItem * item )
{
    if ( !item )
	return;

    LocateListItem * searchResult = dynamic_cast<LocateListItem *>( item );
    CHECK_DYNAMIC_CAST( searchResult, "LocateListItem" );

    CHECK_PTR( _subtree.tree() );

    // logDebug() << "Locating " << searchResult->path() << " in tree" << Qt::endl;
    app()->selectionModel()->setCurrentItem( searchResult->path() );
}


void LocateFilesWindow::itemContextMenu( const QPoint & pos )
{
    // See if the right click was actually on an item
    if ( !_ui->treeWidget->itemAt( pos ) )
	return;

    QMenu menu;

    const QStringList actions = { "actionCopyPath", "actionMoveToTrash", "---" };
    ActionManager::addEnabledActions( &menu, actions );

    ActionManager::addEnabledCleanups( &menu );

    menu.exec( _ui->treeWidget->mapToGlobal( pos ) );
}


void LocateFilesWindow::addCleanupHotkeys()
{
    ActionManager::addActions( this, { "actionMoveToTrash", "actionFindFiles" } );

    ActionManager::addActiveCleanups( this );
}


void LocateFilesWindow::resizeEvent( QResizeEvent * event )
{
    // Calculate a width from the dialog less margins, less a bit more
    elideLabel( _ui->heading, _ui->heading->statusTip(), event->size().width() - 24 );
}





LocateListItem::LocateListItem( FileInfo * item ):
    QTreeWidgetItem ( QTreeWidgetItem::UserType )
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


void LocateListItem::set( int col, const QString & text, Qt::Alignment alignment )
{
    setText( col, text );
    setTextAlignment( col, alignment | Qt::AlignVCenter );
}


bool LocateListItem::operator<( const QTreeWidgetItem & rawOther ) const
{
    if ( !treeWidget() )
	return QTreeWidgetItem::operator<( rawOther );

    // Since this is a reference, the dynamic_cast will throw a std::bad_cast
    // exception if it fails. Not catching this here since this is a genuine
    // error which should not be silently ignored.
    const LocateListItem & other = dynamic_cast<const LocateListItem &>( rawOther );

    switch ( (LocateListColumns)treeWidget()->sortColumn() )
    {
	case LocateListSizeCol:  return _size  < other.size();
	case LocateListMTimeCol: return _mtime < other.mtime();
	case LocateListPathCol:
	case LocateListColumnCount: break;
    }

    return QTreeWidgetItem::operator<( rawOther );
}

