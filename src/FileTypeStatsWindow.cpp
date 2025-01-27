/*
 *   File name: FileTypeStatsWindow.cpp
 *   Summary:   QDirStat file type statistics window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QKeyEvent>
#include <QMenu>
#include <QPointer>

#include "FileTypeStatsWindow.h"
#include "ActionManager.h"
#include "DirTree.h"
#include "DirTreeModel.h"
#include "FileInfo.h"
#include "FileSizeStatsWindow.h"
#include "FormatUtil.h"
#include "LocateFileTypeWindow.h"
#include "Logger.h"
#include "MimeCategory.h"
#include "PercentBar.h"
#include "QDirStatApp.h"
#include "SelectionModel.h"
#include "Settings.h"


using namespace QDirStat;


namespace
{
    /**
     * Returns a WildcardCategory constructed from 'patternCategory'.
     **/
    WildcardCategory wildcardCategory( const PatternCategory * patternCategory )
    {
	if ( patternCategory->caseInsensitive )
	    return { CaseInsensitiveWildcard{ patternCategory->pattern }, patternCategory->category };
	else
	    return { CaseSensitiveWildcard{ patternCategory->pattern }, patternCategory->category };
    }


    /**
     * Returns the name to use for the "other" item grouping all
     * the smallest uncategorised patterns.
     **/
    QString otherName() { return QObject::tr( "<other>" ); }

    /**
     * Returns the current item in 'tree', cast as a
     * PatternFileTypeItem, or 0 if there is no current item or it is
     * not a PatternFileTypeItem.
     **/
    const PatternFileTypeItem * currentItem( const QTreeWidget * treeWidget)
    {
	return dynamic_cast<const PatternFileTypeItem *>( treeWidget->currentItem() );
    }


    /**
     * Create a tree item for a category and add it to the tree.  This
     * is used for the top level items in the tree.
     **/
    FileTypeItem * addCategoryItem( QTreeWidget         * treeWidget,
                                    const FileTypeStats & stats,
                                    const QString       & name,
                                    const CountSize     & countSize )
    {
	auto item = new FileTypeItem{ name, countSize, stats.totalCount(), stats.totalSize() };
	treeWidget->addTopLevelItem( item );

	return item;
    }


    /**
     * Create a tree item for a child of a top level category item.
     * Each item represents the total files in that category which
     * were matched by a particular pattern.
     *
     * The new item is added to 'parentItem'.
     **/
    void addPatternItem( FileTypeItem       * parentItem,
                         const QString      & pattern,
                         bool                 caseInsensitive,
                         const MimeCategory * category,
                         const CountSize    & countSize )
    {
	const PatternCategory patternCategory{ pattern, caseInsensitive, category };
	auto item = new PatternFileTypeItem{ parentItem, pattern, patternCategory, countSize };
	parentItem->addChild( item );
    }


    /**
     * Create a tree item for a child of a top level category items.
     * Each item represents the total files in that category which
     * were not matched by a pattern.  This used for categories such
     * as executables and symlinks.
     *
     * The new item is added to 'parentItem'.
     **/
    void addNonPatternItem( FileTypeItem       * parentItem,
                            const MimeCategory * category,
                            const CountSize    & countSize )
    {
	const QString name = QObject::tr( "﻿<non-pattern rule>" );
	const QString tooltip = QObject::tr( "Items not categorised by filename pattern" );
	auto item = new PatternFileTypeItem{ parentItem, name, tooltip, category, countSize };
	parentItem->addChild( item );
    }


    /**
     * Create a specialised tree item child of <uncategorised> which
     * summarises either items with no extension or items smaller
     * than the configured top X patterns.
     *
     * The new item is added to 'parentItem' and returned.
     **/
    QTreeWidgetItem * addNonItemItem( FileTypeItem    * parentItem,
                                      const QString   & name,
                                      const QString   & tooltip,
                                      const CountSize & countSize )
    {
	auto item = new FileTypeItem{ parentItem, name, tooltip, countSize };
	parentItem->addChild( item );

	return item;
    }


    /**
     * Process the <uncategorised> items so that there are no more
     * than a configured top X number of different patterns.  The
     * pruned pattern items are merged into a single <other> item.
     **/
    void topXItems( FileTypeItem * nonCategoryItem )
    {
	// Find the maximum number of separate pattern items to show in <uncategorised>
	Settings settings;
	settings.beginGroup( "FileTypeStatsWindow" );
	const int topX = settings.value( "TopX", 20 ).toInt();
	settings.setDefaultValue( "TopX", topX );
	settings.endGroup();

	// Do nothing if there are less than the configured top X patternes
	// (plus one to avoid having a single item in <other>)
	if ( topX < 0 || topX+1 >= nonCategoryItem->childCount() )
	    return;

	// Sort by size descending, so we can just merge the tail into one item
	QTreeWidget * treeWidget = nonCategoryItem->treeWidget();
	QHeaderView * header     = treeWidget->header();
	const Qt::SortOrder sortOrder   = header->sortIndicatorOrder();
	const int           sortSection = header->sortIndicatorSection();
	treeWidget->sortByColumn( FT_TotalSizeCol, Qt::DescendingOrder );

	// Loop through the tail, taking items into a list and keeping track of the counts
	QList<QTreeWidgetItem *> otherItems; // QList for QTreeWidgetItem::addChildren
	FileCount otherCount = 0;
	FileSize  otherSize  = 0;
	while ( nonCategoryItem->childCount() > topX )
	{
	    QTreeWidgetItem * otherItem = nonCategoryItem->takeChild( topX );
	    otherItems << otherItem;

	    FileTypeItem * fileTypeItem = dynamic_cast<FileTypeItem *>( otherItem );
	    if ( fileTypeItem )
	    {
		otherCount += fileTypeItem->count();
		otherSize  += fileTypeItem->totalSize();
	    }
	}

	// Create the <other> item and add the pruned items to it
	const QString tooltip =
	    QObject::tr( "Combined totals for all patterns smaller than the top %1 (configurable)" );
	const CountSize countSize{ otherCount, otherSize };
	QTreeWidgetItem * otherParent =
	    addNonItemItem( nonCategoryItem, otherName(), tooltip.arg( topX ), countSize );
	otherParent->addChildren( otherItems );

	// Return to the original sort order in case the user had changed it
	treeWidget->sortByColumn( sortSection, sortOrder );
    }


    /**
     * Populate 'treeWidget' with type statistics collected for
     * 'subtree'.  The statistics are discarded once the tree has
     * been populated.
     **/
    void populateTree( QTreeWidget * treeWidget, FileInfo * subtree )
    {
	// Collect statistics for 'subtree'
	const FileTypeStats stats{ subtree };

	// Create a map of toplevel items for finding pattern item parents
	QHash<const MimeCategory *, FileTypeItem *> categoryItem;
	for ( auto it = stats.categoriesBegin(); it != stats.categoriesEnd(); ++it )
	{
	    // Add a category item
	    const MimeCategory * category = it.key();
	    const QString name = category ? category->name() : QObject::tr( "<uncategorised>" );
	    categoryItem[ category ] = addCategoryItem( treeWidget, stats, name, it.value() );
	}

	// Create items for each individual pattern (below a category)
	for ( auto it = stats.patternsBegin(); it != stats.patternsEnd(); ++it )
	{
	    const PatternCategory key = it.key();
	    const MimeCategory * category = key.category;
	    FileTypeItem * parentItem = categoryItem.value( category, nullptr );
	    if ( parentItem )
	    {
		const QString & pattern = key.pattern;
		if ( !pattern.isEmpty() )
		{
		    addPatternItem( parentItem, pattern, key.caseInsensitive, category, it.value() );
		}
		else if ( category )
		{
		    addNonPatternItem( parentItem, category, it.value() );
		}
		else
		{
		    const QString name = QObject::tr( "﻿<no extension>" );
		    const QString tooltip = QObject::tr( "Items with no recognised extension" );
		    addNonItemItem( parentItem, name, tooltip, it.value() );
		}
	    }
	    else
	    {
		logError() << "No parent category item for " << key.pattern << Qt::endl;
	    }
	}

	// Prune the "Uncategorised" widgets to the configured TopX entries
	FileTypeItem * nonCategoryItem = categoryItem.value( nullptr, nullptr );
	if ( nonCategoryItem )
	    topXItems( nonCategoryItem );

	treeWidget->setCurrentItem( treeWidget->topLevelItem( 0 ) );
    }


    /**
     * One-time initialization of the tree widget.
     **/
    void initTree( QTreeWidget * tree )
    {
	QTreeWidgetItem * headerItem = tree->headerItem();
	headerItem->setText( FT_NameCol,            QObject::tr( "Name" ) );
	headerItem->setText( FT_CountCol,           QObject::tr( "Files" ) );
	headerItem->setText( FT_CountPercentBarCol, QObject::tr( "Files %" ) );
	headerItem->setText( FT_CountPercentCol,    "%" );
	headerItem->setText( FT_TotalSizeCol,       QObject::tr( "Total Size" ) );
	headerItem->setText( FT_SizePercentBarCol,  QObject::tr( "Size %" ) );
	headerItem->setText( FT_SizePercentCol,     "%" );
	headerItem->setTextAlignment( FT_NameCol, Qt::AlignVCenter | Qt::AlignLeft );

	QHeaderView * header = tree->header();
	header->setDefaultAlignment( Qt::AlignCenter );
	header->setSectionResizeMode( QHeaderView::ResizeToContents );

	tree->sortByColumn( FT_TotalSizeCol, Qt::DescendingOrder );

	const int height = app()->dirTreeModel()->dirTreeIconSize().height();
	PercentBarDelegate::createStatsDelegates( tree, height, FT_CountPercentBarCol, FT_SizePercentBarCol );
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
    enableActions( false );

    Settings::readWindowSettings( this, "FileTypeStatsWindow" );
    ActionManager::actionHotkeys( this, "FileTypeStatsWindow" );

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

    connect( _ui->treeWidget,      &QTreeWidget::itemActivated,
             this,                 &FileTypeStatsWindow::itemActivated );

    connect( _ui->actionLocate,    &QAction::triggered,
             this,                 &FileTypeStatsWindow::itemActivated );

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

    const int newHeight = app()->dirTreeModel()->dirTreeIconSize().height();
    PercentBarDelegate::delegateForColumn( _ui->treeWidget, FT_CountPercentBarCol )->setHeight( newHeight );
    PercentBarDelegate::delegateForColumn( _ui->treeWidget, FT_SizePercentBarCol  )->setHeight( newHeight );

    if ( !newSubtree )
	return;

    _subtree = newSubtree;

    _ui->headingLabel->setStatusTip( tr( "File type statistics for " ) % replaceCrLf( _subtree.url() ) );
    showElidedLabel( _ui->headingLabel, this );

    populateTree( _ui->treeWidget, _subtree() );
}


void FileTypeStatsWindow::locateCurrentFileType()
{
    // Make sure we have an item with a pattern that can be searched
    const PatternCategory * patternCategory = currentItemPattern();
    FileInfo * dir = _subtree();
    if ( !patternCategory || !dir )
	return;

    // logDebug() << "Locating " << patternCategory->pattern() << Qt::endl;

    // Use the shared LocateFileTypeWindow instance.
    // Let it pick its own parent so it doesn't get closed along with this window.
    LocateFileTypeWindow::populateSharedInstance( wildcardCategory( patternCategory ), dir );
}


void FileTypeStatsWindow::sizeStatsForCurrentFileType()
{
    // Make sure we have an item with a pattern that can be searched
    const PatternCategory * patternCategory = currentItemPattern();
    FileInfo * dir = _subtree();
    if ( !patternCategory || !dir )
	return;

    //logDebug() << "Size stats for " << patternCategory->pattern << " in " << dir << Qt::endl;

    FileSizeStatsWindow::populateSharedInstance( this->parentWidget(), dir, wildcardCategory( patternCategory ) );
}


const PatternCategory * FileTypeStatsWindow::currentItemPattern() const
{
    const auto * item = currentItem( _ui->treeWidget );
    if ( item )
	return &item->patternCategory();

    return nullptr;
}


void FileTypeStatsWindow::enableActions()
{
    enableActions( currentItemPattern() );
}


void FileTypeStatsWindow::contextMenu( const QPoint & pos )
{
    QMenu menu;
    menu.addAction( _ui->actionRefresh );

    // See if the right click was actually on an item
    if ( _ui->treeWidget->itemAt( pos ) )
    {
	// The clicked item will always be the current item now
	const PatternCategory * patternCategory = currentItemPattern();
	if ( patternCategory )
	{
	    const QString & pattern    = patternCategory->pattern;
	    const bool      hasPattern = !pattern.isEmpty();
	    const QString locateText =
		hasPattern ? tr( "&Locate files matching " ) % pattern : tr( "&Locate files" );
	    _ui->actionLocate->setText( locateText );

	    const QString sizeStatsText =
		hasPattern ? tr( "&Size statistics matching " ) % pattern : tr( "&Size statistics" );
	    _ui->actionSizeStats->setText( sizeStatsText );

	    menu.addSeparator();
	    menu.addAction( _ui->actionLocate );
	    menu.addAction( _ui->actionSizeStats );
	}
    }

    menu.exec( _ui->treeWidget->mapToGlobal( pos ) );
}


void FileTypeStatsWindow::itemActivated()
{
    QTreeWidgetItem * item = _ui->treeWidget->currentItem();
    if ( item->childCount() == 0 )
	locateCurrentFileType();
    else
	item->setExpanded( !item->isExpanded() );
}


void FileTypeStatsWindow::keyPressEvent( QKeyEvent * event )
{
    // Let return/enter trigger itemActivated instead of buttons that don't have focus
    if ( event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter )
	return;

    QDialog::keyPressEvent( event );
}


bool FileTypeStatsWindow::event( QEvent * event )
{
    if ( event->type() == QEvent::FontChange || event->type() == QEvent::Resize )
	showElidedLabel( _ui->headingLabel, this );

    return QDialog::event( event );
}




FileTypeItem::FileTypeItem( const QString   & name,
                            const CountSize & countSize,
                            FileCount         parentCount,
                            FileSize          parentSize,
                            int               treeLevel ):
    QTreeWidgetItem{ UserType },
    _count{ countSize.count },
    _totalSize{ countSize.size }
{
    /**
     * Helper function to set the text and text alignment for a column.
     **/
    const auto set = [ this ]( int col, Qt::Alignment alignment, const QString & text )
    {
	setText( col, text );
	setTextAlignment( col, alignment | Qt::AlignVCenter );
    };

    const float countPercent = parentCount ? 100.0f * _count / parentCount : 100.0f;
    const float sizePercent = parentSize ? 100.0f * _totalSize / parentSize : 100.0f;

    set( FT_NameCol,         Qt::AlignLeft,  name );
    set( FT_CountCol,        Qt::AlignRight, formatCount( _count ) );
    set( FT_CountPercentCol, Qt::AlignRight, formatPercent( countPercent ) );
    set( FT_TotalSizeCol,    Qt::AlignRight, formatSize( _totalSize ) );
    set( FT_SizePercentCol,  Qt::AlignRight, formatPercent( sizePercent ) );

    if ( _totalSize > 999 )
	setToolTip( FT_TotalSizeCol, formatByteSize( _totalSize ) );

    setData( FT_CountPercentBarCol, PercentRole,   countPercent );
    setData( FT_CountPercentBarCol, TreeLevelRole, treeLevel );

    setData( FT_SizePercentBarCol, PercentRole,   sizePercent  );
    setData( FT_SizePercentBarCol, TreeLevelRole, treeLevel );
}


bool FileTypeItem::operator<(const QTreeWidgetItem & rawOther) const
{
    if ( !treeWidget() )
	return QTreeWidgetItem::operator<( rawOther );

    // Since this is a reference, the dynamic_cast will throw a std::bad_cast
    // exception if it fails. Not catching this here since this is a genuine
    // error which should not be silently ignored.
    const FileTypeItem & other = dynamic_cast<const FileTypeItem &>( rawOther );

    // Always sort <other> at the "end" (for numeric descending or by name ascending)
    if ( treeWidget()->sortColumn() != FT_NameCol )
    {
	if ( text( FT_NameCol ) == otherName() )
	    return true;
	else if ( other.text( FT_NameCol ) == otherName() )
	    return false;
    }

    switch ( treeWidget()->sortColumn() )
    {
	case FT_CountCol:
	case FT_CountPercentBarCol:
	case FT_CountPercentCol:
	    return _count < other._count;

	case FT_TotalSizeCol:
	case FT_SizePercentBarCol:
	case FT_SizePercentCol:
	    return _totalSize < other._totalSize;

	default:
	    return QTreeWidgetItem::operator<( rawOther );
    }
}
