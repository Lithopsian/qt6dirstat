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
#include "HeaderTweaker.h"
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
	auto item = new SuffixFileTypeItem{ otherCategory, suffix, count, sum, percent };
	parent->addChild( item );
    }


    /**
     * One-time initialization of the tree widget.
     **/
    void initTree( QTreeWidget * tree )
    {
	app()->dirTreeModel()->setTreeWidgetSizes( tree );

	tree->setHeaderLabels( { QObject::tr( "Name" ),
	                         QObject::tr( "Number" ),
	                         QObject::tr( "Total Size" ),
	                         QObject::tr( "Percentage" ) } );
	tree->header()->setDefaultAlignment( Qt::AlignVCenter | Qt::AlignRight );
	tree->headerItem()->setTextAlignment( FT_NameCol, Qt::AlignVCenter | Qt::AlignLeft );
	tree->header()->setSectionResizeMode( QHeaderView::ResizeToContents );
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

    connect( _ui->treeWidget,      &QTreeWidget::itemDoubleClicked,
             this,                 &FileTypeStatsWindow::locateCurrentFileType );

    connect( _ui->refreshButton,   &QAbstractButton::clicked,
             this,                 &FileTypeStatsWindow::refresh );

    connect( _ui->locateButton,    &QAbstractButton::clicked,
             this,                 &FileTypeStatsWindow::locateCurrentFileType );

    connect( _ui->actionLocate,    &QAction::triggered,
             this,                 &FileTypeStatsWindow::locateCurrentFileType );

    connect( _ui->sizeStatsButton, &QAbstractButton::clicked,
             this,                 &FileTypeStatsWindow::sizeStatsForCurrentFileType );

    connect( _ui->actionSizeStats, &QAction::triggered,
             this,                 &FileTypeStatsWindow::sizeStatsForCurrentFileType );
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


void FileTypeStatsWindow::populateSharedInstance( QWidget        * mainWindow,
                                                  FileInfo       * subtree )
{
    if ( !subtree )
        return;

    FileTypeStatsWindow * instance = sharedInstance( mainWindow );

    // Actually faster to build the tree already sorted as we want
    instance->_ui->treeWidget->sortByColumn( FT_TotalSizeCol, Qt::DescendingOrder );
    instance->populate( subtree );
    instance->show();
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

    _ui->headingUrl->setStatusTip( _subtree.url() );
    resizeEvent( nullptr );

    // Create a map of toplevel items for finding suffix item parents
    QHash<const MimeCategory *, FileTypeItem *> categoryItem;

    FileTypeStats stats{ _subtree() };

    for ( auto it = stats.categoriesBegin(); it != stats.categoriesEnd(); ++it )
    {
	const MimeCategory * category = it.key();
	if ( category )
	{
	    // Add a category item
	    const int      count   = it.value().count;
	    const FileSize sum     = it.value().sum;
	    const float    percent = stats.percentage( sum );
	    categoryItem[ category ] =
		addCategoryItem( _ui->treeWidget, category->name(), count, sum, percent );
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

//    _ui->treeWidget->setCurrentItem( _ui->treeWidget->topLevelItem( 0 ) );
}


void FileTypeStatsWindow::locateCurrentFileType()
{
    const QString suffix = currentSuffix();

    // Can click/press on inappropriate rows
    if ( suffix.isEmpty() )
	return;

    // logDebug() << "Locating " << current->suffix() << Qt::endl;

    // Use the shared LocateFileTypeWindow instance.  Let it pick its own parent
    // so it doesn't get closed along with this window.
    LocateFileTypeWindow::populateSharedInstance( '.' % suffix, _subtree() );
}


void FileTypeStatsWindow::sizeStatsForCurrentFileType()
{
    const QString suffix = currentSuffix().toLower();
    FileInfo * dir = _subtree();
    if ( suffix.isEmpty() || !dir )
        return;

    //logDebug() << "Size stats for " << suffix << " in " << dir << Qt::endl;

    FileSizeStatsWindow::populateSharedInstance( this->parentWidget(), dir, '.' % suffix );
}


QString FileTypeStatsWindow::currentSuffix() const
{
    const SuffixFileTypeItem * item = dynamic_cast<SuffixFileTypeItem *>( _ui->treeWidget->currentItem() );
    if ( item && !item->suffix().isEmpty() )
	return item->suffix();

    return QString{};
}


void FileTypeStatsWindow::enableActions()
{
    enableActions( !currentSuffix().isEmpty() );
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
    const QString suffix = currentSuffix();
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
    if ( event->key() != Qt::Key_Return && event->key() != Qt::Key_Enter )
    {
	QDialog::keyPressEvent( event );
	return;
    }

    QTreeWidgetItem * item = _ui->treeWidget->currentItem();
    SuffixFileTypeItem * suffixItem = dynamic_cast<SuffixFileTypeItem *>( item );
    if ( suffixItem )
	// Try the locate file type window, although it may not be an appropriate suffix
	locateCurrentFileType();
    else
	// For category headings, toggle the expanded state
	item->setExpanded( !item->isExpanded() );
}


void FileTypeStatsWindow::resizeEvent( QResizeEvent * )
{
    // Calculate a width from the dialog less margins, less a bit more
    elideLabel( _ui->headingUrl, _ui->headingUrl->statusTip(), size().width() - 200 );
}




FileTypeItem::FileTypeItem( const QString & name,
                            int             count,
                            FileSize        totalSize,
                            float           percentage ):
    QTreeWidgetItem{ QTreeWidgetItem::UserType },
    _name{ name },
    _count{ count },
    _totalSize{ totalSize },
    _percentage{ percentage }
{
    set( FT_NameCol,       Qt::AlignLeft,  name );
    set( FT_CountCol,      Qt::AlignRight, QString::number( count ) );
    set( FT_TotalSizeCol,  Qt::AlignRight, formatSize( totalSize ) );
    set( FT_PercentageCol, Qt::AlignRight, QString::number( percentage, 'f', 2 ) % '%' );
}


bool FileTypeItem::operator<(const QTreeWidgetItem & rawOther) const
{
    if ( !treeWidget() )
	return QTreeWidgetItem::operator<( rawOther );

    // Since this is a reference, the dynamic_cast will throw a std::bad_cast
    // exception if it fails. Not catching this here since this is a genuine
    // error which should not be silently ignored.
    const FileTypeItem & other = dynamic_cast<const FileTypeItem &>( rawOther );

    switch ( static_cast<FileTypeColumns>( treeWidget()->sortColumn() ) )
    {
	case FT_CountCol:      return count()      < other.count();
	case FT_TotalSizeCol:  return totalSize()  < other.totalSize();
	case FT_PercentageCol: return percentage() < other.percentage();
	case FT_NameCol:
	case FT_ColumnCount: break;
    }

    return QTreeWidgetItem::operator<( rawOther );
}
