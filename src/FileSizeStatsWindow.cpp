/*
 *   File name: FileSizeStatsWindow.cpp
 *   Summary:   QDirStat size type statistics window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QActionGroup>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QPointer>
#include <QResizeEvent>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUrl>

#include "FileSizeStatsWindow.h"
#include "FileSizeStats.h"
#include "FileSizeStatsModels.h"
#include "FormatUtil.h"
#include "HistogramView.h"
#include "Logger.h"
#include "MainWindow.h"
#include "Settings.h"
#include "SignalBlocker.h"


#define MAX_BUCKET_COUNT 100


using namespace QDirStat;


namespace
{
    /**
     * Return the abstract item model (cast as PercentileTableModel) for
     * 'percentileTable'.
     **/
    PercentileTableModel * percentileTableModel( const QTableView * percentileTable )
    {
	return qobject_cast<PercentileTableModel *>( percentileTable->model() );
    }


    /**
     * Return the abstract item model (cast as BucketsTableModel) for
     * 'bucketsTable'.
     **/
    BucketsTableModel * bucketsTableModel( const QTableView * bucketsTable )
    {
	return qobject_cast<BucketsTableModel *>( bucketsTable->model() );
    }


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

} // namespace



FileSizeStatsWindow::FileSizeStatsWindow( QWidget * parent ):
    QDialog{ parent },
    _ui{ new Ui::FileSizeStatsWindow }
{
    //logDebug() << "init" << Qt::endl;

    setAttribute( Qt::WA_DeleteOnClose );

    _ui->setupUi( this );

    initWidgets();
    connectActions();

    Settings::readWindowSettings( this, "FileSizeStatsWindow" );
    readHotkeySettings( this );

    show();
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
    // Start with the options panel closed
    _ui->optionsPanel->hide();

    // Set these here so they can be based on the PercentileStats constants
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
    markersAction( actionGroup, _ui->actionNoPercentiles, 0 );
    markersAction( actionGroup, _ui->actionEvery20th, 20 );
    markersAction( actionGroup, _ui->actionEvery10th, 10 );
    markersAction( actionGroup, _ui->actionEvery5th, 5 );
    markersAction( actionGroup, _ui->actionEvery2nd, 2 );
    markersAction( actionGroup, _ui->actionEveryPercentile, 1 );
    _ui->actionNoPercentiles->setChecked( true );

    // Set up the percentile and buckets tables
    _ui->bucketsTable->setModel( new BucketsTableModel{ this } );
    _ui->bucketsTable->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeToContents );

    QTableView * table = _ui->percentileTable;
    table->setHorizontalHeader( new PercentileTableHeader{ Qt::Horizontal, table } );
    _ui->percentileTable->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeToContents );
    table->setVerticalHeader( new PercentileTableHeader{ Qt::Vertical, table } );
    table->setModel( new PercentileTableModel{ this } );
}


void FileSizeStatsWindow::connectActions()
{
    // The spin boxes are linked to the sliders inside the ui file
    connect( _ui->startPercentileSlider,    &QSlider::valueChanged,
             this,                          &FileSizeStatsWindow::setPercentileRange );

    connect( _ui->endPercentileSlider,      &QSlider::valueChanged,
             this,                          &FileSizeStatsWindow::setPercentileRange );

    connect( _ui->markersComboBox,          QOverload<int>::of( &QComboBox::currentIndexChanged ),
             this,                          &FileSizeStatsWindow::markersChanged );

    connect( _ui->percentileFilterCheckBox, &QCheckBox::toggled,
             this,                          &FileSizeStatsWindow::setPercentileTable );

    connect( _ui->actionLogScale,           &QAction::triggered,
             this,                          &FileSizeStatsWindow::logScale );

    connect( _ui->actionAutoScale,          &QAction::triggered,
             this,                          &FileSizeStatsWindow::autoLogScale );

    // Percentile "all", increment, and decrement actions are connected inside the .ui file
    connect( _ui->actionStartMin,           &QAction::triggered,
             this,                          &FileSizeStatsWindow::setMinPercentile );

    connect( _ui->actionEndMax,             &QAction::triggered,
             this,                          &FileSizeStatsWindow::setMaxPercentile );

    connect( _ui->actionAutoPercentiles,    &QAction::triggered,
             this,                          &FileSizeStatsWindow::autoPercentileRange );

    const auto helpButtons = _ui->helpPage->findChildren<const QCommandLinkButton *>();
    for ( const QCommandLinkButton * helpButton : helpButtons )
    {
	connect( helpButton, &QAbstractButton::clicked,
	         this,       &FileSizeStatsWindow::showHelp );
    }
}


void FileSizeStatsWindow::markersAction( QActionGroup * group, QAction * action, int step )
{
    action->setCheckable( true );
    action->setData( step );
    group->addAction( action );
    _ui->markersComboBox->addItem( action->text().remove( u'&' ), QVariant::fromValue( action ) );

    // Each action simply selects the corresponding combo-box entry
    const int index = _ui->markersComboBox->count() - 1;
    connect( action, &QAction::triggered,
             [ this, index ](){ _ui->markersComboBox->setCurrentIndex( index ); } );
}


void FileSizeStatsWindow::populateSharedInstance( QWidget       * mainWindow,
                                                  FileInfo      * fileInfo,
                                                  const QString & suffix  )
{
    if ( !fileInfo )
	return;

    sharedInstance( mainWindow )->populate( fileInfo, suffix );
}


void FileSizeStatsWindow::populate( FileInfo * fileInfo, const QString & suffix )
{
    const Subtree subtree{ fileInfo };
    const QString & url = subtree.url();
    _ui->headingUrl->setStatusTip( suffix.isEmpty() ? url : tr( "*%1 in %2" ).arg( suffix, url ) );
    resizeEvent( nullptr ); // sets the label from the status tip, to fit the window

    // Existing stats are invalidated; be sure the model and histogram get new pointers promptly
    if ( suffix.isEmpty() )
	_stats.reset( new FileSizeStats{ fileInfo } );
    else
	_stats.reset( new FileSizeStats{ fileInfo, suffix } );

    _stats->calculatePercentiles();

    bucketsTableModel( _ui->bucketsTable )->setStats( _stats.get() );
    percentileTableModel( _ui->percentileTable )->setStats( _stats.get() );

    setPercentileTable();
    initHistogram();
}


void FileSizeStatsWindow::initHistogram()
{
    // Block the slider signals so the histogram doesn't get built multiple (or zero!) times
    SignalBlocker startBlocker( _ui->startPercentileSlider );
    SignalBlocker endBlocker( _ui->endPercentileSlider );
    _ui->histogramView->init( _stats.get() );
    autoPercentileRange();

    // Signals were blocked, so we have to load the buckets explicitly
    // - this will reset the buckets table model and trigger the histogram to draw
    setPercentileRange();
}


void FileSizeStatsWindow::setPercentileTable()
{
    const PercentileBoundary nominalCount = 1.0 * _stats->size() / _stats->maxPercentile();
    const int precision = [ nominalCount ]()
    {
	if ( nominalCount == 0 || nominalCount >= 10 ) return 0;
	if ( nominalCount >= 1 ) return 1;
	return 2;
    }();
    _ui->nominalCount->setText( formatCount( nominalCount, precision ) );

    percentileTableModel( _ui->percentileTable )->resetModel( !_ui->percentileFilterCheckBox->isChecked() );
}


void FileSizeStatsWindow::setPercentileRange()
{
    const int startPercentile = _ui->startPercentileSlider->value();
    const int endPercentile   = _ui->endPercentileSlider->value();
    const int percentileCount = endPercentile - startPercentile;
    const FileCount dataCount = qRound( _stats->size() * percentileCount / 100.0 );
    const int bucketCount     = _stats->bestBucketCount( dataCount, MAX_BUCKET_COUNT );

    BucketsTableModel * model = bucketsTableModel( _ui->bucketsTable );
    model->beginReset();
    _stats->fillBuckets( bucketCount, startPercentile, endPercentile );
    model->endReset();

    _ui->histogramView->setPercentileRange( startPercentile, endPercentile );
}


void FileSizeStatsWindow::markersChanged()
{
    QAction * action = _ui->markersComboBox->currentData().value<QAction *>();
    action->setChecked( true );

    _ui->histogramView->setPercentileStep( action->data().toInt() );
}


void FileSizeStatsWindow::autoPercentileRange()
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
}


void FileSizeStatsWindow::autoLogScale()
{
    _ui->histogramView->enableAutoLogScale();
}


void FileSizeStatsWindow::setMinPercentile()
{
    _ui->startPercentileSlider->setValue( PercentileStats::minPercentile() );
}


void FileSizeStatsWindow::setMaxPercentile()
{
    _ui->endPercentileSlider->setValue( PercentileStats::maxPercentile() );
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
    // Elide a label to the width of the dialog less margins, less a bit more
    elideLabel( _ui->headingUrl, _ui->headingUrl->statusTip(), size().width() - 200 );
}


void FileSizeStatsWindow::changeEvent( QEvent * event )
{
    QDialog::changeEvent( event );

    if ( event->type() == QEvent::PaletteChange )
	setPercentileTable();
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
    for ( QAction * action : { _ui->actionEndMinus1, _ui->actionEndPlus1, _ui->actionEndMax } )
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
