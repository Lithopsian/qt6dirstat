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
	 * Clear and re-populate the window.
	 *
	 * To make the refresh as seemless as possible, the selected items are
	 * stored and re-applied after the items are re-populated.  The
	 * scrollbar position is also re-applied.
	 **/
	void refresh();

	/**
	 * Permanently delete the selected items.
	 **/
	void deleteSelected();

	/**
	 * Restore the selected items.
	 **/
	void restoreSelected();

	/**
	 * Empty all Trash directories for the current user.  The method used
	 * us to move all the files and trashinfo files in each Trash directory
	 * into a "qexpunged" directory, and then delete that directory.  The
	 * initial move should be very fast to reduce the chances of collisions
	 * with further trash operations.  The subsequent delete may be rather
	 * slow.
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
     * Item class for the deleted items list.
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

	void processFinished( int exitCode, QProcess::ExitStatus exitStatus );

	/**
	 * Permanently delete this trash item and the corresponding trashinfo
	 * file.  This TrashItem then deletes itself.
	 *
	 * Note that the object delete is intentionally synchronous.  This
	 * function is  called with a BusyPopup showing and the delete
	 * statements are often the slowest part of the delete.
	 **/
	void deleteItem();

	/**
	 * Restore this trash item to its original location.  This will fail if
	 * an item already exists with the same name, if the original directory
	 * no longer exists, or if the current user has insufficient
	 * permissions.  If the restore succeeds, the corresponding trashinfo
	 * file is removed and this TrashItem deletes itself.
	 *
	 * Note that QFile:rename() is used rather than the simpler C rename().
	 * This matches the corresponding moveToTrash function, which will
	 * copy-and-delete plain files on different filesystems.  Importantly,
	 * it will also fail if the restored file already exists.
	 *
	 * Note that the delete is intentionally synchronous.  This function is
	 * called with a BusyPopup showing and the delete statements are often
	 * the slowest part of the delete.
	 **/
	int restoreItem( bool singleItem, int buttonResponse );


    protected:

	/**
	 * Override the model data, just for the tooltip for the path
	 * column.
	 **/
	QWidget * trashWindow() const { return treeWidget()->parentWidget(); }

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

	QString  _trashRoot;
	QString  _entryName;
	FileSize _totalSize{ 0 };
	time_t   _deletedMTime{ 0 };

    };	// class FilesystemItem

}	// namespace QDirStat

#endif	// TrashWindow_h
