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

#include "Typedefs.h" // FileSize


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
     * (percentile 100 or P100). But in many cases this greatly distorts the
     * display because of outliers (data points far outside the range of the
     * bulk of data points), so the histogram can also display from a given
     * percentile (startPercentile) to another given percentile
     * (endPercentile).
     *
     * The "cutoff" data is clearly communicated to the user so he does not get
     * the impression that the histogram in that mode displays all data; it
     * does not. It does display the most meaningful part of the data, though.
     *
     * In addition to that, some or all of the percentiles, as well as the
     * median and the 1st and the 3rd quartiles (Q1 and Q3) can be displayed as
     * overlays on the histogram.
     **/
    class HistogramView: public QGraphicsView
    {
	Q_OBJECT

    public:

	/**
	 * z-value (altitude) for the different graphics elements
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
	    QGraphicsView{ parent }
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
	 * Automatically determine the best start and end percentiles.  This is
	 * done by identifying the percentiles corresponding to outliers at the
	 * top and bottom end of the distribution.
	 *
	 * Outliers are statistically defined as being more than three times the
	 * inter-quartile range (IQR) below the first quartile or above the third
	 * quartile.  File size distributions are heavily weighted to small files
	 * and there are effectively never outliers at the small end by this
	 * definition, so low size outliers are defined as being the IQR beyond
	 * the 1st quartile.
	 *
	 * The high end outlier calculation can theoretically overflow a (64 bit!)
	 * FileSize integer, so this is first calculated as a floating point value
	 * and only calculated as an integer if it is less than the maximum
	 * FileSize value in the statistics.
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
//	bool percentileDisplayed( int index ) const
//	    { return index >= _startPercentile && index <= _endPercentile; }

	/**
	 * Automatically determine if a logarithmic height scale should be
	 * used. Set the internal _useLogHeightScale variable accordingly and
	 * return it.
	 **/
	void autoLogHeightScale();

	/**
	 * Functions to create the graphical elements.
	 **/
	void addBackground();
	void addAxes();
	void addXAxisLabel();
	void addYAxisLabel();
	void addXStartEndLabels();
	void addYStartEndLabels();
	void addQuartileText();
	void addBars();
	void addMarkers();
	void addOverflowPanel();

	/**
	 * Add a text item at 'pos' and return the bottom left of its bounding
	 * rect.
	 **/
	QPointF addText( QPointF pos, const QStringList & lines );

	/**
	 * Create a text item and make it bold.  The text is added to the scene
	 * but not positioned.
	 **/
	QGraphicsTextItem * createBoldItem( const QString & text );

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
	 * Return or set whether the geometry of the histogram needs
	 * to be re-caclulated.
	 **/
	bool geometryDirty() { return !_size.isValid(); }
	void setGeometryDirty() { _size = QSize{}; }

	/**
	 * Calculate the content geometry to try and fit the viewport.
	 * The histogram may be over-sized (it is always built to a
	 * minimum height, to fit the overflow panel contents) and
	 * will then be scaled down to fit the viewport.
	 **/
	void calcGeometry();

	/**
	 * Return 'true' if an overflow ("cutoff") panel is needed.
	 **/
	bool needOverflowPanel() const
	    { return _startPercentile > 0 || _endPercentile < 100; }

	/**
	 * A whole bunch of fixed values describing the geometry of
	 * the histogram window.
	 **/
	constexpr static qreal leftBorder()        { return  30; }
	constexpr static qreal rightBorder()       { return  10; }
	constexpr static qreal topBorder()         { return  30; }
	constexpr static qreal bottomBorder()      { return  50; }
	constexpr static qreal viewMargin()        { return  10; };
	constexpr static qreal textBorder()        { return  10; };
	constexpr static qreal textSpacing()       { return  30; };
	constexpr static qreal topTextHeight()     { return  34; };
	constexpr static qreal axisExtraLength()   { return   5; };
	constexpr static qreal markerExtraHeight() { return  15; };
	constexpr static qreal overflowWidth()     { return 140; };
	constexpr static qreal overflowBorder()    { return  10; };
	constexpr static qreal overflowSpacing()   { return  15; };
	constexpr static qreal pieDiameter()       { return  60; };
	constexpr static qreal pieSliceOffset()    { return  10; };

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

	// Statistics data to represent
	const FileSizeStats * _stats{ nullptr };

	// Flags not currently configurable
	const bool _showMedian{ true };
	const bool _showQuartiles{ true };
	const int  _leftMarginPercentiles{ 0 };
	const int  _rightMarginPercentiles{ 5 };

	// Configurable settings; will be set for every build (except _percentileStep)
	int  _startPercentile;
	int  _endPercentile;
	bool _useLogHeightScale;
	int  _percentileStep{ 0 };

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
	qreal  _minHeight{ 200 }; // will dynamically increase to fit the overflow panel
	QSizeF _size;

    }; // class HistogramView

} // namespace QDirStat

#endif // ifndef HistogramView_h
