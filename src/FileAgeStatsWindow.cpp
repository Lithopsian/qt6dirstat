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
#include "FileInfo.h"
#include "FormatUtil.h"
#include "HeaderTweaker.h"
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
     * Returns whether a locate could be performed for the current selection.
     **/
    bool canLocate( const YearListItem * item )
    {
	return item && item->filesCount() > 0 && item->filesCount() <= MAX_LOCATE_FILES;
    }


    /**
     * Find the gaps between years.
     **/
    YearsList findGaps( const FileAgeStats & stats, bool startGapsWithCurrentYear )
    {
	YearsList gaps;

	const YearsList & years = stats.years(); // sorted in ascending order
	if ( years.isEmpty() )
	    return gaps;

	const short lastYear = startGapsWithCurrentYear ? stats.thisYear() : years.last();
	if ( lastYear - years.first() == years.count() - 1 )
	    return gaps;

	for ( short yr = years.first(); yr <= lastYear; yr++ )
	{
	    if ( !years.contains( yr ) )
		gaps << yr;
	}

	return gaps;
    }

}


FileAgeStatsWindow::FileAgeStatsWindow( QWidget * parent ):
    QDialog{ parent },
    _ui{ new Ui::FileAgeStatsWindow }
{
    // logDebug() << "init" << Qt::endl;

    setAttribute( Qt::WA_DeleteOnClose );

    _ui->setupUi( this );

    initWidgets();
    readSettings();

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

    connect( _ui->treeWidget,    &QTreeWidget::itemDoubleClicked,
             this,               &FileAgeStatsWindow::locateFiles );

    connect( _ui->treeWidget,    &QTreeWidget::itemSelectionChanged,
             this,               &FileAgeStatsWindow::enableActions );

    connect( this, &FileAgeStatsWindow::locateFilesFromYear,  &DiscoverActions::discoverFilesFromYear );

    connect( this, &FileAgeStatsWindow::locateFilesFromMonth, &DiscoverActions::discoverFilesFromMonth );
}


FileAgeStatsWindow::~FileAgeStatsWindow()
{
    //logDebug() << "destroying" << Qt::endl;

    writeSettings();
}


FileAgeStatsWindow * FileAgeStatsWindow::sharedInstance( QWidget * parent )
{
    static QPointer<FileAgeStatsWindow> _sharedInstance;

    if ( !_sharedInstance )
	_sharedInstance = new FileAgeStatsWindow{ parent };

    return _sharedInstance;
}


void FileAgeStatsWindow::initWidgets()
{
    // Set the row height based on the configured DirTree icon height
    app()->dirTreeModel()->setTreeWidgetSizes( _ui->treeWidget );

    const QStringList headers{ tr( "Year"    ),
                               tr( "Files"   ),
                               tr( "Files %" ),  // percent bar
                               tr( "%"       ),  // percent value
                               tr( "Size"    ),
                               tr( "Size %"  ),  // percent bar
                               tr( "%"       ),  // percent value
                             };
    _ui->treeWidget->setHeaderLabels( headers );
    _ui->treeWidget->header()->setDefaultAlignment( Qt::AlignCenter );
    HeaderTweaker::resizeToContents( _ui->treeWidget->header() );
}


void FileAgeStatsWindow::populateSharedInstance( QWidget        * mainWindow,
                                                 FileInfo       * fileInfo )
{
    if ( !fileInfo )
        return;

    // Get the shared instance, creating it if necessary
    FileAgeStatsWindow * instance = sharedInstance( mainWindow );

    instance->populate( fileInfo );
    instance->_ui->treeWidget->sortByColumn( YearListYearCol, Qt::DescendingOrder );
    instance->show();
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

    _ui->headingUrl->setStatusTip( _subtree.url() );
    resizeEvent( nullptr );

    // For better Performance: disable sorting while inserting (not!) many items
    _ui->treeWidget->setSortingEnabled( false );
    populateListWidget(_subtree() );
    _ui->treeWidget->setSortingEnabled( true );

    enableActions();
}


void FileAgeStatsWindow::populateListWidget( FileInfo * fileInfo )
{
    FileAgeStats stats( fileInfo );

    for ( short year : stats.years() )
    {
	YearStats * yearStats = stats.yearStats( year );
	if ( yearStats )
	{
	    // Add a year item
	    YearListItem * item = new YearListItem{ *yearStats };
	    _ui->treeWidget->addTopLevelItem( item );

	    // Add the month items if applicable
	    if ( stats.monthStatsAvailableFor( year ) )
	    {
		for ( short month = 1; month <= 12; month++ )
		{
		    YearStats * monthStats = stats.monthStats( year, month );
		    if ( monthStats )
		    {
			YearListItem * monthItem = new YearListItem{ *monthStats };
			item->addChild( monthItem );

			if ( monthStats->filesCount == 0 )
			    monthItem->setFlags( Qt::NoItemFlags ); // disabled
		    }
		}
	    }
	}
    }

    fillGaps( stats );
}


void FileAgeStatsWindow::fillGaps( const FileAgeStats & stats )
{
    const auto gaps = findGaps( stats, _startGapsWithCurrentYear );
    for ( short year : gaps )
    {
	YearListItem * item = new YearListItem{ YearStats{ year } };
	item->setFlags( Qt::NoItemFlags ); // disabled
	_ui->treeWidget->addTopLevelItem( item );
    }
}


const YearListItem * FileAgeStatsWindow::selectedItem() const
{
    return dynamic_cast<const YearListItem *>( _ui->treeWidget->currentItem() );
}


void FileAgeStatsWindow::locateFiles()
{
    const YearListItem * item = selectedItem();
    if ( item && canLocate( item ) )
    {
	const short month = item->month();
	const short year  = item->year();

	if ( month > 0 && year > 0 )
	    emit locateFilesFromMonth( _subtree.url(), year, month );
	else if ( year > 0 )
	    emit locateFilesFromYear( _subtree.url(), year );
    }
}


void FileAgeStatsWindow::enableActions()
{
    _ui->locateButton->setEnabled( canLocate( selectedItem() ) );
}


void FileAgeStatsWindow::readSettings()
{
    Settings settings;

    settings.beginGroup( "FileAgeStatsWindow" );

    _ui->syncCheckBox->setChecked( settings.value( "SyncWithMainWindow",       true ).toBool() );
    _startGapsWithCurrentYear    = settings.value( "StartGapsWithCurrentYear", true ).toBool();
    int percentBarWidth          = settings.value( "PercentBarWidth",          120  ).toInt();

    const QColor percentBarBackground =
	settings.colorValue    ( "PercentBarBackground",  QColor{ 160, 160, 160 } );
    const ColorList filesPercentBarColors =
	settings.colorListValue( "FilesPercentBarColors", { 0xbb0000, 0x00aa00 } );
    const ColorList sizePercentBarColors =
	settings.colorListValue( "SizePercentBarColors",  { 0xee0000, 0x00cc00 } );

    settings.setDefaultValue( "StartGapsWithCurrentYear", _startGapsWithCurrentYear );
    settings.setDefaultValue( "PercentBarWidth",          percentBarWidth           );
    settings.setDefaultValue( "PercentBarBackground",     percentBarBackground  );
    settings.setDefaultValue( "FilesPercentBarColors",    filesPercentBarColors );
    settings.setDefaultValue( "SizePercentBarColors",     sizePercentBarColors  );

    settings.endGroup();

    Settings::readWindowSettings( this, "FileAgeStatsWindow" );

    // Delegates for the two percent bars
    _filesPercentBarDelegate = new PercentBarDelegate{ _ui->treeWidget,
                                                       YearListFilesPercentBarCol,
                                                       percentBarWidth,
                                                       percentBarBackground,
                                                       filesPercentBarColors };
    _ui->treeWidget->setItemDelegateForColumn( YearListFilesPercentBarCol, _filesPercentBarDelegate );

    _sizePercentBarDelegate = new PercentBarDelegate{ _ui->treeWidget,
                                                      YearListSizePercentBarCol,
                                                      percentBarWidth,
                                                      percentBarBackground,
                                                      sizePercentBarColors };
    _ui->treeWidget->setItemDelegateForColumn( YearListSizePercentBarCol, _sizePercentBarDelegate );
}


void FileAgeStatsWindow::writeSettings()
{
    Settings settings;

    settings.beginGroup( "FileAgeStatsWindow" );
    settings.setValue( "SyncWithMainWindow", _ui->syncCheckBox->isChecked() );
    settings.endGroup();

    Settings::writeWindowSettings( this, "FileAgeStatsWindow" );
}


void FileAgeStatsWindow::keyPressEvent( QKeyEvent * event )
{
    if ( event->key() != Qt::Key_Return && event->key() != Qt::Key_Enter )
    {
	QDialog::keyPressEvent( event );
	return;
    }

    QTreeWidgetItem * item = _ui->treeWidget->currentItem();
    YearListItem * listItem = dynamic_cast<YearListItem *>( item );
    if ( listItem && listItem->childCount() > 0 )
	item->setExpanded( !item->isExpanded() );
    else
	locateFiles();
}


void FileAgeStatsWindow::resizeEvent( QResizeEvent * )
{
    // Calculate a width from the dialog less margins, less a bit more
    elideLabel( _ui->headingUrl, _ui->headingUrl->statusTip(), size().width() - 200 );
}




YearListItem::YearListItem( const YearStats & stats ) :
    QTreeWidgetItem{ QTreeWidgetItem::UserType },
    _year{ stats.year },
    _month{ stats.month },
    _filesCount{ stats.filesCount },
    _filesPercent{ stats.filesPercent },
    _size{ stats.size },
    _sizePercent{ stats.sizePercent }
{
    const bool monthItem = _month > 0;
    const QString text = monthItem ? monthAbbreviation( _month ) : QString::number( _year );
    set( YearListYearCol, Qt::AlignLeft, text );

    if ( _filesCount > 0 )
    {
	set( YearListFilesCountCol,   Qt::AlignRight, QString::number( _filesCount   ) );
	set( YearListFilesPercentCol, Qt::AlignRight, formatPercent  ( _filesPercent ) );
	set( YearListSizeCol,         Qt::AlignRight, "    " + formatSize( _size     ) );
	set( YearListSizePercentCol,  Qt::AlignRight, formatPercent  ( _sizePercent  ) );

	setData( YearListFilesPercentBarCol, PercentRole, _filesPercent );
	setData( YearListSizePercentBarCol,  PercentRole, _sizePercent  );

	const bool treeLevel = monthItem ? 1 : 0;
	setData( YearListFilesPercentBarCol, TreeLevelRole, treeLevel );
	setData( YearListSizePercentBarCol,  TreeLevelRole, treeLevel );
    }
}


bool YearListItem::operator<( const QTreeWidgetItem & rawOther ) const
{
    if ( !treeWidget() )
	QTreeWidgetItem::operator<( rawOther );

    // Since this is a reference, the dynamic_cast will throw a std::bad_cast
    // exception if it fails. Not catching this here since this is a genuine
    // error which should not be silently ignored.
    const YearListItem & other = dynamic_cast<const YearListItem &>( rawOther );

    switch ( (YearListColumns)treeWidget()->sortColumn() )
    {
	case YearListYearCol:
	    if ( _month > 0 )
		return _month < other._month;
	    else
		return _year  < other._year;

	case YearListFilesCountCol:     return _filesCount   < other._filesCount;
	case YearListFilesPercentBarCol:
	case YearListFilesPercentCol:   return _filesPercent < other._filesPercent;
	case YearListSizeCol:           return _size         < other._size;
	case YearListSizePercentBarCol:
	case YearListSizePercentCol:    return _sizePercent  < other._sizePercent;
	case YearListColumnCount:       break;
    }

    return QTreeWidgetItem::operator<( rawOther );
}
