/*
 *   File name: PercentBar.h
 *   Summary:	Functions and item delegate for percent bar
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */

#ifndef PercentBar_h
#define PercentBar_h

#include <QColor>
#include <QList>
#include <QStyledItemDelegate>
#include <QTreeView>

#include "DirTreeModel.h"      // RawDataRole


typedef QList<QColor> ColorList;


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
     * The default behaviour is to use a percent value that is displayed as
     * text in that same column and render the percent bar instead. If there is
     * no suitable text with a percent number, it calls its superclass,
     * i.e. the text that is in that column (or nothing if there is no text at
     * all) is displayed.
     *
     * So, simply add a column for this percent bar and display the numeric
     * value (optionally followed by a '%' percent sign) in that column; the
     * delegate will display a graphical percent bar instead of that numeric
     * value.
     *
     * Example:
     *
     *    60.0%    ->   [======    ]
     *
     * If the numeric value should be displayed as well, add another column for
     * it, display the numeric value there as well, and don't install this
     * delegate for that other column.
     *
     * For indented tree levels, the percent bar is indented as well, and a
     * different color is used for each indentation level.
     **/
    class PercentBarDelegate: public QStyledItemDelegate
    {
	Q_OBJECT

    public:

	/**
	 * Constructor with all the base fields.  Other constructors will
	 * delegate to this one.
	 **/
	PercentBarDelegate( QTreeView * treeView,
			    int percentBarCol,
			    int startColorIndex,
			    int invisibleLevels );

	/**
	 * Paint one cell in the view.
	 * Inherited from QStyledItemDelegate.
	 **/
	void paint( QPainter		       * painter,
		    const QStyleOptionViewItem & option,
		    const QModelIndex	       & index ) const override;

	/**
	 * Return a size hint for one cell in the view.
	 * Inherited from QStyledItemDelegate.
	 **/
	QSize sizeHint( const QStyleOptionViewItem & option,
			const QModelIndex	   & index) const override;

	/**
	 * Return the percent bar fill colors for each tree level. If there
	 * are more tree levels than colors, the colors will wrap around.
	 *
	 * This object reference can be used directly to add, remove or change
	 * colors.
	 **/
//	ColorList & fillColors() { return _fillColors; }

        /**
         * Set the index of the starting color (default: 0).
         **/
//        void setStartColorIndex( int index ) { _startColorIndex = index; }

        /**
         * Return the index of the starting color.
         **/
//        int startColorIndex() const { return _startColorIndex; }


    public slots:

	/**
	 * Read parameters from the settings file.
	 **/
	void readSettings();

	/**
	 * Write parameters to the settings file.  Unnecessary at the moment as nothing
	 * can change while the program is running.
	 **/
//	void writeSettings();


    protected:

	/**
	 * Return the default fill colors.
	 **/
	ColorList defaultFillColors() const;

	/**
	 * Find out the tree depth level of item 'index' by following its
	 * parent, parent's parent etc. to the top.
	 **/
	int treeLevel( const QModelIndex & index ) const;

	/**
	 * Paint a percent bar into a widget.
	 * 'indentPixel' is the number of pixels to indent the bar.
	 **/
	void paintPercentBar( QPainter			 * painter,
			      const QStyleOptionViewItem & option,
			      const QModelIndex		 & index,
			      float			   percent ) const;


    private:

	//
	// Data Members
	//

	QTreeView * _treeView;
        int         _percentBarCol;
        int         _startColorIndex;
        int         _invisibleLevels;
	ColorList   _fillColors;
	QColor      _barBackground;
	int         _sizeHintWidth;

    }; // class PercentBarDelegate

}      // namespace QDirStat

#endif // PercentBar_h
