/*
 *   File name: LocateFilesWindow.cpp
 *   Summary:   QDirStat "locate files" window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QMenu>
#include <QPointer>
#include <QResizeEvent>

#include "LocateFilesWindow.h"
#include "ActionManager.h"
#include "DirTreeModel.h"       // itemTypeIcon()
#include "Exception.h"
#include "FileInfo.h"
#include "FileInfoIterator.h"
#include "FormatUtil.h"
#include "HeaderTweaker.h"
#include "MainWindow.h"
#include "QDirStatApp.h"        // SelectionModel, DirTreeModel, mainWindow()
#include "SelectionModel.h"
#include "Settings.h"
#include "TreeWalker.h"


using namespace QDirStat;


LocateFilesWindow::LocateFilesWindow( TreeWalker * treeWalker,
                                      QWidget    * parent ):
    QDialog { parent },
    _ui { new Ui::LocateFilesWindow },
    _treeWalker { treeWalker }
{
    // logDebug() << "init" << Qt::endl;

    setAttribute( Qt::WA_DeleteOnClose );

    _ui->setupUi( this );

    initWidgets();
    Settings::readWindowSettings( this, "LocateFilesWindow" );

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

    Settings::writeWindowSettings( this, "LocateFilesWindow" );
}


LocateFilesWindow * LocateFilesWindow::sharedInstance( TreeWalker * treeWalker )
{
    static QPointer<LocateFilesWindow> _sharedInstance;

    if ( _sharedInstance )
	_sharedInstance->_treeWalker.reset( treeWalker );
    else
	_sharedInstance = new LocateFilesWindow( treeWalker, app()->mainWindow() );

    return _sharedInstance;
}


void LocateFilesWindow::refresh()
{
    populate( _subtree() );
    selectFirstItem();
}


void LocateFilesWindow::initWidgets()
{
    app()->dirTreeModel()->setTreeWidgetSizes( _ui->treeWidget );

    _ui->treeWidget->setColumnCount( LocateListColumnCount );
    _ui->treeWidget->setHeaderLabels( { tr( "Size" ), tr( "Last Modified" ), tr( "Path" ) } );
    _ui->treeWidget->header()->setDefaultAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
    _ui->treeWidget->headerItem()->setTextAlignment( LocateListPathCol, Qt::AlignLeft | Qt::AlignVCenter);

    _ui->resultsLabel->setText( "" );

    HeaderTweaker::resizeToContents( _ui->treeWidget->header() );

    addCleanupHotkeys();
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
    instance->selectFirstItem();
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
    showResultsCount();

    _ui->treeWidget->setSortingEnabled( true );

    // Force a redraw of the header from the status tip
    resizeEvent( nullptr );
}


void LocateFilesWindow::populateRecursive( FileInfo * dir )
{
    if ( !dir )
	return;

    for ( FileInfoIterator it( dir ); *it; ++it )
    {
	FileInfo * item = *it;
	if ( _treeWalker->check( item ) )
	    _ui->treeWidget->addTopLevelItem( new LocateListItem( item ) );

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

    // logDebug() << "Locating " << searchResult->path() << " in tree" << Qt::endl;
    app()->selectionModel()->setCurrentItemPath( searchResult->path() );
}


void LocateFilesWindow::itemContextMenu( const QPoint & pos )
{
    // See if the right click was actually on an item
    if ( !_ui->treeWidget->itemAt( pos ) )
	return;

    QMenu menu;

    const QStringList actions { "actionCopyPath", "actionMoveToTrash", "---" };
    ActionManager::addEnabledActions( &menu, actions );

    ActionManager::addEnabledCleanups( &menu );

    menu.exec( _ui->treeWidget->mapToGlobal( pos ) );
}


void LocateFilesWindow::addCleanupHotkeys()
{
    ActionManager::addActions( this, { "actionMoveToTrash", "actionFindFiles" } );

    ActionManager::addActiveCleanups( this );
}


void LocateFilesWindow::resizeEvent( QResizeEvent * )
{
    // Format the heading with the current url, which may be a fallback
    const FileInfo * fileInfo = _subtree();
    const QString heading = _ui->heading->statusTip().arg( fileInfo ? fileInfo->url() : QString() );

    // Calculate a width from the dialog less margins, less a bit more
    elideLabel( _ui->heading, heading, size().width() - 24 );
}





LocateListItem::LocateListItem( FileInfo * item ):
    QTreeWidgetItem { QTreeWidgetItem::UserType }
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

    switch ( (LocateListColumns)treeWidget()->sortColumn() )
    {
	case LocateListSizeCol:  return _size  < other.size();
	case LocateListMTimeCol: return _mtime < other.mtime();

	case LocateListPathCol:
	case LocateListColumnCount:
	    break;
    }

    return QTreeWidgetItem::operator<( rawOther );
}

