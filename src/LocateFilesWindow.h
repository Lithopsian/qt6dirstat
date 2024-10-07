/*
 *   File name: LocateFilesWindow.h
 *   Summary:   QDirStat "locate files" window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef LocateFilesWindow_h
#define LocateFilesWindow_h

#include <memory>

#include <QDialog>
#include <QTreeWidgetItem>

#include "ui_locate-files-window.h"
#include "Subtree.h"
#include "Typedefs.h" // FileSize


namespace QDirStat
{
    class TreeWalker;

    /**
     * Modeless dialog to display search results for "discover" actions.
     *
     * This window shows a file with its complete path. When the user clicks on
     * a search result, that file is located in the QDirStat main window's tree
     * view; its directory branch is opened, and all other open branches of the
     * tree are closed; very much like when clicking on a treemap tile.
     *
     * As a next step, the user can then start cleanup actions on those files
     * from the main window - in the tree view or in the treemap view.
     **/
    class LocateFilesWindow: public QDialog
    {
	Q_OBJECT

	/**
	 * Constructor.  Private, use the static populateSharedInstance()
	 * to access this window.
	 *
	 * Note that this widget will destroy itself upon window close.
	 *
	 * This class takes over ownership of the TreeWalker and will delete it
	 * when appropriate.
	 **/
	LocateFilesWindow( TreeWalker * treeWalker,
                           QWidget    * parent = nullptr );

	/**
	 * Destructor.
	 **/
	~LocateFilesWindow() override;

	/**
	 * Returns the shared instance pointer for this window.  It is created if
	 * it doesn't already exist.
	 **/
	static LocateFilesWindow * sharedInstance( TreeWalker * treeWalker );


    public:

        /**
         * Populate the shared instance window with the given subtree.  The instance
	 * and window is created if necessary.  The window is always created with
	 * MainWindow as its parent, so it can remain open after a triggering dialog
	 * is closed.
         **/
	static void populateSharedInstance( TreeWalker    * treeWalker,
	                                    FileInfo      * fileInfo,
	                                    const QString & headingText,
	                                    int             sortCol,
	                                    Qt::SortOrder   sortOrder );


    protected slots:

	/**
	 * Refresh (reload) all data.
	 **/
	void refresh();

	/**
	 * Open a context menu for an item in the results list.
	 **/
	void itemContextMenu( const QPoint & pos );


    protected:

	/**
	 * Obtain the subtree from the last used URL or 0 if none was found.
	 **/
//	const Subtree & subtree() const { return _subtree; }

	/**
	 * Return the TreeWalker of this window.
	 **/
//	TreeWalker * treeWalker() const { return _treeWalker.get(); }

	/**
	 * Populate the window: Use the TreeWalker to find matching tree items
	 * in 'fileInfo'.
	 *
	 * This clears the old search results first, then searches the subtree
	 * and populates the search result list with the items where
	 * TreeWalker::check() returns 'true'.
	 **/
	void populate( FileInfo * fileInfo );

	/**
	 * Recursively locate directories that contain files matching the
	 * search suffix and create a search result item for each one.
	 **/
	void populateRecursive( FileInfo * dir );

	/**
	 * Resize event, reimplemented from QWidget.
	 *
	 * Elide the title to fit inside the current dialog width, so that
	 * they fill the available width but very long paths don't stretch
	 * the dialog.  A little extra room is left for the user to
	 * shrink the dialog, which would then force the label to be elided
	 * further.
	 **/
	void resizeEvent( QResizeEvent *) override;


    private:

	std::unique_ptr<Ui::LocateFilesWindow> _ui;

	std::unique_ptr<TreeWalker> _treeWalker;
	Subtree                     _subtree;

    };	// class LocateFilesWindow


    /**
     * Column numbers for the file type tree widget
     **/
    enum LocateListColumns
    {
	LL_SizeCol,
	LL_MTimeCol,
	LL_PathCol,
	LL_ColumnCount,
    };


    /**
     * Item class for the locate list, representing one file with its path.
     *
     * This item intentionally does not store a FileInfo or DirInfo pointer
     * for each search result, but its path. This is more expensive to store,
     * and the corresponding DirInfo * has to be fetched again with
     * DirTree::locate() (which is an expensive operation), but it is a lot
     * safer in case the tree is modified, i.e. if the user starts cleanup
     * operations or refreshes the tree from disk: Not only are no pointers
     * stored that might become invalid, but the search result remains valid
     * even after such an operation since the strings (the paths) will still
     * match an object in the tree in most cases.
     *
     * In the worst case, the search result won't find the corresponding
     * FileInfo * anymore (if that directory branch was deleted), but for sure
     * it will not crash.
     **/
    class LocateListItem: public QTreeWidgetItem
    {
    public:

	/**
	 * Constructor.
	 **/
	LocateListItem( FileInfo * item );

	/**
	 * Getters for the item properties.
	 **/
	FileSize        size()  const { return _size;  }
	time_t          mtime() const { return _mtime; }
	const QString & path()  const { return _path;  }


    protected:

	/**
	 * Override the model data, just for the tooltip for the path
	 * column.
	 **/
	QVariant data( int column, int role ) const override;

	/**
	 * Less-than operator for sorting.
	 **/
	bool operator<( const QTreeWidgetItem & other ) const override;


    private:

	FileSize _size;
	time_t   _mtime;
	QString  _path;

    };	// class LocateListItem

}	// namespace QDirStat

#endif // LocateFilesWindow_h
