/*
 *   File name: UnreadableDirsWindow.cpp
 *   Summary:   QDirStat file type statistics window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QAbstractButton>
#include <QPointer>

#include "UnreadableDirsWindow.h"
#include "Attic.h"
#include "DirTree.h"
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
     *
     * Only the result path is known and this must be searched in the
     * tree.  This is to avoid holding stale pointers.  If there is
     * no widget selected or the path is no longer found in the tree
     * (or there is no tree!), nothing happens.
     **/
    void selectResult( QTreeWidgetItem * widgetItem )
    {
	if ( widgetItem )
	    app()->selectionModel()->setCurrentItemPath( widgetItem->text( UD_PathCol ) );
    }


    /**
     * One-time initialization of the widgets in this window.
     **/
    void initTree( QTreeWidget * tree )
    {
	app()->dirTreeModel()->setTreeWidgetSizes( tree );

	QTreeWidgetItem * headerItem = tree->headerItem();
	headerItem->setText( UD_PathCol,  QObject::tr( "Directory" ) );
	headerItem->setText( UD_UserCol,  QObject::tr( "User" ) );
	headerItem->setText( UD_GroupCol, QObject::tr( "Group" ) );
	headerItem->setText( UD_PermCol,  QObject::tr( "Permissions" ) );
	headerItem->setText( UD_OctalCol, QObject::tr( "Perm." ) );
	headerItem->setTextAlignment( UD_PathCol, Qt::AlignLeft | Qt::AlignVCenter );

	QHeaderView * header = tree->header();
	header->setDefaultAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
	header->setSectionResizeMode( QHeaderView::ResizeToContents );

	tree->sortByColumn( UD_PathCol, Qt::AscendingOrder );
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

    connect( _ui->treeWidget, &QTreeWidget::currentItemChanged, &selectResult );

    connect( _ui->refreshButton, &QAbstractButton::clicked,
             this,               &UnreadableDirsWindow::populate );

    show();
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


void UnreadableDirsWindow::populate()
{
    _ui->treeWidget->clear();

    //logDebug() << "Locating all unreadable dirs" << Qt::endl;

    populateRecursive( app()->firstToplevel() );

    const int rowCount = _ui->treeWidget->topLevelItemCount();
    _ui->totalLabel->setText( rowCount > 1 ? tr( "%1 directories" ).arg( rowCount ) : QString{} );

    //logDebug() << rowCount << " directories" << Qt::endl;

    // Make sure something is selected, even if this window is not the active one
    _ui->treeWidget->setCurrentItem( _ui->treeWidget->topLevelItem( 0 ) );
}


void UnreadableDirsWindow::populateRecursive( FileInfo * subtree )
{
    if ( !subtree || !subtree->isDirInfo() )
	return;

    DirInfo * dir = subtree->toDirInfo();
    if ( dir->readError() )
    {
	QTreeWidgetItem * item = new QTreeWidgetItem{ _ui->treeWidget };

	const auto set = [ item ]( UnreadableDirectories col, Qt::Alignment alignment, const QString & text )
	{
	    item->setText( col, text );
	    item->setTextAlignment( col, alignment | Qt::AlignVCenter );
	};

	set( UD_PathCol,  Qt::AlignLeft,  dir->url() );
	set( UD_UserCol,  Qt::AlignLeft,  dir->userName() );
	set( UD_GroupCol, Qt::AlignLeft,  dir->groupName() );
	set( UD_PermCol,  Qt::AlignRight, dir->symbolicPermissions() );
	set( UD_OctalCol, Qt::AlignRight, dir->octalPermissions() );

	item->setIcon( UD_PathCol, app()->dirTreeModel()->unreadableDirIcon() );
    }

    // Recurse through any subdirectories
    for ( DirInfoIterator it{ subtree }; *it; ++it )
	populateRecursive( *it );

    // Dot entries can't contain unreadable dirs, but attics can
    populateRecursive( dir->attic() );
}



/*
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
*/
/*
bool UnreadableDirListItem::operator<( const QTreeWidgetItem & rawOther ) const
{
    if ( !treeWidget() )
	return QTreeWidgetItem::operator<( rawOther );

    // Since this is a reference, the dynamic_cast will throw a std::bad_cast
    // exception if it fails. Not catching this here since this is a genuine
    // error which should not be silently ignored.
    const UnreadableDirListItem & other = dynamic_cast<const UnreadableDirListItem &>( rawOther );

    switch ( treeWidget()->sortColumn() )
    {
	case UD_Path: break;
	case UD_User:        return _dir->userName()            < other._dir->userName();
	case UD_Group:       return _dir->groupName()           < other._dir->groupName();
	case UD_Permissions: return _dir->symbolicPermissions() < other._dir->symbolicPermissions();
	case UD_Octal:       return _dir->octalPermissions()    < other._dir->octalPermissions();
	default:             return QTreeWidgetItem::operator<( rawOther );
}
*/
