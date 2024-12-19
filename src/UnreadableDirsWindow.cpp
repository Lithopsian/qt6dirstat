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
	app()->dirTreeModel()->setTreeIconSize( tree );

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
     * entry to 'treeWidget' for each one.
     **/
    void populateRecursive( FileInfo * subtree, QTreeWidget * treeWidget )
    {
	if ( !subtree || !subtree->isDirInfo() )
	    return;

	DirInfo * dir = subtree->toDirInfo();
	if ( dir->subtreeReadError() )
	    treeWidget->addTopLevelItem( new UnreadableDirsItem{ dir } );

	// Recurse through any subdirectories
	for ( DirInfoIterator it{ subtree }; *it; ++it )
	    populateRecursive( *it, treeWidget );

	// Dot entries can't contain unreadable dirs, but attics can
	populateRecursive( dir->attic(), treeWidget );
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

    populateRecursive( app()->firstToplevel(), _ui->treeWidget );

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
    // This is just for the tooltip on columns that are likely to be long and elided
//    if ( role != Qt::ToolTipRole || column != UD_PathCol )
    if ( role != Qt::ToolTipRole )
	return QTreeWidgetItem::data( column, role );

    const QString tooltipText = QTreeWidgetItem::data( column, Qt::ToolTipRole ).toString();
    return tooltipText.isEmpty() ? tooltipForElided( this, column, 1 ) : tooltipText;
}
