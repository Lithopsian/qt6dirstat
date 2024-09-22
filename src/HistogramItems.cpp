/*
 *   File name: HistogramViewItems.cpp
 *   Summary:   QGraphicsItems for file size histogram for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QGraphicsSceneMouseEvent>
#include <QToolTip>

#include "HistogramItems.h"
#include "FileSizeStats.h"
#include "FormatUtil.h"
#include "HistogramView.h"


using namespace QDirStat;


HistogramBar::HistogramBar( const FileSizeStats * stats,
                            int                   bucketIndex,
                            const QRectF        & rect,
                            qreal                 fillHeight,
                            const QPen          & pen,
                            const QBrush        & brush ):
    QGraphicsRectItem{ rect.normalized() }
{
    setFlags( ItemHasNoContents );
    setAcceptHoverEvents( true );
    setZValue( HistogramView::BarLayer );

    const int numFiles = stats->bucketCount( bucketIndex );
    const QString filesText{ numFiles == 1 ? QObject::tr( "file" ) : QObject::tr( "files" ) };
    const QString startText{ formatSize( stats->bucketStart( bucketIndex ) ) };
    const QString endText  { formatSize( stats->bucketEnd  ( bucketIndex ) ) };
    const QString tooltip = QObject::tr( "Bucket #%1<br/>%L2 %3<br/>%4...%5" )
	.arg( bucketIndex + 1 ).arg( numFiles ).arg( filesText, startText, endText );
    setToolTip( whitespacePre( tooltip ) );

    // Filled rectangle is relative to its parent
    QRectF filledRect{ rect.x(), 0, rect.width(), -fillHeight };
    QGraphicsRectItem * filledBar = new QGraphicsRectItem{ filledRect.normalized(), this };
    filledBar->setPen( pen );
    filledBar->setBrush( brush );
}


void HistogramBar::adjustBar( qreal adjustment )
{
    const QList<QGraphicsItem *> children = childItems();
    if ( !children.isEmpty() )
    {
	QGraphicsRectItem * filledBar = dynamic_cast<QGraphicsRectItem *>( children.first() );
	if ( filledBar )
	    filledBar->setRect( filledBar->rect().adjusted( adjustment, 0, -adjustment, 0 ) );
    }
}


void HistogramBar::hoverEnterEvent( QGraphicsSceneHoverEvent * )
{
    adjustBar( -2 );
    setZValue( HistogramView::HoverBarLayer );
}


void HistogramBar::hoverLeaveEvent( QGraphicsSceneHoverEvent * )
{
    adjustBar( 2 );
    setZValue( HistogramView::BarLayer );
}
