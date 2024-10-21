/*
 *   File name: SizeColDelegate.cpp
 *   Summary:   DirTreeView delegate for the size column
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QApplication>
#include <QPainter>
#include <QPalette>

#include "SizeColDelegate.h"
#include "DirTreeModel.h"
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
    QColor highlightedText( const QStyleOptionViewItem & option,
                            bool                         sparseFile,
                            bool                         disabled )
    {
	// Pick a suitable color for the special text, based on the theme settings, whether
	// it is for a sparse file, and whether the item should be rendered as disabled
	const bool selected = option.state & QStyle::State_Selected;
	const QBrush & background = selected ? option.palette.highlight() : option.palette.base();

	if ( background.color().lightness() < LIGHTNESS_THRESHOLD )
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


void SizeColDelegate::paint( QPainter                   * painter,
                             const QStyleOptionViewItem & option,
                             const QModelIndex          & index ) const
{
    // Let the default delegate draw what it can, which should be the appropriate background for us
    QStyledItemDelegate::paint( painter, option, index );

    const QBrush textBrush = index.data( Qt::ForegroundRole ).value<QBrush>();

    const QStringList data = index.data( SizeTextRole ).toStringList();
    const bool sparseFile = data.size() == 3;
    const QString linksText = sparseFile ? data.at( 2 ) : QString{};

    if ( data.size() == 2 || data.size() == 3 )
    {
	const QString & sizeText  = data.at( 0 ); // "137 B"
	const QString & allocText = data.at( 1 ); // " (4k)"

	// Use the model font since it may be bold (for dominant items)
	painter->setFont( index.data( Qt::FontRole ).value<QFont>() );

	const QPalette & palette = option.palette;
	const bool disabled      = textBrush == palette.brush( QPalette::Disabled, QPalette::WindowText );
	const bool selected      = option.state & QStyle::State_Selected;
	const auto group         = disabled ? QPalette::Disabled : QPalette::Normal;
	const auto role          = selected ? QPalette::HighlightedText : QPalette::WindowText;
	const int alignment      = Qt::AlignRight | Qt::AlignVCenter;

	// Since we align right, move the rect right edge to the left for each piece of text
	QRect rect = option.rect;

	// Draw the links text, if any
	rect.setRight( rect.right() - RIGHT_MARGIN );
	painter->setPen( palette.color( group, role ) );
	painter->drawText( rect, alignment, linksText );

	// Draw the allocated size (" (4k)").
	rect.setRight( rect.right() - textWidth( painter->font(), linksText ) );
	painter->setPen( highlightedText( option, sparseFile, disabled ) );
	painter->drawText( rect, alignment, allocText );

	// Draw the size text ("137 B")
	rect.setRight( rect.right() - textWidth( painter->font(), allocText ) );
	painter->setPen( palette.color( group, role ) );
	painter->drawText( rect, alignment, sizeText );

	return;
    }
}


QSize SizeColDelegate::sizeHint( const QStyleOptionViewItem & option,
                                 const QModelIndex          & index) const
{
    const QStringList data = index.data( SizeTextRole ).toStringList();
    if ( data.size() == 2 || data.size() == 3 )
    {
	const QString text   = data.join( QLatin1String{} );
	const QFont   font   = option.font;
	const int     width  = textWidth( font, text ) + LEFT_MARGIN + RIGHT_MARGIN;
	const int     height = fontHeight( font ) + TOP_MARGIN + BOTTOM_MARGIN;

	return QSize{ width, height };
    }

    return QStyledItemDelegate::sizeHint( option, index );
}
