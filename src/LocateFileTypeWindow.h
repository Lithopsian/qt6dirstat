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
#include "FileInfoSet.h"
#include "Subtree.h"
#include "Typedefs.h" // FileSize


namespace QDirStat
{
    /**
     * Modeless dialog to display search results after clicking "locate" in the
     * file type stats window.
     *
     * This window shows a directory entry for each directory that contains
     * files of the requested type (with the requested suffix). When the user
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
	 * instance.  The suffix should start with '.', but not '*.".
	 **/
	static void populateSharedInstance( const QString & suffix, FileInfo * fileInfo );


    protected slots:

	/**
	 * Refresh (reload) all data.
	 **/
	void refresh();

	/**
	 * Select one of the search results in the main window's tree and
	 * treemap widgets via their SelectionModel.
	 **/
	void selectResult( QTreeWidgetItem * item ) const;


    protected:

	/**
	 * Return the current search suffix with leading '*'.
	 **/
	QString displaySuffix() const { return '*' + _suffix; }

	/**
	 * One-time initialization of the widgets in this window.
	 **/
	void initWidgets();

	/**
	 * Populate the window: Locate files with 'suffix' in 'fileInfo'.
	 *
	 * This clears the old search results first, then searches the subtree
	 * and populates the search result list with the directories where
	 * matching files were found.
	 **/
	void populate( const QString & suffix, FileInfo * fileInfo = nullptr );

	/**
	 * Recursively locate directories that contain files matching the
	 * search suffix and create a search result item for each one.
	 **/
	void populateRecursive( FileInfo * dir );

	/**
	 * Select the first item in the list. This will also select it in the
	 * main window, open the branch where this item is in and scroll the
	 * main window's tree so that item is visible tere.
	 **/
	void selectFirstItem() const
	    { _ui->treeWidget->setCurrentItem( _ui->treeWidget->topLevelItem( 0 ) ); }

	/**
	 * Resize event, reimplemented from QWidget.
	 *
	 * Elide the title to fit inside the current dialog width, so that
	 * they fill the available width but very long paths don't stretch
	 * the dialog.  A little extra room is left for the user to
	 * shrink the dialog, which would then force the label to be elided
	 * further.
	 **/
	void resizeEvent( QResizeEvent * ) override;


    private:

	//
	// Data members
	//

	std::unique_ptr<Ui::LocateFileTypeWindow> _ui;

	Subtree _subtree;
	QString _suffix;
    };


    /**
     * Column numbers for the file type tree widget
     **/
    enum SuffixSearchResultColumns
    {
	SSR_CountCol = 0,
	SSR_TotalSizeCol,
	SSR_PathCol,
	SSR_ColumnCount
    };


    /**
     * Item class for the locate list (which is really a tree widget),
     * representing one directory that contains files with the desired suffix.
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
    class SuffixSearchResultItem: public QTreeWidgetItem
    {
    public:

	/**
	 * Constructor.
	 **/
	SuffixSearchResultItem( const QString & path,
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
	 * Set both the text and text alignment for a column.
	 **/
	void set( int col, const QString & text, Qt::Alignment alignment )
	{
	    setText( col, text );
	    setTextAlignment( col, alignment | Qt::AlignVCenter );
	}

	/**
	 * Less-than operator for sorting.
	 **/
	bool operator<( const QTreeWidgetItem & other ) const override;


    private:

	QString  _path;
	int      _count;
	FileSize _totalSize;
    };

} // namespace QDirStat

#endif // LocateFileTypeWindow_h
