/*
 *   File name: HistogramView.cpp
 *   Summary:   View widget for histogram rendering for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QTextDocument>
#include <QTimer>

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
     * Return the text of the overflow panel headline.
     **/
    QString overflowHeadline()
    {
	return QObject::tr( "Cut-off percentiles" );
    }

    /**
     * Return rich text of the form "Pn". The percentile index is
     * subscripted, but Qt subscripting can be tiny so use a value
     * in between the standard subscript and the full-size font.
     **/
    QString pText( int n )
    {
	return QObject::tr( "P<span style='font-size: large; vertical-align: sub;'>%1</span>" ).arg( n );
    }


    /**
     * Return the base-2 logarithm of 'value' if 'logScale' is
     * true.  Otherwise just return 'value'.
     *
     * Nite that the input value is a 64-but integer and the
     * output is floating point qreal.
     **/
    qreal log2( bool logScale, qint64 value )
    {
	return logScale ? PercentileStats::log2( value ) : value;
    }


    /**
     * Return the percentage represented by 'count' wrt 'total',
     * returning 0.0 if 'total' is 0.
     **/
    double percent( qint64 count, qint64 total )
    {
	return total ? 100.0 * count / total : 0.0;
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
     * Create a rich text item in the given scene and set its Z-value.
     **/
    QGraphicsTextItem * createTextItem( QGraphicsScene * scene, const QString & text )
    {
	QGraphicsTextItem * item = new QGraphicsTextItem{};
	item->setHtml( text );
	item->setZValue( HistogramView::TextLayer );
	scene->addItem( item );

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
}


FileSize HistogramView::percentile( int index ) const
{
    return _stats->percentileValue( index );
}


void HistogramView::setPercentileRange( int startPercentile, int endPercentile, bool logWidths )
{
    PercentileStats::validateIndexRange( startPercentile, endPercentile );

    const bool oldNeedOverflowPanel = needOverflowPanel();

    _startPercentile = startPercentile;
    _endPercentile   = endPercentile;
    _logWidths       = logWidths;

    if ( oldNeedOverflowPanel != needOverflowPanel() )
	setGeometryDirty();

    build();

#if VERBOSE_HISTOGRAM
    if ( _startPercentile >= _endPercentile )
    {
	logError() << "startPercentile must be less than endPercentile: "
	           << _startPercentile << ".." << _endPercentile
	           << Qt::endl;
    }
#endif
}


void HistogramView::autoLogHeights()
{
    // Ignore if the user has overridden the height scaling
    if ( !_autoLogHeights )
	return;

    // Use log heights if the highest bucket is more than 50 times the 85-percentile bucket count
    _logHeights = _stats->skewness() > 50;
}


QSizeF HistogramView::calcGeometry( qreal overflowWidth )
{
    QSizeF size = viewport()->size();
    const qreal verticalPadding = bottomBorder() + topBorder() + 2 * viewMargin() + topTextHeight();

    if ( size.height() < _minHeight + verticalPadding )
    {
	// Will be scaled down to fit the viewport height, so up-scale the width to compensate
	size.rwidth() *= ( _minHeight + verticalPadding ) / size.height();
	size.rheight() = _minHeight;
    }
    else
    {
	size.rheight() -= verticalPadding;
    }

    size.rwidth() -= leftBorder() + rightBorder() + 2 * viewMargin();
    if ( needOverflowPanel() )
	size.rwidth() -= overflowGap() + overflowWidth;

#if VERBOSE_HISTOGRAM
    logDebug() << "Histogram size: " << size << Qt::endl;
#endif

    return size;
}


void HistogramView::fitToViewport( QGraphicsScene * scene )
{
    const QSize visibleSize = viewport()->size();
    const QRectF rect =
	scene->sceneRect().adjusted( -viewMargin(), -viewMargin(), viewMargin(), viewMargin() );

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


void HistogramView::rebuild()
{
    //logInfo() << "Building histogram" << " " << window()->isVisible() << " " << _stats << Qt::endl;

    if ( !_stats )
	return;

    //logInfo() << "Really building histogram" << Qt::endl;

    const qreal overflowPanelWidth = overflowWidth();

    // Delay deleting the old scene to reduce delay and avoid crashing during a show event
    QGraphicsScene * oldScene = scene();
    if ( oldScene )
	oldScene->deleteLater();

    QGraphicsScene * newScene = new QGraphicsScene{ this };
    setScene( newScene );

    if ( geometryDirty() )
	_size = calcGeometry( overflowPanelWidth );

    addBackground( newScene );
    addAxes( newScene );
    addAxisLabels( newScene );
    addXStartEndLabels( newScene );
    addYStartEndLabels( newScene );
    addQuartileText( newScene );
    addBars( newScene );
    addMarkers( newScene );
    addOverflowPanel( newScene, overflowPanelWidth );

    fitToViewport( newScene );
}


void HistogramView::addBackground( QGraphicsScene * scene )
{
    createPanel( scene, { -leftBorder(), -topBorder() - _size.height(), fullWidth(), fullHeight() } );
}


void HistogramView::addAxes( QGraphicsScene * scene )
{
    const auto drawAxis = [ this, scene ]( qreal x, qreal y )
	{ scene->addLine( 0, 0, x, y, lineColor() )->setZValue( AxisLayer ); };

    drawAxis( _size.width() + axisExtraLength(), 0 );
    drawAxis( 0, -_size.height() - axisExtraLength() );
}


void HistogramView::addAxisLabels( QGraphicsScene * scene )
{
    const auto labelItem = [ scene ]( bool logScale, const QString & text )
    {
	const QString labelText = logScale ? "log<sub>2</sub>(%1)  -->" : "%1  -->";
	return createBoldItem( scene, labelText.arg( text ) );
    };

    QGraphicsTextItem * xItem = labelItem( _logWidths, tr( "file size" ) );
    const QRectF xRect = xItem->boundingRect();
    xItem->setPos( ( _size.width() - xRect.width() ) / 2, ( bottomBorder() - xRect.height() ) / 2 );

    QGraphicsTextItem * yItem = labelItem( _logHeights, "n" );
    const QRectF yRect = yItem->boundingRect();
    yItem->setRotation( 270 );
    yItem->setPos( ( yRect.height() + leftBorder() ) / -2, ( yRect.width() - _size.height() ) / 2 );
}


void HistogramView::addXStartEndLabels( QGraphicsScene * scene )
{
    const auto addLabel =
	[ this, scene ]( int x, const QString & prefix, int labelPercentile, Qt::Alignment alignment )
    {
	const QString label = prefix % "<br/>"_L1 % formatSize( percentile( labelPercentile ) );
	QGraphicsTextItem * item = createTextItem( scene, label );
	const QRectF rect = item->boundingRect();
	item->setTextWidth( rect.width() );
	item->document()->setDefaultTextOption( QTextOption{ alignment } );
	if ( alignment == Qt::AlignRight )
	    x -= rect.width();
	item->setPos( x, ( bottomBorder() - rect.height() ) / 2 );
    };

    const QString min = _startPercentile == _stats->minPercentile() ? tr( "Min" ) : pText( _startPercentile );
    addLabel( -axisExtraLength() * 2, min, _startPercentile, Qt::AlignLeft );

    const QString max = _endPercentile == _stats->maxPercentile() ? tr( "Max" ) : pText( _endPercentile );
    addLabel( _size.width() + axisExtraLength() * 2, max, _endPercentile, Qt::AlignRight );
}


void HistogramView::addYStartEndLabels( QGraphicsScene * scene )
{
    const auto addLabel = [ this, scene ]( qreal y, const QString & text)
    {
	QGraphicsTextItem * item = createTextItem( scene, text );
	const QRectF rect = item->boundingRect();
	item->setRotation( 270 );
	item->setPos( ( leftBorder() + rect.height() ) / -2, rect.width() / 2 - y );
    };

    addLabel( 0, "0" );
    addLabel( _size.height(), formatCount( _stats->highestBucketCount() ) );
}


void HistogramView::addQuartileText( QGraphicsScene * scene )
{
    QPointF pos{ 0, -_size.height() - topBorder() - textBorder() };
    const int n = _stats->percentileCount( _startPercentile, _endPercentile );

    // Create text for the total number of files
    const QFontMetrics metrics{ font() };
    const QChar sigma{ 0x2211 };
    const bool fontHasSigma = metrics.inFont( sigma );
    const QString nTextTemplate = fontHasSigma ? sigma % "n: %L2"_L1 : tr( "Files (n): %L1" );
    QGraphicsTextItem * nTextItem = createBoldItem( scene, nTextTemplate.arg( n ) );
    pos.ry() -= nTextItem->boundingRect().height();

    // Only add quartile labels if there is some data in the histogram
    if ( n > 0 )
    {
	/**
	 * Add text at 'pos', bold and in 'color'.  The text is
	 * constructed from 'text' and 'size'.  'pos' is updated
	 * to the end of the added text plus some spacing.
	 **/
	const auto addText = [ this, scene, &pos ]( const QString & text, FileSize size, const QColor & color )
	{
	    QGraphicsTextItem * item = createBoldItem( scene, text % formatSize( size ) );
	    item->setDefaultTextColor( color );
	    item->setPos( pos );
	    pos.rx() += item->boundingRect().width() + textSpacing();
	};

	addText( tr( "Q1: "     ), _stats->q1Value(),     quartileColor() );
	addText( tr( "Median: " ), _stats->medianValue(), medianColor()   );
	addText( tr( "Q3: "     ), _stats->q3Value(),     quartileColor() );
    }

    // Add the number of files text after any quartiles text
    nTextItem->setPos( pos );
}


void HistogramView::addBars( QGraphicsScene * scene )
{
    const qreal barWidth = _size.width() / _stats->bucketsCount();
    const qreal maxVal   = log2( _logHeights, _stats->highestBucketCount() );
    const qreal scaling  = maxVal == 0 ? 0 : _size.height() / maxVal;

    for ( int i=0; i < _stats->bucketsCount(); ++i )
    {
	// logDebug() << "Adding bar #" << i << " with value " << _stats->bucketCount( i ) << Qt::endl;

	const qreal fillHeight = scaling * log2( _logHeights, _stats->bucketCount( i ) );
	const QRectF rect{ i * barWidth, 0, barWidth, -_size.height() };
	scene->addItem( new HistogramBar{ _stats, i, rect, fillHeight, barPen(), barBrush() } );
    }
}


void HistogramView::addMarkers( QGraphicsScene * scene )
{
    // Don't draw markers if there is no meaningful range of data points
    if ( percentile( _endPercentile ) - percentile( _startPercentile ) < 1 )
	return;

    // Find the x-axis scaling, to be applied either to the marker value or log2 of it
    const qreal axisStartVal = log2( _logWidths, percentile( _startPercentile ) );
    const qreal axisEndVal   = log2( _logWidths, percentile( _endPercentile   ) );
    const qreal axisRange    = axisEndVal - axisStartVal;
    const qreal scaling      = _size.width() / axisRange;

    // The bottom point of all the markers is the same
    const qreal y2 = -_size.height() - markerExtraHeight();

    /**
     * Add a vertical line, drawn with 'pen' the full height of the
     * histogram plus a bit more at either end.  A second, wider,
     * transparent, line is then added with a tooltip.
     **/
    const auto addMarker = [ this, scene, axisStartVal, scaling, y2 ]
	( int index, const QString & name, const QPen & pen, GraphicsItemLayer layer )
    {
	const FileSize xValue = percentile( index );
	const qreal    xPos   = scaling * ( log2( _logWidths, xValue ) - axisStartVal );

	// Visible line as requested
	QLineF line{ xPos, markerExtraHeight(), xPos, y2 };
	QGraphicsLineItem * visibleLine = scene->addLine( line, pen );
	visibleLine->setZValue( layer );

	// Wider transparent line to make the tooltip easier to trigger, child of the visible line
	const QPen transparentPen{ Qt::transparent, pen.widthF() + 2 };
	QGraphicsLineItem * tooltipLine = scene->addLine( line, transparentPen );
	tooltipLine->setToolTip( whitespacePre( name % "<br/>"_L1 % formatSize( xValue ) ) );
	tooltipLine->setParentItem( visibleLine );
    };

    // Show percentile marker lines
    const int extraStartMarkers = _startPercentile + _leftExtraPercentiles;
    const int extraEndMarkers   = _endPercentile - _rightExtraPercentiles;
    for ( int i = _startPercentile + 1; i < _endPercentile; ++i )
    {
	if ( i == _stats->median() && _showMedian )
	{
	    addMarker( i, tr( "Median" ), medianPen(), MedianLayer );
	    continue;
	}

	if ( i == _stats->quartile1() && _showQuartiles )
	{
	    addMarker( i, tr( "Q1 (1st quartile)" ), quartilePen(), QuartileLayer );
	    continue;
	}

	if ( i == _stats->quartile3() && _showQuartiles )
	{
	    addMarker( i, tr( "Q3 (3rd quartile)" ), quartilePen(), QuartileLayer );
	    continue;
	}

	// Skip if configured for no percentile markers (other than the quartiles)
	if ( _percentileStep == 0 )
	    continue;

	// Skip markers that aren't in percentileStep increments ...
	// ... unless they are within the percentile "margin" at the start or end
	if ( i % _percentileStep != 0 && i > extraStartMarkers && i < extraEndMarkers )
	    continue;

	addMarker( i, tr( "Percentile " ) % pText( i ), percentilePen( i ), PercentileLayer );
    }
}


qreal HistogramView::overflowWidth()
{
    QFont font;
    font.setBold( true);
    const qreal headlineWidth = textWidth( font, overflowHeadline() );

    return qMax( pieDiameter() + pieSliceOffset() * 2, headlineWidth ) + 2 * overflowBorder();
}


void HistogramView::addOverflowPanel( QGraphicsScene * scene, qreal panelWidth )
{
    if ( !needOverflowPanel() )
	return;

    // Create the panel area
    QPointF nextPos{ _size.width() + rightBorder() + overflowGap(), -topBorder() - _size.height() };
    const QRectF rect{ nextPos.x(), nextPos.y(), panelWidth, fullHeight() };
    createPanel( scene, rect );

    // Headline
    QGraphicsTextItem * headline = createBoldItem( scene, overflowHeadline() );
    const QRectF headlineRect = headline->boundingRect();
    headline->setPos( nextPos + QPointF{ ( panelWidth - headlineRect.width() ), overflowSpacing() } / 2 );
    nextPos.ry() += headlineRect.height() + overflowSpacing();

    /**
     * Add text on multiple lines starting at 'nextPos', then add a
     * margin at the bottom.  Each line is aligned in the centre of
     * the overflow panel.  nextPos is updated to the bottom left of
     * the margin.
     **/
    const auto addText = [ this, scene, panelWidth, &nextPos ]( const QString & lines )
    {
	QGraphicsTextItem * textItem = createTextItem( scene, lines );
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
    const auto addPie = [ this, scene, panelWidth, &nextPos ]( FileSize valSlice, FileSize valPie )
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
	const QRectF rect{ -pieDiameter() / 2.0, -pieDiameter() / 2.0, pieDiameter(), pieDiameter() };

	// Convert the slice value to a segment in Qt units of 1/16th degree
	const int fullCircle = 360 * 16;
	const int segment    = qRound( 1.0 * valSlice / ( valPie + valSlice ) * fullCircle );

	// Create a circle with a segment missing
	QGraphicsEllipseItem * ellipsePie = scene->addEllipse( rect );
	ellipsePie->setStartAngle( segment / 2 );
	ellipsePie->setSpanAngle( fullCircle - segment );
	ellipsePie->setBrush( brushPie );
	ellipsePie->setPen( Qt::NoPen );

	// Construct a segment of a circle, offset by pieSliceOffset() pixels to the right
	QGraphicsEllipseItem * ellipseSlice = scene->addEllipse( rect.translated( pieSliceOffset(), 0 ) );
	ellipseSlice->setStartAngle( -segment / 2 );
	ellipseSlice->setSpanAngle( segment );
	ellipseSlice->setBrush( brushSlice );
	ellipseSlice->setPen( Qt::NoPen );

	// Add the circle and segment to a group so we can rotate them together
	QGraphicsItemGroup * pie = scene->createItemGroup( { ellipsePie, ellipseSlice } );
	pie->setRotation( -45 );

	// Move the group to its position in the overflow panel
	pie->setPos( nextPos + QPointF{ panelWidth / 2, pieDiameter() / 2.0 } );
	nextPos.ry() += pieDiameter();
    };

    const auto cutOff = [ this ]( int limitPercentile, int limit, int minPercentile, int maxPercentile )
    {
	return limitPercentile == limit ?
	       tr( "no files cut off" ) :
	       formatSize( percentile( minPercentile ) ) % "..."_L1 % formatSize( percentile( maxPercentile ) );
    };

    const QString cutoffLines =
	tr( "Min (%1) ... " ).arg( pText( _stats->minPercentile() ) ) % pText( _startPercentile )
	% "<br/>"_L1
	% cutOff( _startPercentile, _stats->minPercentile(), _stats->minPercentile(), _startPercentile )
	% "<br/><br/>"_L1
	% pText( _endPercentile ) % tr( " ... Max (%1)" ).arg( pText( _stats->maxPercentile() ) )
	% "<br/>"_L1
	% cutOff( _endPercentile, _stats->maxPercentile(), _endPercentile, _stats->maxPercentile() );
    addText( cutoffLines );
    nextPos.ry() += overflowSpacing();

    // Upper pie chart: number of files cut off
    const FileCount histogramFiles = _stats->percentileCount( _startPercentile, _endPercentile );
    const FileCount missingFiles   = _stats->size() - histogramFiles;
    addPie( missingFiles, histogramFiles );

    // Caption for the upper pie chart
    const int missingPercent = qRound( percent( missingFiles, _stats->size() ) );
    const QString cutoffCaption =
	missingFiles == 1 ? tr( "1 file cut off" ) : tr( "%L1 files cut off" ).arg( missingFiles );
    addText( cutoffCaption % tr( "<br/>%1% of all files" ).arg( missingPercent ) );

    // Lower pie chart: disk space in outlier percentiles
    const FileSize histogramDiskSpace = _stats->percentileSum( _startPercentile, _endPercentile );
    const FileSize totalDiskSpace     = _stats->cumulativeSum( _stats->maxPercentile() );
    const FileSize cutoffDiskSpace    = totalDiskSpace - histogramDiskSpace;
    addPie( cutoffDiskSpace, histogramDiskSpace );

    // Caption for the lower pie chart
    const double cutoffSpacePercent = percent( cutoffDiskSpace, totalDiskSpace );
    const QString pieCaption = tr( " cut off<br/>%1% of disk space" ).arg( cutoffSpacePercent, 0, 'f', 1 );
    addText( formatSize( cutoffDiskSpace ) % pieCaption );

    // Remember the panel contents as a minimum height for building the histogram
    const qreal contentsHeight = nextPos.y() - rect.y() - topBorder() - bottomBorder();
    if ( contentsHeight != _minHeight )
    {
	// Rebuild now if the height of the contents is different from the cached value
	// -so it is critical that the height of the panel contents does not depend on _minHeight
	_minHeight = contentsHeight;
	rebuildDirty();

#if VERBOSE_HISTOGRAM
	logDebug() << "Reset minimum histogram height to: " << _minHeight << Qt::endl;
#endif
    }
}


bool HistogramView::needOverflowPanel() const
{
    if ( _startPercentile > PercentileStats::minPercentile() )
	return true;

    if ( _endPercentile < PercentileStats::maxPercentile() )
	return true;

    return false;
}


void HistogramView::resizeEvent( QResizeEvent * )
{
    //logDebug() << "Event size: " << event->oldSize() << Qt::endl;
    //logDebug() << "Event size: " << event->size() << Qt::endl;

    // Not safe to delete and create children during the recursive showChildren() in a show event
//    if ( event->oldSize().width() > 0 && event->oldSize().height() > 0 )
	rebuildDirty();
}


void HistogramView::changeEvent( QEvent * event )
{
    QGraphicsView::changeEvent( event );

    if ( event->type() == QEvent::PaletteChange )
	rebuildDirty();
}
