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

    enum UnreadableDirectories
    {
	UD_PathCol,
	UD_UserCol,
	UD_GroupCol,
	UD_PermCol,
	UD_OctalCol,
    };


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
    class UnreadableDirsWindow final : public QDialog
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
	static void populateSharedInstance()
	    { sharedInstance()->populate(); }


    protected slots:

	/**
	 * Populate the window: locate unreadable directories.
	 *
	 * This clears the old search results first, then searches from
	 * the top level of the directory tree.
	 **/
	void populate();


    private:

	std::unique_ptr<Ui::UnreadableDirsWindow> _ui;

    };	// class UnreadableDirsWindow



    /**
     * Item class for the filesystems list.
     **/
    class UnreadableDirsItem final : public QTreeWidgetItem
    {
    public:

	/**
	 * Constructor.
	 **/
	UnreadableDirsItem( const DirInfo * dir );


    protected:

	/**
	 * Override the model data, just for the tooltip for the path
	 * column.
	 **/
	QVariant data( int column, int role ) const override;

    };	// class FilesystemItem

}	// namespace QDirStat

#endif	// UnreadableDirsWindow_h
