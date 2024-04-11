/*
 *   File name: SizeColDelegate.cpp
 *   Summary:	DirTreeView delegate for the size column
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */

#include <QPainter>
#include <QPalette>
#include <QTreeView>
#include <QApplication>

#include "SizeColDelegate.h"
#include "DirTreeModel.h"
#include "Exception.h"
#include "FileInfo.h"
#include "FormatUtil.h"
#include "Logger.h"

#define SPARSE_COLOR_NORMAL "#FF22AA"
#define SPARSE_COLOR_DARK   "#FF8888"
#define ALLOC_COLOR_NORMAL  "#2222FF"
#define ALLOC_COLOR_DARK    "#CCCCFF"
#define LIGHTNESS_THRESHOLD 144  // between the two alloc shades

#define TOP_MARGIN    0
#define BOTTOM_MARGIN 0
#define RIGHT_MARGIN  4
#define LEFT_MARGIN   6

using namespace QDirStat;


namespace
{
    /**
     * Determine the color to use for the highlighted (allocated) porition
     * of the delegate text size string.  This is based on the actual background
     * colour of the cell, to account for both dark themes and whether the item
     * is selected.
     **/
    inline QColor highlightedText( const QStyleOptionViewItem & option,
				   bool sparseFile,
				   bool disabled )
    {
	// Pick a suitable color for the special text, based on the theme settings, whether
	// it is for a sparse file, and whether the item should be rendered as disabled
	const bool selected = option.state & QStyle::State_Selected;
	const QColor background = selected ? option.palette.highlight().color() : option.palette.base().color();

       if ( background.lightness() < LIGHTNESS_THRESHOLD )
       {
	   const QColor color = sparseFile ? SPARSE_COLOR_DARK : ALLOC_COLOR_DARK;
	   return disabled ? color.darker( 125 ) : color;
       }
       else
       {
	   const QColor color = sparseFile ? SPARSE_COLOR_NORMAL : ALLOC_COLOR_NORMAL;
	   return disabled ? color.lighter( 125 ) : color;
       }
    }

} // namespace


void SizeColDelegate::paint( QPainter			* painter,
                             const QStyleOptionViewItem	& option,
                             const QModelIndex		& index ) const
{
    // Let the default delegate draw what it can, which should be the appropriate background for us
    QStyledItemDelegate::paint( painter, option, index );

    const QBrush textBrush = index.data( Qt::ForegroundRole ).value<QBrush>();

    QStringList data = index.data( RawDataRole ).toStringList();
    const bool sparseFile = data.size() == 3;
    const QString linksText = sparseFile ? data.takeLast() : "";

    if ( data.size() == 2 )
    {
	const int alignment = Qt::AlignRight | Qt::AlignVCenter;
	const bool disabled = textBrush == option.palette.brush( QPalette::Disabled, QPalette::WindowText );

	QRect rect               = option.rect;
	const QPalette & palette = option.palette;

	// We are responsible even for the highlight background of selected rows
	const bool selected = option.state & QStyle::State_Selected;
//	if ( selected )
//	    painter->fillRect( rect, palette.highlight() );

	const QString sizeText  = data.takeFirst(); // "137 B"
	const QString allocText = data.takeFirst(); // " (4k)"

	// Use the model font since it may be bold (for dominant items)
	painter->setFont( index.data( Qt::FontRole ).value<QFont>() );
	const int allocWidth = textWidth( painter->font(), allocText );
	const int linksWidth = textWidth( painter->font(), linksText );
	rect.setWidth( rect.width() - allocWidth - linksWidth - RIGHT_MARGIN );
	painter->setPen( palette.color( disabled ? QPalette::Disabled : QPalette::Normal,
					selected ? QPalette::HighlightedText : QPalette::WindowText ) );

	// Since we align right, we need to move the rectangle to the left
	// to reserve some space for the allocated size and any links text.
	painter->drawText( rect, alignment, sizeText );
	rect.setWidth( option.rect.width() - RIGHT_MARGIN );
	painter->drawText( rect, alignment, linksText );

	// Draw the allocated size (" (4k)").
	rect.setWidth( option.rect.width() - linksWidth - RIGHT_MARGIN );
	painter->setPen( highlightedText( option, sparseFile, disabled ) );
	painter->drawText( rect, alignment, allocText );

	return;
    }
}


QSize SizeColDelegate::sizeHint( const QStyleOptionViewItem & option,
                                 const QModelIndex          & index) const
{
    const QStringList data = index.data( RawDataRole ).toStringList();
    if ( data.size() == 2 || data.size() == 3 )
    {
	const QString text = data.join( "" );
	const QFont font = index.data( Qt::FontRole ).value<QFont>();
	const int width  = textWidth( font, text ) + LEFT_MARGIN + RIGHT_MARGIN;
	const int height = fontHeight( font ) + TOP_MARGIN + BOTTOM_MARGIN;
	const QSize size( width, height );
#if 0
	logDebug() << "size hint for \"" << text << "\": " << width << ", " << height << Qt::endl;
#endif
	return QSize( width, height );
    }

    return QStyledItemDelegate::sizeHint( option, index );
}
