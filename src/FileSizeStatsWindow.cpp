/*
 *   File name: FileSizeStatsWindow.cpp
 *   Summary:	QDirStat size type statistics window
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <QDesktopServices>
#include <QTableWidget>
#include <QTableWidgetItem>

#include "FileSizeStatsWindow.h"
#include "FileSizeStats.h"
#include "BucketsTableModel.h"
#include "DirTree.h"
#include "FormatUtil.h"
#include "HeaderTweaker.h"
#include "HistogramView.h"
#include "MainWindow.h"
#include "SettingsHelpers.h"
#include "Logger.h"
#include "Exception.h"

using namespace QDirStat;


/**
 * Set the font to bold for all items in a table row.
 **/
static void setRowBold( QTableWidget * table, int row )
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

#if 0
/**
 * Set the foreground (the text color) for all items in a table row.
 **/
static void setRowForeground( QTableWidget * table, int row, const QBrush & brush )
{
    for ( int col=0; col < table->columnCount(); ++col )
    {
	QTableWidgetItem * item = table->item( row, col );
	if ( item )
	    item->setForeground( brush );
    }
}
#endif

/**
 * Set the background for all items in a table row.
 **/
static void setRowBackground( QTableWidget * table, int row, const QBrush & brush )
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
static void setColAlignment( QTableWidget * table, int col, Qt::Alignment alignment )
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
static QTableWidgetItem * addItem( QTableWidget	 * table,
				   int		   row,
				   int		   col,
				   const QString & text )
{
    QTableWidgetItem * item = new QTableWidgetItem( text );
    CHECK_NEW( item );
    table->setItem( row, col, item );

    return item;
}



FileSizeStatsWindow::FileSizeStatsWindow( QWidget * parent ):
    QDialog ( parent ),
    _ui { new Ui::FileSizeStatsWindow }
{
    //logDebug() << "init" << Qt::endl;

    setAttribute( Qt::WA_DeleteOnClose );

    CHECK_NEW( _ui );

    _ui->setupUi( this );
    initWidgets();
    readWindowSettings( this, "FileSizeStatsWindow" );
}


FileSizeStatsWindow::~FileSizeStatsWindow()
{
    //logDebug() << "destroying" << Qt::endl;

    writeWindowSettings( this, "FileSizeStatsWindow" );
    delete _stats;
    delete _ui;
}


FileSizeStatsWindow * FileSizeStatsWindow::sharedInstance( QWidget * mainWindow )
{
    //logDebug() << _sharedInstance << Qt::endl;

    static QPointer<FileSizeStatsWindow> _sharedInstance = nullptr;

    if ( !_sharedInstance )
    {
	_sharedInstance = new FileSizeStatsWindow( mainWindow );
	CHECK_NEW( _sharedInstance );
    }

    return _sharedInstance;
}


void FileSizeStatsWindow::initWidgets()
{
    _bucketsTableModel = new BucketsTableModel( this, _ui->histogramView );
    CHECK_NEW( _bucketsTableModel );
    _ui->bucketsTable->setModel( _bucketsTableModel );

    const QList<QCommandLinkButton *> helpButtons = _ui->helpPage->findChildren<QCommandLinkButton *>();
    for ( const QCommandLinkButton * helpButton : helpButtons )
    {
	connect( helpButton, &QAbstractButton::clicked,
		 this,	     &FileSizeStatsWindow::showHelp );
    }

    _ui->optionsPanel->hide();

    connect( _ui->closeOptionsButton,	    &QPushButton::clicked,
	     this,			    &FileSizeStatsWindow::closeOptions );

    connect( _ui->openOptionsButton,	    &QPushButton::clicked,
	     this,			    &FileSizeStatsWindow::openOptions );

    connect( _ui->autoButton,		    &QPushButton::clicked,
	     this,			    &FileSizeStatsWindow::autoPercentiles );

    // The spin boxes are linked to the sliders inside the UI
    connect( _ui->startPercentileSlider,    &QSlider::valueChanged,
	     this,			    &FileSizeStatsWindow::startValueChanged );

    connect( _ui->endPercentileSlider,	    &QSlider::valueChanged,
	     this,			    &FileSizeStatsWindow::endValueChanged );

    connect( _ui->markersComboBox,	    qOverload<int>( &QComboBox::currentIndexChanged ),
	     this,			    &FileSizeStatsWindow::markersChanged );

    connect( _ui->percentileFilterCheckBox, &QCheckBox::stateChanged,
	     this,			    &FileSizeStatsWindow::fillPercentileTable );
}


void FileSizeStatsWindow::populateSharedInstance( QWidget	* mainWindow,
						  FileInfo	* subtree,
						  const QString & suffix  )
{
    if ( !subtree )
	return;

    sharedInstance( mainWindow )->populate( subtree, suffix );
    sharedInstance( mainWindow )->show();
}


void FileSizeStatsWindow::populate( FileInfo * subtree, const QString & suffix )
{
    QString url = subtree->debugUrl();
    if ( url == "<root>" )
	url = subtree->tree()->url();

    _ui->headingUrl->setText( suffix.isEmpty() ? url : tr( "%1 in %2" ).arg( suffix ).arg( url ) );

    delete _stats;
    if ( suffix.isEmpty() )
	_stats = new FileSizeStats( subtree );
    else
	_stats = new FileSizeStats( subtree, suffix );
    CHECK_NEW( _stats );
    _stats->calculatePercentiles();

    fillHistogram();
    fillPercentileTable();
}


void FileSizeStatsWindow::fillPercentileTable()
{
    const int step = _ui->percentileFilterCheckBox->isChecked() ? 1 : 5;
    fillQuantileTable( _ui->percentileTable, 100, "P", step, 2 );
}


void FileSizeStatsWindow::fillQuantileTable( QTableWidget *	    table,
					     int		    order,
					     const QString &	    namePrefix,
					     int		    step,
					     int		    extremesMargin )
{
    enum TableColumns
    {
	NumberCol,
	ValueCol,
	NameCol,
	SumCol,
	CumulativeSumCol
    };

    table->clear();
    table->setRowCount( order + 1 );

    QStringList headers;

    switch ( order )
    {
	case 100:	headers << tr( "Percentile"  ); break;
	case  10:	headers << tr( "Decile"      ); break;
	case   4:	headers << tr( "Quartile"    ); break;
	default:	headers << tr( "%1-Quantile" ).arg( order ); break;
    }

    headers << tr( "Size cutoff" ) << tr( "Name" );

    if ( !_stats->percentileSums().isEmpty() )
    {
	headers << tr( "Sum %1(n-1)..%2(n)" ).arg( namePrefix ).arg( namePrefix );
	headers << tr( "Cumulative sum" );
    }

    table->setColumnCount( headers.size() );
    table->setHorizontalHeaderLabels( headers );

    const int median = order / 2;
    const int quartile1 = order % 4 == 0 ? order / 4     : -1;
    const int quartile3 = order % 4 == 0 ? quartile1 * 3 : -1;

    int row = 0;

    for ( int i=0; i <= order; ++i )
    {
	if ( step > 1 && i > extremesMargin && i < order - extremesMargin && i % step != 0 )
	    continue;

	addItem( table, row, NumberCol, namePrefix + QString::number( i ) );
	addItem( table, row, ValueCol, formatSize( _stats->quantile( order, i ) ) );
	addItem( table, row, SumCol, i > 0 ? formatSize( _stats->percentileSums().at( i ) ) : "" );
	addItem( table, row, CumulativeSumCol, i > 0 ? formatSize( _stats->cumulativeSums().at( i ) ) : "" );

	const QString text = [=]()
	    {
		if ( i == 0 )		return tr( "Min" );
		if ( i == order  )	return tr( "Max" );
		if ( i == median )	return tr( "Median" );
		if ( i == quartile1 )	return tr( "1. Quartile" );
		if ( i == quartile3 )	return tr( "3. Quartile" );

		return QString();
	    }();

	if ( !text.isEmpty() )
	{
	    addItem( table, row, NameCol, text );
	    setRowBold( table, row );
	}
	else if ( order > 20 && i % 10 == 0 && step <= 1 )
	{
	    // Fill the empty cell or the background won't show
	    addItem( table, row, NameCol, "" );

	    // Derive a color with some contrast in light or dark themes.
	    QColor base = table->palette().color( QPalette::Base );
	    const int lightness = base.lightness();
	    base.setHsl( base.hue(), base.saturation(), lightness > 128 ? lightness - 64 : lightness + 64 );
	    setRowBackground( table, row, base );
	}

	++row;
    }

    table->setRowCount( row );

    setColAlignment( table, NumberCol, Qt::AlignRight );
    setColAlignment( table, ValueCol,  Qt::AlignRight );
    setColAlignment( table, NameCol,   Qt::AlignCenter);
    setColAlignment( table, SumCol,    Qt::AlignRight );
    setColAlignment( table, CumulativeSumCol, Qt::AlignRight );

    HeaderTweaker::resizeToContents( table->horizontalHeader() );
}


void FileSizeStatsWindow::loadHistogram()
{
    const int startPercentile = _ui->histogramView->startPercentile();
    const int endPercentile   = _ui->histogramView->endPercentile();
    const int percentileCount = endPercentile - startPercentile;
    const int dataCount       = qRound( _stats->size() * percentileCount / 100.0 );
    const int bucketCount     = _ui->histogramView->bestBucketCount( dataCount );
    _stats->fillBuckets( bucketCount, startPercentile, endPercentile );

    _bucketsTableModel->reset();
    HeaderTweaker::resizeToContents( _ui->bucketsTable->horizontalHeader() );

    _ui->histogramView->build();
}


void FileSizeStatsWindow::fillHistogram()
{
    HistogramView * histogram = _ui->histogramView;
    CHECK_PTR( histogram );

    histogram->clear();
    histogram->setStats( _stats );
    histogram->autoStartEndPercentiles();
    updateOptions();
    loadHistogram();
}


void FileSizeStatsWindow::openOptions()
{
    _ui->optionsPanel->show();
    _ui->openOptionsButton->hide();
    updateOptions();
}


void FileSizeStatsWindow::closeOptions()
{
    _ui->optionsPanel->hide();
    _ui->openOptionsButton->show();
}


void FileSizeStatsWindow::startValueChanged( int newStart )
{
    if ( newStart != _ui->histogramView->startPercentile() )
    {
	//logDebug() << "New start: " << newStart << Qt::endl;

	_ui->histogramView->setStartPercentile( newStart );
	loadHistogram();
    }
}


void FileSizeStatsWindow::endValueChanged( int newEnd )
{
    if ( newEnd != _ui->histogramView->endPercentile() )
    {
	//logDebug() << "New end: " << newEnd << Qt::endl;

	_ui->histogramView->setEndPercentile( newEnd );
	loadHistogram();
    }
}


void FileSizeStatsWindow::markersChanged( int markersIndex )
{
    const int step = [markersIndex]()
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
    loadHistogram();
}


void FileSizeStatsWindow::autoPercentiles()
{
    _ui->histogramView->autoStartEndPercentiles();
    updateOptions();
    loadHistogram();
}


void FileSizeStatsWindow::updateOptions()
{
    // just set the sliders, let signals set the spinboxes
    _ui->startPercentileSlider->setValue ( _ui->histogramView->startPercentile() );
    _ui->endPercentileSlider->setValue ( _ui->histogramView->endPercentile() );
}


void FileSizeStatsWindow::showHelp()
{
//    const QString topic = "Statistics.md";
    const QWidget * button = qobject_cast<QWidget *>( sender() );
    if ( !button )
	return;

    const QString helpUrl = "https://github.com/shundhammer/qdirstat/blob/master/doc/stats/" + button->statusTip();
    QDesktopServices::openUrl( helpUrl );
//    QString program = "/usr/bin/xdg-open";

    // logInfo() << "Starting  " << program << " " << helpUrl << Qt::endl;
//    QProcess::startDetached( program, QStringList() << helpUrl );
}