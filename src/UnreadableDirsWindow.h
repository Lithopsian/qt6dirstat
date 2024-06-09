/*
 *   File name: UnreadableDirsWindow.h
 *   Summary:   QDirStat "locate files" window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef UnreadableDirsWindow_h
#define UnreadableDirsWindow_h

#include <memory>

#include <QDialog>
#include <QTreeWidgetItem>

#include "ui_unreadable-dirs-window.h"
#include "Subtree.h"


namespace QDirStat
{
    class DirTree;
    class FileInfo;
    class FileTypeStats;
    class MimeCategory;

    /**
     * Modeless dialog to display directories that could not be read when
     * reading a directory tree.
     *
     * This window shows an entry for each directory with a read error with:
     *
     *	 - complete path
     *	 - user name
     *	 - group name
     *	 - permissions in octal ("0750")
     *	 - symbolic permissions "drwxrw----"
     *
     * Upon click, the directory is located in the main window, i.e.  in the
     * main window's tree view all parent directories are opened, the directory
     * is selected, and the tree view is scrolled so that this directory is
     * visible. The directory is also highlighted in the tree map, and the
     * details panel (unless disabled) shows all available details for that
     * directory.
     *
     * This window is meant to be connected to a panel message's "Details"
     * hyperlink.
     **/
    class UnreadableDirsWindow: public QDialog
    {
	Q_OBJECT

	/**
	 * Constructor.  Private, use the static populateSharedInstance() to access
	 * this window.
	 *
	 * Note that this widget will destroy itself upon window close.
	 **/
	UnreadableDirsWindow( QWidget *	parent = nullptr );

	/**
	 * Destructor.
	 **/
	~UnreadableDirsWindow() override;

        /**
         * Static method for using one shared instance of this class between
         * multiple parts of the application. This will create a new instance
         * if there is none yet (or anymore).
         *
         * Do not hold on to this pointer; the instance destroys itself when
         * the user closes the window, and then the pointer becomes invalid.
         *
         * After getting this shared instance, call populate() and show().
         **/
        static UnreadableDirsWindow * sharedInstance();


    public:

        /**
         * Convenience function for creating, populating and showing the shared
         * instance.
         **/
        static void populateSharedInstance( FileInfo * fileInfo );

	/**
	 * Obtain the subtree from the last used URL or 0 if none was found.
	 **/
//	const Subtree & subtree() const { return _subtree; }


    protected slots:

	/**
	 * Select one of the search results in the main window's tree and
	 * treemap widgets via their SelectionModel.
	 **/
	void selectResult( QTreeWidgetItem * item );


    protected:

	/**
	 * One-time initialization of the widgets in this window.
	 **/
	void initWidgets();

	/**
	 * Populate the window: Locate unreadable directories.
	 *
	 * This clears the old search results first, then searches the subtree
	 * and populates the search result list with the directories could not
	 * be read.
	 **/
	void populate( FileInfo * fileInfo );

	/**
	 * Recursively find unreadable directories in a subtree and add an
	 * entry to the tree widget for each one.
	 **/
	void populateRecursive( FileInfo * fileInfo );


    private:

	//
	// Data members
	//

	std::unique_ptr<Ui::UnreadableDirsWindow> _ui;

	Subtree _subtree;

    };



    /**
     * Item class for the directory list (which is really a tree widget),
     * representing one directory that could not be read.
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

    enum UnreadableDirectories
    {
	UD_Path,
	UD_User,
	UD_Group,
	UD_Permissions,
	UD_Octal
    };

    class UnreadableDirListItem: public QTreeWidgetItem
    {
    public:

	/**
	 * Constructor.
	 **/
	UnreadableDirListItem( DirInfo * dir );

	/**
	 * Return the path of this directory.
	 **/
	DirInfo * dir() const { return _dir; }


    protected:

	/**
	 * Set the text and alignment for a column.
	 **/
	void set( UnreadableDirectories col, const QString & text, Qt::Alignment alignment )
	{
	    setText( col, text );
	    setTextAlignment( col, alignment | Qt::AlignVCenter );
	}

	/**
	 * Less-than operator for sorting.
	 *
	 * Currently, all columns sort by their text value, so this override
	 * is not required.
	 **/
//	bool operator<( const QTreeWidgetItem & other ) const override;


    private:

	DirInfo * _dir;
    };

} // namespace QDirStat


#endif // UnreadableDirsWindow_h
