/*
 *   File name: FileAgeStatsWindow.h
 *   Summary:   QDirStat "File Age Statistics" window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QKeyEvent>
#include <QPointer>

#include "FileAgeStatsWindow.h"
#include "DirTree.h"
#include "DirTreeModel.h"
#include "DiscoverActions.h"
#include "FileAgeStats.h"
#include "FileInfo.h"
#include "FormatUtil.h"
#include "Logger.h"
#include "PercentBar.h"
#include "QDirStatApp.h"
#include "SelectionModel.h"
#include "Settings.h"


// Remember to adapt the tooltip text for the "Locate" button in the .ui file
// and the method docs in the .h file if this value is changed
#define MAX_LOCATE_FILES  10000


using namespace QDirStat;


namespace
{
    /**
     * Return the number of recent years for which to display monthly
     * statistics.  This is configurable by manually editing the config
     * file.
     **/
    int yearsWithMonths()
    {
	const int defaultValue = 5;

	Settings settings;
	settings.beginGroup( "FileAgeStatsWindow" );
	const int yearsWithMonths = settings.value( "YearsWithMonths", defaultValue ).toInt();
	settings.setDefaultValue( "YearsWithMonths", defaultValue );
	settings.endGroup();

	return yearsWithMonths;
    }


    /**
     * Read settings from the config file
     **/
    void readSettings( FileAgeStatsWindow * window, QCheckBox * syncCheckBox  )
    {
	Settings settings;

	settings.beginGroup( "FileAgeStatsWindow" );
	syncCheckBox->setChecked( settings.value( "SyncWithMainWindow", true ).toBool() );
	settings.endGroup();

	Settings::readWindowSettings( window, "FileAgeStatsWindow" );
    }


    /**
     * Write settings to the config file
     **/
    void writeSettings( const FileAgeStatsWindow * window, bool sync )
    {
	Settings settings;

	settings.beginGroup( "FileAgeStatsWindow" );
	settings.setValue( "SyncWithMainWindow", sync );
	settings.endGroup();

	Settings::writeWindowSettings( window, "FileAgeStatsWindow" );
    }


    /**
     * Returns the current item in 'tree', cast as a YearListItem,
     * or 0 if there is no current item or it is not a YearListItem.
     **/
    const YearListItem * currentItem( const QTreeWidget * treeWidget)
    {
	return dynamic_cast<const YearListItem *>( treeWidget->currentItem() );
    }


    /**
     * Returns whether a locate could be performed for the current selection.
     **/
    bool canLocate( const YearListItem * item )
    {
	return item && item->count() > 0 && item->count() <= MAX_LOCATE_FILES;
    }


    /**
     * One-time initialization of the widgets in this window.
     **/
    void initTree( QTreeWidget * tree )
    {
	QTreeWidgetItem * headerItem = tree->headerItem();
	headerItem->setText( YL_YearMonthCol,       QObject::tr( "Year" ) );
	headerItem->setText( YL_FilesCountCol,      QObject::tr( "Files" ) );
	headerItem->setText( YL_FilesPercentBarCol, QObject::tr( "Files %" ) );
	headerItem->setText( YL_FilesPercentCol,    "%" );
	headerItem->setText( YL_SizeCol,            QObject::tr( "Total Size" ) );
	headerItem->setText( YL_SizePercentBarCol,  QObject::tr( "Size %" ) );
	headerItem->setText( YL_SizePercentCol,     "%" );

	QHeaderView * header = tree->header();
	header->setDefaultAlignment( Qt::AlignCenter );
	header->setSectionResizeMode( QHeaderView::ResizeToContents );

	tree->sortByColumn( YL_YearMonthCol, Qt::DescendingOrder );

	const int height = app()->dirTreeModel()->dirTreeIconSize().height();
	PercentBarDelegate::createStatsDelegates( tree, height, YL_FilesPercentBarCol, YL_SizePercentBarCol );
    }


    /**
     * Create an item in the years tree / list widget for each year,
     * including "gaps", empty months and years newer than the oldest
     * year.
     **/
    void populateTree( FileInfo * fileInfo, QTreeWidget * treeWidget, short yearsWithMonths )
    {
	const FileAgeStats stats{ fileInfo };

	/**
	 * Create a YearListItem and all its children for 'year'.
	 **/
	const auto createYearItem = [ &stats, treeWidget, yearsWithMonths ]( short year )
	{
	    // Missing years (ie. no files) will be filled in with disabled entries later
	    if ( !stats.yearStatsAvailable( year ) )
		return;

	    const YearMonthStats yearStats = stats.yearStats( year );

	    const FileCount yearCount        = yearStats.count;
	    const float     yearCountPercent = stats.countPercent( yearCount );
	    const FileSize  yearSize         = yearStats.size;
	    const float     yearSizePercent  = stats.sizePercent( yearSize );

	    YearListItem * item =
		new YearListItem{ year, 0, yearCount, yearCountPercent, yearSize, yearSizePercent };
	    treeWidget->addTopLevelItem( item );

	    // Only display months for a set number of recent years
	    if ( year <= stats.thisYear() - yearsWithMonths )
		return;

	    // Loop through all the months, even those that have no files
	    const short lastMonthOfYear = year == stats.thisYear() ? stats.thisMonth() : 12;
	    for ( short month = 1; month <= lastMonthOfYear; month++ )
	    {
		const YearMonthStats monthStats = stats.monthStats( year, month );

		const FileCount count        = monthStats.count;
		const float     countPercent = yearCount == 0 ? 100.0f : 100.0f * count / yearCount;
		const FileSize  size         = monthStats.size;
		const float     sizePercent  = yearSize == 0 ? 100.0f : 100.0f * size / yearSize;

		item->addChild( new YearListItem{ year, month, count, countPercent, size, sizePercent } );
	    }
	};

	// Loop through all the years for which files were found
	const auto years = stats.years();
	for ( short year : years )
	    createYearItem( year );

	// Create empty entries for years which didn't have any files
	if ( treeWidget->topLevelItemCount() > 0 )
	{
	    // Select the first row before filling gaps, so an enabled row is selected
	    treeWidget->setCurrentItem( treeWidget->topLevelItem( 0 ) );

	    const short firstYear = *std::min_element( years.cbegin(), years.cend() );
	    for ( short year = firstYear; year <= stats.thisYear(); ++year )
	    {
		if ( !years.contains( year ) )
		    treeWidget->addTopLevelItem( new YearListItem{ year } );
	    }
	}
    }

} // namespace


FileAgeStatsWindow::FileAgeStatsWindow( QWidget * parent ):
    QDialog{ parent },
    _ui{ new Ui::FileAgeStatsWindow }
{
    // logDebug() << "init" << Qt::endl;

    setAttribute( Qt::WA_DeleteOnClose );

    _ui->setupUi( this );

    initTree( _ui->treeWidget );

    readSettings( this, _ui->syncCheckBox );

    const DirTree        * dirTree        = app()->dirTree();
    const SelectionModel * selectionModel = app()->selectionModel();

    connect( dirTree,            &DirTree::aborted,
             this,               &FileAgeStatsWindow::syncedRefresh );

    connect( dirTree,            &DirTree::finished,
             this,               &FileAgeStatsWindow::syncedRefresh );

    connect( selectionModel,     &SelectionModel::currentItemChanged,
             this,               &FileAgeStatsWindow::syncedPopulate );

    connect( _ui->refreshButton, &QPushButton::clicked,
             this,               &FileAgeStatsWindow::refresh );

    connect( _ui->locateButton,  &QPushButton::clicked,
             this,               &FileAgeStatsWindow::locateFiles );

    connect( _ui->treeWidget,    &QTreeWidget::itemActivated,
             this,               &FileAgeStatsWindow::itemActivated );

    connect( _ui->treeWidget,    &QTreeWidget::currentItemChanged,
             this,               &FileAgeStatsWindow::enableActions );

    show();
}


FileAgeStatsWindow::~FileAgeStatsWindow()
{
    //logDebug() << "destroying" << Qt::endl;

    writeSettings( this, _ui->syncCheckBox->isChecked() );
}


FileAgeStatsWindow * FileAgeStatsWindow::sharedInstance( QWidget * parent )
{
    static QPointer<FileAgeStatsWindow> _sharedInstance;

    if ( !_sharedInstance )
	_sharedInstance = new FileAgeStatsWindow{ parent };

    return _sharedInstance;
}


void FileAgeStatsWindow::refresh()
{
    populate( _subtree() );
}


void FileAgeStatsWindow::syncedRefresh()
{
    if ( _ui->syncCheckBox->isChecked() )
	refresh();
}


void FileAgeStatsWindow::syncedPopulate()
{
    // Refresh if the checkbox is set
    if ( !_ui->syncCheckBox->isChecked() )
	return;

    FileInfo * newSelection = app()->currentDirInfo();
    if ( newSelection != _subtree() )
	populate( newSelection );
}


void FileAgeStatsWindow::populate( FileInfo * fileInfo )
{
    //logDebug() << "populating with " << fileInfo << Qt::endl;

    _ui->treeWidget->clear();

    if ( !fileInfo )
	return;

    _subtree = fileInfo;

    _ui->headingLabel->setStatusTip( tr( "File age statistics for " ) % replaceCrLf( _subtree.url() ) );
    showElidedLabel( _ui->headingLabel, this );

    populateTree(_subtree(), _ui->treeWidget, yearsWithMonths() );

    enableActions();
}


void FileAgeStatsWindow::locateFiles()
{
    const YearListItem * item = currentItem( _ui->treeWidget );
    if ( canLocate( item ) && item->year() > 0 )
    {
	if ( item->month() > 0 )
	    DiscoverActions::discoverFilesFromMonth( _subtree.url(), item->year(), item->month() );
	else
	    DiscoverActions::discoverFilesFromYear( _subtree.url(), item->year() );
    }
}


void FileAgeStatsWindow::enableActions()
{
    _ui->locateButton->setEnabled( canLocate( currentItem( _ui->treeWidget ) ) );
}


void FileAgeStatsWindow::itemActivated( QTreeWidgetItem * item, int )
{
    if ( item->childCount() == 0 )
	locateFiles();
    else
	item->setExpanded( !item->isExpanded() );
}


void FileAgeStatsWindow::keyPressEvent( QKeyEvent * event )
{
    // Let return/enter trigger itemActivated instead of buttons that don't have focus
    if ( event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter )
	return;

    QDialog::keyPressEvent( event );
}


bool FileAgeStatsWindow::event( QEvent * event )
{
    if ( event->type() == QEvent::FontChange || event->type() == QEvent::Resize )
	showElidedLabel( _ui->headingLabel, this );

    return QDialog::event( event );
}




YearListItem::YearListItem( short     year,
                            short     month,
                            FileCount count,
                            float     countPercent,
                            FileSize  size,
                            float     sizePercent ):
    QTreeWidgetItem{ UserType },
    _year{ year },
    _month{ month },
    _count{ count },
    _size{ size }
{
    /**
     * Helper function to set the text and text alignment for a column.
     **/
    const auto set = [ this ]( int col, Qt::Alignment alignment, const QString & text )
    {
	setText( col, text );
	setTextAlignment( col, alignment | Qt::AlignVCenter );
    };

    const bool monthItem = _month > 0;
    const QString text = monthItem ? monthAbbreviation( _month ) : QString::number( _year );
    set( YL_YearMonthCol, Qt::AlignLeft, text );

    if ( _count > 0 )
    {
	set( YL_FilesCountCol,   Qt::AlignRight, formatCount  ( count        ) );
	set( YL_FilesPercentCol, Qt::AlignRight, formatPercent( countPercent ) );
	set( YL_SizeCol,         Qt::AlignRight, formatSize   ( size         ) );
	set( YL_SizePercentCol,  Qt::AlignRight, formatPercent( sizePercent  ) );

	if ( size > 999 )
	    setToolTip( YL_SizeCol, formatByteSize( size ) );

	setData( YL_FilesPercentBarCol, PercentRole, countPercent );
	setData( YL_SizePercentBarCol,  PercentRole, sizePercent  );

	const int treeLevel = monthItem ? 1 : 0;
	setData( YL_FilesPercentBarCol, TreeLevelRole, treeLevel );
	setData( YL_SizePercentBarCol,  TreeLevelRole, treeLevel );
    }
    else
    {
	setFlags( Qt::NoItemFlags );
    }
}


bool YearListItem::operator<( const QTreeWidgetItem & rawOther ) const
{
    if ( !treeWidget() )
	return QTreeWidgetItem::operator<( rawOther );

    // Since this is a reference, the dynamic_cast will throw a std::bad_cast
    // exception if it fails. Not catching this here since this is a genuine
    // error which should not be silently ignored.
    const YearListItem & other = dynamic_cast<const YearListItem &>( rawOther );

    switch ( treeWidget()->sortColumn() )
    {
	case YL_YearMonthCol:
	    return _month > 0 ? _month < other._month : _year  < other._year;

	case YL_FilesCountCol:
	case YL_FilesPercentBarCol:
	case YL_FilesPercentCol:
	    return _count < other._count;

	case YL_SizeCol:
	case YL_SizePercentBarCol:
	case YL_SizePercentCol:
	    return _size < other._size;

	default:
	    return QTreeWidgetItem::operator<( rawOther );
    }
}
