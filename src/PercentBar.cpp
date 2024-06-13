/*
 *   File name: PercentBar.cpp
 *   Summary:   Functions and item delegate for percent bar
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QPainter>
#include <QTreeView>

#include "PercentBar.h"
#include "DirTreeModel.h"
#include "Logger.h"


#define MIN_PERCENT_BAR_HEIGHT 16


using namespace QDirStat;


namespace
{
    /**
     * Return a color that contrasts with 'contrastColor'.
     **/
    QColor contrastingColor( const QColor & desiredColor, const QColor & contrastColor )
    {
	if ( desiredColor != contrastColor )
	    return desiredColor;

	if ( contrastColor != contrastColor.lighter() )
	    // try a little lighter
	    return contrastColor.lighter();
	else
	    // try a little darker
	    return contrastColor.darker();
    }

} // namespace


void PercentBarDelegate::paint( QPainter                   * painter,
				const QStyleOptionViewItem & option,
				const QModelIndex          & index ) const
{
    // Let the default delegate draw what it can, which should be the appropriate background for us
    QStyledItemDelegate::paint( painter, option, index );

    if ( index.isValid() && index.column() == _percentBarCol )
    {
	bool ok = true;
	const float percent = index.data( PercentRole ).toFloat( &ok );
	if ( ok && percent >= 0.0f )
	    PercentBarDelegate::paintPercentBar( painter, option, index, qMin( percent, 100.0f ) );
    }
}


QSize PercentBarDelegate::sizeHint( const QStyleOptionViewItem & option,
                                    const QModelIndex          & index) const
{
    QSize size = QStyledItemDelegate::sizeHint( option, index );

    if ( !index.isValid() || index.column() != _percentBarCol )
	return size;

    size.setWidth( _sizeHintWidth );
    size.setHeight( qMax( size.height(), MIN_PERCENT_BAR_HEIGHT ) );

    return size;
}


void PercentBarDelegate::paintPercentBar( QPainter                   * painter,
					  const QStyleOptionViewItem & option,
					  const QModelIndex          & index,
					  float                        percent ) const
{
    const int depth       = index.data( TreeLevelRole ).toInt();
    const int indentPixel = depth * _indentation / 2;

    const int xMargin = 4;
    const int yMargin = option.rect.height() / 6;

    const int x = option.rect.x() + xMargin + indentPixel;
    const int y = option.rect.y() + yMargin;
    const int w = option.rect.width() - 2 * xMargin - indentPixel;
    const int h = option.rect.height() - 2 * yMargin;

    if ( w > 0 )
    {
	const int penWidth = 2;

	painter->setBrush( Qt::NoBrush );

	// Fill bar background
	painter->fillRect( x + penWidth, y + penWidth,
			   w - 2 * penWidth + 1, h - 2 * penWidth + 1,
			   _barBackground );

	/**
	 * The Xlib XDrawRectangle() function always fills one pixel less
	 * than specified. Although this is very likely just a plain old bug,
	 * it is documented that way. Qt maps the fillRect() call directly
	 * to XDrawRectangle() so they inherited that feature.
	 **/

	// Fill the percentage
	const int fillWidth = ( w - 2 * penWidth ) * percent / 100;
	painter->fillRect( x + penWidth, y + penWidth,
			   fillWidth + 1, h - 2 * penWidth + 1,
			   _fillColors.at( qMin( depth, _fillColors.size()-1 ) ) );

	// Draw 3D shadows.
	const QColor & background = painter->background().color();

	painter->setPen( contrastingColor( Qt::black, background ) );
	painter->drawLine( x, y, x+w, y );
	painter->drawLine( x, y, x, y+h );

	painter->setPen( contrastingColor( _barBackground.darker(), background ) );
	painter->drawLine( x+1, y+1, x+w-1, y+1 );
	painter->drawLine( x+1, y+1, x+1, y+h-1 );

	painter->setPen( contrastingColor( _barBackground.lighter(), background ) );
	painter->drawLine( x+1, y+h, x+w, y+h );
	painter->drawLine( x+w, y, x+w, y+h );

	painter->setPen( contrastingColor( Qt::white, background ) );
	painter->drawLine( x+2, y+h-1, x+w-1, y+h-1 );
	painter->drawLine( x+w-1, y+1, x+w-1, y+h-1 );
    }
}
