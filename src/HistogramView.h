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

#include <QGraphicsItem>
#include <QGraphicsView>
#include <QResizeEvent>

#include "Typedefs.h" // FileSize


namespace QDirStat
{
    class FileSizeStats;

    /**
     * Histogram widget.
     *
     * This widget is based on buckets and percentiles, both of which have to
     * be fed from the outside as a pointer to FileSizeStats.
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
	    QuartileLayer,
	    MedianLayer,
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
	 * Set the percentile range (0..100) for which to display data.
	 **/
	void setPercentileRange( int startPercentile, int endPercentile, bool logWidths );

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
//	void setLeftExtraPercentiles( int number = 0 )
//	    { _leftExtraPercentiles = number; }

	/**
	 * Return the left extra percentiles or 0 if none are shown.
	 **/
//	int leftExtraPercentiles() const { return _leftExtraPercentiles; }

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
//	void setRightExtraPercentiles( int number= 2 )
//	    { _leftExtraPercentiles = number; }

	/**
	 * Return the right extra percentiles or 0 if none are shown.
	 **/
//	int rightExtraPercentiles() const { return _rightExtraPercentiles; }

	/**
	 * Set whether to automatically determine the type of y-axis
	 * scaling.
	 **/
	void enableAutoLogHeights() { _autoLogHeights = true; build(); }
	void disableAutoLogHeights() { _autoLogHeights = false; }

	/**
	 * Return whether the y-axis is currently showing as a log scale.
	 **/
	bool logHeights() const { return _logHeights; }
	void toggleLogHeights() { _logHeights = !_logHeights; build(); }


    protected:

	/**
	 * Return the stored value for percentile no. 'index' (0..100).
	 **/
	FileSize percentile( int index ) const;

	/**
	 * Build the histogram, probably based on a new set of data.
	 * The preferred y-axis scaling will be recalculated.
	 **/
	void build() { autoLogHeights(); rebuild(); }

	/**
	 * Rebuild the histogram based on the current data.  The geometry
	 * may be recalculated, for example if the window has been resized.
	 **/
	void rebuild();

	/**
	 * Automatically determine if a logarithmic height scale should be
	 * used. Set the internal _logHeightScale variable accordingly and
	 * return it.
	 **/
	void autoLogHeights();

	/**
	 * Calculate the content geometry to try and fit the viewport.
	 * The histogram may be over-sized (it is always built to a
	 * minimum height, to fit the overflow panel contents) and
	 * will then be scaled down to fit the viewport.  If the
	 * histogram is forced to a minimum height then the width is
	 * increased proportionally so that it will scale down to
	 * fill the viewport width.
	 **/
	inline QSizeF calcGeometry( qreal overflowWidth );

	/**
	 * Functions to create the graphical elements.  These are simply
	 * executed one after another to avoid having a single huge
	 * function, so they are declared as inline.
	 **/
	inline void addBackground( QGraphicsScene * scene );
	inline void addAxes( QGraphicsScene * scene );
	inline void addAxisLabels( QGraphicsScene * scene );
	inline void addXStartEndLabels( QGraphicsScene * scene );
	inline void addYStartEndLabels( QGraphicsScene * scene );
	inline void addQuartileText( QGraphicsScene * scene );
	inline void addBars( QGraphicsScene * scene );
	inline void addMarkers( QGraphicsScene * scene );
	inline void addOverflowPanel( QGraphicsScene * scene, qreal panelWidth );

	/**
	 * Fit the graphics into the viewport.
	 **/
	inline void fitToViewport( QGraphicsScene * scene );

	/**
	 * Return or set whether the geometry of the histogram needs
	 * to be re-caclulated.
	 **/
	bool geometryDirty() const { return !_size.isValid(); }
	void setGeometryDirty() { _size = QSize{}; }

	/**
	 * Set the geometry dirty and rebuild the histogram.
	 **/
	void rebuildDirty() { setGeometryDirty(); rebuild(); }

	/**
	 * Create a background rectangle in the scene and colour it
	 * as a panel.  The z-value is set to a negative number so
	 * that all other items appear in front of it.
	 **/
	void createPanel(  QGraphicsScene * scene, const QRectF & rect )
	    { scene->addRect( rect, Qt::NoPen, panelBackground() )->setZValue( PanelBackgroundLayer ); };

	/**
	 * Return true if an overflow ("cutoff") panel is needed.
	 **/
	bool needOverflowPanel() const;

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
	static QPen barPen() { return QColor{ 0x40, 0x40, 0x50 }; }

	/**
	 * Return a brush for part of the histogram
	 **/
	static QBrush barBrush()           { return QColor{ 0xB0, 0xB0, 0xD0 }; }
	static QBrush pieBrush()           { return barBrush(); }
	static QBrush overflowSliceBrush() { return QColor{ 0xD0, 0x40, 0x20 }; }

	/**
	 * Resize the view.
	 *
	 * Reimplemented from QGraphicsView (from QWidget).
	 **/
	void resizeEvent( QResizeEvent * ) override;

	/**
	 * Detect palette changes.
	 *
	 * Reimplemented from QGraphicsView (from QWidget).
	 **/
	void changeEvent( QEvent * event ) override;


    private:

	//
	// Data Members
	//

	// Statistics data to represent
	const FileSizeStats * _stats{ nullptr };

	// Flags not currently configurable
	const bool _showMedian{ true }; // always show median marker line
	const bool _showQuartiles{ true }; // always show quartiles marker lines
	const int  _leftExtraPercentiles{ 0 }; // no left extra percentile markers
	const int  _rightExtraPercentiles{ 0 }; // no right extra percentile markers

	// Configurable settings
	int  _startPercentile;
	int  _endPercentile;
	bool _logWidths;
	bool _logHeights;
	bool _autoLogHeights{ true }; // start with the log scale determined automatically
	int  _percentileStep{ 0 }; // no markers initially

	// Geometry
	qreal  _minHeight{ 100 }; // at least 100 pixels high in case there is no overflow panel
	QSizeF _size;

    }; // class HistogramView

} // namespace QDirStat

#endif // ifndef HistogramView_h
