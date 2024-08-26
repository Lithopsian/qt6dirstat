/*
 *   File name: FileSizeStatsWindow.cpp
 *   Summary:   QDirStat size type statistics window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QActionGroup>
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
     * Read hotkey settings and apply to the existing actions found
     * within 'tree'.  The ui file hotkeys are used as default values.
     **/
    void readHotkeySettings( QWidget * tree )
    {
	Settings settings;

	settings.beginGroup( "FileSizeStatsWindow" );

	const auto actions = tree->findChildren<QAction *>( nullptr, Qt::FindDirectChildrenOnly );
	for ( QAction * action : actions )
	{
	    tree->addAction( action );
	    settings.applyActionHotkey( action );
	}

	settings.endGroup();
    }


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
    readHotkeySettings( this );
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
    _ui->actionStartMin->setText( _ui->actionStartMin->text().arg( PercentileStats::minPercentile() ) );
    _ui->actionEndMax->setText( _ui->actionEndMax->text().arg( PercentileStats::maxPercentile() ) );

    // Put the percentile marker actions in a group so only one is ever checked
    QActionGroup * actionGroup = new QActionGroup{ this };
    markerAction( actionGroup, _ui->actionNoPercentiles, 0 );
    markerAction( actionGroup, _ui->actionEvery20th, 20 );
    markerAction( actionGroup, _ui->actionEvery10th, 10 );
    markerAction( actionGroup, _ui->actionEvery5th, 5 );
    markerAction( actionGroup, _ui->actionEvery2nd, 2 );
    markerAction( actionGroup, _ui->actionEveryPercentile, 1 );
    _ui->actionNoPercentiles->setChecked( true );

    const auto helpButtons = _ui->helpPage->findChildren<const QCommandLinkButton *>();
    for ( const QCommandLinkButton * helpButton : helpButtons )
    {
	connect( helpButton, &QAbstractButton::clicked,
	         this,       &FileSizeStatsWindow::showHelp );
    }

    _ui->optionsPanel->hide();

    // The spin boxes are linked to the sliders inside the ui file
    connect( _ui->startPercentileSlider,    &QSlider::valueChanged,
             this,                          &FileSizeStatsWindow::startValueChanged );

    connect( _ui->endPercentileSlider,      &QSlider::valueChanged,
             this,                          &FileSizeStatsWindow::endValueChanged );

    connect( _ui->markersComboBox,          QOverload<int>::of( &QComboBox::currentIndexChanged ),
             this,                          &FileSizeStatsWindow::markersChanged );

    connect( _ui->percentileFilterCheckBox, &QCheckBox::stateChanged,
             this,                          &FileSizeStatsWindow::fillPercentileTable );

    connect( _ui->actionLogScale,           &QAction::triggered,
             this,                          &FileSizeStatsWindow::logScale );

    connect( _ui->actionAutoScale,          &QAction::triggered,
             this,                          &FileSizeStatsWindow::autoLogScale );

    connect( _ui->actionStartPlus1,         &QAction::triggered,
             this,                          [ this ]()
       { _ui->startPercentileSlider->setValue( _ui->startPercentileSlider->value() + 1 ); } );

    connect( _ui->actionStartMinus1,        &QAction::triggered,
             this,                          [ this ]()
       { _ui->startPercentileSlider->setValue( _ui->startPercentileSlider->value() - 1 ); } );

    connect( _ui->actionStartMin,           &QAction::triggered,
             this,                          [ this ]()
       { _ui->startPercentileSlider->setValue( PercentileStats::minPercentile() ); } );

    connect( _ui->actionEndPlus1,           &QAction::triggered,
             this,                          [ this ]()
       { _ui->endPercentileSlider->setValue( _ui->endPercentileSlider->value() + 1 ); } );

    connect( _ui->actionEndMinus1,          &QAction::triggered,
             this,                          [ this ]()
       { _ui->endPercentileSlider->setValue( _ui->endPercentileSlider->value() - 1 ); } );

    connect( _ui->actionEndMax,             &QAction::triggered,
             this,                          [ this ]()
       { _ui->endPercentileSlider->setValue( PercentileStats::maxPercentile() ); } );

    connect( _ui->actionAutoPercentiles,    &QAction::triggered,
             this,                          &FileSizeStatsWindow::autoStartEndPercentiles );
}


void FileSizeStatsWindow::markerAction( QActionGroup * group, QAction * action, int step )
{
    action->setCheckable( true );
    action->setData( step );
    group->addAction( action );
    _ui->markersComboBox->addItem( action->text().remove( u'&' ), QVariant::fromValue( action ) );

    const int index = _ui->markersComboBox->count() - 1;
    connect( action, &QAction::triggered,
             this,   [ this, index ](){ _ui->markersComboBox->setCurrentIndex( index ); } );
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

    // We have to set the percentiles and build it explicitly because there were no signals
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


void FileSizeStatsWindow::startValueChanged( int newStart )
{
    //logDebug() << "New start: " << newStart << Qt::endl;

    _ui->histogramView->setStartPercentile( newStart );
    loadHistogram();
}


void FileSizeStatsWindow::endValueChanged( int newEnd )
{
    //logDebug() << "New end: " << newEnd << Qt::endl;

    _ui->histogramView->setEndPercentile( newEnd );
    loadHistogram();
}


void FileSizeStatsWindow::markersChanged()
{
    QAction * action = _ui->markersComboBox->currentData().value<QAction *>();
    action->setChecked( true );

    _ui->histogramView->setPercentileStep( action->data().toInt() );
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

    // Find the threshold values for the low and high outliers
    const FileSize minVal = qMax( q1Value - iqr, _stats->minValue() );
    const FileSize maxVal = ( 3.0 * iqr + q3Value > maxValue ) ? maxValue : ( 3 * iqr + q3Value );

    // Find the highest percentile that has a value less than minVal
    PercentileValue startPercentile = _stats->minPercentile();
    while ( _stats->percentileValue( startPercentile ) < minVal )
	++startPercentile;
    _ui->startPercentileSpinBox->setValue( startPercentile );

    // Find the lowest percentile that has a value greater than maxVal
    PercentileValue endPercentile = _stats->maxPercentile();
    while ( _stats->percentileValue( endPercentile ) > maxVal )
	--endPercentile;
    _ui->endPercentileSpinBox->setValue( endPercentile );

#if 0
    logInfo() << "Q1: " << formatSize( q1Value )
              << "  Q3: " << formatSize( q3Value )
              << "  minVal: " << formatSize( minVal )
              << "  maxVal: " << formatSize( maxVal )
              << Qt::endl;
    logInfo() << "startPercentile: " << startPercentile
              << "  " << formatSize( _stats->percentileValue( startPercentile ) )
              << "  endPercentile: " << endPercentile
              << "  " << formatSize( _stats->percentileValue( endPercentile ) )
              << Qt::endl;
#endif
}


void FileSizeStatsWindow::logScale()
{
    _ui->histogramView->disableAutoLogScale();
    _ui->histogramView->toggleLogScale();
    _ui->histogramView->build();
}


void FileSizeStatsWindow::autoLogScale()
{
    _ui->histogramView->enableAutoLogScale();
    _ui->histogramView->build();
}


void FileSizeStatsWindow::showHelp()
{
    const QWidget * button = qobject_cast<QWidget *>( sender() );
    if ( button )
    {
	const QString helpUrl =
	    "https://github.com/shundhammer/qdirstat/blob/master/doc/stats/"_L1 % button->statusTip();
	QDesktopServices::openUrl( helpUrl );
    }
}


void FileSizeStatsWindow::resizeEvent( QResizeEvent * )
{
    // Calculate a width from the dialog less margins, less a bit more
    elideLabel( _ui->headingUrl, _ui->headingUrl->statusTip(), size().width() - 200 );
}


void FileSizeStatsWindow::contextMenuEvent( QContextMenuEvent * event )
{
    // This context menu would be confusing anywhere except on the histogram tab
    if ( _ui->tabWidget->currentWidget() != _ui->histogramPage )
	return;

    // Build a new menu from scratch every time
    QMenu * menu = new QMenu{ this };
    menu->addAction( _ui->actionAutoScale );
    menu->addAction( _ui->actionLogScale );
    _ui->actionLogScale->setChecked( _ui->histogramView->logScale() );
    menu->addSeparator();

    QMenu * startPercentile = menu->addMenu( tr( "Start percentile" ) );
    for ( QAction * action : { _ui->actionStartPlus1, _ui->actionStartMinus1, _ui->actionStartMin } )
	startPercentile->addAction( action );
    const auto startValue = _ui->startPercentileSlider->value();
    _ui->actionStartPlus1->setEnabled( startValue < PercentileStats::quartile1() - 1 );
    _ui->actionStartMinus1->setEnabled( startValue > PercentileStats::minPercentile() );
    _ui->actionStartMin->setEnabled( startValue > PercentileStats::minPercentile() );

    QMenu * endPercentile = menu->addMenu( tr( "End percentile"   ) );
    for ( QAction * action : { _ui->actionEndMinus1, _ui->actionEndMinus1, _ui->actionEndMax } )
	endPercentile->addAction( action );
    const auto endValue = _ui->endPercentileSlider->value();
    _ui->actionEndMinus1->setEnabled( endValue > PercentileStats::quartile3() + 1 );
    _ui->actionEndPlus1->setEnabled( endValue < PercentileStats::maxPercentile() );
    _ui->actionEndMax->setEnabled( endValue < PercentileStats::maxPercentile() );

    menu->addAction( _ui->actionAllPercentiles );
    const bool allPercentiles =
	startValue == PercentileStats::minPercentile() && endValue == PercentileStats::maxPercentile();
    _ui->actionAllPercentiles->setEnabled( !allPercentiles );
    menu->addAction( _ui->actionAutoPercentiles );
    menu->addSeparator();

    for ( QAction * action : { _ui->actionNoPercentiles, _ui->actionEvery20th, _ui->actionEvery10th,
                               _ui->actionEvery5th, _ui->actionEvery2nd, _ui->actionEveryPercentile } )
	menu->addAction( action );

    menu->exec( event->globalPos() );

    // Enable all the actions again, they are safe to be triggered at any time
    for ( QAction * action : { _ui->actionStartPlus1, _ui->actionStartMinus1, _ui->actionStartMin,
                               _ui->actionEndPlus1,   _ui->actionEndMinus1,   _ui->actionEndMax,
                               _ui->actionAllPercentiles } )
	action->setEnabled( true );
}
