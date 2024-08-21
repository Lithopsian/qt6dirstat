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


using namespace QDirStat;


namespace
{
    /**
     * Return the percentage represented by 'count' wrt 'total',
     * returning 0.0 if 'total' is 0.
     **/
    double percent( qint64 count, qint64 total )
    {
	return total ? 100.0 * count / total : 0.0;
    }


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


    /**
     * Create a text item and make it bold.  The text is added to 'scene'
     * but not positioned.
     **/
    QGraphicsTextItem * createBoldItem( QGraphicsScene * scene, const QString & text )
    {
	QGraphicsTextItem * textItem = createTextItem( scene, text );
	setBold( textItem );

	return textItem;
    }

} // namespace


void HistogramView::init( const FileSizeStats * stats )
{
    CHECK_PTR( stats );
    _stats = stats;

    setGeometryDirty();

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
        setGeometryDirty();
}


void HistogramView::setEndPercentile( int index )
{
    CHECK_PERCENTILE_INDEX( index );

    const bool oldNeedOverflowPanel = needOverflowPanel();
    _endPercentile = index;

    if ( oldNeedOverflowPanel != needOverflowPanel() )
	setGeometryDirty();

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
    const FileSize minVal = qMax( q1Value - iqr, _stats->minValue() );
    const FileSize maxVal = ( 3.0 * iqr + q3Value > maxValue ) ? maxValue : ( 3 * iqr + q3Value );

    // Find the highest percentile that has a value less than minVal
    _startPercentile = _stats->minPercentile();
    while ( percentile( _startPercentile ) < minVal )
	++_startPercentile;

    // Find the lowest percentile that has a value greater than maxVal
    _endPercentile = _stats->maxPercentile();
    while ( percentile( _endPercentile ) > maxVal )
	--_endPercentile;

    if ( oldNeedOverflowPanel != needOverflowPanel() )
	setGeometryDirty();

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
	const int largestBucket = *std::max_element( _stats->bucketsBegin(), _stats->bucketsEnd() );

	// We compare the largest bucket with the P85 percentile of the buckets,
	// but make sure we're not comparing with an empty bucket
	const int referencePercentileValue =
	    qMax( _stats->bucket( qRound( _stats->bucketCount() * 85.0 / 100.0 ) ), 1 );

	_useLogHeightScale = largestBucket > referencePercentileValue * 20;

#if VERBOSE_HISTOGRAM
	logInfo() << "Largest bucket: " << largestBucket
	          << " bucket P85: " << referencePercentileValue
	          << "	 -> use log height scale: " << _useLogHeightScale
	          << Qt::endl;
#endif
    }
}


void HistogramView::calcGeometry()
{
    _size = viewport()->size();
    const qreal verticalPadding = bottomBorder() + topBorder() + 2 * viewMargin() + topTextHeight();

    if ( _size.height() < _minHeight + verticalPadding )
    {
	// Will be scaled down to fit the viewport height, so up-scale the width to compensate
	_size.rwidth() *= ( _minHeight + verticalPadding ) / _size.height();
	_size.rheight() = _minHeight;
    }
    else
    {
	_size.rheight() -= verticalPadding;
    }

    _size.rwidth() -= leftBorder() + rightBorder() + 2 * viewMargin();
    if ( needOverflowPanel() )
	_size.rwidth() -= overflowGap() + overflowWidth();

#if VERBOSE_HISTOGRAM
    logDebug() << "Histogram width: " << _size.width()
               << " height: " << _size.height()
               << Qt::endl;
#endif
}


void HistogramView::fitToViewport()
{
    const QSize visibleSize = viewport()->size();
    const QRectF rect =
	scene()->sceneRect().adjusted( -viewMargin(), -viewMargin(), viewMargin(), viewMargin() );

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

    if ( geometryDirty() )
	calcGeometry();

    delete scene();
    setScene( new QGraphicsScene{ this } );

    setBackgroundBrush( background() );

    addBackground();
    addAxes();
    addXAxisLabel();
    addYAxisLabel();
    addXStartEndLabels();
    addYStartEndLabels();
    addQuartileText();
    addBars();
    addMarkers();
    addOverflowPanel();

    fitToViewport();
}


void HistogramView::addBackground()
{
    const QPointF pos{ -leftBorder(), -topBorder() - _size.height() };
    const QSizeF size{ _size + QSizeF{ leftBorder() + rightBorder(), topBorder() + bottomBorder() } };
    const QRectF rect{ pos, size };
    QGraphicsRectItem * panel = scene()->addRect( rect, Qt::NoPen, panelBackground() );
    panel->setZValue( PanelBackgroundLayer );
}


void HistogramView::addAxes()
{
    auto drawLine = [ this ]( qreal x, qreal y )
	{
	    QGraphicsItem * line = scene()->addLine( 0, 0, x, y, linePen() );
	    line->setZValue( AxisLayer );
	};

    drawLine( _size.width() + axisExtraLength(), 0 );
    drawLine( 0, -_size.height() - axisExtraLength() );
}


void HistogramView::addXAxisLabel()
{
    QGraphicsTextItem * item = createBoldItem( scene(), tr( "File Size  -->" ) );

    const qreal textWidth   = item->boundingRect().width();
    const qreal textHeight  = item->boundingRect().height();

    item->setPos( ( _size.width() - textWidth ) / 2, bottomBorder() - textHeight );
}


void HistogramView::addYAxisLabel()
{
    QGraphicsTextItem * item = createBoldItem( scene(), "n   -->" );
    if ( _useLogHeightScale )
	item->setHtml( "log<sub>2</sub>(n)   -->" );

    const qreal textWidth  = item->boundingRect().width();
    const qreal textHeight = item->boundingRect().height();

    item->setRotation( 270 );
    item->setPos( ( textHeight + leftBorder() ) / -2, ( textWidth - _size.height() ) / 2 );
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
    endItem->setPos( _size.width() - endTextWidth, bottomBorder() - endItem->boundingRect().height() );
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
    endItem->setPos( ( endRect.height() + leftBorder() ) / -2, endRect.width() / 2 - _size.height() );
}


void HistogramView::addQuartileText()
{
    QPointF pos{ 0, -_size.height() - topBorder() - textBorder() };
    const int n = _stats->bucketsTotalSum();

    // Only add quartile labels if there is some data in the histogram
    if ( n > 0 )
    {
	const QString q1Text     = tr( "Q1: "     ) % formatSize( _stats->q1Value()     );
	const QString medianText = tr( "Median: " ) % formatSize( _stats->medianValue() );
	const QString q3Text     = tr( "Q3: "     ) % formatSize( _stats->q3Value()     );

	QGraphicsTextItem * q1Item     = createBoldItem( scene(), q1Text     );
	QGraphicsTextItem * medianItem = createBoldItem( scene(), medianText );
	QGraphicsTextItem * q3Item     = createBoldItem( scene(), q3Text     );

	q1Item->setDefaultTextColor    ( quartilePen() );
	medianItem->setDefaultTextColor( medianPen()   );
	q3Item->setDefaultTextColor    ( quartilePen() );

	pos.ry() -= medianItem->boundingRect().height();

	q1Item->setPos( pos );
	pos.rx() += q1Item->boundingRect().width() + textSpacing();
	medianItem->setPos( pos );
	pos.rx() += medianItem->boundingRect().width() + textSpacing();
	q3Item->setPos( pos );
	pos.rx() += q3Item->boundingRect().width() + textSpacing();
    }

    // Add text for the total number of files
    const QFontMetrics metrics{ font() };
    const QChar sigma{ 0x2211 };
    const QString nTextTemplate =
	metrics.inFont( sigma ) ? QString{ "%1n: %L2" }.arg( sigma ) : tr( "Files (n): %L1" );
    QGraphicsTextItem * nTextItem = createBoldItem( scene(), nTextTemplate.arg( n ) );

    // Position the text line if it wasn't already done for the quartile
    if ( n == 0 )
	pos.ry() -= nTextItem->boundingRect().height();

    nTextItem->setPos( pos );
}


void HistogramView::addBars()
{
    const auto applyLogHeight = [ this ]( int height )
    {
	if ( !_useLogHeightScale )
	    return static_cast<double>( height );

	return height > 1 ? std::log2( height ) : height / 2.0;
    };

    const qreal barWidth = _size.width() / _stats->bucketCount();
    const double maxVal =
	applyLogHeight( *std::max_element( _stats->bucketsBegin(), _stats->bucketsEnd() ) );

    for ( int i=0; i < _stats->bucketCount(); ++i )
    {
	// logDebug() << "Adding bar #" << i << " with value " << _stats->bucket( i ) << Qt::endl;

	const double val = applyLogHeight( _stats->bucket( i ) );
	const QRectF rect{ i * barWidth,
	                   topBorder() + viewMargin(),
	                   barWidth,
	                   -_size.height() - axisExtraLength() };
	const qreal fillHeight = maxVal == 0 ? 0 : _size.height() * val / maxVal;
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
	    addLine( _stats->median(), tr( "Median" ), QPen{ medianPen(), 2 } );
	    continue;
	}

	if ( i == _stats->quartile1() && _showQuartiles )
	{
	    addLine( _stats->quartile1(), tr( "Q1 (1st quartile)" ), QPen{ quartilePen(), 2} );
	    continue;
	}

	if ( i == _stats->quartile3() && _showQuartiles )
	{
	    addLine( _stats->quartile3(), tr( "Q3 (3rd quartile)" ), QPen{ quartilePen(), 2 } );
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

	addLine( i, tr( "Percentile P%1" ).arg( i ), i % 10 == 0 ? decilePen() : percentilePen() );
    }
}


qreal HistogramView::overflowWidth()
{
    QFont font;
    font.setBold( true);
    const qreal headlineWidth = textWidth( font, overflowHeadline() );

    return qMax( pieDiameter() + pieSliceOffset() * 2, headlineWidth ) + 2 * overflowBorder();
}


void HistogramView::addOverflowPanel()
{
    if ( !needOverflowPanel() )
	return;

    // Create the panel area
    const qreal panelWidth = overflowWidth();
    const QRectF rect{ _size.width() + rightBorder() + overflowGap(),
                       -topBorder() - _size.height(),
                       panelWidth,
                       topBorder() + _size.height() + bottomBorder() };
    QGraphicsRectItem * overflowPanel = scene()->addRect( rect, Qt::NoPen, panelBackground() );
    QPointF nextPos{ rect.x(), rect.y() };

    // Headline
    QGraphicsTextItem * headline = createBoldItem( scene(), overflowHeadline() );
    const qreal xAlign = ( panelWidth - headline->boundingRect().width() ) / 2;
    headline->setPos( nextPos + QPointF{ xAlign, overflowSpacing() / 2 } );
    nextPos.ry() += headline->boundingRect().height() + overflowSpacing();

    /**
     * Add multiple text items on separate lines starting at 'nextPos',
     * then add a margin at the bottom.  Each line is aligned in the
     * centre of the overflow panel.  nextPos is updated to the bottom
     * left of the margin.
     **/
    auto addText = [ this, panelWidth, &nextPos ]( const QStringList & lines )
    {
	QGraphicsTextItem * textItem = createTextItem( scene(), lines.join( u'\n' ) );
	textItem->setPos( nextPos );
	textItem->setTextWidth( panelWidth );
	textItem->document()->setDefaultTextOption( QTextOption{ Qt::AlignHCenter } );

	nextPos.ry() += textItem->boundingRect().height() + overflowSpacing();
    };

    /**
     * Add a pie diagram with two values valPie and valSlice, aligned
     * in the centre of the overflow panel. A segment of the pie
     * proportional to valSlice is drawn offset from the main pie and
     * in a different colour.  The smaller value (and its brush) is
     * always used for the offset segment.
     *
     * The pie is constructed with the segment extracted towards
     * the right and then rotated 45 degrees anti-clockwise.
     * This keeps the bounding rectangle a constant height so
     * that it doesn't move about as the cutoff percentiles are
     * changed.
     *
     * nextPos is updated by the height of the pie.
     **/
    auto addPie = [ this, panelWidth, &nextPos ]( FileSize valSlice, FileSize valPie )
    {
	if ( valPie == 0 && valSlice == 0 )
	    return;

	// If pie is bigger than slice, swap them including the brushes
	const bool swapped = valSlice > valPie;
	if ( swapped )
	{
	    FileSize val = valSlice;
	    valSlice = valPie;
	    valPie = val;
	}
	const QBrush brushSlice = swapped ? barBrush() : overflowSliceBrush();
	const QBrush brushPie   = swapped ? overflowSliceBrush() : barBrush();

	// Create the pie at the origin, so it can be rotated and then positioned afterwards
	const QRectF rect{ -pieDiameter() / 2, -pieDiameter() / 2, pieDiameter(), pieDiameter() };

	// Convert the slice value to a segment in Qt units of 1/16th degree
	const int fullCircle = 360 * 16;
	const int segment    = qRound( 1.0 * valSlice / ( valPie + valSlice ) * fullCircle );

	// Create a circle with a segment missing
	QGraphicsEllipseItem * ellipsePie = scene()->addEllipse( rect );
	ellipsePie->setStartAngle( segment / 2 );
	ellipsePie->setSpanAngle( fullCircle - segment );
	ellipsePie->setBrush( brushPie );
	ellipsePie->setPen( Qt::NoPen );

	// Construct a segment of a circle, offset by pieSliceOffset() pixels to the right
	QGraphicsEllipseItem * ellipseSlice = scene()->addEllipse( rect.translated( pieSliceOffset(), 0 ) );
	ellipseSlice->setStartAngle( -segment / 2 );
	ellipseSlice->setSpanAngle( segment );
	ellipseSlice->setBrush( brushSlice );
	ellipseSlice->setPen( Qt::NoPen );

	// Add the circle and segment to a group so we can rotate them together
	QGraphicsItemGroup * pie = scene()->createItemGroup( { ellipsePie, ellipseSlice } );
	pie->setRotation( -45 );

	// Move the group to its position in the overflow panel
	pie->setPos( nextPos + QPointF{ panelWidth / 2, pieDiameter() / 2 } );
	nextPos.ry() += pieDiameter();
    };

    const QStringList cutoffLines
	{ tr( "Min (P%1) ... P%2" ).arg( _stats->minPercentile() ).arg( _startPercentile ),
	  _startPercentile == _stats->minPercentile() ?
		tr( "no files cut off" ) :
		formatSize( _stats->minValue() ) % "..."_L1 % formatSize( percentile( _startPercentile ) ),
	  "",
	  tr( "P%1 ... Max (P%2)" ).arg( _endPercentile ).arg( _stats->maxPercentile() ),
	  _endPercentile == _stats->maxPercentile() ?
		tr( "no files cut off" ) :
		formatSize( percentile( _endPercentile ) ) % "..."_L1 % formatSize( _stats->maxValue() ),
	};
    addText( cutoffLines );
    nextPos.ry() += overflowSpacing();

    // Upper pie chart: number of files cut off
    const FileCount histogramFiles = _stats->bucketsTotalSum();
    const FileCount missingFiles   = _stats->size() - histogramFiles;
    addPie( missingFiles, histogramFiles );

    // Caption for the upper pie chart
    const int missingPercent = qRound( percent( missingFiles, _stats->size() ) );
    const QString cutoffCaption  = missingFiles == 1 ?
                                   tr( "1 file cut off" ) :
                                   tr( "%L1 files cut off" ).arg( missingFiles );
    addText( { cutoffCaption, tr( "%1% of all files" ).arg( missingPercent ) } );

    // Lower pie chart: disk space in outlier percentiles
    const FileSize histogramDiskSpace = percentileSum( _stats, _startPercentile+1, _endPercentile );
    const FileSize cutoffDiskSpace = percentileSum( _stats, _stats->minPercentile(), _startPercentile ) +
                                     percentileSum( _stats, _endPercentile+1, _stats->maxPercentile() );
    addPie( cutoffDiskSpace, histogramDiskSpace );

    // Caption for the lower pie chart
    const double cutoffSpacePercent = percent( cutoffDiskSpace, histogramDiskSpace + cutoffDiskSpace );
    const QStringList pieCaption2{ formatSize( cutoffDiskSpace ) % " cut off"_L1,
                                   tr( "%1% of disk space" ).arg( cutoffSpacePercent, 0, 'f', 1 ),
                                 };
    addText( pieCaption2 );

    // Remember the panel contents height as a minimum for building the histogram
    const qreal contentsHeight = nextPos.y() - overflowPanel->rect().y() - topBorder() - bottomBorder();
    if ( contentsHeight != _minHeight )
    {
	// Rebuild now if the height of the contents is different from the cached value
	// -so it is critical that the height of the panel contents does not depend on _minHeight
	_minHeight = contentsHeight;
	setGeometryDirty();
	rebuild();

#if VERBOSE_HISTOGRAM
	logDebug() << "Reset minimum histogram height to: " << _minHeight << Qt::endl;
#endif
    }
}


void HistogramView::addLine( int percentileIndex, const QString & name, const QPen & pen )
{
    const FileSize xValue       = percentile( percentileIndex  );
    const FileSize axisStartVal = percentile( _startPercentile );
    const FileSize axisEndVal   = percentile( _endPercentile   );
    const FileSize totalWidth   = axisEndVal - axisStartVal;
    const qreal    x            = _size.width() * ( xValue - axisStartVal ) / totalWidth;

    QGraphicsLineItem * line =
	new QGraphicsLineItem{ x, markerExtraHeight(), x, -_size.height() - markerExtraHeight() };
    line->setToolTip( whitespacePre( name % "<br/>"_L1 % formatSize( xValue ) ) );
    line->setZValue( name.isEmpty() ? MarkerLayer : SpecialMarkerLayer );
    line->setPen( pen );

    scene()->addItem( line );
}


void HistogramView::resizeEvent( QResizeEvent * event )
{
    // logDebug() << "Event size: " << event->size() << Qt::endl;

    QGraphicsView::resizeEvent( event );
    setGeometryDirty();
    rebuild();
}
