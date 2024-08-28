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
	    PercentileLayer,
	    ToolTipPercentileLayer,
	    QuartileLayer,
	    TooltipQuartileLayer,
	    MedianLayer,
	    TooltipMedianLayer,
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
	 * HistogramView does not take ownership of the stats object.
	 **/
	void init( const FileSizeStats * stats );

	/**
	 * Return the stored value for percentile no. 'index' (0..100).  This
	 * directly accesses the percentile list with the assumption that it is
	 * already populated with 101 entries.
	 **/
	FileSize percentile( int index ) const;

	/**
	 * Set the percentile range (0..100) for which to display data.
	 **/
	void setPercentileRange( int startPercentile, int endPercentile );

	/**
	 * Set the percentile (0..100) from which on to display data, i.e. set
	 * the left border of the histogram. The real value to use is taken
	 * from the stored percentiles.
	 **/
//	void setStartPercentile( int index );

	/**
	 * Return the percentile from which on to display data, i.e. the left
	 * border of the histogram. Use percentile() with the result of this to
	 * get the numeric value.
	 **/
//	int startPercentile() const { return _startPercentile; }

	/**
	 * Set the percentile (0..100) until which on to display data, i.e. set
	 * the right border of the histogram. The real value to use is taken
	 * from the stored percentiles.
	 **/
//	void setEndPercentile( int index );

	/**
	 * Return the percentile up to which to display data, i.e. the right
	 * border of the histogram. Use percentile() with the result of this to
	 * get the numeric value.
	 **/
//	int endPercentile() const { return _endPercentile; }

	/**
	 * Enable or disable showing percentiles as an overlay over the
	 * histogram: 'step' specifies how many of them to display; for
	 * example, '5' will display P5, P10, P15 etc.; step = 0 disables
	 * them completely.
	 **/
	void setPercentileStep( int step )
	    { _percentileStep = step; rebuild(); }

	/**
	 * Set how many percentiles to display as an overlay at the left
	 * margin in addition to those shown based on _percentileStep.
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
	 * margin in addition to those shown based on _percentileStep.
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
	static QBrush barBrush() { return QColor{ 0xB0, 0xB0, 0xD0 }; }
	static QPen   barPen()   { return QColor{ 0x40, 0x40, 0x50 }; }

	/**
	 * Set whether to automatically determine the type of y-axis
	 * scaling.
	 **/
	void enableAutoLogScale() { _autoLogScale = true; }
	void disableAutoLogScale() { _autoLogScale = false; }

	/**
	 * Return whether the y-axis is currently showing as a log scale.
	 **/
	bool logScale() { return _useLogScale; }
	void toggleLogScale() { _useLogScale = !_useLogScale; }

	/**
	 * Build the histogram, probably based on a new set of data.
	 * The preferred y-axis scaling will be recalculated.
	 **/
	void build()
	    { autoLogScale(); rebuild(); }


    protected:

	/**
	 * Rebuild the histogram based on the current data.  The geometry
	 * may be recalculated, for example if the window has been resized.
	 **/
	void rebuild();

	/**
	 * Automatically determine if a logarithmic height scale should be
	 * used. Set the internal _useLogScale variable accordingly and
	 * return it.
	 **/
	void autoLogScale();

	/**
	 * Functions to create the graphical elements.
	 **/
	void addBackground()
	    { createPanel( { -leftBorder(), -topBorder() - _size.height(), fullWidth(), fullHeight() } ); }
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
	 * Add a vertical line, from slightly below the x-axis (zero)
	 * to slightly above the top of the histogram.  Two lines are
	 * actually created: one visible line and a wider transparent
	 * line to make the tooltip easier to trigger.  The tooltip
	 * shows the name and the value it marks.  The z-value of the
	 * visible line is set to 'layer' and the transparent line one
	 * higher.
	 **/
	void addMarker( int                 index,
	                const QString     & name,
	                const QPen        & pen,
	                GraphicsItemLayer   layer );

	/**
	 * Fit the graphics into the viewport.
	 **/
	void fitToViewport();

	/**
	 * Return or set whether the geometry of the histogram needs
	 * to be re-caclulated.
	 **/
	bool geometryDirty() const { return !_size.isValid(); }
	void setGeometryDirty() { _size = QSize{}; }

	/**
	 * Calculate the content geometry to try and fit the viewport.
	 * The histogram may be over-sized (it is always built to a
	 * minimum height, to fit the overflow panel contents) and
	 * will then be scaled down to fit the viewport.  If the
	 * histogram is forced to a minimum height then the width is
	 * increased proportionally so that it will scale down to
	 * fill the viewport width.
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
	constexpr static qreal textSpacing()       { return  25; };
	constexpr static qreal topTextHeight()     { return  34; };
	constexpr static qreal axisExtraLength()   { return   5; };
	constexpr static qreal markerExtraHeight() { return  10; };
	constexpr static qreal overflowBorder()    { return  12; };
	constexpr static qreal overflowGap()       { return  15; };
	constexpr static qreal overflowSpacing()   { return  10; };
	constexpr static qreal pieDiameter()       { return  75; };
	constexpr static qreal pieSliceOffset()    { return  10; };

	/**
	 * Convenience functions for the height and width of the
	 * histogram including borders.
	 **/
	qreal fullWidth()  const { return _size.width()  + leftBorder() + rightBorder();  }
	qreal fullHeight() const { return _size.height() + topBorder()  + bottomBorder(); }

	/**
	 * Calculate the width of the overflow panel based on the
	 * width of the headline text, which may vary depending
	 * on the theme font (and possibly a translation).  The
	 * width may be set to fit the pie if that is wider then
	 * the headline.
	 *
	 * The width returned from this function includes a border
	 * on both sides.  The border is not expected to normally
	 * contain any text or graphics, but does allow for, for
	 * example, the margin around graphics text items.
	 *
	 * Note that any text line (including its margins) longer
	 * than the headline (plus the borders) will wrap and not
	 * affect the required width, although it will increase
	 * the height required for the overflow panel.
	 **/
	static qreal overflowWidth();
	static QString overflowHeadline() { return tr( "Cut-off percentiles" ); }

	/**
	 * Return a brush for a background area.
	 **/
	const QBrush & background()      const { return palette().base(); }
	const QBrush & panelBackground() const { return palette().alternateBase(); }

	/**
	 * Return colours and pens for items in the histogram.
	 **/
	const QColor & lineColor()       const { return palette().color( QPalette::ButtonText ); }
	const QColor & medianColor()     const { return palette().color( QPalette::LinkVisited ); }
	const QColor & quartileColor()   const { return palette().color( QPalette::Link ); }
	const QColor & decileColor()     const { return lineColor(); }
	const QColor & percentileColor() const
	    { return palette().color( QPalette::Disabled, QPalette::ButtonText ); }

	const QPen medianPen()            const { return QPen{ medianColor(), 2 }; }
	const QPen quartilePen()          const { return QPen{ quartileColor(), 2 }; }
	const QPen percentilePen( int i ) const
	    { return i % 10 == 0 ? decileColor() : percentileColor(); }

	/**
	 * Return a brush for part of a cut-off pie.
	 **/
	static QBrush pieBrush()           { return barBrush(); }
	static QBrush overflowSliceBrush() { return QColor{ 0xD0, 0x40, 0x20 }; }

	/**
	 * Create a panel item in the current scene, set its Z-value, and
	 * give it a background.
	 **/
	void createPanel( const QRectF & rect );

	/**
	 * Resize the view.
	 *
	 * Reimplemented from QGraphicsView (from QWidget).
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
	const int  _rightMarginPercentiles{ 0 };

	// Configurable settings
	int  _startPercentile;
	int  _endPercentile;
	bool _useLogScale;
	bool _autoLogScale{ true };
	int  _percentileStep{ 0 };

	// Geometry
	qreal  _minHeight{ 100 };
	QSizeF _size;

    }; // class HistogramView

} // namespace QDirStat

#endif // ifndef HistogramView_h
