/*
 *   File name: PercentBar.cpp
 *   Summary:	Functions and item delegate for percent bar
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <QPainter>
#include <QTreeView>

#include "PercentBar.h"
#include "Settings.h"
#include "SettingsHelpers.h"
#include "Exception.h"
#include "Logger.h"


#define MIN_PERCENT_BAR_HEIGHT 16


using namespace QDirStat;


PercentBarDelegate::PercentBarDelegate( QTreeView * treeView,
                                        int         percentBarCol,
					int         startColorIndex,
					int         invisibleLevels ):
    QStyledItemDelegate { treeView },
    _treeView { treeView },
    _percentBarCol { percentBarCol },
    _startColorIndex { startColorIndex },
    _invisibleLevels { invisibleLevels }
{
    readSettings();
}


ColorList PercentBarDelegate::defaultFillColors() const
{
    return ColorList( { QColor(   0,   0, 255 ),
			QColor( 128,   0, 128 ),
			QColor( 231, 147,  43 ),
			QColor(   4, 113,   0 ),
			QColor( 176,   0,   0 ),
			QColor( 204, 187,   0 ),
			QColor( 162,  98,  30 ),
			QColor(   0, 148, 146 ),
			QColor( 217,  94,   0 ),
			QColor(   0, 194,  65 ),
			QColor( 194, 108, 187 ),
			QColor(   0, 179, 255 ),
		      } );
}


void PercentBarDelegate::readSettings()
{
    Settings settings;

    settings.beginGroup( "PercentBar" );
    _fillColors	   = readColorListEntry( settings, "Colors"    , defaultFillColors() );
    _barBackground = readColorEntry    ( settings, "Background", QColor( 160, 160, 160 ) );
    _sizeHintWidth = settings.value( "PercentBarColumnWidth", 180 ).toInt();
    settings.endGroup();
}

/*
void PercentBarDelegate::writeSettings()
{
    Settings settings;
    settings.beginGroup( "PercentBar" );

    writeColorListEntry( settings, "Colors"    , _fillColors	);
    writeColorEntry    ( settings, "Background", _barBackground );

    settings.setValue( "PercentBarColumnWidth",  _sizeHintWidth	);

    settings.endGroup();
}
*/

void PercentBarDelegate::paint( QPainter		   * painter,
				const QStyleOptionViewItem & option,
				const QModelIndex	   & index ) const
{
    // Let the default delegate draw what it can, which should be the appropriate background for us
    QStyledItemDelegate::paint( painter, option, index );

    if ( index.isValid() && index.column() == _percentBarCol )
    {
	const QVariant data = index.data( RawDataRole );
	bool ok = true;
	float percent = data.toFloat( &ok );

	if ( ok && percent >= 0.0f )
	{
	    if ( percent > 100.0f )
	    {
//		if ( percent > 103.0f )
//		    logError() << "Percent maxed out: " << percent << Qt::endl;
		percent = 100.0f;
	    }

	    PercentBarDelegate::paintPercentBar( painter, option, index, percent );
	}
    }
}


int PercentBarDelegate::treeLevel( const QModelIndex & index ) const
{
    int level = 0;
    for ( QModelIndex item = index; item.isValid(); item = item.parent() )
	++level;

    return level;
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


void PercentBarDelegate::paintPercentBar( QPainter		     * painter,
					  const QStyleOptionViewItem & option,
					  const QModelIndex	     & index,
					  float			       percent ) const
{
    const int depth        = treeLevel( index ) - _invisibleLevels;
    const int indentPixel  = ( depth * _treeView->indentation() ) / 2;

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
	/*
	 * Notice: The Xlib XDrawRectangle() function always fills one
	 * pixel less than specified. Although this is very likely just a
	 * plain old bug, it is documented that way. Obviously, Qt just
	 * maps the fillRect() call directly to XDrawRectangle() so they
	 * inherited that bug (although the Qt doc stays silent about
	 * it). So it is really necessary to compensate for that missing
	 * pixel in each dimension.
	 **/

	// Fill the percentage
	const int fillWidth = ( w - 2 * penWidth ) * percent / 100;
	const int colorIndex = depth + _startColorIndex;
	painter->fillRect( x + penWidth, y + penWidth,
			   fillWidth + 1, h - 2 * penWidth + 1,
			   _fillColors.at( colorIndex % _fillColors.size() ) );

	// Draw 3D shadows.
	const QColor background = painter->background().color();

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


QColor PercentBarDelegate::contrastingColor( const QColor &desiredColor,
					     const QColor &contrastColor )
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
