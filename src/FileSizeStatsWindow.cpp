/*
 *   File name: FileSizeStatsWindow.cpp
 *   Summary:   QDirStat size type statistics window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QActionGroup>
#include <QCommandLinkButton>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QMenu>
#include <QPointer>

#include "FileSizeStatsWindow.h"
#include "ActionManager.h"
#include "FileInfo.h"
#include "FileSizeStats.h"
#include "FileSizeStatsModels.h"
#include "FormatUtil.h"
#include "HistogramView.h"
#include "Logger.h"
#include "Settings.h"
#include "SignalBlocker.h"


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
     * Populate the portion of the context menu related to the
     * percentiles range.  This is common to both the histogram
     * and buckets table context menus.
     **/
    void percentilesContextMenu( QMenu * menu, Ui::FileSizeStatsWindow * ui )
    {
	QMenu * startPercentile = menu->addMenu( QObject::tr( "Start percentile" ) );
	for ( QAction * action : { ui->actionStartPlus1, ui->actionStartMinus1, ui->actionStartMin } )
	    startPercentile->addAction( action );
	const auto startValue = ui->startPercentileSlider->value();
	ui->actionStartPlus1->setEnabled( startValue < PercentileStats::quartile1() - 1 );
	ui->actionStartMinus1->setEnabled( startValue > PercentileStats::minPercentile() );
	ui->actionStartMin->setEnabled( startValue > PercentileStats::minPercentile() );

	QMenu * endPercentile = menu->addMenu( QObject::tr( "End percentile"   ) );
	for ( QAction * action : { ui->actionEndMinus1, ui->actionEndPlus1, ui->actionEndMax } )
	    endPercentile->addAction( action );
	const auto endValue = ui->endPercentileSlider->value();
	ui->actionEndMinus1->setEnabled( endValue > PercentileStats::quartile3() + 1 );
	ui->actionEndPlus1->setEnabled( endValue < PercentileStats::maxPercentile() );
	ui->actionEndMax->setEnabled( endValue < PercentileStats::maxPercentile() );

	menu->addAction( ui->actionAllPercentiles );
	const bool allPercentiles =
	    startValue == PercentileStats::minPercentile() && endValue == PercentileStats::maxPercentile();
	ui->actionAllPercentiles->setEnabled( !allPercentiles );
	menu->addAction( ui->actionAutoPercentiles );
    }


    /**
     * Populate a context menu for the histogram tab.
     **/
    void histogramContextMenu( QMenu * menu, Ui::FileSizeStatsWindow * ui )
    {
	for ( QAction * action : {  ui->actionLogWidths, ui->actionAutoScale, ui->actionLogHeights } )
	    menu->addAction( action );
	ui->actionLogWidths->setChecked( ui->logWidthsCheckBox->isChecked() );
	ui->actionLogHeights->setChecked( ui->histogramView->logHeights() );
	menu->addSeparator();

	percentilesContextMenu( menu, ui );
	menu->addSeparator();

	for ( QAction * action : { ui->actionNoPercentiles, ui->actionEvery10th,
	                           ui->actionEvery5th, ui->actionEvery2nd, ui->actionEveryPercentile } )
	    menu->addAction( action );
    }


    /**
     * Populate a context menu for the histogram tab.
     **/
    void bucketsContextMenu( QMenu * menu, Ui::FileSizeStatsWindow * ui )
    {
	menu->addAction( ui->actionLogWidths );
	ui->actionLogWidths->setChecked( ui->logWidthsCheckBox->isChecked() );
	menu->addSeparator();

	percentilesContextMenu( menu, ui );
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
    ActionManager::actionHotkeys( this, "FileSizeStatsWindow" );

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
    _ui->startPercentileSpinBox->setRange( PercentileStats::minPercentile(), PercentileStats::quartile1() - 1 );
    _ui->startPercentileSlider->setRange( PercentileStats::minPercentile(), PercentileStats::quartile1() - 1 );
    _ui->endPercentileSpinBox->setRange( PercentileStats::quartile3() + 1, PercentileStats::maxPercentile() );
    _ui->endPercentileSlider->setRange( PercentileStats::quartile3() + 1, PercentileStats::maxPercentile() );
    _ui->actionStartMin->setText( _ui->actionStartMin->text().arg( PercentileStats::minPercentile() ) );
    _ui->actionEndMax->setText( _ui->actionEndMax->text().arg( PercentileStats::maxPercentile() ) );

    // Put the percentile marker actions in a group so only one is ever checked
    QComboBox * comboBox = _ui->markersComboBox;
    QActionGroup * group = new QActionGroup{ this };
    const auto markersAction = [ comboBox, group ]( QAction * action, int step )
    {
	action->setCheckable( true );
	action->setData( step );
	group->addAction( action );

	// Create a combo-box entry from the action text, with a pointer to the action in userData()
	comboBox->addItem( action->text().remove( u'&' ), QVariant::fromValue( action ) );

	// Each action simply selects the combo-box entry just created from it
	const int index = comboBox->count() - 1;
	connect( action, &QAction::triggered,
	         [ comboBox, index ](){ comboBox->setCurrentIndex( index ); } );
    };
    markersAction( _ui->actionNoPercentiles, 0 );
    markersAction( _ui->actionEvery10th, 10 );
    markersAction( _ui->actionEvery5th, 5 );
    markersAction( _ui->actionEvery2nd, 2 );
    markersAction( _ui->actionEveryPercentile, 1 );
    _ui->actionNoPercentiles->setChecked( true );

    // Set up the percentile and buckets tables
    _ui->bucketsTable->setModel( new BucketsTableModel{ this } );
    _ui->bucketsTable->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeToContents );

    QTableView * table = _ui->percentileTable;
    table->setModel( new PercentileTableModel{ this } );
    table->setHorizontalHeader( new PercentileTableHeader{ Qt::Horizontal, table } );
    table->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeToContents );
    table->setVerticalHeader( new PercentileTableHeader{ Qt::Vertical, table } );
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

    connect( _ui->logWidthsCheckBox,        &QCheckBox::toggled,
             this,                          &FileSizeStatsWindow::setPercentileRange );

    connect( _ui->actionLogHeights,         &QAction::triggered,
             this,                          &FileSizeStatsWindow::logHeights );

    connect( _ui->actionAutoScale,          &QAction::triggered,
             this,                          &FileSizeStatsWindow::autoLogHeights );

    // Percentile "all", increment, and decrement actions are connected inside the .ui file
    connect( _ui->actionStartMin,           &QAction::triggered,
             this,                          &FileSizeStatsWindow::setMinPercentile );

    connect( _ui->actionEndMax,             &QAction::triggered,
             this,                          &FileSizeStatsWindow::setMaxPercentile );

    connect( _ui->actionAutoPercentiles,    &QAction::triggered,
             this,                          &FileSizeStatsWindow::autoPercentileRange );

    connect( _ui->excludeSymLinksCheckBox,  &QCheckBox::toggled,
             this,                          &FileSizeStatsWindow::refresh );

    const auto helpButtons = _ui->helpPage->findChildren<const QCommandLinkButton *>();
    for ( const QCommandLinkButton * helpButton : helpButtons )
    {
	connect( helpButton, &QAbstractButton::clicked,
	         this,       &FileSizeStatsWindow::showHelp );
    }
}


void FileSizeStatsWindow::populate( FileInfo * fileInfo, const QString & suffix )
{
    _subtree = fileInfo;
    _suffix = suffix;

    const QString & url = fileInfo->debugUrl();
    const QString headerUrl = suffix.isEmpty() ? url : tr( "*%1 in %2" ).arg( suffix, url );
    _ui->headingLabel->setStatusTip( tr( "File size statistics for %1" ) % replaceCrLf( headerUrl ) );
    resizeEvent( nullptr ); // sets the label from the status tip, to fit the window

    loadStats( fileInfo, suffix );

    initHistogram();
}


void FileSizeStatsWindow::refresh()
{
    loadStats( _subtree(), _suffix );

    setPercentileRange();
}


void FileSizeStatsWindow::loadStats( FileInfo * fileInfo, const QString & suffix )
{
    // Existing stats are invalidated; be sure the model and histogram get new pointers promptly
    _stats.reset( new FileSizeStats{ fileInfo, suffix, _ui->excludeSymLinksCheckBox->isChecked() } );
    _stats->calculatePercentiles();

    const FileSizeStats * stats = _stats.get();
    bucketsTableModel( _ui->bucketsTable )->setStats( stats );
    percentileTableModel( _ui->percentileTable )->setStats( stats );
    _ui->histogramView->init( stats );

    setPercentileTable();
}


void FileSizeStatsWindow::initHistogram()
{
    // Block the slider signals so the histogram doesn't get built multiple (or zero!) times
    SignalBlocker startBlocker( _ui->startPercentileSlider );
    SignalBlocker endBlocker( _ui->endPercentileSlider );
    autoPercentileRange();

    // Signals were blocked, so we have to load the buckets explicitly
    // - this will reset the buckets table model and trigger the histogram to draw
    setPercentileRange();
}


void FileSizeStatsWindow::setPercentileTable()
{
    const double nominalCount = 1.0 * _stats->size() / _stats->maxPercentile();
    const int precision = [ nominalCount ]()
    {
	if ( nominalCount == 0 || nominalCount >= 10 ) return 0;
	if ( nominalCount >= 1 ) return 1;
	return 2;
    }();
    const QString text = tr( "Nominal files per percentile: " ) % formatCount( nominalCount, precision );
    _ui->nominalCountLabel->setText( text );

    const bool filterRows = !_ui->percentileFilterCheckBox->isChecked();
    percentileTableModel( _ui->percentileTable )->resetModel( filterRows );
}


void FileSizeStatsWindow::setPercentileRange()
{
    const int       startPercentile = _ui->startPercentileSlider->value();
    const int       endPercentile   = _ui->endPercentileSlider->value();
    const FileCount dataCount       = _stats->percentileCount( startPercentile, endPercentile );
    const int       bucketCount     = _stats->bestBucketCount( dataCount, _stats->maxPercentile() );
    const bool      logWidths       = _ui->logWidthsCheckBox->isChecked();

    _ui->bucketsLabel->setText( tr( "%1 files between percentiles %2 and %3" )
                                .arg( formatCount( dataCount ) )
                                .arg( startPercentile )
                                .arg( endPercentile ) );

    BucketsTableModel * model = bucketsTableModel( _ui->bucketsTable );
    model->beginReset();
    _stats->fillBuckets( logWidths, bucketCount, startPercentile, endPercentile );
    model->endReset();

    _ui->histogramView->setPercentileRange( startPercentile, endPercentile, logWidths );
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
    int startPercentile = _stats->minPercentile();
    while ( _stats->percentileValue( startPercentile ) < minVal )
	++startPercentile;
    _ui->startPercentileSpinBox->setValue( startPercentile );

    // Find the lowest percentile that has a value greater than maxVal
    int endPercentile = _stats->maxPercentile();
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


void FileSizeStatsWindow::logHeights()
{
    _ui->histogramView->disableAutoLogHeights();
    _ui->histogramView->toggleLogHeights();
}


void FileSizeStatsWindow::autoLogHeights()
{
    _ui->histogramView->enableAutoLogHeights();
}


void FileSizeStatsWindow::setMinPercentile()
{
    _ui->startPercentileSlider->setValue( PercentileStats::minPercentile() );
}


void FileSizeStatsWindow::setMaxPercentile()
{
    _ui->endPercentileSlider->setValue( PercentileStats::maxPercentile() );
}


void FileSizeStatsWindow::showHelp() const
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
    // Calculate the last available pixel from the edge of the dialog less the right-hand layout margin
    const int lastPixel = contentsRect().right() - layout()->contentsMargins().right();
    elideLabel( _ui->headingLabel, _ui->headingLabel->statusTip(), lastPixel );
}


void FileSizeStatsWindow::changeEvent( QEvent * event )
{
    QDialog::changeEvent( event );

    if ( event->type() == QEvent::PaletteChange )
	setPercentileTable();

    if ( event->type() == QEvent::FontChange )
	resizeEvent( nullptr );
}


void FileSizeStatsWindow::contextMenuEvent( QContextMenuEvent * event )
{
    // Build a new menu from scratch every time
    QMenu * menu = new QMenu{ this };

    // Different context menus, or none, on different tabs
    const QWidget * currentWidget = _ui->tabWidget->currentWidget();
    if ( currentWidget == _ui->histogramPage )
	histogramContextMenu( menu, _ui.get() );
    else if ( currentWidget == _ui->bucketsPage )
	bucketsContextMenu( menu, _ui.get() );

    menu->exec( event->globalPos() );

    // Enable all the actions again, they are safe to be triggered at any time
    for ( QAction * action : { _ui->actionStartPlus1, _ui->actionStartMinus1, _ui->actionStartMin,
                               _ui->actionEndPlus1,   _ui->actionEndMinus1,   _ui->actionEndMax,
                               _ui->actionAllPercentiles } )
	action->setEnabled( true );
}
