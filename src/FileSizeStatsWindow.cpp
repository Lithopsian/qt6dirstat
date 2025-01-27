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
#include "MimeCategory.h"
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


    /**
     * One-time initialization of the widgets in 'window'.
     **/
    void initWidgets( FileSizeStatsWindow * window, Ui::FileSizeStatsWindow * ui )
    {
	// Start with the options panel closed
	ui->optionsPanel->hide();

	// Set these here so they can be based on the PercentileStats constants
	const short firstStartPercentile = PercentileStats::minPercentile();
	const short lastStartPercentile  = PercentileStats::quartile1() - 1;
	const short firstEndPercentile   = PercentileStats::quartile3() + 1;
	const short lastEndPercentile    = PercentileStats::maxPercentile();
	ui->startPercentileSpinBox->setRange( firstStartPercentile, lastStartPercentile );
	ui->startPercentileSlider->setRange( firstStartPercentile, lastStartPercentile );
	ui->endPercentileSpinBox->setRange( firstEndPercentile, lastEndPercentile );
	ui->endPercentileSlider->setRange( firstEndPercentile, lastEndPercentile );
	ui->actionStartMin->setText( ui->actionStartMin->text().arg( firstStartPercentile ) );
	ui->actionEndMax->setText( ui->actionEndMax->text().arg( lastEndPercentile ) );

	// Put the percentile marker actions in a group so only one is ever checked
	QComboBox * comboBox = ui->markersComboBox;
	QActionGroup * group = new QActionGroup{ window };
	const auto markersAction = [ comboBox, group ]( QAction * action, int step )
	{
	    action->setCheckable( true );
	    action->setData( step );
	    group->addAction( action );

	    // Create a combo-box entry from the action text, with a pointer to the action in userData()
	    comboBox->addItem( action->text().remove( u'&' ), QVariant::fromValue( action ) );

	    // Each action simply selects the combo-box entry just created from it
	    const int index = comboBox->count() - 1;
	    QObject::connect( action, &QAction::triggered,
	                      [ comboBox, index ](){ comboBox->setCurrentIndex( index ); } );
	};
	markersAction( ui->actionNoPercentiles, 0 );
	markersAction( ui->actionEvery10th, 10 );
	markersAction( ui->actionEvery5th, 5 );
	markersAction( ui->actionEvery2nd, 2 );
	markersAction( ui->actionEveryPercentile, 1 );
	ui->actionNoPercentiles->setChecked( true );

	// Set up the percentile and buckets tables
	ui->bucketsTable->setModel( new BucketsTableModel{ window } );
	ui->bucketsTable->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeToContents );

	QTableView * table = ui->percentileTable;
	table->setModel( new PercentileTableModel{ window } );
	table->setHorizontalHeader( new PercentileTableHeader{ Qt::Horizontal, table } );
	table->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeToContents );
	table->setVerticalHeader( new PercentileTableHeader{ Qt::Vertical, table } );
    }

} // namespace



FileSizeStatsWindow::FileSizeStatsWindow( QWidget * parent ):
    QDialog{ parent },
    _ui{ new Ui::FileSizeStatsWindow }
{
    //logDebug() << "init" << Qt::endl;

    setAttribute( Qt::WA_DeleteOnClose );

    _ui->setupUi( this );

    initWidgets( this, _ui.get() );
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

    connect( _ui->excludeSymlinksCheckBox,  &QCheckBox::toggled,
             this,                          &FileSizeStatsWindow::refresh );

    const auto helpButtons = _ui->helpPage->findChildren<const QCommandLinkButton *>();
    for ( const QCommandLinkButton * helpButton : helpButtons )
    {
	connect( helpButton, &QAbstractButton::clicked,
	         this,       &FileSizeStatsWindow::showHelp );
    }
}


void FileSizeStatsWindow::populate( FileInfo * fileInfo, const WildcardCategory & wildcardCategory )
{
    _subtree = fileInfo;
    _wildcardCategory = wildcardCategory;

    // Confusing and pointless to exclude (or not) symlinks for a file-type-based dataset
    const bool filteredResults = !wildcardCategory.isEmpty();
    _ui->excludeSymlinksCheckBox->setEnabled( !filteredResults );
    if ( filteredResults )
	_ui->excludeSymlinksCheckBox->setChecked( false );

    const QString header = [ filteredResults, fileInfo, &wildcardCategory ]()
    {
	if ( filteredResults )
	    return tr( "File size statistics for " );

	const QString & pattern = wildcardCategory.wildcard.pattern();
	const MimeCategory * category = wildcardCategory.category;
	const QString patternName = pattern.isEmpty() && category ? category->name() : pattern;
	return tr( "File size statistics for %1 files in " ).arg( patternName );
    }();
    _ui->headingLabel->setStatusTip( header % replaceCrLf( fileInfo->debugUrl() ));
    showElidedLabel( _ui->headingLabel, this ); // sets the label from the status tip, to fit the window

    loadStats( fileInfo );

    initHistogram();
}


void FileSizeStatsWindow::refresh()
{
    loadStats( _subtree() );

    setPercentileRange();
}


void FileSizeStatsWindow::loadStats( FileInfo * fileInfo )
{
    FileSizeStats * stats = _wildcardCategory.isEmpty() ?
                            new FileSizeStats{ fileInfo, _ui->excludeSymlinksCheckBox->isChecked() } :
                            new FileSizeStats{ fileInfo, _wildcardCategory };
    stats->calculatePercentiles();

    bucketsTableModel( _ui->bucketsTable )->setStats( stats );
    percentileTableModel( _ui->percentileTable )->setStats( stats );
    _ui->histogramView->init( stats );

    _stats.reset( stats );

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


bool FileSizeStatsWindow::event( QEvent * event )
{
    switch ( event->type() )
    {
	case QEvent::FontChange:
	case QEvent::Resize:
	    showElidedLabel( _ui->headingLabel, this );
	    break;

	case QEvent::PaletteChange:
	    setPercentileTable();
	    break;

	default:
	    break;
    }

    return QDialog::event( event );
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
