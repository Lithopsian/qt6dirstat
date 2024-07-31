/*
 *   File name: HistogramView.h
 *   Summary:   View widget for histogram rendering for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef HistogramItems_h
#define HistogramItems_h

#include <QGraphicsRectItem>


namespace QDirStat
{
    class FileSizeStats;
    class HistogramView;

    /**
     * QGraphicsItem class for a histogram bar: solely to be able
     * to pick up hover events.
     *
     * This creates an invisible full-height item so it is easy to
     * highlight a bucket and get a tooltip, and a visible child
     * rectangle to display the bucket height.
     **/
    class HistogramBar: public QGraphicsRectItem
    {
    public:
	/**
	 * Constructor: 'number' is the number of the bar (0 being the
	 * leftmost) in the histogram
	 **/
	HistogramBar( HistogramView       * parent,
		      const FileSizeStats * stats,
		      int                   number,
		      const QRectF        & rect,
		      qreal                 fillHeight );


    protected:
	/**
	 * Mouse hover events
	 *
	 * Reimplemented from QGraphicsItem
	 **/
	void hoverEnterEvent( QGraphicsSceneHoverEvent * ) override;
	void hoverLeaveEvent( QGraphicsSceneHoverEvent * ) override;

	/**
	 * Change the width of the filled rectangle.
	 **/
	void adjustBar( qreal adjustment );

    };

}	// namespace QDirStat

#endif // ifndef HistogramItems_h
