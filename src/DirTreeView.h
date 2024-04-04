/*
 *   File name: DirTreeView.h
 *   Summary:	Tree view widget for directory tree
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */

#ifndef DirTreeView_h
#define DirTreeView_h


#include <QTreeView>
#include <QTreeWidget>

class QAction;


namespace QDirStat
{
    class DirTreeModel;
    class FileInfo;
    class HeaderTweaker;
    class PercentBarDelegate;
    class SizeColDelegate;

    /**
     * Tree view widget for the QDirStat directory tree.
     *
     * This is a thin wrapper around TreeView that takes care about basic setup
     * and configuration of the tree view and adds support for synchronizing
     * current / selected items between the DirTree, the DirTreeMap and this
     * DirTreeView.
     *
     * The Qt model / view abstraction is kept up as good as possible, but this
     * widget is really meant to be used with a QDirStat::DirTreeModel and not
     * just any random subclass of QAbstractItemModel.
     **/
    class DirTreeView: public QTreeView
    {
	Q_OBJECT

    public:

	/**
	 * Constructor
	 **/
	DirTreeView( QWidget * parent = nullptr );

	/**
	 * Destructor
	 **/
	~DirTreeView() override;

	/**
	 * Return this view's header tweaker.
	 **/
	HeaderTweaker * headerTweaker() const { return _headerTweaker; }

        /**
         * Expand or collapse an item based on a FileInfo pointer.
         **/
        void setExpanded( FileInfo * item, bool expanded = true );

        /**
         * Scroll to the current item (index).  This will open any necessary
	 * branches and attempt to center the item in the viewport.
         **/
        void scrollToCurrent() { scrollTo ( currentIndex(), QAbstractItemView::PositionAtCenter ); }


    public slots:

	/**
	 * Close (collapse) all branches except the one that 'branch' is in.
	 **/
	void closeAllExcept( const QModelIndex & branch );


    protected slots:

	/**
	 * Post the common context menu with actions (cleanup and other) for
	 * 'item' at 'pos'.
	 **/
	void contextMenu( const QPoint & pos );


    protected:

	/**
	 * Obtain the DirTreeModel object for this tree view.
	 **/
	const DirTreeModel * dirTreeModel() const;

	/**
	 * Return the list of items that are currently expanded.
	 **/
	QModelIndexList expandedIndexes() const;

	/**
	 * Change the current item. Overwritten from QTreeView to make sure
	 * the branch of the new current item is expanded and scrolled to
	 * the visible area.
	 *
	 * Doesn't currently do anything different from the default implementation.
	 **/
//	void currentChanged( const QModelIndex & current,
//			     const QModelIndex & oldCurrent ) override;

        /**
         * Keyboard event handler.
         *
         * Reimplemented from QTreeView.
         **/
//        void keyPressEvent( QKeyEvent * event ) override;

        /**
         * Mouse button handler.
         *
         * Don't let QTreeView steal and misappropriate the mouse back /
         * forward buttons; we want consistent history buttons throughout the
         * application.
         *
         * Reimplemented from QTreeView.
         **/
        void mousePressEvent( QMouseEvent * event ) override;


    private:

	// Data members

	PercentBarDelegate * _percentBarDelegate;
        SizeColDelegate    * _sizeColDelegate;
	HeaderTweaker      * _headerTweaker;

    };	// class DirTreeView

}	// namespace QDirStat


#endif	// DirTreeView_h
