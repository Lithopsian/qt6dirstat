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
#include "DirTreeModel.h"
#include "FileInfoIterator.h"
#include "FormatUtil.h"
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
	QTreeWidgetItem * headerItem = tree->headerItem();
	headerItem->setText( UD_PathCol,  QObject::tr( "Directory" ) );
	headerItem->setText( UD_UserCol,  QObject::tr( "User" ) );
	headerItem->setText( UD_GroupCol, QObject::tr( "Group" ) );
	headerItem->setText( UD_PermCol,  QObject::tr( "Permissions" ) );
	headerItem->setText( UD_OctalCol, QObject::tr( "Perm." ) );
	headerItem->setTextAlignment( UD_PathCol, Qt::AlignLeft | Qt::AlignVCenter );

	QHeaderView * header = tree->header();
	header->setDefaultAlignment( Qt::AlignHCenter | Qt::AlignVCenter );

	tree->sortByColumn( UD_PathCol, Qt::AscendingOrder );
    }


    /**
     * Recursively find unreadable directories in 'subtree' and add an
     * entry to 'unreadableDirs' for each one.  'unreadableDirs' is a QSet to
     * avoid duplicates.
     *
     * In the most common case, DirPermissionDenied, a directory does not allow
     * access to its children and it is added to 'unreadableDirs'.
     *
     * In the less common cases of DirError, DirMissing, and DirNoAccess, a
     * directory allows access to an item, but stat() fails to fetch
     * information about it.  Since there would be no information to display
     * about such items, see if the parent can be accessed and add the parent.
     * If the parent itself cannot be accessed, an ancestor of it will already
     * have been added.
     **/
    void populateRecursive( QSet<const DirInfo *> & unreadableDirs, FileInfo * subtree )
    {
	if ( !subtree || !subtree->isDirInfo() )
	    return;

	const DirInfo * dir = subtree->toDirInfo();
	if ( dir->readState() == DirPermissionDenied )
	{
	    unreadableDirs << dir;
	}
	else if ( dir->readError() )
	{
	    const DirInfo * parent = dir->parent();
	    if ( parent && !parent->readError() )
		unreadableDirs << parent;
	}

	// Recurse through any subdirectories
	for ( DirInfoIterator it{ subtree }; *it; ++it )
	    populateRecursive( unreadableDirs, *it );

	// Dot entries can't contain unreadable dirs, but attics can
	populateRecursive( unreadableDirs, subtree->attic() );
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
    _ui->treeWidget->setIconSize( app()->dirTreeModel()->dirTreeIconSize() );

    //logDebug() << "Locating all unreadable dirs" << Qt::endl;

    QSet<const DirInfo *> unreadableDirs;
    populateRecursive( unreadableDirs, app()->firstToplevel() );

    for ( const DirInfo * dir : asConst( unreadableDirs ) )
	_ui->treeWidget->addTopLevelItem( new UnreadableDirsItem{ dir } );

    const int rowCount = _ui->treeWidget->topLevelItemCount();
    _ui->totalLabel->setText( rowCount > 1 ? tr( "%L1 directories" ).arg( rowCount ) : QString{} );

    //logDebug() << rowCount << " directories" << Qt::endl;

    // Make sure something is selected, even if this window is not the active one
    _ui->treeWidget->setCurrentItem( _ui->treeWidget->topLevelItem( 0 ) );

    resizeTreeColumns( _ui->treeWidget );
}




UnreadableDirsItem::UnreadableDirsItem( const DirInfo * dir ):
    QTreeWidgetItem{ UserType }
{
    const auto set = [ this ]( UnreadableDirectories col, Qt::Alignment alignment, const QString & text )
    {
	setText( col, text );
	setTextAlignment( col, alignment | Qt::AlignVCenter );
    };

    set( UD_PathCol,  Qt::AlignLeft,  replaceCrLf( dir->url() ) );
    set( UD_UserCol,  Qt::AlignLeft,  dir->userName() );
    set( UD_GroupCol, Qt::AlignLeft,  dir->groupName() );
    set( UD_PermCol,  Qt::AlignRight, dir->symbolicPermissions() );
    set( UD_OctalCol, Qt::AlignRight, dir->octalPermissions() );

    setIcon( UD_PathCol, app()->dirTreeModel()->unreadableDirIcon() );

    if ( text( UD_PathCol ) != dir->url() )
	setToolTip( UD_PathCol, dir->url() );
}


QVariant UnreadableDirsItem::data( int column, int role ) const
{
    // This is just for the tooltip on columns that are elided and don't otherwise have a tooltip
    const QVariant data = QTreeWidgetItem::data( column, role );
    if ( role != Qt::ToolTipRole || data.isValid() )
	return data;

    return tooltipForElided( this, column, 1 );
}
