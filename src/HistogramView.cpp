/*
 *   File name: HistogramView.cpp
 *   Summary:   View widget for histogram rendering for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <cmath> // log2(), round()
#include <algorithm> // std::max_element

#include <QGraphicsItem>
#include <QResizeEvent>
#include <QTextDocument>

#include "HistogramView.h"
#include "Exception.h"
#include "FileSizeStats.h"
#include "FormatUtil.h"
#include "HistogramItems.h"


#define VERBOSE_HISTOGRAM 0

#define MinHistogramWidth	500.0_qr // don't need this because the width is constrained in the .ui file
#define MinHistogramHeight	300.0_qr // about the height of the overflow panel


using namespace QDirStat;


namespace
{
    /**
     * Return the percentile sums from 'fromIndex' to 'toIndex'
     * inclusive.
     **/
    FileSize percentileSum( const FileSizeStats * stats, int fromIndex, int toIndex )
    {
	FileSize sum = 0LL;

	for ( int i = fromIndex; i <= toIndex; ++i )
	    sum += stats->percentileSum( i );

	return sum;
    }


    /**
     * Make the QGraphicsTextItem text bold.
     **/
    void setBold( QGraphicsTextItem * item )
    {
	QFont font{ item->font() };
	font.setBold( true );
	item->setFont( font );
    }

    /**
     * Create a text item in the given scene and set its Z-value.
     **/
    QGraphicsTextItem * createTextItem( QGraphicsScene * scene, const QString & text )
    {
	QGraphicsTextItem * item = scene->addText( text );
	item->setZValue( HistogramView::TextLayer );

	return item;
    }

} // namespace


void HistogramView::init( const FileSizeStats * stats )
{
    CHECK_PTR( stats );
    _stats = stats;

    _geometryDirty  = true;

    autoStartEndPercentiles();
}


FileSize HistogramView::percentile( int index ) const
{
    return _stats->percentileValue( index );
}


void HistogramView::setStartPercentile( int index )
{
    CHECK_PERCENTILE_INDEX( index );

    const bool oldNeedOverflowPanel = needOverflowPanel();
    _startPercentile = index;

    if ( oldNeedOverflowPanel != needOverflowPanel() )
        _geometryDirty = true;
}


void HistogramView::setEndPercentile( int index )
{
    CHECK_PERCENTILE_INDEX( index );

    const bool oldNeedOverflowPanel = needOverflowPanel();
    _endPercentile = index;

    if ( oldNeedOverflowPanel != needOverflowPanel() )
        _geometryDirty = true;

#if VERBOSE_HISTOGRAM
    if ( _startPercentile >= _endPercentile )
    {
	logError() << "startPercentile must be less than endPercentile: "
	           << _startPercentile << ".." << _endPercentile
	           << Qt::endl;
    }
#endif
}


void HistogramView::autoStartEndPercentiles()
{
    const bool oldNeedOverflowPanel = needOverflowPanel();

    // Outliers are classed as more than three times the IQR beyond the 3rd quartile
    // Just use the IQR beyond the 1st quartile because of the usual skewed file size distribution
    const FileSize q1Value  = _stats->q1Value();
    const FileSize q3Value  = _stats->q3Value();
    const FileSize iqr      = q3Value - q1Value;
    const FileSize maxValue = _stats->maxValue();

    // Find the threashold values for the low and high outliers
    const FileSize minVal = qMax( q1Value - iqr, 0LL );
    const FileSize maxVal = ( q3Value + iqr * 3.0 > maxValue ) ? maxValue : ( q3Value + iqr * 3 );

    // Find the highest percentile that has a value less than minVal
    _startPercentile = _stats->minPercentile();
    while ( percentile( _startPercentile ) < minVal )
	++_startPercentile;

    // Find the lowest percentile that has a value greater than maxVal
    _endPercentile = _stats->maxPercentile();
    while ( percentile( _endPercentile ) > maxVal )
	--_endPercentile;

    if ( oldNeedOverflowPanel != needOverflowPanel() )
        _geometryDirty = true;

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


void HistogramView::autoLogHeightScale()
{
    _useLogHeightScale = false;

    if ( _stats->bucketCount() > 3 )
    {
	const int largest = *std::max_element( _stats->bucketsBegin(), _stats->bucketsEnd() );

	// We compare the largest bucket with the P85 percentile of the buckets
	// (not to be confused with the P85 percentile of the data the buckets
	// were collected from)
	const int referencePercentileValue =
	    _stats->bucket( qRound( _stats->bucketCount() * 85.0 / 100.0 ) );

	_useLogHeightScale = largest > referencePercentileValue * 10;

#if VERBOSE_HISTOGRAM
	logInfo() << "Largest bucket: " << largest
	          << " bucket P85: " << referencePercentileValue
	          << "	 -> use log height scale: " << _useLogHeightScale
	          << Qt::endl;
#endif
    }
}


void HistogramView::calcGeometry( QSize newSize )
{
    _histogramWidth  = newSize.width();
    _histogramWidth -= leftBorder() + rightBorder() + 2 * viewMargin();

    if ( needOverflowPanel() )
    {
	_histogramWidth -= overflowSpacing() + overflowWidth();
        _histogramWidth -= 2 * overflowBorder();
    }

    _histogramHeight  = newSize.height();
    _histogramHeight -= bottomBorder() + topBorder() + 2 * viewMargin();
    _histogramHeight -= topTextHeight(); // compensate for text above

    // Constrain to at least the minimum overflow panel height and no more than the histogram width
    _histogramHeight  = qBound( MinHistogramHeight, _histogramHeight, _histogramWidth );

#if VERBOSE_HISTOGRAM
    logDebug() << "Histogram width: " << _histogramWidth
               << " height: " << _histogramHeight
               << Qt::endl;
#endif

    _geometryDirty    = false;
}


void HistogramView::fitToViewport()
{
    const QSize visibleSize = viewport()->size();
    QRectF rect = scene()->sceneRect();
    rect.adjust( -viewMargin(), -viewMargin(), viewMargin(), viewMargin() );

    // Scale everything down if the minimum item sizes are still too big
    if ( rect.width() > visibleSize.width() || rect.height() > visibleSize.height() )
    {
#if VERBOSE_HISTOGRAM
	logDebug() << "Scaling down histogram in " << rect.size()
	           << " to fit into visible size " << visibleSize
	           << Qt::endl;
#endif
	fitInView( rect, Qt::KeepAspectRatio );
    }
    else
    {
	// The histogram has already been sized to fit any large enough viewport
#if VERBOSE_HISTOGRAM
	logDebug() << "Histogram in " << rect.size()
	           << " fits into visible size " << visibleSize
	           << Qt::endl;
#endif
	resetTransform(); // Reset scaling
    }
}


void HistogramView::build()
{
    autoLogHeightScale();
    rebuild();
}


void HistogramView::rebuild()
{
     //logInfo() << "Building histogram" << Qt::endl;

    // Don't try this if the viewport geometry isn't set yet or we don't have any stats
    if ( !window()->isVisible() || !_stats )
	return;

    if ( _geometryDirty )
	calcGeometry( viewport()->size() );

    delete scene();
    setScene( new QGraphicsScene{ this } );

    // Get the palette in case the theme changed
    const QPalette palette = scene()->palette();
    scene()->setBackgroundBrush( palette.base() );

    // Keep all the brushes and pens together, even the ones that don't depend on the palette
    _panelBackground    = palette.alternateBase();
    _barBrush           = QColor{ 0xB0, 0xB0, 0xD0 };
    _barPen             = QColor{ 0x40, 0x40, 0x50 };
    _medianPen          = QPen{ palette.linkVisited().color(), 2 };
    _quartilePen        = QPen{ palette.link().color(), 2 };
    _percentilePen      = palette.color( QPalette::Disabled, QPalette::ButtonText );
    _decilePen          = palette.buttonText().color();
    _piePen             = palette.text().color();
    _overflowSliceBrush = QColor{ 0xD0, 0x40, 0x20 };

    addHistogramBackground();
    addAxes();
    addXAxisLabel();
    addYAxisLabel();
    addXStartEndLabels();
    addYStartEndLabels();
    addQuartileText();
    addHistogramBars();
    addMarkers();
    addOverflowPanel();

    fitToViewport();
}


void HistogramView::addHistogramBackground()
{
    const QRectF rect{ -leftBorder(),
                       -( topBorder() + _histogramHeight ),
                       leftBorder() + _histogramWidth + rightBorder(),
                       topBorder() + _histogramHeight + bottomBorder() };
    QGraphicsRectItem * panel = scene()->addRect( rect, QPen{ Qt::NoPen }, _panelBackground );
    panel->setZValue( PanelBackgroundLayer );
}


void HistogramView::addAxes()
{
    QPen pen{ scene()->palette().text().color(), 2 };

    QGraphicsItem * xAxis = scene()->addLine( 0, 0, _histogramWidth + axisExtraLength(), 0, pen );
    QGraphicsItem * yAxis = scene()->addLine( 0, 0, 0, -( _histogramHeight + axisExtraLength() ), pen );

    xAxis->setZValue( AxisLayer );
    yAxis->setZValue( AxisLayer );
}


void HistogramView::addXAxisLabel()
{
    QGraphicsTextItem * item = createBoldItem( tr( "File Size  -->" ) );

    const qreal textWidth   = item->boundingRect().width();
    const qreal textHeight  = item->boundingRect().height();

    item->setPos( ( _histogramWidth - textWidth ) / 2, bottomBorder() - textHeight );
}


void HistogramView::addYAxisLabel()
{
    QGraphicsTextItem * item = createBoldItem( "" );
    item->setHtml( (_useLogHeightScale ? "log<sub>2</sub>(n)   -->" : "n   -->") );

    const qreal textWidth  = item->boundingRect().width();
    const qreal textHeight = item->boundingRect().height();

    item->setRotation( 270 );
    item->setPos( ( textHeight + leftBorder() ) / -2, ( textWidth - _histogramHeight  ) / 2 );
}


void HistogramView::addXStartEndLabels()
{
    const QString startPercentileText =
	_startPercentile == _stats->minPercentile() ? tr( "Min" ) : tr( "P%1" ).arg( _startPercentile );
    const QString startLabel = startPercentileText % '\n' % formatSize( percentile( _startPercentile ) );
    QGraphicsTextItem * startItem = createTextItem( scene(), startLabel );
    startItem->setPos( 0, bottomBorder() - startItem->boundingRect().height() );

    const QString endPercentileText =
	_endPercentile == _stats->maxPercentile() ? tr( "Max" ) : tr( "P%1" ).arg( _endPercentile );
    const QString endLabel = endPercentileText % '\n' % formatSize( percentile( _endPercentile ) );
    QGraphicsTextItem * endItem = createTextItem( scene(), endLabel );
    const qreal endTextWidth = endItem->boundingRect().width();
    endItem->setTextWidth( endTextWidth );
    endItem->document()->setDefaultTextOption( QTextOption{ Qt::AlignRight } );
    endItem->setPos( _histogramWidth - endTextWidth, bottomBorder() - endItem->boundingRect().height() );
}


void HistogramView::addYStartEndLabels()
{
    QGraphicsTextItem * startItem = createTextItem( scene(), "0" );
    const QRectF startRect = startItem->boundingRect();
    startItem->setRotation( 270 );
    startItem->setPos( ( leftBorder() + startRect.height() ) / -2, startRect.width() / 2 );

    const FileCount max = *std::max_element( _stats->bucketsBegin(), _stats->bucketsEnd() );
    QGraphicsTextItem * endItem = createTextItem( scene(), QString{ "%L1" }.arg( max ) );
    const QRectF endRect = endItem->boundingRect();
    endItem->setRotation( 270 );
    endItem->setPos( ( endRect.height() + leftBorder() ) / -2, endRect.width() / 2 - _histogramHeight );
}


void HistogramView::addQuartileText()
{
    qreal     x = 0.0_qr;
    qreal     y = -_histogramHeight - topBorder() - textBorder();
    const int n = _stats->bucketsTotalSum();

    // Only add quartile labels if there is some data in the histogram
    if ( n > 0 )
    {
	const QString q1Text     = tr( "Q1: "     ) % formatSize( _stats->q1Value()     );
	const QString medianText = tr( "Median: " ) % formatSize( _stats->medianValue() );
	const QString q3Text     = tr( "Q3: "     ) % formatSize( _stats->q3Value()     );

	QGraphicsTextItem * q1Item     = createBoldItem( q1Text     );
	QGraphicsTextItem * medianItem = createBoldItem( medianText );
	QGraphicsTextItem * q3Item     = createBoldItem( q3Text     );

	q1Item->setDefaultTextColor    ( _quartilePen.color() );
	medianItem->setDefaultTextColor( _medianPen.color()   );
	q3Item->setDefaultTextColor    ( _quartilePen.color() );

	y -= medianItem->boundingRect().height();

	q1Item->setPos( x, y );
	x += q1Item->boundingRect().width() + textSpacing();
	medianItem->setPos( x, y );
	x += medianItem->boundingRect().width() + textSpacing();
	q3Item->setPos( x, y );
	x += q3Item->boundingRect().width() + textSpacing();
    }

    // Add text for the total number of files
    QGraphicsTextItem * nTextItem = createBoldItem( tr( "Files (n): %L1" ).arg( n ) );

    // Change the text to use the sigma character if it is available in the configured font
    const QFontMetrics metrics{ nTextItem->font() };
    const QChar sigma{ 0x2211 };
    if ( metrics.inFont( sigma ) )
	nTextItem->setPlainText( QString{ "%1n: %L2" }.arg( sigma ).arg( n ) );

    // Position the text line if it wasn't already done for the quartile
    if ( n == 0 )
	y -= nTextItem->boundingRect().height();

    nTextItem->setPos( x, y );
}


void HistogramView::addHistogramBars()
{
    const auto applyLogHeight = [ this ]( int height )
    {
	if ( !_useLogHeightScale )
	    return static_cast<double>( height );

	return height > 1 ? std::log2( height ) : height;
    };

    const qreal  barWidth = _histogramWidth / _stats->bucketCount();
    const double maxVal   = applyLogHeight( *std::max_element( _stats->bucketsBegin(), _stats->bucketsEnd() ) );

    for ( int i=0; i < _stats->bucketCount(); ++i )
    {
	// logDebug() << "Adding bar #" << i << " with value " << _stats->bucket( i ) << Qt::endl;

	const double val = applyLogHeight( _stats->bucket( i ) );
	const QRectF rect{ i * barWidth,
	                   topBorder() + viewMargin(),
	                   barWidth,
	                   -( _histogramHeight + axisExtraLength() ) };
	const qreal fillHeight = maxVal == 0 ? 0 : _histogramHeight * val / maxVal;
	scene()->addItem( new HistogramBar{ this, _stats, i, rect, fillHeight } );
    }
}


void HistogramView::addMarkers()
{
    const FileSize totalWidth = percentile( _endPercentile ) - percentile( _startPercentile );
    if ( totalWidth < 1 )
	return;

    // Show percentile marker lines
    for ( int i = _startPercentile + 1; i < _endPercentile; ++i )
    {
	if ( i == _stats->median() && _showMedian )
	{
	    addLine( _stats->median(), tr( "Median" ), _medianPen );
	    continue;
	}

	if ( i == _stats->quartile1() && _showQuartiles )
	{
	    addLine( _stats->quartile1(), tr( "Q1 (1st quartile)" ), _quartilePen );
	    continue;
	}

	if ( i == _stats->quartile3() && _showQuartiles )
	{
	    addLine( _stats->quartile3(), tr( "Q3 (3rd quartile)" ), _quartilePen );
	    continue;
	}

	// Skip start and end percentiles, and if configured for no percentile lines
	if ( _percentileStep == 0 || i == _stats->minPercentile() || i == _stats->maxPercentile() )
	    continue;

	// Skip markers that aren't in percentileStep increments ...
	// ... unless they are within the "margin" of the start or end percentile
	if ( _percentileStep != 1 && i % _percentileStep != 0 &&
	     i > _startPercentile + _leftMarginPercentiles &&
             i < _endPercentile - _rightMarginPercentiles )
	{
	    continue;
        }

	addLine( i, tr( "Percentile P%1" ).arg( i ), i % 10 == 0 ? _decilePen : _percentilePen );
    }
}


QPointF HistogramView::addText( QPointF pos, const QStringList & lines )
{
    QGraphicsTextItem * textItem = createTextItem( scene(), lines.join( u'\n' ) );
    textItem->setPos( pos );

    return { pos.x(), pos.y() + textItem->boundingRect().height() };
}


QGraphicsTextItem * HistogramView::createBoldItem( const QString & text )
{
    QGraphicsTextItem * textItem = createTextItem( scene(), text );
    setBold( textItem );
//    textItem->setDefaultTextColor( scene()->palette().text().color() );

    return textItem;
}

void HistogramView::addOverflowPanel()
{
    if ( ! needOverflowPanel() )
	return;

    // Create the panel area
    const QRectF rect{ _histogramWidth + rightBorder() + overflowSpacing(),
                       -( topBorder() + _histogramHeight ),
                       overflowWidth() + 2 * overflowBorder(),
                       topBorder() + _histogramHeight + bottomBorder() };
    QGraphicsRectItem * cutoffPanel = scene()->addRect( rect, QPen{ Qt::NoPen }, _panelBackground );

    // Headline
    QPointF nextPos{ rect.x() + overflowBorder(), rect.y() };
    QGraphicsTextItem * textItem = createBoldItem( tr( "Cut off percentiles" ) );
    textItem->setPos( nextPos );
    nextPos = { nextPos.x(), nextPos.y() + textItem->boundingRect().height() + 4 };

    const auto cutoffLines = [ this ]()
    {
	return QStringList
	    { tr( "Min (P0) ... P%1" ).arg( _startPercentile ),
	      _startPercentile == _stats->minPercentile() ?
		    tr( "no files cut off" ) :
		    formatSize( _stats->minValue() ) % "..."_L1 % formatSize( percentile( _startPercentile ) ),
	      "",
	      tr( "P%1 ... Max (P100)" ).arg( _endPercentile ),
	      _endPercentile == _stats->maxPercentile() ?
		    tr( "no files cut off" ) :
		    formatSize( percentile( _endPercentile ) ) % "..."_L1 % formatSize( _stats->maxValue() ),
	      "",
	    };
    };
    nextPos = addText( nextPos, cutoffLines() );

    // Upper pie chart: number of files cut off
    nextPos.setY( nextPos.y() + pieSliceOffset() );
    QRectF pieRect{ nextPos, QSizeF{ pieDiameter(), pieDiameter() } };

    const int cutoff = _startPercentile + _stats->maxPercentile() - _endPercentile;
    nextPos = addPie( pieRect, _stats->maxPercentile() - cutoff, cutoff, _barBrush, _overflowSliceBrush );

    // Caption for the upper pie chart
    const FileCount histogramFiles = _stats->bucketsTotalSum();
    const FileCount totalFiles     = std::round( histogramFiles * 100.0 / ( _endPercentile - _startPercentile ) );
    const FileCount missingFiles   = totalFiles - histogramFiles;
    const QString cutoffCaption = missingFiles == 1 ?
                                  tr( "1 file cut off" ) :
                                  tr( "%L1 files cut off" ).arg( missingFiles );
    nextPos = addText( nextPos, { cutoffCaption, tr( "%1% of all files" ).arg( cutoff ), "" } );

    // Lower pie chart: disk space disregarded
    const FileSize histogramDiskSpace = percentileSum( _stats, _startPercentile, _endPercentile );
    const FileSize cutoffDiskSpace = [ this ]()
	{
	    const FileSize startSpace = percentileSum( _stats, _stats->minPercentile(), _startPercentile-1 );
	    const FileSize endSpace   = percentileSum( _stats, _endPercentile+1, _stats->maxPercentile() );
	    return startSpace + endSpace;
	}();

    nextPos.setY( nextPos.y() + pieSliceOffset() );
    pieRect = QRectF{ nextPos, QSizeF{ pieDiameter(), pieDiameter() } };

    if ( cutoffDiskSpace > histogramDiskSpace )
	nextPos = addPie( pieRect, cutoffDiskSpace, histogramDiskSpace, _overflowSliceBrush, _barBrush );
    else
	nextPos = addPie( pieRect, histogramDiskSpace, cutoffDiskSpace, _barBrush, _overflowSliceBrush );

    // Caption for the lower pie chart
    const double cutoffSpacePercent = 100.0 * cutoffDiskSpace / ( histogramDiskSpace + cutoffDiskSpace );
    const QStringList pieCaption2{ formatSize( cutoffDiskSpace ) % " cut off"_L1,
                                   tr( "%1% of disk space" ).arg( cutoffSpacePercent, 0, 'f', 1 ),
                                   ""
                                 };
    nextPos = addText( nextPos, pieCaption2 );

    // Make sure the panel is tall enough to fit everything in
    if ( nextPos.y() > cutoffPanel->rect().bottom() )
    {
	QRectF rect{ cutoffPanel->rect() };
	rect.setBottomLeft( { rect.x(), nextPos.y() } );
	cutoffPanel->setRect( rect );
    }
}


void HistogramView::addLine( int             percentileIndex,
                             const QString & name,
                             const QPen    & pen )
{
    const FileSize xValue       = percentile( percentileIndex  );
    const FileSize axisStartVal = percentile( _startPercentile );
    const FileSize axisEndVal   = percentile( _endPercentile   );
    const FileSize totalWidth   = axisEndVal - axisStartVal;
    const qreal    x            = _histogramWidth * ( xValue - axisStartVal ) / totalWidth;

    QGraphicsLineItem * line =
	new QGraphicsLineItem{ x, markerExtraHeight(), x, -( _histogramHeight + markerExtraHeight() ) };
    line->setToolTip( whitespacePre( name % "<br/>"_L1 % formatSize( xValue ) ) );
    line->setZValue( name.isEmpty() ? MarkerLayer : SpecialMarkerLayer );
    line->setPen( pen );
    line->setFlags( QGraphicsLineItem::ItemIsSelectable );

    scene()->addItem( line );
}


QPointF HistogramView::addPie( const QRectF & rect,
                               FileSize       val1,
                               FileSize       val2,
                               const QBrush & brush1,
                               const QBrush & brush2 )
{
    if ( val1 == 0 && val2 == 0 )
	return rect.topLeft();

    const int fullCircle = 360 * 16; // Qt uses 1/16 degrees
    const int angle1     = qRound( 1.0 * val1 / ( val1 + val2 ) * fullCircle );
    const int angle2     = fullCircle - angle1;

    QGraphicsEllipseItem * slice1 = scene()->addEllipse( rect );
    slice1->setStartAngle( angle2 / 2 );
    slice1->setSpanAngle( angle1 );
    slice1->setBrush( brush1 );
    slice1->setPen( _piePen );

    QRectF rect2{ rect };
    rect2.moveTopLeft( rect.topLeft() + QPointF{ pieSliceOffset(), 0.0_qr } );

    QGraphicsEllipseItem * slice2 = scene()->addEllipse( rect2 );
    slice2->setStartAngle( -angle2 / 2 );
    slice2->setSpanAngle( angle2 );
    slice2->setBrush( brush2 );
    slice2->setPen( _piePen );

    QGraphicsItemGroup * pie = scene()->createItemGroup( { slice1, slice2 } );
    const QPointF pieCenter = rect.center();

    QTransform transform;
    transform.translate( pieCenter.x(), pieCenter.y() );
    transform.rotate( -45.0_qr );
    transform.translate( -pieCenter.x(), -pieCenter.y() );
    pie->setTransform( transform );

    return { rect.x(), rect.y() + pie->boundingRect().height() };
}


void HistogramView::resizeEvent( QResizeEvent * event )
{
    // logDebug() << "Event size: " << event->size() << Qt::endl;

    QGraphicsView::resizeEvent( event );
//    calcGeometry( event->size() );
    _geometryDirty = true;

    rebuild();
}
