/*
 *   File name: PercentBar.h
 *   Summary:   Functions and item delegate for percent bar
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef PercentBar_h
#define PercentBar_h

#include <QColor>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QTreeWidget>

#include "Typedefs.h" // ColorList


namespace QDirStat
{
    /**
     * Item delegate class to paint the percent bar in the PercentBarCol.
     *
     * This is a generic class that can be used for all kinds of QTreeView-
     * derived widgets, including the simplistic QTreeWidget.
     *
     * This delegate can handle one percent bar in one column; but you can
     * install multiple of them, one for each column that should get a percent
     * bar.
     *
     * The percent value is obtained from the RawDataRole and the percent bar
     * is rendered with that value.  No bar will be displayed
     * if the QVariant() returned is invalid or the float derived from it is
     * less than zero.
     *
     * Example:
     *
     *    60.0%    ->   [======    ]
     *
     * The percent bar is indented using the value obtained from the TreeLevelRole
     * and the tree indentation, and a  different color may be used for each
     * indentation level.
     **/
    class PercentBarDelegate final : public QStyledItemDelegate
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	PercentBarDelegate( QTreeView * treeView,
	                    int         barWidth,
	                    QColor      barBackground,
	                    ColorList   fillColors ):
	    QStyledItemDelegate{ treeView },
	    _sizeHintWidth{ barWidth },
	    _barBackground{ barBackground },
	    _fillColors{ fillColors },
	    _indentation{ treeView->indentation() }
	{}

	/**
	 * Paint one cell in the view.
	 *
	 * Inherited from QStyledItemDelegate.
	 **/
	void paint( QPainter                   * painter,
	            const QStyleOptionViewItem & option,
	            const QModelIndex          & index ) const override;

	/**
	 * Return a size hint for one cell in the view.
	 *
	 * Inherited from QStyledItemDelegate.
	 **/
	QSize sizeHint( const QStyleOptionViewItem & option,
	                const QModelIndex          & index) const override;

	/**
	 * Create two PercentBarDelegates from configuration settings and
	 * add them to the 'filesCol' and 'sizeCol' columns of 'tree'.
	 **/
	static void createStatsDelegates( QTreeWidget * treeWidget, int filesCol, int sizeCol );


    protected:

	/**
	 * Paint a percent bar into a widget.
	 *
	 * 'indentPixel' is the number of pixels to indent the bar.
	 **/
	void paintPercentBar( QPainter                   * painter,
	                      const QStyleOptionViewItem & option,
	                      const QModelIndex          & index,
	                      float                        percent ) const;


    private:

	int               _sizeHintWidth;
	const QColor      _barBackground;
	const ColorList   _fillColors;
	int               _indentation;

    };	// class PercentBarDelegate

}	// namespace QDirStat

#endif	// PercentBar_h
