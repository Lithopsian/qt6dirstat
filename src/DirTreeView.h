/*
 *   File name: DirTreeView.h
 *   Summary:   Tree view widget for directory tree
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef DirTreeView_h
#define DirTreeView_h

#include <QTreeView>


namespace QDirStat
{
    class DirTreeModel;
    class FileInfo;
    class HeaderTweaker;

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
	void setExpandedItem( FileInfo * item, bool expanded = true );

	/**
	 * Scroll to the current item (index).  This will open any necessary
	 * branches and attempt to center the item in the viewport.
	 **/
	void scrollToCurrent() { scrollTo( currentIndex(), QAbstractItemView::PositionAtCenter ); }

	/**
	 * Helpers to get model data or headerData values directly from
	 * DirTreView.
	 **/
	QVariant data( const QModelIndex & index, int role ) const
	    { return model()->data( index, role ); }
	QVariant headerData( int section, Qt::Orientation orientation, int role ) const
	    { return model()->headerData( section, orientation, role ); }


    public slots:

	/**
	 * Close (collapse) all branches except the one that 'branch' is in.
	 **/
	void closeAllExcept( const QModelIndex & branch );


    protected slots:

	/**
	 * Check that auto-sized columns are wide enough for the contents of
	 * the current visible rows.  Qt only checks up to 1,000 rows when it
	 * first displays a tree, and doesn't re-check until a repaint is
	 * forced, so a column may not be wide enough for the contents of
	 * some rows when there are more than 1,000 items visible in the tree.
	 *
	 * We check here and emit a signal if a column needs to be wider to
	 * fit any of the rows currently visible.  Only certain columns are
	 * checked, where rows can have different widths: NameCol,
	 * PercentNumCol, SizeCol, TotalItemsCol, TotalFilesCol,
	 * TotalSubDirsCol, UserCol, and GroupCol.
	 **/
	void scrolled( int value );

	/**
	 * Post the common context menu with actions (cleanup and other) for
	 * item at 'pos'.
	 **/
	void contextMenu( const QPoint & pos );


    protected:

	/**
	 * Obtain the DirTreeModel object for this tree view.
	 **/
	const DirTreeModel * dirTreeModel() const;

	/**
	 * Read the settings.  Currently just for the percent bar delegate.
	 **/
	void readSettings();

	/**
	 * Return the list of items that are currently expanded.
	 **/
	QModelIndexList expandedIndexes() const;

	/**
	 * Keyboard event handler.
	 *
	 * Reimplemented from QTreeView.
	 **/
        void keyPressEvent( QKeyEvent * event ) override;

	/**
	 * Mouse button handler.
	 *
	 * Don't let QTreeView take the mouse back/forward buttons; we
	 * want consistent history buttons throughout the application.
	 *
	 * Reimplemented from QTreeView.
	 **/
	void mousePressEvent( QMouseEvent * event ) override;

	/**
	 * Tooltip event handler.
	 *
	 * This is implemented here to detect elided text in the name
	 * column.  This could be done in a delegate, but can't be done
	 * from the model where other tooltips are handled.
	 *
	 * Reimplemented from QTreeView.
	 **/
	bool viewportEvent( QEvent * event ) override;


    private:

	HeaderTweaker * _headerTweaker;

    };	// class DirTreeView

}	// namespace QDirStat

#endif	// DirTreeView_h
