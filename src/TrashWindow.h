/*
 *   File name: TrashWindow.h
 *   Summary:   QDirStat "locate files" window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef TrashWindow_h
#define TrashWindow_h

#include <memory>

#include <QDialog>
#include <QProcess>
#include <QStringBuilder>
#include <QTreeWidgetItem>

#include "ui_trash-window.h"
#include "Typedefs.h" //FileSize


namespace QDirStat
{
    class ProcessStarter;

    /**
     * Simple struct to contain the unique identifying values for TrashDir
     * object.
     **/
    struct TrashEntry
    {
	QString trashRoot;
	QString entryName;
    }; // struct TrashEntry


    /**
     * Define operator== and qHash for TrashEntry so that it can be used as a
     * QHash key (for QSet).
     **/
#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
    inline uint qHash( const TrashEntry & key, uint seed )
#else
    inline size_t qHash( const TrashEntry & key, size_t seed )
#endif
    {
	return ::qHash( key.trashRoot, seed ) + ::qHash( key.entryName, seed );
    }
    inline bool operator==( const TrashEntry & lhs, const TrashEntry & rhs )
    {
	return lhs.trashRoot == rhs.trashRoot && lhs.entryName == rhs.entryName;
    }


    /**
     * Modeless dialog to display items in trash.
     **/
    class TrashWindow final : public QDialog
    {
	Q_OBJECT

	/**
	 * Constructor.  Private, use the static populateSharedInstance() to access
	 * this window.
	 *
	 * Note that this widget will destroy itself upon window close.
	 **/
	TrashWindow( QWidget * parent = nullptr );

	/**
	 * Destructor.
	 **/
	~TrashWindow() override;

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
	static TrashWindow * sharedInstance();


    public:

	/**
	 * Convenience function for creating, populating and showing the shared
	 * instance.
	 **/
	static void populateSharedInstance()
	    { sharedInstance()->populate(); }

	static QLatin1String expungedDirName() { return "qexpunged"_L1; }
	static QString expungedDirPath( const QString & trashRoot )
	    { return trashRoot % '/' % expungedDirName(); }


    protected slots:

	/**
	 * Total up the sizes of all the items and update the heading label.
	 **/
	void calculateTotalSize();

	/**
	 * Clear and re-populate the window.  This is expected to be relatively
	 * fast since all the relevant trash files will have been read
	 * previously and are likely to still be cached, so no BusyPopup is
	 * shown.
	 *
	 * To make the refresh as seemless as possible, the selected items are
	 * stored and re-applied after the items are re-populated.  The
	 * scrollbar position is also re-applied.
	 **/
	void refresh();

	/**
	 * Permanently delete the selected items.  This is done by calling each
	 * selected item to move its trash entry and trashinfo file into
	 * a "qexpunged" directory, and then deleting all "qexpunged"
	 * directories.  The directory delete operation may be relatively slow,
	 * and the tree item deletion even slower, so a BusyPopup message is
	 * shown during the entire operation.
	 **/
	void deleteSelected();

	/**
	 * Restore the selected items to their original locations.  This is
	 * done by calling each selected item to move the trash entry to its
	 * original location and remove the corresponding trashinfo file.
	 * Although the move and delete should be fast, the delete of the tree
	 * items and subsequent repaints can be relatively slow and a BusyPopup
	 * message is displayed during the entire operation.
	 **/
	void restoreSelected();

	/**
	 * Empty all Trash directories for the current user.  The method used
	 * us to move all the files and trashinfo files in each Trash directory
	 * into a "qexpunged" directory, and then delete that directory.
	 *
	 * The initial move is very fast to reduce the chances of collisions
	 * with further trash operations.  The subsequent delete may be
	 * relatively slow and a BusyPopup message is shown during the entire
	 * operation.
	 **/
	void empty();

	/**
	 * Enabled or disable actions based on the current selection.
	 **/
	void enableActions();

	/**
	 * Custom context menu signalled for the tree.
	 **/
	void contextMenu( const QPoint & pos );


    protected:

	/**
	 * Populate the window, select the first item, and resize the columns
	 * to match the data as much as possible.
	 *
	 * Note that although this operation will be relatively slow if there
	 * are many uncached trash entries, no BusyPopup is shown.  The called
	 * is expected to use a BusyPopup since the application will be blocked
	 * until the populate completes.
	 **/
	void populate();

	/**
	 * Populate the tree: locate all trash folders for the current user
	 * and list entries from those folders.
	 **/
	void populateTree();

	/**
	 * Key press event for detecting enter/return.
	 *
	 * Reimplemented from QWidget.
	 **/
	void keyPressEvent( QKeyEvent * event ) override;


    private:

	std::unique_ptr<Ui::TrashWindow> _ui;

    };	// class TrashWindow


    enum TrashCols
    {
	TW_NameCol,
	TW_SizeCol,
	TW_DeletedCol,
	TW_DirCol,
    };


    /**
     * Item class for one trash entry.  This inherits both QTreeWidgetItem
     * (which is not a QObject) and QObject so that it can send and receive
     * a QProcess::finished signal.
     **/
    class TrashItem final : public QObject, public QTreeWidgetItem
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	TrashItem( ProcessStarter * processStarter,
	           const QString  & trashDir,
	           int              filesDirFd,
	           const char     * entryName );

	/**
	 * Return a pair of values uniquely identifying this TrashDir object.
	 **/
	TrashEntry trashEntry() const { return { _trashRoot, _entryName }; }

	/**
	 * Getter for '_totalSize'.
	 **/
	FileSize totalSize() const { return _totalSize; }

	/**
	 * Permanently delete this trash item and the corresponding trashinfo
	 * file.  This TrashItem then deletes itself.  This may fail, primarily
	 * for lack of permissions to the trash directories which is a highly
	 * unexpected situation.  The only output in this case is a log
	 * message and the trash entry will not be removed from the tree.
	 *
	 * Note that the object delete is intentionally synchronous.  This
	 * function is called with a BusyPopup showing and the delete
	 * statements are often the slowest part of the operation.
	 **/
	void deleteItem();

	/**
	 * Restore this trash item to its original location.  To avoid common
	 * reasons why this might fail, the parent directory and all its
	 * ancestors are created if necessary (and possible).  If a file or
	 * directory already exists with the same name as this item, the user
	 * is queried for whether to replace it.  The restore still might fail
	 * if if there are insufficient permissions to create the parent,
	 * remove an exiting item, or create the new one. If the restore
	 * succeeds, the corresponding trashinfo file is removed and this
	 * TrashItem deletes itself.
	 *
	 * The return value is a StandardButton enum, either as provided in
	 * 'buttonResponse' or as returned from a message box when user input
	 * is requested.
	 *
	 * Note that QFile:rename() is used rather than the simpler C rename().
	 * This matches the corresponding moveToTrash function, which will
	 * copy-and-delete plain files on different filesystems.
	 *
	 * Note that the delete is intentionally synchronous.  This function is
	 * called with a BusyPopup showing and the delete statements are often
	 * the slowest part of the operation.
	 **/
	int restoreItem( bool singleItem, int buttonResponse );


    protected slots:

	/**
	 * Parse the output of a du command.  This is expected to be a single
	 * line starting with a series of plain digits representing the size in
	 * bytes.
	 *
	 * The parsed value should always be greater than zero, being either
	 * the own size of a directory (ie. 4kB) or the total size of the
	 * directory and all its contents.  A value of zero indicates a failure
	 * and is ignored.
	 **/
	void processFinished( int exitCode, QProcess::ExitStatus exitStatus );


    protected:

	/**
	 * Return the window widget that displays and owns this item.
	 **/
	QWidget * trashWindow() const { return treeWidget()->parentWidget(); }

	/**
	 * Override the model data for the tooltips of elided columns.
	 **/
	QVariant data( int column, int role ) const override;

	/**
	 * Less-than operator for sorting.
	 **/
	bool operator<( const QTreeWidgetItem & other ) const override;


    private:

	QString  _trashRoot;
	QString  _entryName;
	FileSize _totalSize{ 0 };
	time_t   _deletedMTime{ 0 };

    };	// class FilesystemItem

}	// namespace QDirStat

#endif	// TrashWindow_h
