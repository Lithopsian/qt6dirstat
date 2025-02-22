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
     * current / selected items between the DirTree, the DirTreeModel and this
     * DirTreeView.
     **/
    class DirTreeView final : public QTreeView
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
	 * Expand an item based on a FileInfo pointer.
	 **/
	void expandItem( FileInfo * item );

	/**
	 * Scroll to the current item (index).  This will open any necessary
	 * branches and attempt to center the item in the viewport.
	 **/
	void scrollToCurrent()
	    { scrollTo( currentIndex(), QAbstractItemView::PositionAtCenter ); }

	/**
	 * Helpers to get model data or headerData values directly from
	 * DirTreeView.
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

	/**
	 * Update the viewport to show changes in the visible rows.  This
	 * includes moved rows as well as changes to the column data.
	 *
	 * When there are relatively few rows (less than a thousand), the
	 * entire tree is layed out.  This becomes too slow for many rows and
	 * a simpler approch is used: update the sizes of the columns and then
	 * re-paint the visible viewport.  This shows the correct data for each
	 * row, but may not update the child indicator when rows are moved, but
	 * allows updates to complete in a sensible timeframe when, for
	 * example, the tree is opened several levels deep, or in a package
	 * view when there are over a thousand top-level items.
	 *
	 **/
	void rowsChanged( const QModelIndex & index );


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
	 * Tooltip event handler: detect elided text in the name
	 * column.  This can't be done from the model where other
	 * tooltips are handled because the actual tree geometry can't
	 * be accessed.
	 *
	 * Mouse button handler: don't let QTreeView take the mouse
	 * back/forward buttons; we want consistent history buttons
	 * throughout the application.
	 *
	 * Reimplemented from QTreeView.
	 **/
	bool viewportEvent( QEvent * event ) override;


    private:

	HeaderTweaker * _headerTweaker;

    };	// class DirTreeView

}	// namespace QDirStat

#endif	// DirTreeView_h
