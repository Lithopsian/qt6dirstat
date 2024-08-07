/*
 *   File name: HistogramView.h
 *   Summary:   View widget for histogram rendering for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef HistogramView_h
#define HistogramView_h

#include <QGraphicsView>

#include "Typedefs.h" // _qr


namespace QDirStat
{
    class FileSizeStats;

    /**
     * Histogram widget.
     *
     * This widget is based on buckets and percentiles, both of which have to
     * be fed from the outside, i.e. the data collection is abstracted.
     *
     * The histogram can be displayed in a traditional way, i.e. from the
     * minimum data value (percentile 0 or P0) to the maximum data value
     * (percentile 100 or P100). But in many cases, this greatly distorts the
     * display because of outliers (data points way outside the range of
     * "normal" data points), so this histogram can also display from a given
     * percentile (startPercentile) to another given percentile
     * (endPercentile).
     *
     * In many cases, those outliers are only a very small percentage, so
     * displaying not from P0..P100, but from P3..P97 instead gives a much more
     * meaningful histogram, while still displaying 96% of all data: By
     * definition, each percentile contains 1% of all data points, so you lose
     * one percent at the left and one percent at the right for each percentile
     * omitted like this.
     *
     * That "cutoff" should be clearly communicated to the user so he does not
     * get the impression that this histogram in that mode displays all data;
     * it does not. It does display the meaningful part of the data, though.
     *
     * In addition to that, the percentiles (or at least every 5th of them,
     * depending on configuration) as well as the median, the 1st and the 3rd
     * quartile (Q1 and Q3) can be displayed as an overlay to the histogram.
     **/
    class HistogramView: public QGraphicsView
    {
	Q_OBJECT

    public:

	/**
	 * zValue (altitude) for the different graphics elements
	 **/
	enum GraphicsItemLayer
	{
	    PanelBackgroundLayer = -100,
	    MiscLayer = 0, // Default if no zValue specified
	    BarLayer,
	    AxisLayer,
	    HoverBarLayer,
	    MarkerLayer,
	    SpecialMarkerLayer,
	    TextLayer,
	};


	/**
	 * Constructor.
	 **/
	HistogramView( QWidget * parent = nullptr ):
	    QGraphicsView { parent }
	{}

	/**
	 * Initialise the view: set the percentiles for the data points all at
	 * once. Unlike the buckets, these have a value; in the context of
	 * QDirStat, this is the FileSize.
	 *
	 * The definition of a percentile n is "the data value where n percent
	 * of all sorted data are taken into account". The median is the 50th
	 * percentile. By a little stretch of the definition, percentile 0 is
	 * the data minimum, percentile 100 is the data maximum.
	 *
	 * The interval between one percentile and the next contains exactly 1%
	 * of the data points.
	 *
	 * HistogramView does now take ownership of the stats object.
	 **/
	void init( const FileSizeStats * stats );

	/**
	 * Return the stored value for percentile no. 'index' (0..100).  This
	 * directly accesses the percentile list with the assumption that it is
	 * already populated with 101 entries.
	 *
	 * Note that the floating point boundary value from the stats is
	 * truncated to the floor integer to match the > or <= definition for
	 * being in that percentile.
	 **/
	FileSize percentile( int index ) const;

	/**
	 * Set the percentile (0..100) from which on to display data, i.e. set
	 * the left border of the histogram. The real value to use is taken
	 * from the stored percentiles.
	 **/
	void setStartPercentile( int index );

	/**
	 * Return the percentile from which on to display data, i.e. the left
	 * border of the histogram. Use percentile() with the result of this to
	 * get the numeric value.
	 **/
	int startPercentile() const { return _startPercentile; }

	/**
	 * Set the percentile (0..100) until which on to display data, i.e. set
	 * the right border of the histogram. The real value to use is taken
	 * from the stored percentiles.
	 **/
	void setEndPercentile( int index );

	/**
	 * Return the percentile until which to display data, i.e. the right
	 * border of the histogram. Use percentile() with the result of this to
	 * get the numeric value.
	 **/
	int endPercentile() const { return _endPercentile; }

	/**
	 * Automatically determine the best start and end percentile
	 **/
	void autoStartEndPercentiles();

	/**
	 * Enable or disable showing percentiles as an overlay over the
	 * histogram: 'step' specifies how many of them to display; for
	 * example, '5' will display P5, P10, P15 etc.; step = 0 disables
	 * them completely.
	 **/
	void setPercentileStep( int step ) { _percentileStep = step; }

	/**
	 * Set how many percentiles to display as an overlay at the left margin
	 * in addition to those shown with showPercentiles().
	 *
	 * A value of 2 with a histogram showing data from min to max means
	 * show also P1 and P2.
	 *
	 * A value of 2 with a histogram showing data from P3 to P97 means show
	 * also P4 and P5.
	 *
	 * A value of 0 means show no additional percentiles.
	 **/
//	void setLeftMarginPercentiles( int number = 0 )
//	    { _leftMarginPercentiles = number; }

	/**
	 * Return the left margin percentiles or 0 if none are shown.
	 **/
//	int leftMarginPercentiles() { return _leftMarginPercentiles; }

	/**
	 * Set how many percentiles to display as an overlay at the right
	 * margin in addition to those shown with showPercentiles().
	 *
	 * A value of 2 with a histogram showing data from min to max means
	 * show also P98 and P99.
	 *
	 * A value of 2 with a histogram showing data from P3 to P97 means show
	 * also P95 and P96.
	 *
	 * A value of 0 means show no additional percentiles.
	 **/
//	void setRightMarginPercentiles( int number= 2 )
//	    { _leftMarginPercentiles = number; }

	/**
	 * Return the right margin percentiles or 0 if none are shown.
	 **/
//	int rightMarginPercentiles() { return _rightMarginPercentiles; }

	/**
	 * Pen and brush for HistogramBar.
	 **/
	QBrush barBrush() const { return _barBrush; }
	QPen   barPen()   const { return _barPen;   }

	/**
	 * Build the histogram based on a new set of data.
	 **/
	void build();


    protected:

	/**
	 * Rebuild the histogram based on the current data.
	 **/
	void rebuild();

	/**
	 * Return 'true' if percentile no. 'index' is in range for being
	 * displayed, i.e. if it is between _startPercentile and
	 * _endPercentile.
	 **/
	bool percentileDisplayed( int index ) const
	    { return index >= _startPercentile && index <= _endPercentile; }

	/**
	 * Automatically determine if a logarithmic height scale should be
	 * used. Set the internal _useLogHeightScale variable accordingly and
	 * return it.
	 **/
	void autoLogHeightScale();

	/**
	 * Functions to create the graphical elements.
	 **/
	void addHistogramBackground();
	void addAxes();
	void addYAxisLabel();
	void addXAxisLabel();
	void addXStartEndLabels();
	void addQuartileText();
	void addHistogramBars();
	void addMarkers();
	void addOverflowPanel();

	/**
	 * Add a text item at 'pos' and return the bottom left of its bounding
	 * rect.
	 **/
	QGraphicsTextItem * addText( QPointF pos, const QString & text );

	/**
	 * Add a text item at 'pos' and return the bottom left of its bounding
	 * rect.
	 **/
	QPointF addText( QPointF pos, const QStringList & lines );

	/**
	 * Add a bold font text item at 'pos' and return the bottom left of its
	 * bounding rect.
	 **/
	QPointF addBoldText( QPointF pos, const QString & text );

	/**
	 * Add a line.
	 **/
	void addLine( int             percentileIndex,
	              const QString & name,
	              const QPen    & pen );

	/**
	 * Add a pie diagram with two values val1 and val2.
	 * Return the bottom left of the bounding rect.
	 **/
	QPointF addPie( const QRectF & rect,
	                FileSize       val1,
	                FileSize       val2,
	                const QBrush & brush1,
	                const QBrush & brush2 );

	/**
	 * Fit the graphics into the viewport.
	 **/
	void fitToViewport();

	/**
	 * Calculate the content geometry to fit into 'newSize'.
	 **/
	void calcGeometry( QSize newSize );

	/**
	 * Return 'true' if an overflow ("cutoff") panel is needed.
	 **/
	bool needOverflowPanel() const
	    { return _startPercentile > 0 || _endPercentile < 100; }

	/**
	 * A whole bunch of fixed values describing the geometry of
	 * the histogram window. Not static since there will only ever
	 * be one HistogramView and most of the time none.
	 **/
	qreal leftBorder()        const { return  40.0_qr; }
	qreal rightBorder()       const { return  10.0_qr; }
	qreal topBorder()         const { return  30.0_qr; }
	qreal bottomBorder()      const { return  50.0_qr; }
	qreal viewMargin()        const { return  10.0_qr; };
	qreal textBorder()        const { return  10.0_qr; };
	qreal textSpacing()       const { return  30.0_qr; };
	qreal topTextHeight()     const { return  34.0_qr; };
	qreal axisExtraLength()   const { return   5.0_qr; };
	qreal markerExtraHeight() const { return  15.0_qr; };
	qreal overflowWidth()     const { return 140.0_qr; };
	qreal overflowBorder()    const { return  10.0_qr; };
	qreal overflowSpacing()   const { return  15.0_qr; }; // between histogram and overflow area
	qreal pieDiameter()       const { return  60.0_qr; };
	qreal pieSliceOffset()    const { return  10.0_qr; };

	/**
	 * Resize the view.
	 *
	 * Reimplemented from QFrame.
	 **/
	void resizeEvent( QResizeEvent * event ) override;


    private:

	//
	// Data Members
	//

	// Collected statistics data
	const FileSizeStats * _stats { nullptr };

	// Flags not currently configurable
	const bool _showMedian			{ true };
	const bool _showQuartiles		{ true };
	const int  _leftMarginPercentiles	{ 0 };
	const int  _rightMarginPercentiles	{ 5 };

	// Configurable settings
	int  _startPercentile;
	int  _endPercentile;
	bool _useLogHeightScale;
	int  _percentileStep { 0 };

	// Brushes and pens
	QBrush _panelBackground;
	QBrush _barBrush;
	QPen   _barPen;
	QPen   _medianPen;
	QPen   _quartilePen;
	QPen   _percentilePen;
	QPen   _decilePen;
	QPen   _piePen;
	QBrush _overflowSliceBrush;

	// Geometry
	qreal _histogramWidth;
	qreal _histogramHeight;
	bool  _geometryDirty;

    }; // class HistogramView

} // namespace QDirStat

#endif // ifndef HistogramView_h
