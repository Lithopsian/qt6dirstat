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

#include <QGraphicsItem>

#include "Typedefs.h" // FileSize


namespace QDirStat
{
    class FileSizeStats;
    class HistogramView;

    /**
     * QGraphicsItem class for a histogram bar: a class to be able
     * to pick up hover events and easily adjust the bar size.
     *
     * This creates an invisible full-height item so it is easy to
     * highlight a bucket and get a tooltip, and a visible child
     * rectangle to display the bucket height.
     **/
    class HistogramBar: public QGraphicsRectItem
    {
    public:
	/**
	 * Constructor: 'bucketIndex' is the number of the bar (0
	 * being the leftmost) in the histogram.
	 **/
	HistogramBar( const FileSizeStats * stats,
	              int                   bucketIndex,
	              const QRectF        & rect,
	              qreal                 fillHeight,
	              const QPen          & pen,
	              const QBrush        & brush );

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

    }; // class HistogramBar

} // namespace QDirStat

#endif // ifndef HistogramItems_h
