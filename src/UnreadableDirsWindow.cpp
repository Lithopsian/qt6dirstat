/*
 *   File name: UnreadableDirsWindow.cpp
 *   Summary:	QDirStat file type statistics window
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include "UnreadableDirsWindow.h"
#include "Attic.h"
#include "DirTree.h"
#include "DirTreeModel.h"
#include "HeaderTweaker.h"
#include "QDirStatApp.h"        // dirTreeModel
#include "SelectionModel.h"
#include "SettingsHelpers.h"
#include "Logger.h"
#include "Exception.h"


using namespace QDirStat;


UnreadableDirsWindow::UnreadableDirsWindow( QWidget * parent ):
    QDialog ( parent ),
    _ui { new Ui::UnreadableDirsWindow }
{
    // logDebug() << "init" << Qt::endl;

    setAttribute( Qt::WA_DeleteOnClose );

    CHECK_NEW( _ui );
    _ui->setupUi( this );

    initWidgets();
    readWindowSettings( this, "UnreadableDirsWindow" );

    connect( _ui->treeWidget, &QTreeWidget::currentItemChanged,
             this,            &UnreadableDirsWindow::selectResult );
}


UnreadableDirsWindow::~UnreadableDirsWindow()
{
    // logDebug() << "destroying" << Qt::endl;
    writeWindowSettings( this, "UnreadableDirsWindow" );
    delete _ui;
}


UnreadableDirsWindow * UnreadableDirsWindow::sharedInstance()
{
    static QPointer<UnreadableDirsWindow> _sharedInstance = nullptr;

    if ( !_sharedInstance )
    {
	_sharedInstance = new UnreadableDirsWindow( app()->findMainWindow() );
	CHECK_NEW( _sharedInstance );
    }

    return _sharedInstance;
}


void UnreadableDirsWindow::clear()
{
    _ui->treeWidget->clear();
}


void UnreadableDirsWindow::initWidgets()
{
    app()->setWidgetFontSize( _ui->treeWidget );

    const QStringList headerLabels = { tr( "Directory" ),
				       tr( "User" ),
				       tr( "Group" ),
				       tr( "Permissions" ),
				       tr( "Perm." )
				     };

    _ui->treeWidget->setIconSize( app()->dirTreeModel()->dirTreeIconSize() );
    _ui->treeWidget->setColumnCount( headerLabels.size() );
    _ui->treeWidget->setHeaderLabels( headerLabels );

    // Center the column headers except the first (Directory).
    _ui->treeWidget->header()->setDefaultAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
    _ui->treeWidget->headerItem()->setTextAlignment( 0, Qt::AlignLeft | Qt::AlignVCenter );

    HeaderTweaker::resizeToContents( _ui->treeWidget->header() );
}


void UnreadableDirsWindow::populateSharedInstance( FileInfo * subtree )
{
    if ( !subtree )
        return;

    sharedInstance()->populate( subtree );
    sharedInstance()->show();
}


void UnreadableDirsWindow::populate( FileInfo * newSubtree )
{
    clear();
    _subtree = newSubtree;

    //logDebug() << "Locating all unreadable dirs below " << _subtree.url() << Qt::endl;

    populateRecursive( newSubtree ? newSubtree : _subtree() );
    _ui->treeWidget->sortByColumn( UD_Path, Qt::AscendingOrder );

    const int rowCount = _ui->treeWidget->topLevelItemCount();
    _ui->totalLabel->setText( rowCount > 1 ? QString::number( rowCount ) + " directories" : QString() );

    //logDebug() << count << " directories" << Qt::endl;

    // Make sure something is selected, even if this window is not the active
    // one (for example because the user just clicked on another suffix in the
    // file type stats window). When the window is activated, the tree widget
    // automatically uses the topmost item as the current item, and in the
    // default selection mode, this item is also selected. When the window is
    // not active, this does not happen yet - until the window is activated.
    _ui->treeWidget->setCurrentItem( _ui->treeWidget->topLevelItem( 0 ) );
}


void UnreadableDirsWindow::populateRecursive( FileInfo * subtree )
{
    if ( !subtree || !subtree->isDirInfo() )
	return;

    DirInfo * dir = subtree->toDirInfo();
    if ( dir->readError() )
    {
	UnreadableDirListItem * searchResultItem = new UnreadableDirListItem( dir );
	CHECK_NEW( searchResultItem );

	_ui->treeWidget->addTopLevelItem( searchResultItem );
    }

    // Recurse through any subdirectories
    for ( FileInfo * child = dir->firstChild(); child; child = child->next() )
    {
	if ( child->isDir() )
	    populateRecursive( child );
    }

    if ( dir->attic() )
        populateRecursive( dir->attic() );

    // No need to recurse through dot entries; they can't have any read error
    // or any subdirectory children which might have a read error.
}


void UnreadableDirsWindow::selectResult( QTreeWidgetItem * widgetItem )
{
    const UnreadableDirListItem * item = dynamic_cast<UnreadableDirListItem *>( widgetItem );
    if ( item )
	app()->selectionModel()->setCurrentItem( item->dir(), true ); // select
}






UnreadableDirListItem::UnreadableDirListItem( DirInfo * dir ) :
    QTreeWidgetItem ( QTreeWidgetItem::UserType ),
    _dir { dir }
{
    set( UD_Path,        dir->url() + "    ",        Qt::AlignLeft  );
    set( UD_User,        dir->userName(),            Qt::AlignLeft  );
    set( UD_Group,       dir->groupName(),           Qt::AlignLeft  );
    set( UD_Permissions, dir->symbolicPermissions(), Qt::AlignRight );
    set( UD_Octal,       dir->octalPermissions(),    Qt::AlignRight );

    setIcon( UD_Path, app()->dirTreeModel()->unreadableDirIcon() );
}


void UnreadableDirListItem::set( UnreadableDirectories col, const QString & text, Qt::Alignment alignment )
{
    setText( col, text );
    setTextAlignment( col, alignment | Qt::AlignVCenter );
}


bool UnreadableDirListItem::operator<( const QTreeWidgetItem & rawOther ) const
{
    // Since this is a reference, the dynamic_cast will throw a std::bad_cast
    // exception if it fails. Not catching this here since this is a genuine
    // error which should not be silently ignored.
    const UnreadableDirListItem & other = dynamic_cast<const UnreadableDirListItem &>( rawOther );

    const UnreadableDirectories col = treeWidget() ? (UnreadableDirectories)treeWidget()->sortColumn() : UD_Path;
    switch ( col )
    {
	case UD_Path: break;
	case UD_User:        return _dir->userName()            < other._dir->userName();
	case UD_Group:       return _dir->groupName()           < other._dir->groupName();
	case UD_Permissions: return _dir->symbolicPermissions() < other._dir->symbolicPermissions();
	case UD_Octal:       return _dir->octalPermissions()    < other._dir->octalPermissions();
    }

    return QTreeWidgetItem::operator<( rawOther );
}

