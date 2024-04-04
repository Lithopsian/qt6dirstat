/*
 *   File name: LocateFileTypeWindow.h
 *   Summary:	QDirStat "locate files by type" window
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#ifndef LocateFileTypeWindow_h
#define LocateFileTypeWindow_h

#include <QDialog>
#include <QTreeWidgetItem>

#include "ui_locate-file-type-window.h"
#include "FileInfoSet.h"
#include "FileSize.h"
#include "Subtree.h"


namespace QDirStat
{
    class DirTree;
    class FileTypeStats;
    class MimeCategory;
    class SelectionModel;


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
        static void populateSharedInstance( const QString & suffix, FileInfo * subtree );

        /**
         * Obtain the subtree from the last used URL or 0 if none was found.
         **/
//        const Subtree & subtree() const { return _subtree; }

	/**
	 * Return the current search suffix with leading '*'.
	 **/
	QString displaySuffix() const { return "*" + _suffix; }


    public slots:

	/**
	 * Refresh (reload) all data.
	 **/
	void refresh();


    protected slots:

	/**
	 * Select one of the search results in the main window's tree and
	 * treemap widgets via their SelectionModel.
	 **/
	void selectResult( QTreeWidgetItem * item );


    protected:

	/**
	 * Clear all data and widget contents.
	 **/
	void clear();

	/**
	 * One-time initialization of the widgets in this window.
	 **/
	void initWidgets();

	/**
	 * Populate the window: Locate files with 'suffix' in 'subtree'.
	 *
	 * This clears the old search results first, then searches the subtree
	 * and populates the search result list with the directories where
	 * matching files were found.
	 **/
	void populate( const QString & suffix, FileInfo * subtree = nullptr );

	/**
	 * Recursively locate directories that contain files matching the
	 * search suffix and create a search result item for each one.
	 **/
	void populateRecursive( FileInfo * dir );

	/**
	 * Return all direct file children matching the current search suffix.
	 **/
	FileInfoSet matchingFiles( FileInfo * dir );

        /**
         * Select the first item in the list. This will also select it in the
         * main window, open the branch where this item is in and scroll the
         * main window's tree so that item is visible tere.
         **/
        void selectFirstItem()
	    { _ui->treeWidget->setCurrentItem( _ui->treeWidget->topLevelItem( 0 ) ); }


    private:

	//
	// Data members
	//

	Ui::LocateFileTypeWindow * _ui;
        Subtree                    _subtree;
	QString			   _suffix;
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
     * Notice that this item intentionally does not store a FileInfo or DirInfo
     * pointer for each search result, but its path. This is more expensive to
     * store, and the corresponding DirInfo * has to be fetched again with
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
				int		count,
				FileSize	totalSize );

	/**
	 * Getters for the item properties.
	 **/
	QString	 path()	      const { return _path; }
	int	 count()      const { return _count; }
	FileSize totalSize()  const { return _totalSize; }


    protected:

	/**
	 * Set both the text and text alignment for a column.
	 **/
	void set( int col, const QString & text, Qt::Alignment alignment );

	/**
	 * Less-than operator for sorting.
	 **/
	bool operator<( const QTreeWidgetItem & other ) const override;


    private:

	QString		_path;
	int		_count;
	FileSize	_totalSize;
    };

} // namespace QDirStat


#endif // LocateFileTypeWindow_h