/*
 *   File name: FileTypeStatsWindow.cpp
 *   Summary:	QDirStat file type statistics window
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <algorithm>

#include <QKeyEvent>
#include <QMenu>
#include <QPointer>

#include "FileTypeStatsWindow.h"
#include "FileTypeStats.h"
#include "FileInfo.h"
#include "FileSizeStatsWindow.h"
#include "FormatUtil.h"
#include "HeaderTweaker.h"
#include "LocateFileTypeWindow.h"
#include "MimeCategory.h"
#include "QDirStatApp.h"
#include "SelectionModel.h"
#include "SettingsHelpers.h"
#include "Logger.h"
#include "Exception.h"


// Number of suffixes in the "other" category
#define TOP_X 20

using namespace QDirStat;


FileTypeStatsWindow::FileTypeStatsWindow( QWidget        * parent,
					  SelectionModel * selectionModel ):
    QDialog ( parent ),
    _ui { new Ui::FileTypeStatsWindow }
{
    // logDebug() << "init" << Qt::endl;

    setAttribute( Qt::WA_DeleteOnClose );

    CHECK_NEW( _ui );
    _ui->setupUi( this );

    initWidgets();
    readWindowSettings( this, "FileTypeStatsWindow" );

    // Add actions to this window to get the hotkeys
    addAction( _ui->actionLocate );
    addAction( _ui->actionSizeStats );

    connect( _ui->treeWidget,	   &QTreeWidget::currentItemChanged,
	     this,		   &FileTypeStatsWindow::enableActions );

    connect( _ui->treeWidget,	   &QTreeWidget::customContextMenuRequested,
	      this,		   &FileTypeStatsWindow::contextMenu);

    connect( _ui->treeWidget,	   &QTreeWidget::itemDoubleClicked,
	     this,		   &FileTypeStatsWindow::locateCurrentFileType );

    connect( _ui->refreshButton,   &QAbstractButton::clicked,
	     this,		   &FileTypeStatsWindow::refresh );

    connect( _ui->locateButton,	   &QAbstractButton::clicked,
	     this,		   &FileTypeStatsWindow::locateCurrentFileType );

    connect( _ui->actionLocate,	   &QAction::triggered,
	     this,		   &FileTypeStatsWindow::locateCurrentFileType );

    connect( _ui->sizeStatsButton, &QAbstractButton::clicked,
	     this,		   &FileTypeStatsWindow::sizeStatsForCurrentFileType );

    connect( _ui->actionSizeStats, &QAction::triggered,
	     this,		   &FileTypeStatsWindow::sizeStatsForCurrentFileType );

    connect( selectionModel, qOverload<FileInfo *, const FileInfo *>( &SelectionModel::currentItemChanged ),
	     this,           &FileTypeStatsWindow::syncedPopulate );
}


FileTypeStatsWindow::~FileTypeStatsWindow()
{
    //logDebug() << "destroying" << Qt::endl;

    writeWindowSettings( this, "FileTypeStatsWindow" );

    delete _stats;
    delete _ui;
}


FileTypeStatsWindow * FileTypeStatsWindow::sharedInstance( QWidget        * parent,
							   SelectionModel * selectionModel)
{
    static QPointer<FileTypeStatsWindow> _sharedInstance = nullptr;

    if ( !_sharedInstance )
    {
	_sharedInstance = new FileTypeStatsWindow( parent, selectionModel );
	CHECK_NEW( _sharedInstance );
    }

    return _sharedInstance;
}


void FileTypeStatsWindow::clear()
{
    _ui->treeWidget->clear();
    enableActions( nullptr );
}


void FileTypeStatsWindow::initWidgets()
{
    app()->setWidgetFontSize( _ui->treeWidget );

    _ui->treeWidget->setColumnCount( FT_ColumnCount );
    _ui->treeWidget->setHeaderLabels( { tr( "Name" ), tr( "Number" ), tr( "Total Size" ), tr( "Percentage" ) } );

    _ui->treeWidget->header()->setDefaultAlignment( Qt::AlignVCenter | Qt::AlignRight );
    _ui->treeWidget->headerItem()->setTextAlignment( FT_NameCol, Qt::AlignVCenter | Qt::AlignLeft );
    HeaderTweaker::resizeToContents( _ui->treeWidget->header() );
}


void FileTypeStatsWindow::refresh()
{
    populate( _subtree() );
}


void FileTypeStatsWindow::populateSharedInstance( QWidget        * mainWindow,
						  FileInfo       * subtree,
						  SelectionModel * selectionModel )
{
    if ( !subtree || !selectionModel )
        return;

    FileTypeStatsWindow * instance = sharedInstance( mainWindow, selectionModel );
    instance->populate( subtree );
    instance->_ui->treeWidget->sortByColumn( FT_TotalSizeCol, Qt::DescendingOrder );
    instance->show();
}


void FileTypeStatsWindow::syncedPopulate( FileInfo * )
{
    if ( !_ui->syncCheckBox->isChecked() )
	return;

    FileInfo * newSelection = app()->selectedDirInfoOrRoot();
    if ( newSelection != _subtree() )
        populate( newSelection );
}



void FileTypeStatsWindow::populate( FileInfo * newSubtree )
{
    clear();
    _subtree = newSubtree;

    delete _stats;
    _stats = new FileTypeStats( _subtree() );

    _ui->headingUrl->setText( _subtree.url() );

    // Don't sort until all items are added
    _ui->treeWidget->setSortingEnabled( false );

    // Create toplevel items for the categories
    QMap<const MimeCategory *, FileTypeItem *> categoryItem;
    FileTypeItem * otherCategoryItem = nullptr;

    for ( CategoryFileSizeMapIterator it = _stats->categorySumBegin(); it != _stats->categorySumEnd(); ++it )
    {
	const MimeCategory * category = it.key();
	if ( category )
	{
            // Add a category item
	    FileSize sum = it.value();
            int count = _stats->categoryCount( category );
	    FileTypeItem * catItem = addCategoryItem( category->name(), count, sum );
	    categoryItem[ category ] = catItem;

	    if ( category == _stats->otherCategory() )
	    {
		otherCategoryItem = catItem;
	    }
            else if ( _stats->categoryNonSuffixRuleCount( category ) > 0 )
	    {
		// Add an <Other> item below the category for files
		// matching any non-suffix rules
		SuffixFileTypeItem * item = addNonSuffixRuleItem( category );
		catItem->addChild( item );
	    }
	}
    }

    // Prepare to collect items for a category "other"
    QList<FileTypeItem *> otherItems;
    int	     otherCount = 0;
    FileSize otherSum	= 0LL;

    // Create items for each individual suffix (below a category)
    for ( StringFileSizeMapIterator it = _stats->suffixSumBegin(); it != _stats->suffixSumEnd(); ++it )
    {
	const QString & suffix = it.key().first;
	const MimeCategory * category = it.key().second;
	const FileSize sum = it.value();
	const int count = _stats->suffixCount( suffix, category );
	SuffixFileTypeItem * item = addSuffixFileTypeItem( suffix, count, sum );

	if ( category )
	{
	    QTreeWidgetItem * parentItem = categoryItem.value( category, nullptr );
	    if ( parentItem )
	    {
		parentItem->addChild( item );
	    }
	    else
	    {
		logError() << "ERROR: No parent category item for " << suffix << Qt::endl;
		otherItems << item;
		otherCount += count;
		otherSum   += sum;
	    }
	}
	else // No category for this suffix
	{
	    //logDebug() << "other" << Qt::endl;
	    otherItems << item;
	    otherCount += count;
	    otherSum   += sum;
	}
    }

    // Put remaining "other" items below a separate category

    if ( !otherItems.isEmpty() )
    {
	otherCategoryItem = addCategoryItem( _stats->otherCategory()->name(),
                                             otherCount,
                                             otherSum );

	const QString name = otherItems.size() > TOP_X ? tr( "Other (Top %1)" ).arg( TOP_X ) : tr( "Other" );
        otherCategoryItem->setText( 0, name );

	addTopXOtherItems( otherCategoryItem, otherItems );
    }

    _ui->treeWidget->setSortingEnabled( true );
//    _ui->treeWidget->setCurrentItem( _ui->treeWidget->topLevelItem( 0 ) );
}


FileTypeItem * FileTypeStatsWindow::addCategoryItem( const QString & name, int count, FileSize sum )
{
    const double percentage = _stats->percentage( sum );

    FileTypeItem * item = new FileTypeItem( name, count, sum, percentage );
    CHECK_NEW( item );

    _ui->treeWidget->addTopLevelItem( item );
//    item->setBold();

    return item;
}


SuffixFileTypeItem * FileTypeStatsWindow::addNonSuffixRuleItem( const MimeCategory * category )
{
    const QString  suffix = NON_SUFFIX_RULE;
    const FileSize sum    = _stats->categoryNonSuffixRuleSum  ( category );
    const int      count  = _stats->categoryNonSuffixRuleCount( category );

    SuffixFileTypeItem * item = addSuffixFileTypeItem( suffix, count, sum );

    return item;
}


SuffixFileTypeItem * FileTypeStatsWindow::addSuffixFileTypeItem( const QString & suffix,
                                                                 int             count,
                                                                 FileSize        sum )
{
    const double percentage = _stats->percentage( sum );

    SuffixFileTypeItem * item = new SuffixFileTypeItem( suffix, count, sum, percentage );
    CHECK_NEW( item );

    return item;
}


void FileTypeStatsWindow::addTopXOtherItems( FileTypeItem          * otherCategoryItem,
                                             QList<FileTypeItem *> & otherItems )
{
    FileTypeItemCompare cmp;
    std::sort( otherItems.begin(), otherItems.end(), cmp );

    const int top_x = qMin( TOP_X, otherItems.size() );

    for ( int i=0; i < top_x; ++i )
    {
        // Take the X first items out of the otherItems list
        // and add them as children of the "Other" category

        FileTypeItem * item = otherItems.takeFirst();
        otherCategoryItem->addChild( item );
    }

    if ( !otherItems.empty() )
    {
#if 1
        QStringList suffixes;

        for ( FileTypeItem * item : otherItems )
            suffixes << item->text(0);

        logDebug() << "Discarding " << (int)otherItems.size()
                   << " suffixes below <other>: "
                   << suffixes.join( ", " )
                   << Qt::endl;
#endif
        // Delete all items that are not in the top X
        qDeleteAll( otherItems );
    }
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
    LocateFileTypeWindow::populateSharedInstance( "." + suffix, _subtree() );
}


void FileTypeStatsWindow::sizeStatsForCurrentFileType()
{
    const QString suffix = currentSuffix().toLower();
    FileInfo * dir = _subtree();
    if ( suffix.isEmpty() || !dir )
        return;

    //logDebug() << "Size stats for " << suffix << Qt::endl;

    FileSizeStatsWindow::populateSharedInstance( this->parentWidget(), dir, suffix );
}


QString FileTypeStatsWindow::currentSuffix() const
{
    const SuffixFileTypeItem * current = dynamic_cast<SuffixFileTypeItem *>( _ui->treeWidget->currentItem() );
    if ( !current || current->suffix() == NO_SUFFIX || current->suffix() == NON_SUFFIX_RULE )
	return QString();

    return current->suffix();
}


void FileTypeStatsWindow::enableActions( const QTreeWidgetItem * currentItem )
{
    bool enabled = false;

    if ( currentItem )
    {
	const SuffixFileTypeItem * suffixItem = dynamic_cast<const SuffixFileTypeItem *>( currentItem );

	enabled = suffixItem && suffixItem->suffix() != NO_SUFFIX && suffixItem->suffix() != NON_SUFFIX_RULE;
    }

    _ui->locateButton->setEnabled( enabled );
    _ui->sizeStatsButton->setEnabled( enabled );
}


void FileTypeStatsWindow::contextMenu( const QPoint & pos )
{
    // See if the right click was actually on an item
    if ( !_ui->treeWidget->itemAt( pos ) )
	return;

    // The clicked item will always be the current item now
    QString suffix = currentSuffix();
    if ( suffix.isEmpty() )
	return;

    suffix.remove( 0, 1 );
    _ui->actionLocate->setText( tr( "&Locate files with suffix *" ) + suffix );
    _ui->actionSizeStats->setText( tr( "&Size statistics for suffix *" ) + suffix );

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




QString SuffixFileTypeItem::itemName( const QString & suffix )
{
    if ( suffix == NO_SUFFIX )       return QObject::tr( "<no extension>"    );
    if ( suffix == NON_SUFFIX_RULE ) return QObject::tr( "<non-suffix rule>" );
    return "*." + suffix;
}



FileTypeItem::FileTypeItem( const QString & name,
			    int		    count,
			    FileSize	    totalSize,
			    float	    percentage ):
    QTreeWidgetItem ( QTreeWidgetItem::UserType ),
    _name { name },
    _count { count },
    _totalSize { totalSize },
    _percentage { percentage }
{
    setText( FT_NameCol,       name );
    setText( FT_CountCol,      QString::number( count ) );
    setText( FT_TotalSizeCol,  formatSize( totalSize ) );
    setText( FT_PercentageCol, QString::number( percentage, 'f', 2 ) + "%" );

    setTextAlignment( FT_NameCol,       Qt::AlignVCenter | Qt::AlignLeft  );
    setTextAlignment( FT_CountCol,      Qt::AlignVCenter | Qt::AlignRight );
    setTextAlignment( FT_TotalSizeCol,  Qt::AlignVCenter | Qt::AlignRight );
    setTextAlignment( FT_PercentageCol, Qt::AlignVCenter | Qt::AlignRight );
}


bool FileTypeItem::operator<(const QTreeWidgetItem & rawOther) const
{
    // Since this is a reference, the dynamic_cast will throw a std::bad_cast
    // exception if it fails. Not catching this here since this is a genuine
    // error which should not be silently ignored.
    const FileTypeItem & other = dynamic_cast<const FileTypeItem &>( rawOther );

    const FileTypeColumns col = treeWidget() ? (FileTypeColumns)treeWidget()->sortColumn() : FT_TotalSizeCol;
    switch ( col )
    {
	case FT_CountCol:      return count()      < other.count();
	case FT_TotalSizeCol:  return totalSize()  < other.totalSize();
	case FT_PercentageCol: return percentage() < other.percentage();
	case FT_NameCol:
	case FT_ColumnCount: break;
    }

    return QTreeWidgetItem::operator<( rawOther );
}

/*
void FileTypeItem::setBold()
{
    QFont boldFont = font( 0 );
    boldFont.setBold( true );

    for ( int col=0; col < FT_ColumnCount; ++col )
	setFont( col, boldFont );
}
*/
