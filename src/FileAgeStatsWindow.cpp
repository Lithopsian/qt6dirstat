/*
 *   File name: FileAgeStatsWindow.h
 *   Summary:   QDirStat "File Age Statistics" window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QPointer>
#include <QKeyEvent>

#include "FileAgeStatsWindow.h"
#include "FileAgeStats.h"
#include "DiscoverActions.h"
#include "FileInfo.h"
#include "FormatUtil.h"
#include "HeaderTweaker.h"
#include "PercentBar.h"
#include "QDirStatApp.h"
#include "SelectionModel.h"
#include "Settings.h"
#include "SettingsHelpers.h"
#include "Exception.h"
#include "Logger.h"


// Remember to adapt the tooltip text for the "Locate" button in the .ui file
// and the method docs in the .h file if this value is changed
#define MAX_LOCATE_FILES        10000


using namespace QDirStat;


FileAgeStatsWindow::FileAgeStatsWindow( QWidget        * parent,
					SelectionModel * selectionModel ):
    QDialog ( parent ),
    _ui { new Ui::FileAgeStatsWindow }
{
    // logDebug() << "init" << Qt::endl;

    setAttribute( Qt::WA_DeleteOnClose );

    CHECK_NEW( _ui );
    _ui->setupUi( this );

    initWidgets();
    readSettings();

    connect( selectionModel,     qOverload<FileInfo *, const FileInfo *>( &SelectionModel::currentItemChanged ),
             this,               &FileAgeStatsWindow::syncedPopulate );

    connect( _ui->refreshButton, &QPushButton::clicked,
             this,               &FileAgeStatsWindow::refresh );

    connect( _ui->locateButton,  &QPushButton::clicked,
             this,               &FileAgeStatsWindow::locateFiles );

    connect( _ui->treeWidget,    &QTreeWidget::itemDoubleClicked,
             this,               &FileAgeStatsWindow::locateFiles );

    connect( _ui->treeWidget,    &QTreeWidget::itemSelectionChanged,
             this,               &FileAgeStatsWindow::enableActions );

    connect( this, &FileAgeStatsWindow::locateFilesFromYear, &DiscoverActions::discoverFilesFromYear );

    connect( this, &FileAgeStatsWindow::locateFilesFromMonth, &DiscoverActions::discoverFilesFromMonth );
}


FileAgeStatsWindow::~FileAgeStatsWindow()
{
    //logDebug() << "destroying" << Qt::endl;

    writeSettings();

    delete _stats;
    delete _ui;
}


FileAgeStatsWindow * FileAgeStatsWindow::sharedInstance( QWidget         * parent,
							 SelectionModel  * selectionModel )
{
    static QPointer<FileAgeStatsWindow> _sharedInstance = nullptr;

    if ( !_sharedInstance )
    {
	_sharedInstance = new FileAgeStatsWindow( parent, selectionModel );
	CHECK_NEW( _sharedInstance );
    }

    return _sharedInstance;
}


void FileAgeStatsWindow::clear()
{
    _ui->treeWidget->clear();
}


void FileAgeStatsWindow::refresh()
{
    populate( _subtree() );
}


void FileAgeStatsWindow::initWidgets()
{
    // Set the row height based on the configured DirTree icon height
    app()->setWidgetFontSize( _ui->treeWidget );

    const QStringList headers = { tr( "Year"    ),
                                  tr( "Files"   ),
                                  tr( "Files %" ),  // percent bar
                                  tr( "%"       ),  // percent value
                                  tr( "Size"    ),
                                  tr( "Size %"  ),  // percent bar
                                  tr( "%"       ),  // percent value
                                };
    _ui->treeWidget->setHeaderLabels( headers );

    // Delegates for the two percent bars
    _filesPercentBarDelegate = new PercentBarDelegate( _ui->treeWidget, YearListFilesPercentBarCol, 4, 1 );
    CHECK_NEW( _filesPercentBarDelegate );
    _ui->treeWidget->setItemDelegateForColumn( YearListFilesPercentBarCol, _filesPercentBarDelegate );

    _sizePercentBarDelegate = new PercentBarDelegate( _ui->treeWidget, YearListSizePercentBarCol, 0, 1 );
    CHECK_NEW( _sizePercentBarDelegate );
    _ui->treeWidget->setItemDelegateForColumn( YearListSizePercentBarCol, _sizePercentBarDelegate );

    // Center the column headers
    _ui->treeWidget->header()->setDefaultAlignment( Qt::AlignCenter );

    HeaderTweaker::resizeToContents( _ui->treeWidget->header() );
}


void FileAgeStatsWindow::populateSharedInstance( QWidget        * mainWindow,
						 FileInfo       * subtree,
						 SelectionModel * selectionModel )
{
    if ( !subtree || !selectionModel )
        return;

    // Get the shared instance, creating it if necessary
    FileAgeStatsWindow * instance = sharedInstance( mainWindow, selectionModel );

    instance->populate( subtree );
    instance->_ui->treeWidget->sortByColumn( YearListYearCol, Qt::DescendingOrder );
    instance->show();
}


void FileAgeStatsWindow::syncedPopulate( FileInfo * )
{
    // Refresh if the checkbox is set
    if ( !_ui->syncCheckBox->isChecked() )
	return;

    FileInfo * newSelection = app()->selectedDirInfoOrRoot();
    if ( newSelection != _subtree() )
        populate( newSelection );
}


void FileAgeStatsWindow::populate( FileInfo * newSubtree )
{
    //logDebug() << "populating with " << newSubtree << Qt::endl;

    clear();
    _subtree = newSubtree;

    _ui->headingUrl->setStatusTip( _subtree.url() );

    // For better Performance: disable sorting while inserting many (not many!) items
    _ui->treeWidget->setSortingEnabled( false );

    delete _stats;
    _stats = new FileAgeStats( _subtree() );
    CHECK_NEW( _stats );

    populateListWidget();

    _ui->treeWidget->setSortingEnabled( true );

    enableActions();
}


void FileAgeStatsWindow::populateListWidget()
{
    for ( short year : _stats->years() )
    {
        const YearStats * yearStats = _stats->yearStats( year );
        if ( yearStats )
        {
            // Add a year item
            YearListItem * item = new YearListItem( *yearStats );
            CHECK_NEW( item );
            _ui->treeWidget->addTopLevelItem( item );

            // Add the month items if applicable
            if ( _stats->monthStatsAvailableFor( year ) )
            {
                for ( short month = 1; month <= 12; month++ )
                {
                    YearStats * monthStats = _stats->monthStats( year, month );
                    if ( monthStats )
                    {
                        YearListItem * monthItem = new YearListItem( *monthStats );
                        CHECK_NEW( monthItem );

                        if ( monthStats->filesCount == 0 )
                            monthItem->setFlags( Qt::NoItemFlags ); // disabled

                        item->addChild( monthItem );
                    }
                }
            }
        }
    }

    fillGaps();
}


void FileAgeStatsWindow::fillGaps()
{
    for ( short year : findGaps() )
    {
        YearListItem * item = new YearListItem( YearStats( year ) );
        CHECK_NEW( item );

        item->setFlags( Qt::NoItemFlags ); // disabled
        _ui->treeWidget->addTopLevelItem( item );
    }
}


YearsList FileAgeStatsWindow::findGaps()
{
    YearsList gaps;

    const YearsList & years = _stats->years(); // sorted in ascending order
    if ( years.isEmpty() )
        return gaps;

    const short lastYear = _startGapsWithCurrentYear ? _stats->thisYear() : years.last();
    if ( lastYear - years.first() == years.count() - 1 )
        return gaps;

    for ( short yr = years.first(); yr <= lastYear; yr++ )
    {
        if ( !years.contains( yr ) )
            gaps << yr;
    }

    return gaps;
}


const YearListItem * FileAgeStatsWindow::selectedItem() const
{
    const QTreeWidgetItem * currentItem = _ui->treeWidget->currentItem();

    return currentItem ? dynamic_cast<const YearListItem *>( currentItem ) : nullptr;
}


void FileAgeStatsWindow::locateFiles()
{
    const YearListItem * item = selectedItem();
    if ( item && canLocate( item ) )
    {
        const short month = item->stats().month;
        const short year  = item->stats().year;

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


bool FileAgeStatsWindow::canLocate( const YearListItem * item ) const
{
    return item && item->stats().filesCount > 0 && item->stats().filesCount <= MAX_LOCATE_FILES;
}


void FileAgeStatsWindow::readSettings()
{
    Settings settings;

    settings.beginGroup( "FileAgeStatsWindow" );
    _ui->syncCheckBox->setChecked( settings.value( "SyncWithMainWindow",       true ).toBool() );
    _startGapsWithCurrentYear    = settings.value( "StartGapsWithCurrentYear", true ).toBool();
    settings.endGroup();

    readWindowSettings( this, "FileAgeStatsWindow" );
}


void FileAgeStatsWindow::writeSettings()
{
    Settings settings;

    settings.beginGroup( "FileAgeStatsWindow" );
    settings.setValue( "SyncWithMainWindow",       _ui->syncCheckBox->isChecked() );
    settings.setValue( "StartGapsWithCurrentYear", _startGapsWithCurrentYear      );
    settings.endGroup();

    writeWindowSettings( this, "FileAgeStatsWindow" );
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


void FileAgeStatsWindow::resizeEvent( QResizeEvent * event )
{
    // Calculate a width from the dialog less margins, less a bit more
    elideLabel( _ui->headingUrl, _ui->headingUrl->statusTip(), event->size().width() - 200 );
}




YearListItem::YearListItem( const YearStats & yearStats ) :
    QTreeWidgetItem ( QTreeWidgetItem::UserType ),
    _stats { yearStats }
{
    const QString text = _stats.month > 0 ? monthAbbreviation( _stats.month ) : QString::number( _stats.year );
    set( YearListYearCol, Qt::AlignLeft, text );

    if ( _stats.filesCount > 0 )
    {
	set( YearListFilesCountCol,   Qt::AlignRight, QString::number( _stats.filesCount   ) );
	set( YearListFilesPercentCol, Qt::AlignRight, formatPercent  ( _stats.filesPercent ) );
	set( YearListSizeCol,         Qt::AlignRight, "    " + formatSize( _stats.size ) );
	set( YearListSizePercentCol,  Qt::AlignRight, formatPercent  ( _stats.sizePercent  ) );

        setData( YearListFilesPercentBarCol, RawDataRole, _stats.sizePercent );
        setData( YearListSizePercentBarCol,  RawDataRole, _stats.sizePercent );
    }
}


void YearListItem::set( YearListColumns col, Qt::Alignment alignment, const QString & text )
{
    setText( col, text );
    setTextAlignment( col, Qt::AlignVCenter | alignment );
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
            {
                if ( _stats.month > 0 )
                    return _stats.month < other._stats.month;
                else
                    return _stats.year  < other._stats.year;
            }
	case YearListFilesCountCol:     return _stats.filesCount   < other._stats.filesCount;
	case YearListFilesPercentBarCol:
	case YearListFilesPercentCol:   return _stats.filesPercent < other._stats.filesPercent;
	case YearListSizeCol:           return _stats.size         < other._stats.size;
	case YearListSizePercentBarCol:
	case YearListSizePercentCol:    return _stats.sizePercent  < other._stats.sizePercent;
	case YearListColumnCount:       break;
    }

    return QTreeWidgetItem::operator<( rawOther );
}
