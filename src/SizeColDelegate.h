/*
 *   File name: SizeColDelegate.h
 *   Summary:   DirTreeView delegate for the size column
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef SizeColDelegate_h
#define SizeColDelegate_h

#include <QStyledItemDelegate>
#include <QTreeView>


namespace QDirStat
{
    class DirTreeModel;

    /**
     * Item delegate for the size column in the DirTreeView.
     *
     * This class can handle different font attributes and colors.
     **/
    class SizeColDelegate: public QStyledItemDelegate
    {
	Q_OBJECT

    public:

        /**
         * Constructor.
         **/
	SizeColDelegate( QTreeView * treeView ):
	    QStyledItemDelegate { treeView }
	{}

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

    };  // class SizeColDelegate

}       // namespace QDirStat

#endif  // SizeColDelegate_h
