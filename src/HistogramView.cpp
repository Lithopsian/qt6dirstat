/*
 *   File name: HistogramView.cpp
 *   Summary:   View widget for histogram rendering for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <cmath> // log2()
#include <algorithm> // std::max_element

#include <QGraphicsItem>
#include <QResizeEvent>
#include <QTextDocument>

#include "HistogramView.h"
#include "HistogramItems.h"
#include "Exception.h"
#include "FileSizeStats.h"
#include "FormatUtil.h"


#define VERBOSE_HISTOGRAM 0

#define MinHistogramWidth	 500.0_qr
#define MinHistogramHeight	 300.0_qr


using namespace QDirStat;


namespace
{
    /**
     * Make the QGraphicsTextItem text bold.
     **/
    void setBold( QGraphicsTextItem * item )
    {
	QFont font( item->font() );
	font.setBold( true );
	item->setFont( font );
    }

} // namespace


void HistogramView::init( const FileSizeStats * stats )
{
    CHECK_PTR( stats );
    _stats = stats;

    _geometryDirty  = true;

    _startPercentile   = 0;    // data min
    _endPercentile     = 100;  // data max
    _useLogHeightScale = false;
}


FileSize HistogramView::percentile( int index ) const
{
    return _stats->percentileList( index );
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

FileSize HistogramView::percentileSum( int fromIndex, int toIndex ) const
{
    FileSize sum = 0LL;

    for ( int i = fromIndex; i <= toIndex; ++i )
	sum += _stats->percentileSums( i );

    return sum;
}


void HistogramView::autoStartEndPercentiles()
{
    const FileSize q1    = percentile( 25 );
    const FileSize q3    = percentile( 75 );
    const FileSize qDist = q3 - q1;

    // Outliers are classed as more than three times the IQR beyond the 3rd quartile
    // Just use the IQR beyond the 1st quartile to match the typical skewed file size distribution
    const FileSize minVal = qMax( q1 - qDist, 0LL );
    const FileSize maxVal = qMin( q3 + qDist * 3, percentile( 100 ) );

    const bool oldNeedOverflowPanel = needOverflowPanel();

    for ( _startPercentile = 0;
	  _startPercentile < 25 && percentile( _startPercentile ) < minVal;
	  ++_startPercentile );

    for ( _endPercentile = 100;
	  _endPercentile > 75 && percentile( _endPercentile ) > maxVal;
	  --_endPercentile );

    if ( oldNeedOverflowPanel != needOverflowPanel() )
        _geometryDirty = true;

#if VERBOSE_HISTOGRAM
    logInfo() << "Q1: " << formatSize( q1 )
	      << "  Q3: " << formatSize( q3 )
	      << "  minVal: " << formatSize( minVal )
	      << "  maxVal: " << formatSize( maxVal )
	      << Qt::endl;
    logInfo() << "startPercentile: " << _startPercentile
	      << "  " << formatSize( percentile( _startPercentile ) )
	      << "  endPercentile: " << _endPercentile
	      << "  " << formatSize( percentile( _endPercentile	 ) )
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

    if ( _histogramWidth < MinHistogramWidth )
	_histogramWidth = MinHistogramWidth;

    _histogramHeight  = newSize.height();
    _histogramHeight -= bottomBorder() + topBorder() + 2 * viewMargin();
    _histogramHeight -= topTextHeight(); // compensate for text above

    _histogramHeight  = qBound( MinHistogramHeight, _histogramHeight, 1.5 * _histogramWidth );
    _geometryDirty    = false;

#if VERBOSE_HISTOGRAM
    logDebug() << "Histogram width: " << _histogramWidth
	       << " height: " << _histogramHeight
	       << Qt::endl;
#endif
}


void HistogramView::resizeEvent( QResizeEvent * event )
{
    // logDebug() << "Event size: " << event->size() << Qt::endl;

    QGraphicsView::resizeEvent( event );
    calcGeometry( event->size() );

    rebuild();
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
	setTransform( QTransform() ); // Reset scaling
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

    // Don't try this if the viewport geometry isn't set yet
    if ( !isVisible() )
	return;

    if ( _geometryDirty )
	calcGeometry( viewport()->size() );

    delete scene();
    setScene( new QGraphicsScene( this ) );

    const QPalette palette = scene()->palette();
    scene()->setBackgroundBrush( palette.base() );

    _panelBackground    = palette.alternateBase();
    _barBrush           = QColor( 0xB0, 0xB0, 0xD0 );
    _barPen             = QColor( 0x40, 0x40, 0x50 );
    _medianPen          = QPen( palette.linkVisited().color(), 2 );
    _quartilePen        = QPen( palette.link().color(), 2 );
    _percentilePen      = palette.color( QPalette::Disabled, QPalette::ButtonText );
    _decilePen          = palette.buttonText().color();
    _piePen             = palette.text().color();
    _overflowSliceBrush = QColor( 0xD0, 0x40, 0x20 );

    addHistogramBackground();
    addAxes();
    addYAxisLabel();
    addXAxisLabel();
    addXStartEndLabels();
    addQuartileText();
    addHistogramBars();
    addMarkers();
    addOverflowPanel();

    fitToViewport();
}


void HistogramView::addHistogramBackground()
{
    const QRectF rect = QRectF(-leftBorder(),
			       -( topBorder() + _histogramHeight ),
			       leftBorder() + _histogramWidth + rightBorder(),
			       topBorder() + _histogramHeight + bottomBorder() );
    QGraphicsRectItem * panel = scene()->addRect( rect, QPen( Qt::NoPen ), _panelBackground );
    panel->setZValue( PanelBackgroundLayer );
}


void HistogramView::addAxes()
{
    QPen pen( scene()->palette().text().color(), 2 );

    QGraphicsItem * xAxis = scene()->addLine( 0, 0, _histogramWidth + axisExtraLength() , 0, pen );
    QGraphicsItem * yAxis = scene()->addLine( 0, 0, 0, -( _histogramHeight + axisExtraLength() ) , pen );

    xAxis->setZValue( AxisLayer );
    yAxis->setZValue( AxisLayer );
}


void HistogramView::addYAxisLabel()
{
    QGraphicsTextItem * item = scene()->addText( "" );
    item->setHtml( (_useLogHeightScale ? "log<sub>2</sub>(n)   -->" : "n   -->") );
    setBold( item );

    const qreal   textWidth   = item->boundingRect().width();
    const qreal   textHeight  = item->boundingRect().height();
    const QPointF labelCenter = { -leftBorder() / 2, -_histogramHeight / 2 };

    item->setRotation( 270 );
    item->setPos( labelCenter.x() - textHeight / 2, labelCenter.y() + textWidth  / 2 );

    item->setZValue( TextLayer );
}


void HistogramView::addXAxisLabel()
{
    QGraphicsTextItem * item = scene()->addText( tr( "File Size  -->" ) );
    setBold( item );

    const qreal   textWidth   = item->boundingRect().width();
    const qreal   textHeight  = item->boundingRect().height();
    const QPointF labelCenter = { _histogramWidth / 2, bottomBorder() };
    item->setPos( labelCenter.x() - textWidth / 2, labelCenter.y() - textHeight ); // Align bottom

    item->setZValue( TextLayer );
}


void HistogramView::addXStartEndLabels()
{
    QString startLabel = _startPercentile == 0 ? tr( "Min" ) : 'P' % QString::number( _startPercentile );
    startLabel += '\n' % formatSize( percentile( _startPercentile ) );

    QString endLabel = _endPercentile == 100 ? tr( "Max" ) : 'P' % QString::number( _endPercentile );
    endLabel += '\n' % formatSize( percentile( _endPercentile ) );

    QGraphicsTextItem * startItem = scene()->addText( startLabel );
    QGraphicsTextItem * endItem   = scene()->addText( endLabel );

    const qreal endTextWidth = endItem->boundingRect().width();
    endItem->setTextWidth( endTextWidth );
    endItem->document()->setDefaultTextOption( QTextOption( Qt::AlignRight ) );

    startItem->setPos( 0, bottomBorder() - startItem->boundingRect().height() );
    endItem->setPos( _histogramWidth - endTextWidth, bottomBorder() - endItem->boundingRect().height() );

    startItem->setZValue( TextLayer );
    endItem->setZValue( TextLayer );

}


void HistogramView::addQuartileText()
{
    qreal     x = 0.0_qr;
    qreal     y = -_histogramHeight - topBorder() - textBorder();
    const int n = _stats->bucketsTotalSum();

    if ( n > 0 ) // Only useful if there is any data at all
    {
	const QString q1Text     = tr( "Q1: "     ) % formatSize( percentile( 25 ) );
	const QString medianText = tr( "Median: " ) % formatSize( percentile( 50 ) );
	const QString q3Text     = tr( "Q3: "     ) % formatSize( percentile( 75 ) );

	QGraphicsTextItem * q1Item     = scene()->addText( q1Text     );
	QGraphicsTextItem * medianItem = scene()->addText( medianText );
	QGraphicsTextItem * q3Item     = scene()->addText( q3Text     );

	q1Item->setDefaultTextColor    ( _quartilePen.color() );
	medianItem->setDefaultTextColor( _medianPen.color()   );
	q3Item->setDefaultTextColor    ( _quartilePen.color() );

	setBold( medianItem);
	setBold( q1Item);
	setBold( q3Item);

	y -= medianItem->boundingRect().height();

	const qreal q1Width     = q1Item->boundingRect().width();
	const qreal medianWidth = medianItem->boundingRect().width();
	const qreal q3Width     = q3Item->boundingRect().width();

	q1Item->setPos( x, y );
	x += q1Width + textSpacing();
	medianItem->setPos( x, y );
	x += medianWidth + textSpacing();
	q3Item->setPos( x, y );
	x += q3Width + textSpacing();

	q1Item->setZValue( TextLayer );
	medianItem->setZValue( TextLayer );
	q3Item->setZValue( TextLayer );
    }

    // Add text for the total number of files
    QGraphicsTextItem * nTextItem = scene()->addText( tr( "Files (n): %1" ).arg( n ) );
    setBold( nTextItem );

    const QFontMetrics metrics( nTextItem->font() );
    const QChar sigma( 0x2211 );
    if ( metrics.inFont( sigma ) )
	nTextItem->setPlainText( QString( "%1n: %L2" ).arg( sigma ).arg( n ) );

    if ( n == 0 )
	y -= nTextItem->boundingRect().height();

    nTextItem->setPos( x, y );
    nTextItem->setZValue( TextLayer );
}


void HistogramView::addHistogramBars()
{
    auto applyLogHeight = [ this ]( int height ) -> double
    {
	if ( !_useLogHeightScale )
	    return height;

	return height > 1 ? std::log2( height ) : height / 2.0;
    };

    const qreal barWidth = _histogramWidth / _stats->bucketCount();
    const double maxVal = applyLogHeight( *std::max_element( _stats->bucketsBegin(), _stats->bucketsEnd() ) );

    for ( int i=0; i < _stats->bucketCount(); ++i )
    {
	// logDebug() << "Adding bar #" << i << " with value " << _stats->bucket( i ) << Qt::endl;

	const double val = applyLogHeight( _stats->bucket( i ) );
	const QRectF rect( i * barWidth, topBorder() + viewMargin(), barWidth, -( _histogramHeight + axisExtraLength() ) );
	const qreal fillHeight = maxVal == 0 ? 0 : _histogramHeight * val / maxVal;
	scene()->addItem( new HistogramBar( this, _stats, i, rect, fillHeight ) );
    }
}


void HistogramView::addMarkers()
{
    const FileSize totalWidth = percentile( _endPercentile ) - percentile( _startPercentile );
    if ( totalWidth < 1 )
	return;

    // Show ordinary percentiles (all except Q1, Median, Q3)
    for ( int i = _startPercentile + 1; i < _endPercentile; ++i )
    {
	if ( _percentileStep == 0 || i == 0 || i == 100 )
	    continue;

	if ( i == 50 && _showMedian )
	    continue;

	if ( ( i == 25 || i == 75 ) && _showQuartiles )
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

    if ( _showQuartiles )
    {
	if ( percentileDisplayed( 25 ) )
	    addLine( 25, tr( "Q1 (1st quartile)" ), _quartilePen );

	if ( percentileDisplayed( 75 ) )
	    addLine( 75, tr( "Q3 (3rd quartile)" ), _quartilePen );
    }

    if ( _showMedian && percentileDisplayed( 50 ) )
	addLine( 50, tr( "Median" ), _medianPen );
}


QGraphicsTextItem * HistogramView::addText( QPointF pos, const QString & text )
{
    QGraphicsTextItem * textItem = scene()->addText( text );
    textItem->setPos( pos );
    textItem->setDefaultTextColor( scene()->palette().text().color() );

    return textItem;
}

QPointF HistogramView::addText( QPointF pos, const QStringList & lines )
{
    const QGraphicsTextItem * textItem = addText( pos, lines.join( u'\n' ) );

    return { pos.x(), pos.y() + textItem->boundingRect().height() };
}


QPointF HistogramView::addBoldText( QPointF pos, const QString & text )
{
    QGraphicsTextItem * textItem = addText( pos, text );
    setBold( textItem );

    return { pos.x(), pos.y() + textItem->boundingRect().height() + 4 };
}


void HistogramView::addOverflowPanel()
{
    if ( ! needOverflowPanel() )
	return;

    // Create the panel area
    const QRectF rect( _histogramWidth + rightBorder() + overflowSpacing(),
		       -( topBorder() + _histogramHeight ),
		       overflowWidth() + 2 * overflowBorder(),
		       topBorder() + _histogramHeight + bottomBorder() );
    QGraphicsRectItem * cutoffPanel = scene()->addRect( rect, QPen( Qt::NoPen ), _panelBackground );

    const auto cutoffLines = [ this ]()
    {
	return QStringList
	    { tr( "Min (P0) ... P%1" ).arg( _startPercentile ),
	      _startPercentile == 0 ?
		    tr( "no files cut off" ) :
		    formatSize( percentile( 0 ) ) % "..."_L1 % formatSize( percentile( _startPercentile ) ),
	      "",
	      tr( "P%1 ... Max (P100)" ).arg( _endPercentile ),
	      _endPercentile == 100 ?
		    tr( "no files cut off" ) :
		    formatSize( percentile( _endPercentile ) ) % "..."_L1 % formatSize( percentile( 100 ) ),
	      "",
	    };
    };

    // Headline
    QPointF nextPos( rect.x() + overflowBorder(), rect.y() );
    nextPos = addBoldText( nextPos, tr( "Cut off percentiles" ) );
    nextPos = addText( nextPos, cutoffLines() );

    // Upper pie chart: number of files cut off
    nextPos.setY( nextPos.y() + pieSliceOffset() );
    QRectF pieRect( QRectF( nextPos, QSizeF( pieDiameter(), pieDiameter() ) ) );

    const int cutoff = _startPercentile + 100 - _endPercentile;
    nextPos = addPie( pieRect, 100 - cutoff, cutoff, _barBrush, _overflowSliceBrush );

    // Caption for the upper pie chart
    const FileCount histogramFiles = _stats->bucketsTotalSum();
    const FileCount totalFiles     = std::round( histogramFiles * 100.0 / ( _endPercentile - _startPercentile ) );
    const FileCount missingFiles   = totalFiles - histogramFiles;
    const QString cutoffCaption = missingFiles == 1 ?
                                  tr( "1 file cut off" ) :
                                  tr( "%L1 files cut off" ).arg( missingFiles );
    nextPos = addText( nextPos, { cutoffCaption, tr( "%1% of all files" ).arg( cutoff ), "" } );

    // Lower pie chart: disk space disregarded
    const FileSize histogramDiskSpace = percentileSum( _startPercentile, _endPercentile );
    FileSize cutoffDiskSpace          = percentileSum( 0, _startPercentile );

    if ( _endPercentile < 100 )
        cutoffDiskSpace += percentileSum( _endPercentile, 100 );

    nextPos.setY( nextPos.y() + pieSliceOffset() );
    pieRect = QRectF( nextPos, QSizeF( pieDiameter(), pieDiameter() ) );

    if ( cutoffDiskSpace > histogramDiskSpace )
	nextPos = addPie( pieRect, cutoffDiskSpace, histogramDiskSpace, _overflowSliceBrush, _barBrush );
    else
	nextPos = addPie( pieRect, histogramDiskSpace, cutoffDiskSpace, _barBrush, _overflowSliceBrush );

    // Caption for the lower pie chart
    const double cutoffSpacePercent = 100.0 * cutoffDiskSpace / ( histogramDiskSpace + cutoffDiskSpace );
    const QStringList pieCaption2 { formatSize( cutoffDiskSpace ) % " cut off"_L1,
                                    tr( "%1% of disk space" ).arg( cutoffSpacePercent, 0, 'f', 1 ),
                                    ""
                                  };
    nextPos = addText( nextPos, pieCaption2 );

    // Make sure the panel is tall enough to fit everything in
    if ( nextPos.y() > cutoffPanel->rect().bottom() )
    {
	QRectF rect( cutoffPanel->rect() );
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
	new QGraphicsLineItem( x, markerExtraHeight(), x, -( _histogramHeight + markerExtraHeight() ) );
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

    QRectF rect2( rect );
    rect2.moveTopLeft( rect.topLeft() + QPointF( pieSliceOffset(), 0.0_qr) );

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
