/*
 *   File name: FileSizeStatsWindow.cpp
 *   Summary:   QDirStat size type statistics window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QDesktopServices>
#include <QPointer>
#include <QResizeEvent>
#include <QTableWidget>
#include <QTableWidgetItem>

#include "FileSizeStatsWindow.h"
#include "FileSizeStats.h"
#include "BucketsTableModel.h"
#include "DirTree.h"
#include "FormatUtil.h"
#include "HeaderTweaker.h"
#include "HistogramView.h"
#include "Logger.h"
#include "MainWindow.h"
#include "Settings.h"
#include "SignalBlocker.h"


#define MAX_BUCKET_COUNT 100
#define EXTREMES_MARGIN    4
#define FILTERED_STEP      5


using namespace QDirStat;


namespace
{
    /**
     * Set the font to bold for all items in a table row.
     **/
    void setRowBold( QTableWidget * table, int row )
    {
	for ( int col=0; col < table->columnCount(); ++col )
	{
	    QTableWidgetItem * item = table->item( row, col );
	    if ( item )
	    {
		QFont font = item->font();
		font.setBold( true );
		item->setFont( font );
	    }
	}
    }


    /**
     * Set the background for all items in a table row.
     **/
    void setRowBackground( QTableWidget * table, int row, const QBrush & brush )
    {
	for ( int col=0; col < table->columnCount(); ++col )
	{
	    QTableWidgetItem * item = table->item( row, col );
	    if ( item )
		item->setBackground( brush );
	}
    }


    /**
     * Set the text alignment for all items in a table column.
     **/
    void setColAlignment( QTableWidget * table, int col, Qt::Alignment alignment )
    {
	for ( int row=0; row < table->rowCount(); ++row )
	{
	    QTableWidgetItem * item = table->item( row, col );
	    if ( item )
		item->setTextAlignment( alignment | Qt::AlignVCenter );
	}
    }


    /**
     * Add an item to a table.
     **/
    QTableWidgetItem * addItem( QTableWidget  * table,
                                int             row,
                                int             col,
                                const QString & text )
    {
	QTableWidgetItem * item = new QTableWidgetItem{ text };
	table->setItem( row, col, item );

	return item;
    }


    /**
     * Fill the percentile table 'table' from the given stat.
     *
     * 'step' is the step width, expected to be 1 or 5 although
     * other values are supported; * 'extremesMargin' specifies
     * how far from the extremes (min, max) the step width
     * should be 1 instead of the given step.
     **/
    void fillTable( const FileSizeStats * stats,
                    QTableWidget        * table,
                    int                   step,
                    int                   extremesMargin )
    {
	enum TableColumns
	{
	    NumberCol,
	    NameCol,
	    ValueCol,
	    SumCol,
	    CumulativeSumCol
	};

	table->clear();

	const QString percentilePrefix = QObject::tr( "P" );

	const QStringList headers{ QString{},
	                           QObject::tr( "Name" ),
	                           QObject::tr( "Size limit" ),
	                           QObject::tr( "Sum %01(n-1)...%01(n)" ).arg( percentilePrefix ),
	                           QObject::tr( "Sum %010...%01(n)" ).arg( percentilePrefix ),
//	                           QObject::tr( "Cumulative sum" ),
	                         };
	table->setColumnCount( headers.size() );
	table->setHorizontalHeaderLabels( headers );

	table->setRowCount( stats->maxPercentile() - stats->minPercentile() + 1 );

	const int minMargin = stats->minPercentile() + extremesMargin;
	const int maxMargin = stats->maxPercentile() - extremesMargin;

	int row = 0;
	for ( int i = stats->minPercentile(); i <= stats->maxPercentile(); ++i )
	{
	    // Skip rows that aren't in the specified 'step' interval or the "margins"
	    if ( step > 1 && i % step != 0 && i > minMargin && i < maxMargin )
		continue;

	    addItem( table, row, NumberCol, percentilePrefix % QString::number( i ) );
	    addItem( table, row, ValueCol,  formatSize( stats->percentileValue( i ) ) );
	    if ( i > 0 )
	    {
		addItem( table, row, SumCol,           formatSize( stats->percentileSum( i ) ) );
		addItem( table, row, CumulativeSumCol, formatSize( stats->cumulativeSum( i ) ) );
	    }

	    const QString rowName = [ i ]()
	    {
		switch ( i )
		{
		    case PercentileStats::minPercentile(): return QObject::tr( "Min" );
		    case PercentileStats::quartile1():     return QObject::tr( "1st quartile" );
		    case PercentileStats::median():        return QObject::tr( "Median" );
		    case PercentileStats::quartile3():     return QObject::tr( "3rd quartile" );
		    case PercentileStats::maxPercentile(): return QObject::tr( "Max" );
		    default: return QString{};
		}
	    }();

	    if ( !rowName.isEmpty() )
	    {
		addItem( table, row, NameCol, rowName );
		setRowBold( table, row );
	    }
	    else if ( i % 10 == 0 && step == 1 )
	    {
		// Fill the empty cell or the background won't show
		addItem( table, row, NameCol, QString{} );

		// Derive a color with some contrast in light or dark themes.
		QColor base = table->palette().color( QPalette::Base );
		const int lightness = base.lightness();
		base.setHsl( base.hue(), base.saturation(), lightness > 128 ? lightness - 64 : lightness + 64 );
		setRowBackground( table, row, base );
	    }

	    ++row;
	}

	table->setRowCount( row );

	setColAlignment( table, NumberCol,        Qt::AlignRight );
	setColAlignment( table, ValueCol,         Qt::AlignRight );
	setColAlignment( table, NameCol,          Qt::AlignCenter);
	setColAlignment( table, SumCol,           Qt::AlignRight );
	setColAlignment( table, CumulativeSumCol, Qt::AlignRight );

	HeaderTweaker::resizeToContents( table->horizontalHeader() );
    }

} // namespace



FileSizeStatsWindow::FileSizeStatsWindow( QWidget * parent ):
    QDialog{ parent },
    _ui{ new Ui::FileSizeStatsWindow }
{
    //logDebug() << "init" << Qt::endl;

    setAttribute( Qt::WA_DeleteOnClose );

    _ui->setupUi( this );

    _ui->bucketsTable->setModel( new BucketsTableModel{ this } );

    initWidgets();

    Settings::readWindowSettings( this, "FileSizeStatsWindow" );
}


FileSizeStatsWindow::~FileSizeStatsWindow()
{
    //logDebug() << "destroying" << Qt::endl;

    Settings::writeWindowSettings( this, "FileSizeStatsWindow" );
}


FileSizeStatsWindow * FileSizeStatsWindow::sharedInstance( QWidget * mainWindow )
{
    //logDebug() << _sharedInstance << Qt::endl;

    static QPointer<FileSizeStatsWindow> _sharedInstance;

    if ( !_sharedInstance )
	_sharedInstance = new FileSizeStatsWindow{ mainWindow };

    return _sharedInstance;
}


void FileSizeStatsWindow::initWidgets()
{
    // Set these here so they can be based on the PercentileStats values
    _ui->startPercentileSpinBox->setMinimum( PercentileStats::minPercentile() );
    _ui->startPercentileSpinBox->setMaximum( PercentileStats::quartile1() - 1 );
    _ui->startPercentileSlider->setMinimum( PercentileStats::minPercentile() );
    _ui->startPercentileSlider->setMaximum( PercentileStats::quartile1() - 1 );
    _ui->endPercentileSpinBox->setMinimum( PercentileStats::quartile3() + 1 );
    _ui->endPercentileSpinBox->setMaximum( PercentileStats::maxPercentile() );
    _ui->endPercentileSlider->setMinimum( PercentileStats::quartile3() + 1 );
    _ui->endPercentileSlider->setMaximum( PercentileStats::maxPercentile() );

    const auto helpButtons = _ui->helpPage->findChildren<const QCommandLinkButton *>();
    for ( const QCommandLinkButton * helpButton : helpButtons )
    {
	connect( helpButton, &QAbstractButton::clicked,
	         this,       &FileSizeStatsWindow::showHelp );
    }

    _ui->optionsPanel->hide();

    connect( _ui->closeOptionsButton,       &QPushButton::clicked,
             this,                          &FileSizeStatsWindow::closeOptions );

    connect( _ui->openOptionsButton,        &QPushButton::clicked,
             this,                          &FileSizeStatsWindow::openOptions );

    connect( _ui->autoButton,               &QPushButton::clicked,
             this,                          &FileSizeStatsWindow::autoStartEndPercentiles );

    // The spin boxes are linked to the sliders inside the ui file
    connect( _ui->startPercentileSlider,    &QSlider::valueChanged,
             this,                          &FileSizeStatsWindow::startValueChanged );

    connect( _ui->endPercentileSlider,      &QSlider::valueChanged,
             this,                          &FileSizeStatsWindow::endValueChanged );

    connect( _ui->markersComboBox,          QOverload<int>::of( &QComboBox::currentIndexChanged ),
             this,                          &FileSizeStatsWindow::markersChanged );

    connect( _ui->percentileFilterCheckBox, &QCheckBox::stateChanged,
             this,                          &FileSizeStatsWindow::fillPercentileTable );
}


void FileSizeStatsWindow::populateSharedInstance( QWidget       * mainWindow,
                                                  FileInfo      * fileInfo,
                                                  const QString & suffix  )
{
    if ( !fileInfo )
	return;

    // Show the window first, or it will trigger extra histogram rebuilds
    sharedInstance( mainWindow )->show();
    sharedInstance( mainWindow )->populate( fileInfo, suffix );
}


BucketsTableModel * FileSizeStatsWindow::bucketsTableModel() const
{
    return qobject_cast<BucketsTableModel *>( _ui->bucketsTable->model() );
}


void FileSizeStatsWindow::populate( FileInfo * fileInfo, const QString & suffix )
{
    const Subtree subtree{ fileInfo };
    const QString & url = subtree.url();
    _ui->headingUrl->setStatusTip( suffix.isEmpty() ? url : tr( "*%1 in %2" ).arg( suffix, url ) );
    resizeEvent( nullptr ); // sets the label from the status tip, to fit the window

    if ( suffix.isEmpty() )
	_stats.reset( new FileSizeStats{ fileInfo } );
    else
	_stats.reset( new FileSizeStats{ fileInfo, suffix } );
    _stats->calculatePercentiles();

    initHistogram();
    fillPercentileTable();
    bucketsTableModel()->setStats( _stats.get() );
}


void FileSizeStatsWindow::initHistogram()
{
    // Block the slider signals so the histogram doesn't get built multiple (or zero!) times
    SignalBlocker startBlocker( _ui->startPercentileSlider );
    SignalBlocker endBlocker( _ui->endPercentileSlider );
    _ui->histogramView->init( _stats.get() );
    autoStartEndPercentiles();

    // Now we have to set the percentiles and build it explicitly because there were no signals
    _ui->histogramView->setStartPercentile( _ui->startPercentileSlider->value() );
    _ui->histogramView->setEndPercentile( _ui->endPercentileSlider->value() );
    loadHistogram();
}


void FileSizeStatsWindow::fillPercentileTable()
{
    const int step = _ui->percentileFilterCheckBox->isChecked() ? 1 : FILTERED_STEP;
    fillTable( _stats.get(), _ui->percentileTable, step, EXTREMES_MARGIN );
}


void FileSizeStatsWindow::loadHistogram()
{
    const int startPercentile = _ui->startPercentileSlider->value();
    const int endPercentile   = _ui->endPercentileSlider->value();
    const int percentileCount = endPercentile - startPercentile;
    const int dataCount       = qRound( _stats->size() * percentileCount / 100.0 );
    const int bucketCount     = _stats->bestBucketCount( dataCount, MAX_BUCKET_COUNT );

    bucketsTableModel()->beginReset();
    _stats->fillBuckets( bucketCount, startPercentile, endPercentile );
    bucketsTableModel()->endReset();

    HeaderTweaker::resizeToContents( _ui->bucketsTable->horizontalHeader() );

    _ui->histogramView->build();
}


void FileSizeStatsWindow::openOptions()
{
    _ui->optionsPanel->show();
    _ui->openOptionsButton->hide();
}


void FileSizeStatsWindow::closeOptions()
{
    _ui->optionsPanel->hide();
    _ui->openOptionsButton->show();
}


void FileSizeStatsWindow::startValueChanged( int newStart )
{
//    if ( newStart != _ui->histogramView->startPercentile() )
    {
	//logDebug() << "New start: " << newStart << Qt::endl;

	_ui->histogramView->setStartPercentile( newStart );
	loadHistogram();
    }
}


void FileSizeStatsWindow::endValueChanged( int newEnd )
{
//    if ( newEnd != _ui->histogramView->endPercentile() )
    {
	//logDebug() << "New end: " << newEnd << Qt::endl;

	_ui->histogramView->setEndPercentile( newEnd );
	loadHistogram();
    }
}


void FileSizeStatsWindow::markersChanged( int markersIndex )
{
    const int step = [ markersIndex ]()
    {
	switch ( markersIndex )
	{
	    case 1:  return 20;
	    case 2:  return 10;
	    case 3:  return 5;
	    case 4:  return 2;
	    case 5:  return 1;
	    default: return 0;
	}
    }();

    _ui->histogramView->setPercentileStep( step );
    _ui->histogramView->build();
}


void FileSizeStatsWindow::autoStartEndPercentiles()
{
    // Outliers are classed as more than three times the IQR beyond the 3rd quartile
    // Just use the IQR beyond the 1st quartile because of the usual skewed file size distribution
    const FileSize q1Value  = _stats->q1Value();
    const FileSize q3Value  = _stats->q3Value();
    const FileSize iqr      = q3Value - q1Value;
    const FileSize maxValue = _stats->maxValue();

    // Find the threashold values for the low and high outliers
    const FileSize minVal = qMax( q1Value - iqr, _stats->minValue() );
    const FileSize maxVal = ( 3.0 * iqr + q3Value > maxValue ) ? maxValue : ( 3 * iqr + q3Value );

    // Find the highest percentile that has a value less than minVal
    PercentileValue startPercentile = _stats->minPercentile();
    while ( _stats->percentileValue( startPercentile ) < minVal )
	++startPercentile;
    _ui->startPercentileSpinBox->setValue( startPercentile );

    // Find the lowest percentile that has a value greater than maxVal
    PercentileValue  endPercentile = _stats->maxPercentile();
    while ( _stats->percentileValue( endPercentile ) > maxVal )
	--endPercentile;
    _ui->endPercentileSpinBox->setValue( endPercentile );

#if VERBOSE_HISTOGRAM
    logInfo() << "Q1: " << formatSize( q1Value )
              << "  Q3: " << formatSize( q3Value )
              << "  minVal: " << formatSize( minVal )
              << "  maxVal: " << formatSize( maxVal )
              << Qt::endl;
    logInfo() << "startPercentile: " << _startPercentile
              << "  " << formatSize( percentile( _startPercentile ) )
              << "  endPercentile: " << _endPercentile
              << "  " << formatSize( percentile( _endPercentile ) )
              << Qt::endl;
#endif
}

/*
void FileSizeStatsWindow::autoPercentiles()
{
    autoStartEndPercentiles();
//    updateOptions();
//    loadHistogram();
}
*/
/*
void FileSizeStatsWindow::updateOptions()
{
    // just set the sliders, let signals set the spinboxes
    _ui->startPercentileSlider->setValue ( _ui->histogramView->startPercentile() );
    _ui->endPercentileSlider->setValue ( _ui->histogramView->endPercentile() );
}
*/

void FileSizeStatsWindow::showHelp()
{
    const QWidget * button = qobject_cast<QWidget *>( sender() );
    if ( !button )
	return;

    const QString helpUrl =
	"https://github.com/shundhammer/qdirstat/blob/master/doc/stats/"_L1 % button->statusTip();
    QDesktopServices::openUrl( helpUrl );
}


void FileSizeStatsWindow::resizeEvent( QResizeEvent * )
{
    // Calculate a width from the dialog less margins, less a bit more
    elideLabel( _ui->headingUrl, _ui->headingUrl->statusTip(), size().width() - 200 );
}
