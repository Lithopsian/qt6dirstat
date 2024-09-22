/*
 *   File name: UnreadableDirsWindow.cpp
 *   Summary:   QDirStat file type statistics window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QPointer>

#include "UnreadableDirsWindow.h"
#include "Attic.h"
#include "DirTreeModel.h"
#include "FileInfoIterator.h"
#include "Logger.h"
#include "MainWindow.h"
#include "QDirStatApp.h"
#include "SelectionModel.h"
#include "Settings.h"


using namespace QDirStat;


namespace
{
    /**
     * Select one of the search results in the main window's tree and
     * treemap widgets via their SelectionModel.
     **/
    void selectResult( QTreeWidgetItem * widgetItem )
    {
	const UnreadableDirListItem * item = dynamic_cast<UnreadableDirListItem *>( widgetItem );
	if ( item )
	    app()->selectionModel()->setCurrentItem( item->dir(), true ); // select
    }


    /**
     * One-time initialization of the widgets in this window.
     **/
    void initTree( QTreeWidget * tree )
    {
	app()->dirTreeModel()->setTreeWidgetSizes( tree );

	const QString directory   = QObject::tr( "Directory" );
	const QString user        = QObject::tr( "User" );
	const QString group       = QObject::tr( "Group" );
	const QString permissions = QObject::tr( "Permissions" );
	const QString perm        = QObject::tr( "Perm." );
	tree->setHeaderLabels( { directory, user, group, permissions, perm } );
	tree->header()->setDefaultAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
	tree->headerItem()->setTextAlignment( UD_Path, Qt::AlignLeft | Qt::AlignVCenter );
	tree->header()->setSectionResizeMode( QHeaderView::ResizeToContents );
    }

} // namespace


UnreadableDirsWindow::UnreadableDirsWindow( QWidget * parent ):
    QDialog{ parent },
    _ui{ new Ui::UnreadableDirsWindow }
{
    // logDebug() << "init" << Qt::endl;

    setAttribute( Qt::WA_DeleteOnClose );

    _ui->setupUi( this );

    initTree( _ui->treeWidget );
    Settings::readWindowSettings( this, "UnreadableDirsWindow" );

    connect( _ui->treeWidget, &QTreeWidget::currentItemChanged,
             this,            &selectResult );
}


UnreadableDirsWindow::~UnreadableDirsWindow()
{
    // logDebug() << "destroying" << Qt::endl;
    Settings::writeWindowSettings( this, "UnreadableDirsWindow" );
}


UnreadableDirsWindow * UnreadableDirsWindow::sharedInstance()
{
    static QPointer<UnreadableDirsWindow> _sharedInstance;

    if ( !_sharedInstance )
	_sharedInstance = new UnreadableDirsWindow{ app()->mainWindow() };

    return _sharedInstance;
}


void UnreadableDirsWindow::populateSharedInstance( FileInfo * fileInfo )
{
    if ( !fileInfo )
        return;

    sharedInstance()->populate( fileInfo );
    sharedInstance()->show();
}


void UnreadableDirsWindow::populate( FileInfo * fileInfo )
{
    _ui->treeWidget->clear();
    _subtree = fileInfo;

    //logDebug() << "Locating all unreadable dirs below " << _subtree.url() << Qt::endl;

    populateRecursive( fileInfo ? fileInfo : _subtree() );
    _ui->treeWidget->sortByColumn( UD_Path, Qt::AscendingOrder );

    const int rowCount = _ui->treeWidget->topLevelItemCount();
    _ui->totalLabel->setText( rowCount > 1 ? tr( "%1 directories" ).arg( rowCount ) : QString{} );

    //logDebug() << count << " directories" << Qt::endl;

    // Make sure something is selected, even if this window is not the active
    // one (for example because the user just clicked on another suffix in the
    // file type stats window). When the window is activated, the tree widget
    // automatically uses the topmost item as the current item, and in the
    // default selection mode, this item is also selected. When the window is
    // not active, this does not happen yet - until the window is activated.
    _ui->treeWidget->setCurrentItem( _ui->treeWidget->topLevelItem( 0 ) );
}


void UnreadableDirsWindow::populateRecursive( FileInfo * fileInfo )
{
    if ( !fileInfo || !fileInfo->isDirInfo() )
	return;

    DirInfo * dir = fileInfo->toDirInfo();
    if ( dir->readError() )
	_ui->treeWidget->addTopLevelItem( new UnreadableDirListItem{ dir } );

    // Recurse through any subdirectories
    for ( DirInfoIterator it{ fileInfo }; *it; ++it )
	populateRecursive( *it );

    // Dot entries can't contain unreadable dirs, but attics can
    populateRecursive( dir->attic() );
}




UnreadableDirListItem::UnreadableDirListItem( DirInfo * dir ) :
    QTreeWidgetItem{ QTreeWidgetItem::UserType },
    _dir{ dir }
{
    set( UD_Path,        dir->url(),                 Qt::AlignLeft  );
    set( UD_User,        dir->userName(),            Qt::AlignLeft  );
    set( UD_Group,       dir->groupName(),           Qt::AlignLeft  );
    set( UD_Permissions, dir->symbolicPermissions(), Qt::AlignRight );
    set( UD_Octal,       dir->octalPermissions(),    Qt::AlignRight );

    setIcon( UD_Path, app()->dirTreeModel()->unreadableDirIcon() );
}

/*
bool UnreadableDirListItem::operator<( const QTreeWidgetItem & rawOther ) const
{
    if ( !treeWidget() )
	return QTreeWidgetItem::operator<( rawOther );

    // Since this is a reference, the dynamic_cast will throw a std::bad_cast
    // exception if it fails. Not catching this here since this is a genuine
    // error which should not be silently ignored.
    const UnreadableDirListItem & other = dynamic_cast<const UnreadableDirListItem &>( rawOther );

    switch ( (UnreadableDirectories)treeWidget()->sortColumn() )
    {
	case UD_Path: break;
	case UD_User:        return _dir->userName()            < other._dir->userName();
	case UD_Group:       return _dir->groupName()           < other._dir->groupName();
	case UD_Permissions: return _dir->symbolicPermissions() < other._dir->symbolicPermissions();
	case UD_Octal:       return _dir->octalPermissions()    < other._dir->octalPermissions();
    }

    return QTreeWidgetItem::operator<( rawOther );
}
*/
