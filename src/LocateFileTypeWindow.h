/*
 *   File name: LocateFileTypeWindow.h
 *   Summary:   QDirStat "locate files by type" window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef LocateFileTypeWindow_h
#define LocateFileTypeWindow_h

#include <memory>

#include <QDialog>
#include <QTreeWidgetItem>

#include "ui_locate-file-type-window.h"
#include "MimeCategorizer.h" // WildcardCategory
#include "Subtree.h"
#include "Typedefs.h" // FileSize


namespace QDirStat
{
    /**
     * Modeless dialog to display search results after clicking "locate" in the
     * file type stats window.
     *
     * This window shows a directory entry for each directory that contains
     * files of the requested type (with the requested pattern). When the user
     * clicks on a search result, that directory is opened in the QDirStat main
     * window's tree view (and all other open branches of the tree are closed),
     * and the matching files in that directory are selected.
     *
     * As a next step, the user can then start cleanup actions on those files
     * from the main window - in the tree view or in the treemap view.
     **/
    class LocateFileTypeWindow: public QDialog
    {
	Q_OBJECT

	/**
	 * Constructor.  Private, use the populateSharedInstance() function to
	 * access this window.
	 *
	 * Note that this widget will destroy itself upon window close.
	 **/
	LocateFileTypeWindow( QWidget *	parent = nullptr );

	/**
	 * Destructor.
	 **/
	~LocateFileTypeWindow() override;

	/**
	 * Static method for using one shared instance of this class between
	 * multiple parts of the application. This will create a new instance
	 * if there is none yet (or any more).
	 **/
	static LocateFileTypeWindow * sharedInstance();


    public:

	/**
	 * Convenience function for creating, populating and showing the shared
	 * instance.
	 **/
	static void populateSharedInstance( const WildcardCategory & wildcardCategory, FileInfo * fileInfo );


    protected slots:

	/**
	 * Refresh (reload) all data.
	 **/
	void refresh();

	/**
	 * Select one of the search results in the main window's tree and
	 * treemap widgets via their SelectionModel.  The actual selection
	 * is done after a short timeout to avoid blocking the dialog tree
	 * itself.
	 **/
	void selectResult() const;
	void selectResults() const;


    protected:

	/**
	 * Populate the window: Locate files with 'wildcardCategory' in
	 * 'fileInfo'.
	 *
	 * This clears the old search results first, then searches the subtree
	 * and populates the search result list with the directories where
	 * matching files were found.
	 **/
	void populate( const WildcardCategory & wildcardCategory, FileInfo * fileInfo = nullptr );

	/**
	 * Recursively locate directories that contain files matching the
	 * sWildcardCategory and create a search result item for each one.
	 **/
	void populateRecursive( FileInfo * dir );

	/**
	 * Event handler, reimplemented from QDialog/QWidget.
	 *
	 * Elide the title to fit inside the current dialog width, so that
	 * they fill the available width but very long paths don't stretch
	 * the dialog.  A little extra room is left for the user to
	 * shrink the dialog, which would then force the label to be elided
	 * further.
	 **/
	bool event( QEvent * event ) override;


    private:

	std::unique_ptr<Ui::LocateFileTypeWindow> _ui;

	Subtree          _subtree;
	WildcardCategory _wildcardCategory;

    };	// class LocateFileTypeWindow


    /**
     * Column numbers for the file type tree widget
     **/
    enum PatternSearchResultColumns
    {
	PSR_CountCol = 0,
	PSR_TotalSizeCol,
	PSR_PathCol,
	PSR_ColumnCount,
    };


    /**
     * Item class for the locate list (which is really a tree widget),
     * representing one directory that contains files with the desired
     * pattern.
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
     * DirInfo * anymore (if that directory branch was deleted), but for sure
     * it will not crash.
     **/
    class PatternSearchResultItem: public QTreeWidgetItem
    {
    public:

	/**
	 * Constructor.
	 **/
	PatternSearchResultItem( const QString & path,
	                        int             count,
	                        FileSize        totalSize );

	/**
	 * Getters for the item properties.
	 **/
	const QString & path()       const { return _path; }
	int             count()      const { return _count; }
	FileSize        totalSize()  const { return _totalSize; }


    protected:

	/**
	 * Override the model data, just for the tooltip for the
	 * directory column.
	 **/
	QVariant data( int column, int role ) const override;

	/**
	 * Less-than operator for sorting.
	 **/
	bool operator<( const QTreeWidgetItem & other ) const override;


    private:

	QString  _path;
	int      _count;
	FileSize _totalSize;

    };	// class PatternSearchResultItem

}	// namespace QDirStat

#endif	// LocateFileTypeWindow_h
