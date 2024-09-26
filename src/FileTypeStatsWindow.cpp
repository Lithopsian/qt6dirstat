/*
 *   File name: FileTypeStatsWindow.cpp
 *   Summary:   QDirStat file type statistics window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QMenu>
#include <QPointer>

#include "FileTypeStatsWindow.h"
#include "FileTypeStats.h"
#include "DirTree.h"
#include "DirTreeModel.h"
#include "FileInfo.h"
#include "FileSizeStatsWindow.h"
#include "FormatUtil.h"
#include "LocateFileTypeWindow.h"
#include "Logger.h"
#include "MimeCategory.h"
#include "QDirStatApp.h"
#include "SelectionModel.h"
#include "Settings.h"


using namespace QDirStat;


namespace
{
    /**
     * Returns the current item in 'tree', cast as a
     * SuffixFileTypeItem, or 0 if there is no current item or it is
     * not a SuffixFileTypeItem.
     **/
    const SuffixFileTypeItem * currentItem( const QTreeWidget * treeWidget)
    {
	return dynamic_cast<const SuffixFileTypeItem *>( treeWidget->currentItem() );
    }


    /**
     * Process the "Other" category so that it contains no more than
     * a configured "topX" number of different suffixes (including
     * "<no extension>").  The pruned suffix items are still included
     * in the category totals, but no longer appear in the tree.
     **/
    void topXOtherItems( FileTypeItem * otherCategoryItem )
    {
	// Find the maximum number of suffix items to show in the "Other" category
	Settings settings;
	settings.beginGroup( "FileTypeStatsWindow" );
	const int topX = settings.value( "TopX", 50 ).toInt();
	settings.setDefaultValue( "TopX", topX );
	settings.endGroup();

	// Do nothing if there are less than the configured topX suffixes
	const int childCount = otherCategoryItem->childCount();
	if ( childCount <= topX )
	    return;

	// Already sorted by size descending, so just truncate the list
	for ( int i=childCount-1; i >= topX; --i )
	    otherCategoryItem->removeChild( otherCategoryItem->child( i ) );

	// Change the displayed category name to indicate it has been pruned
	otherCategoryItem->setText( FT_NameCol, QObject::tr( "Other (top %1)" ).arg( topX ) );
    }


    /**
     * Create a tree item for a category and add it to the tree.  This
     * is used for the top level items in the tree.
     **/
    FileTypeItem * addCategoryItem( QTreeWidget   * treeWidget,
                                    const QString & name,
                                    int             count,
                                    FileSize        sum,
                                    float           percent )
    {
	auto item = new FileTypeItem{ name, count, sum, percent };
	treeWidget->addTopLevelItem( item );

	return item;
    }


    /**
     * Create a specialised tree item for the children of the top level
     * category items.  Each item represents the total files in that
     * category which were matched by a particular suffix, by a non-suffix
     * rules, or that have no suffix.
     *
     * The new item is added to the given parent.
     * item.
     **/
    void addSuffixItem( FileTypeItem  * parent,
                        bool            otherCategory,
                        const QString & suffix,
                        int             count,
                        FileSize        sum,
                        float           percent )
    {
	parent->addChild( new SuffixFileTypeItem{ otherCategory, suffix, count, sum, percent } );
    }


    /**
     * One-time initialization of the tree widget.
     **/
    void initTree( QTreeWidget * tree )
    {
	app()->dirTreeModel()->setTreeWidgetSizes( tree );

	QTreeWidgetItem * headerItem = tree->headerItem();
	headerItem->setText( FT_NameCol,       QObject::tr( "Name" ) );
	headerItem->setText( FT_CountCol,      QObject::tr( "Files" ) );
	headerItem->setText( FT_TotalSizeCol,  QObject::tr( "Total Size" ) );
	headerItem->setText( FT_PercentageCol, QObject::tr( "Size %" ) );
	headerItem->setTextAlignment( FT_NameCol, Qt::AlignVCenter | Qt::AlignLeft );

	QHeaderView * header = tree->header();
	header->setDefaultAlignment( Qt::AlignCenter );
	header->setSectionResizeMode( QHeaderView::ResizeToContents );

	tree->sortByColumn( FT_TotalSizeCol, Qt::DescendingOrder );
    }


    /**
     * Populate 'treeWidget' with type statistics collected for
     * 'subtree'.  The statistics are discarded once the tree has
     * been populated.
     **/
    void populateTree( QTreeWidget * treeWidget, FileInfo * subtree )
    {
	// Create a map of toplevel items for finding suffix item parents
	QHash<const MimeCategory *, FileTypeItem *> categoryItem;

	FileTypeStats stats{ subtree };

	for ( auto it = stats.categoriesBegin(); it != stats.categoriesEnd(); ++it )
	{
	    const MimeCategory * category = it.key();
	    if ( category )
	    {
		// Add a category item
		const int       count   = it.value().count;
		const FileSize  sum     = it.value().sum;
		const float     percent = stats.percentage( sum );
		const QString & name    = category->name();

		categoryItem[ category ] = addCategoryItem( treeWidget, name, count, sum, percent );
	    }
	}

	// Create items for each individual suffix (below a category)
	for ( auto it = stats.suffixesBegin(); it != stats.suffixesEnd(); ++it )
	{
	    const QString      & suffix   = it.key().suffix;
	    const MimeCategory * category = it.key().category;

	    if ( category )
	    {
		FileTypeItem * parentItem = categoryItem.value( category, nullptr );
		if ( parentItem )
		{
		    const bool     otherCategory = category == stats.otherCategory();
		    const int      count         = it.value().count;
		    const FileSize sum           = it.value().sum;
		    const float    percent       = stats.percentage( sum );

		    addSuffixItem( parentItem, otherCategory, suffix, count, sum, percent );
		}
		else
		{
		    logError() << "No parent category item for " << suffix << Qt::endl;
		}
	    }
	    else
	    {
		logError() << "No category for suffix " << suffix << Qt::endl;
	    }
	}

	// Prune the <Other> category widgets to the configured TopX entries
	FileTypeItem * otherCategoryItem = categoryItem.value( stats.otherCategory(), nullptr );
	if ( otherCategoryItem )
	    topXOtherItems( otherCategoryItem );

	treeWidget->setCurrentItem( treeWidget->topLevelItem( 0 ) );
    }

} // namespace


FileTypeStatsWindow::FileTypeStatsWindow( QWidget * parent ):
    QDialog{ parent },
    _ui{ new Ui::FileTypeStatsWindow }
{
    // logDebug() << "constructor" << Qt::endl;

    setAttribute( Qt::WA_DeleteOnClose );

    _ui->setupUi( this );

    initTree( _ui->treeWidget );
    readSettings();

    const DirTree        * dirTree        = app()->dirTree();
    const SelectionModel * selectionModel = app()->selectionModel();

    connect( dirTree,              &DirTree::aborted,
             this,                 &FileTypeStatsWindow::syncedRefresh );

    connect( dirTree,              &DirTree::finished,
             this,                 &FileTypeStatsWindow::syncedRefresh );

    connect( selectionModel,       &SelectionModel::currentItemChanged,
             this,                 &FileTypeStatsWindow::syncedPopulate );

    connect( _ui->treeWidget,      &QTreeWidget::currentItemChanged,
             this,                 QOverload<>::of( &FileTypeStatsWindow::enableActions ) );

    connect( _ui->treeWidget,      &QTreeWidget::customContextMenuRequested,
             this,                 &FileTypeStatsWindow::contextMenu);

    connect( _ui->refreshButton,   &QAbstractButton::clicked,
             this,                 &FileTypeStatsWindow::refresh );

    connect( _ui->locateButton,    &QAbstractButton::clicked,
             this,                 &FileTypeStatsWindow::locateCurrentFileType );

    connect( _ui->sizeStatsButton, &QAbstractButton::clicked,
             this,                 &FileTypeStatsWindow::sizeStatsForCurrentFileType );

    // See also signal/slot connections in file-type-stats-window.ui

    show();
}


FileTypeStatsWindow::~FileTypeStatsWindow()
{
    //logDebug() << "destroying" << Qt::endl;

    Settings::writeWindowSettings( this, "FileTypeStatsWindow" );
}


FileTypeStatsWindow * FileTypeStatsWindow::sharedInstance( QWidget * parent )
{
    static QPointer<FileTypeStatsWindow> _sharedInstance;

    if ( !_sharedInstance )
	_sharedInstance = new FileTypeStatsWindow{ parent };

    return _sharedInstance;
}


void FileTypeStatsWindow::readSettings()
{
    Settings::readWindowSettings( this, "FileTypeStatsWindow" );

    Settings settings;
    settings.beginGroup( "FileTypeStatsWindow" );
    settings.applyActionHotkey( _ui->actionLocate );
    settings.applyActionHotkey( _ui->actionSizeStats );
    settings.endGroup();

    // Add actions to this window to get the hotkeys
    addAction( _ui->actionLocate );
    addAction( _ui->actionSizeStats );
}


void FileTypeStatsWindow::refresh()
{
    populate( _subtree() );
}


void FileTypeStatsWindow::syncedRefresh()
{
    if ( _ui->syncCheckBox->isChecked() )
	refresh();
}


void FileTypeStatsWindow::syncedPopulate()
{
    if ( !_ui->syncCheckBox->isChecked() )
	return;

    FileInfo * newSelection = app()->currentDirInfo();
    if ( newSelection != _subtree() )
	populate( newSelection );
}


void FileTypeStatsWindow::populate( FileInfo * newSubtree )
{
    _ui->treeWidget->clear();
    enableActions( false );

    if ( !newSubtree )
	return;

    _subtree = newSubtree;

    _ui->headingLabel->setStatusTip( tr( "File type statistics for %1" ).arg( _subtree.url() ) );
    resizeEvent( nullptr );

    populateTree( _ui->treeWidget, _subtree() );
}


void FileTypeStatsWindow::locateCurrentFileType()
{
    const QString suffix = currentItemSuffix();

    if ( suffix.isEmpty() )
	return;

    // logDebug() << "Locating " << current->suffix() << Qt::endl;

    // Use the shared LocateFileTypeWindow instance.  Let it pick its own parent
    // so it doesn't get closed along with this window.
    LocateFileTypeWindow::populateSharedInstance( '.' % suffix, _subtree() );
}


void FileTypeStatsWindow::sizeStatsForCurrentFileType()
{
    const QString suffix = currentItemSuffix();
    FileInfo * dir = _subtree();
    if ( suffix.isEmpty() || !dir )
	return;

    //logDebug() << "Size stats for " << suffix << " in " << dir << Qt::endl;

    FileSizeStatsWindow::populateSharedInstance( this->parentWidget(), dir, '.' % suffix );
}


QString FileTypeStatsWindow::currentItemSuffix() const
{
    const auto * item = currentItem( _ui->treeWidget );
    if ( item && !item->suffix().isEmpty() )
	return item->suffix();

    return QString{};
}


void FileTypeStatsWindow::enableActions()
{
    enableActions( !currentItemSuffix().isEmpty() );
}


void FileTypeStatsWindow::enableActions( bool enable )
{
    _ui->locateButton->setEnabled( enable );
    _ui->sizeStatsButton->setEnabled( enable );
}


void FileTypeStatsWindow::contextMenu( const QPoint & pos )
{
    // See if the right click was actually on an item
    if ( !_ui->treeWidget->itemAt( pos ) )
	return;

    // The clicked item will always be the current item now
    const QString suffix = currentItemSuffix();
    if ( suffix.isEmpty() )
	return;

    _ui->actionLocate->setText( tr( "&Locate files with suffix ." ) % suffix );
    _ui->actionSizeStats->setText( tr( "&Size statistics for suffix ." ) % suffix );

    QMenu menu;
    menu.addAction( _ui->actionLocate );
    menu.addAction( _ui->actionSizeStats );
    menu.exec( _ui->treeWidget->mapToGlobal( pos ) );
}


void FileTypeStatsWindow::keyPressEvent( QKeyEvent * event )
{
    if ( event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter )
    {
	// Act on enter key here so it doesn't go to the default (dialog) button
	QTreeWidgetItem * item = _ui->treeWidget->currentItem();
	if ( item->childCount() > 0 )
	    // For category headings, toggle the expanded state
	    item->setExpanded( !item->isExpanded() );
	else if ( item->type() == QTreeWidgetItem::UserType )
	    // Try the locate file type action, although it may not be an appropriate suffix
	    locateCurrentFileType();

	return;
    }

    QDialog::keyPressEvent( event );
}


void FileTypeStatsWindow::resizeEvent( QResizeEvent * )
{
    // Calculate a width from the dialog less margins, less a bit more
    elideLabel( _ui->headingLabel, _ui->headingLabel->statusTip(), size().width() - 24 );
}




FileTypeItem::FileTypeItem( const QString & name,
                            int             count,
                            FileSize        totalSize,
                            float           percentage ):
    QTreeWidgetItem{ UserType },
    _count{ count },
    _totalSize{ totalSize },
    _percentage{ percentage }
{
    /**
     * Helper function to set the text and text alignment for a column.
     **/
    const auto set = [ this ]( int col, Qt::Alignment alignment, const QString & text )
    {
	setText( col, text );
	setTextAlignment( col, alignment | Qt::AlignVCenter );
    };

    set( FT_NameCol,       Qt::AlignLeft,  name );
    set( FT_CountCol,      Qt::AlignRight, formatCount( count ) );
    set( FT_TotalSizeCol,  Qt::AlignRight, formatSize( totalSize ) );
    set( FT_PercentageCol, Qt::AlignRight, formatPercent( percentage ) );
}


bool FileTypeItem::operator<(const QTreeWidgetItem & rawOther) const
{
    if ( !treeWidget() )
	return QTreeWidgetItem::operator<( rawOther );

    // Since this is a reference, the dynamic_cast will throw a std::bad_cast
    // exception if it fails. Not catching this here since this is a genuine
    // error which should not be silently ignored.
    const FileTypeItem & other = dynamic_cast<const FileTypeItem &>( rawOther );

    switch ( treeWidget()->sortColumn() )
    {
	case FT_CountCol:      return _count      < other._count;
	case FT_TotalSizeCol:  return _totalSize  < other._totalSize;
	case FT_PercentageCol: return _percentage < other._percentage;
	default:               return QTreeWidgetItem::operator<( rawOther );
    }
}
