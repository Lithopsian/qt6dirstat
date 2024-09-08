/*
 *   File name: HistogramView.cpp
 *   Summary:   View widget for histogram rendering for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <cmath> // log2()

#include <QResizeEvent>
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
}


FileSize HistogramView::percentile( int index ) const
{
    return _stats->percentileValue( index );
}


void HistogramView::setPercentileRange( int startPercentile, int endPercentile )
{
    PercentileStats::validatePercentileRange( startPercentile, endPercentile );

    const bool oldNeedOverflowPanel = needOverflowPanel();

    _startPercentile = startPercentile;
    _endPercentile   = endPercentile;

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


void HistogramView::autoLogScale()
{
    if ( !_autoLogScale )
	return;

    _useLogScale = false;

    if ( _stats->bucketCount() > 3 )
    {
	// We compare the largest bucket with the P85 percentile of the buckets,
	// but make sure we're not comparing with an empty bucket
	const FileCount largestBucket = _stats->largestBucket();
	const int referencePercentileValue =
	    qMax( _stats->bucket( qRound( _stats->bucketCount() * 85.0 / 100.0 ) ), 1 );

	_useLogScale = largestBucket > referencePercentileValue * 20;

#if VERBOSE_HISTOGRAM
	logInfo() << "Largest bucket: " << largestBucket
	          << " bucket P85: " << referencePercentileValue
	          << "	 -> use log height scale: " << _useLogScale
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


void HistogramView::rebuild()
{
    //logInfo() << "Building histogram" << " " << window()->isVisible() << " " << _stats << Qt::endl;

    if ( !_stats )
	return;

    //logInfo() << "Really building histogram" << Qt::endl;

    delete scene();
    setScene( new QGraphicsScene{ this } );

    if ( geometryDirty() )
	calcGeometry();

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
    createPanel( { -leftBorder(), -topBorder() - _size.height(), fullWidth(), fullHeight() } );
}


void HistogramView::addAxes()
{
    const auto drawAxis = [ this ]( qreal x, qreal y )
	{ scene()->addLine( 0, 0, x, y, lineColor() )->setZValue( AxisLayer ); };

    drawAxis( _size.width() + axisExtraLength(), 0 );
    drawAxis( 0, -_size.height() - axisExtraLength() );
}


void HistogramView::addXAxisLabel()
{
    QGraphicsTextItem * item = createBoldItem( scene(), tr( "File Size  -->" ) );
    const QRectF rect = item->boundingRect();
    item->setPos( ( _size.width() - rect.width() ) / 2, bottomBorder() - rect.height() );
}


void HistogramView::addYAxisLabel()
{
    QGraphicsTextItem * item = createBoldItem( scene(), "n   -->" );
    if ( _useLogScale )
	item->setHtml( "log<sub>2</sub>(n)   -->" );

    const QRectF rect = item->boundingRect();
    item->setRotation( 270 );
    item->setPos( ( rect.height() + leftBorder() ) / -2, ( rect.width() - _size.height() ) / 2 );
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
    const QRectF rect = endItem->boundingRect();
    endItem->setTextWidth( rect.width() );
    endItem->document()->setDefaultTextOption( QTextOption{ Qt::AlignRight } );
    endItem->setPos( _size.width() - rect.width(), bottomBorder() - rect.height() );
}


void HistogramView::addYStartEndLabels()
{
    const auto addLabel = [ this ]( qreal y, const QString & text)
    {
	QGraphicsTextItem * item = createTextItem( scene(), text );
	const QRectF rect = item->boundingRect();
	item->setRotation( 270 );
	item->setPos( ( leftBorder() + rect.height() ) / -2, y + rect.width() / 2 );
    };

    addLabel( 0, "0" );

    const FileCount max = _stats->largestBucket();
    addLabel( -_size.height(), QString{ "%L1" }.arg( max ) );
}


void HistogramView::addQuartileText()
{
    QPointF pos{ 0, -_size.height() - topBorder() - textBorder() };
    const int n = _stats->bucketsTotalSum();

    // Create text for the total number of files
    const QFontMetrics metrics{ font() };
    const QChar sigma{ 0x2211 };
    const bool fontHasSigma = metrics.inFont( sigma );
    const QString nTextTemplate = fontHasSigma ? sigma % "n: %L2"_L1 : tr( "Files (n): %L1" );
    QGraphicsTextItem * nTextItem = createBoldItem( scene(), nTextTemplate.arg( n ) );
    pos.ry() -= nTextItem->boundingRect().height();

    // Only add quartile labels if there is some data in the histogram
    if ( n > 0 )
    {
	const QString q1Text     = tr( "Q1: "     ) % formatSize( _stats->q1Value()     );
	const QString medianText = tr( "Median: " ) % formatSize( _stats->medianValue() );
	const QString q3Text     = tr( "Q3: "     ) % formatSize( _stats->q3Value()     );

	QGraphicsTextItem * q1Item     = createBoldItem( scene(), q1Text     );
	QGraphicsTextItem * medianItem = createBoldItem( scene(), medianText );
	QGraphicsTextItem * q3Item     = createBoldItem( scene(), q3Text     );

	q1Item->setDefaultTextColor    ( quartileColor() );
	medianItem->setDefaultTextColor( medianColor()   );
	q3Item->setDefaultTextColor    ( quartileColor() );

	const QRectF medianRect = medianItem->boundingRect();

	q1Item->setPos( pos );
	pos.rx() += q1Item->boundingRect().width() + textSpacing();
	medianItem->setPos( pos );
	pos.rx() += medianRect.width() + textSpacing();
	q3Item->setPos( pos );
	pos.rx() += q3Item->boundingRect().width() + textSpacing();
    }

    // Add the number of files text after any quartiles text
    nTextItem->setPos( pos );
}


void HistogramView::addBars()
{
    const auto logHeight = [ this ]( FileCount value )
    {
	if ( !_useLogScale )
	    return static_cast<double>( value );

	return value > 1 ? std::log2( value ) : value / 2.0;
    };

    const qreal barWidth = _size.width() / _stats->bucketCount();
    const double maxVal = logHeight( _stats->largestBucket() );
    const double scaling = maxVal == 0 ? 0 : _size.height() / maxVal;

    for ( int i=0; i < _stats->bucketCount(); ++i )
    {
	// logDebug() << "Adding bar #" << i << " with value " << _stats->bucket( i ) << Qt::endl;

	const qreal fillHeight = scaling * logHeight( _stats->bucket( i ) );
	const QRectF rect{ i * barWidth, 0, barWidth, -_size.height() };
	scene()->addItem( new HistogramBar{ _stats, i, rect, fillHeight, barPen(), barBrush() } );
    }
}


void HistogramView::addMarkers()
{
    if ( percentile( _endPercentile ) - percentile( _startPercentile ) < 1 )
	return;

    const FileSize axisStartVal = percentile( _startPercentile );
    const FileSize axisEndVal   = percentile( _endPercentile   );
    const FileSize axisRange    = axisEndVal - axisStartVal;
    const qreal    scaling      = _size.width() / axisRange;
    const qreal    y2           = -_size.height() - markerExtraHeight();

    // Lambda to create a single marker line and add it to the scene
    const auto addMarker = [ this, axisStartVal, scaling, y2 ]
	( int index, const QString & name, const QPen & pen, GraphicsItemLayer layer )
    {
	const FileSize xValue = percentile( index );
	const qreal    x      = scaling * ( xValue - axisStartVal );

	// Visible line as requested
	QLineF line{ x, markerExtraHeight(), x, y2 };
	QGraphicsLineItem * visibleLine = scene()->addLine( line, pen );
	visibleLine->setZValue( layer );

	// Wider transparent line to make the tooltip easier to trigger, child of the visible line
	const QPen transparentPen{ Qt::transparent, pen.widthF() + 2 };
	QGraphicsLineItem * tooltipLine = scene()->addLine( line, transparentPen );
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

	// Skip if configured for no percentile markers
	if ( _percentileStep == 0 )
	    continue;

	// Skip markers that aren't in percentileStep increments ...
	// ... unless they are within the percentile "margin" at the start or end
	if ( i % _percentileStep != 0 && i > extraStartMarkers && i < extraEndMarkers )
	    continue;

	addMarker( i, tr( "Percentile P%1" ).arg( i ), percentilePen( i ), PercentileLayer );
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
    QPointF nextPos{ _size.width() + rightBorder() + overflowGap(), -topBorder() - _size.height() };
    const QRectF rect{ nextPos.x(), nextPos.y(), panelWidth, fullHeight() };
    createPanel( rect );

    // Headline
    QGraphicsTextItem * headline = createBoldItem( scene(), overflowHeadline() );
    const QRectF headlineRect = headline->boundingRect();
    headline->setPos( nextPos + QPointF{ ( panelWidth - headlineRect.width() ), overflowSpacing() } / 2 );
    nextPos.ry() += headlineRect.height() + overflowSpacing();

    /**
     * Add multiple text items on separate lines starting at 'nextPos',
     * then add a margin at the bottom.  Each line is aligned in the
     * centre of the overflow panel.  nextPos is updated to the bottom
     * left of the margin.
     **/
    const auto addText = [ this, panelWidth, &nextPos ]( const QStringList & lines )
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
    const auto addPie = [ this, panelWidth, &nextPos ]( FileSize valSlice, FileSize valPie )
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
	  QString{},
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
    const QStringList pieCaption{ formatSize( cutoffDiskSpace ) % " cut off"_L1,
                                  tr( "%1% of disk space" ).arg( cutoffSpacePercent, 0, 'f', 1 ),
                                };
    addText( pieCaption );

    // Remember the panel contents height as a minimum for building the histogram
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


void HistogramView::resizeEvent( QResizeEvent * )
{
    //logDebug() << "Event size: " << event->size() << Qt::endl;
    //logDebug() << "Event size: " << event->oldSize() << Qt::endl;

    // Not safe to delete and create children during a recursive showChildren()
    QTimer::singleShot( 0, this, &HistogramView::rebuildDirty );
}


void HistogramView::changeEvent( QEvent * event )
{
    QGraphicsView::changeEvent( event );

    if ( event->type() == QEvent::PaletteChange )
	rebuildDirty();
}
