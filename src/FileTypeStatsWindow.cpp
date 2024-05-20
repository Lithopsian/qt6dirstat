/*
 *   File name: FileTypeStatsWindow.cpp
 *   Summary:   QDirStat file type statistics window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <algorithm>	// std::sort

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
#include "Exception.h"
#include "Logger.h"


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

    connect( _ui->treeWidget,      &QTreeWidget::currentItemChanged,
	     this,                 &FileTypeStatsWindow::enableActions );

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

    connect( selectionModel,       &SelectionModel::currentItemChanged,
	     this,                 &FileTypeStatsWindow::syncedPopulate );
}


FileTypeStatsWindow::~FileTypeStatsWindow()
{
    //logDebug() << "destroying" << Qt::endl;

    writeWindowSettings( this, "FileTypeStatsWindow" );

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

    _ui->headingUrl->setStatusTip( _subtree.url() );

    // Don't sort until all items are added
    _ui->treeWidget->setSortingEnabled( false );

    // Create toplevel items for the categories
    QHash<const MimeCategory *, FileTypeItem *> categoryItem;
    FileTypeItem * otherCategoryItem = nullptr;

    FileTypeStats stats( _subtree() );
    for ( CategoryFileSizeMapIterator it = stats.categorySumBegin(); it != stats.categorySumEnd(); ++it )
    {
	const MimeCategory * category = it.key();
	if ( category )
	{
            // Add a category item
	    const FileSize   sum     = it.value();
            const int        count   = stats.categoryCount( category );
	    FileTypeItem   * catItem = addCategoryItem( category->name(), count, sum, stats.percentage( sum ) );
	    categoryItem[ category ] = catItem;

	    if ( category == stats.otherCategory() )
	    {
		otherCategoryItem = catItem;
	    }
            else if ( stats.categoryNonSuffixRuleCount( category ) > 0 )
	    {
		// Add an <Other> item below the category for files
		// matching any non-suffix rules
		const int      count   = stats.categoryNonSuffixRuleCount( category );
		const FileSize sum     = stats.categoryNonSuffixRuleSum  ( category );
		const float    percent = stats.percentage( sum );

		SuffixFileTypeItem * item = addSuffixFileTypeItem( NON_SUFFIX_RULE, count, sum, percent );
		catItem->addChild( item );
	    }
	}
    }

    // Prepare to collect items for a category "other"
    QList<FileTypeItem *> otherItems;
    int      otherCount = 0;
    FileSize otherSum   = 0LL;

    // Create items for each individual suffix (below a category)
    for ( StringFileSizeMapIterator it = stats.suffixSumBegin(); it != stats.suffixSumEnd(); ++it )
    {
	const QString      & suffix   = it.key().first;
	const MimeCategory * category = it.key().second;
	const FileSize       sum      = it.value();
	const int            count    = stats.suffixCount( suffix, category );
	const float          percent  = stats.percentage( sum );
	SuffixFileTypeItem * item     = addSuffixFileTypeItem( suffix, count, sum, percent );

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
	otherCategoryItem = addCategoryItem( stats.otherCategory()->name(),
                                             otherCount,
                                             otherSum,
					     stats.percentage( otherSum ) );

	const QString name = otherItems.size() > TOP_X ? tr( "Other (top %1)" ).arg( TOP_X ) : tr( "Other" );
	otherCategoryItem->setText( 0, name );

	addTopXOtherItems( otherCategoryItem, otherItems );
    }

    _ui->treeWidget->setSortingEnabled( true );
//    _ui->treeWidget->setCurrentItem( _ui->treeWidget->topLevelItem( 0 ) );
}


FileTypeItem * FileTypeStatsWindow::addCategoryItem( const QString & name,
						     int count,
						     FileSize sum,
						     float percent )
{
    FileTypeItem * item = new FileTypeItem( name, count, sum, percent );
    CHECK_NEW( item );

    _ui->treeWidget->addTopLevelItem( item );

    return item;
}


SuffixFileTypeItem * FileTypeStatsWindow::addSuffixFileTypeItem( const QString & suffix,
                                                                 int             count,
                                                                 FileSize        sum,
                                                                 float           percent )
{
    SuffixFileTypeItem * item = new SuffixFileTypeItem( suffix, count, sum, percent );
    CHECK_NEW( item );

    return item;
}


void FileTypeStatsWindow::addTopXOtherItems( FileTypeItem          * otherCategoryItem,
                                             QList<FileTypeItem *> & otherItems )
{
    std::sort( otherItems.begin(), otherItems.end(), FileTypeItemCompare() );

    const int topX = qMin( TOP_X, otherItems.size() );
    for ( int i=0; i < topX; ++i )
    {
        // Take the X first items out of the otherItems list
        // and add them as children of the "Other" category
        FileTypeItem * item = otherItems.takeFirst();
        otherCategoryItem->addChild( item );
    }

    if ( !otherItems.empty() )
    {
#if 0
        QStringList suffixes;

        for ( FileTypeItem * item : otherItems )
            suffixes << item->text(0);

        logDebug() << "Discarding " << (int)otherItems.size()
                   << " suffixes below <other>: "
                   << suffixes.join( QLatin1String( ", " ) )
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
    LocateFileTypeWindow::populateSharedInstance( '.' + suffix, _subtree() );
}


void FileTypeStatsWindow::sizeStatsForCurrentFileType()
{
    const QString suffix = currentSuffix().toLower();
    FileInfo * dir = _subtree();
    if ( suffix.isEmpty() || !dir )
        return;

    //logDebug() << "Size stats for " << suffix << " in " << dir << Qt::endl;

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
    const QString suffix = currentSuffix();
    if ( suffix.isEmpty() )
	return;

    _ui->actionLocate->setText( tr( "&Locate files with suffix ." ) + suffix );
    _ui->actionSizeStats->setText( tr( "&Size statistics for suffix ." ) + suffix );

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


void FileTypeStatsWindow::resizeEvent( QResizeEvent * event )
{
    // Calculate a width from the dialog less margins, less a bit more
    elideLabel( _ui->headingUrl, _ui->headingUrl->statusTip(), event->size().width() - 200 );
}




QString SuffixFileTypeItem::itemName( const QString & suffix )
{
    if ( suffix == NO_SUFFIX )       return QObject::tr( "<no extension>"    );
    if ( suffix == NON_SUFFIX_RULE ) return QObject::tr( "<non-suffix rule>" );
    return "*." + suffix;
}



FileTypeItem::FileTypeItem( const QString & name,
			    int             count,
			    FileSize        totalSize,
			    float           percentage ):
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
    if ( !treeWidget() )
	return QTreeWidgetItem::operator<( rawOther );

    // Since this is a reference, the dynamic_cast will throw a std::bad_cast
    // exception if it fails. Not catching this here since this is a genuine
    // error which should not be silently ignored.
    const FileTypeItem & other = dynamic_cast<const FileTypeItem &>( rawOther );

    switch ( (FileTypeColumns)treeWidget()->sortColumn() )
    {
	case FT_CountCol:      return count()      < other.count();
	case FT_TotalSizeCol:  return totalSize()  < other.totalSize();
	case FT_PercentageCol: return percentage() < other.percentage();
	case FT_NameCol:
	case FT_ColumnCount: break;
    }

    return QTreeWidgetItem::operator<( rawOther );
}
