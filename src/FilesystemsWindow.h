/*
 *   File name: FilesystemsWindow.h
 *   Summary:   QDirStat "Mounted Filesystems" window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef FilesystemsWindow_h
#define FilesystemsWindow_h

#include <memory>

#include <QDialog>
#include <QTreeWidgetItem>

#include "ui_filesystems-window.h"
#include "Typedefs.h" // FileSize


namespace QDirStat
{
    class MountPoint;

    /**
     * Modeless dialog to display details about mounted filesystems:
     *
     *	 - device
     *	 - mount point
     *	 - filesystem type
     *	 - used disk space
     *	 - free disk space for nonprivileged users
     *	 - free disk space for root
     *
     * The sizes may not be available on all platforms (no Qt 4 support!) or
     * for some filesystem types.
     **/
    class FilesystemsWindow final : public QDialog
    {
	Q_OBJECT

	/**
	 * Constructor.
	 *
	 * This widget will destroy itself upon window close.
	 *
	 **/
	FilesystemsWindow( QWidget * parent );

	/**
	 * Destructor.
	 **/
	~FilesystemsWindow() override;

	/**
	 * Static method for using one shared instance of this class between
	 * multiple parts of the application. This will create a new instance
	 * if there is none yet (or any more).
	 **/
	static FilesystemsWindow * sharedInstance( QWidget * parent );


    public:

	/**
	 * Convenience function for creating, populating and showing the shared
	 * instance.
	 **/
	static void populateSharedInstance( QWidget * parent )
	    { sharedInstance( parent )->populate(); }


    protected slots:

	/**
	 * Populate the window with all normal filesystems. Bind mounts and
	 * filesystems mounted several times.
	 **/
	void populate();

	/**
	 * Enable or disable widgets such as the "Read" button.
	 **/
	void enableActions();

	/**
	 * Notification that the "Read" button was clicked:
	 * Emit the readFilesystem() signal.
	 **/
	void readSelectedFilesystem();


    protected:

	/**
	 * Clear all data and widget contents.
	 **/
	void clear()
	    { _ui->fsTree->clear(); }

	/**
	 * Key press event for detecting evnter/return.
	 *
	 * Reimplemented from QWidget.
	 **/
	void keyPressEvent( QKeyEvent * event ) override;


    private:

	std::unique_ptr<Ui::FilesystemsWindow> _ui;

	bool _warnedAboutBtrfs{ false };

    };	// class FilesystemsWindow


    /**
     * Column numbers for the filesystems tree widget
     **/
    enum FilesystemColumns
    {
	FS_DeviceCol = 0,
	FS_TypeCol,
	FS_TotalSizeCol,
	FS_UsedSizeCol,
	FS_ReservedSizeCol,
	FS_FreeSizeCol,
	FS_FreePercentCol,
	FS_MountPathCol,
    };



    /**
     * Item class for the filesystems list.
     **/
    class FilesystemItem final : public QTreeWidgetItem
    {
    public:

	/**
	 * Constructor.
	 **/
	FilesystemItem( MountPoint * mountPoint );

	/**
	 * Getters for the column values.
	 **/
	const QString & device()         const { return _device; }
	const QString & mountPath()      const { return _mountPath; }
	const QString & fsType()         const { return _fsType; }
	FileSize        totalSize()      const { return _totalSize; }
	FileSize        usedSize()       const { return _usedSize; }
	FileSize        reservedSize()   const { return _reservedSize; }
	float           freePercent()    const { return 100.0f * _freeSize / _totalSize; }
	FileSize        freeSize()       const { return _freeSize; }
	bool            isNetworkMount() const { return _isNetworkMount; }
	bool            isReadOnly()     const { return _isReadOnly; }


    protected:

	/**
	 * Override the model data for the tooltips of elided columns.
	 **/
	QVariant data( int column, int role ) const override;

	/**
	 * Less-than operator for sorting.
	 **/
	bool operator<( const QTreeWidgetItem & rawOther ) const override;


    private:

	QString  _device;
	QString  _mountPath;
	QString  _fsType;
	FileSize _totalSize;
	FileSize _usedSize;
	FileSize _reservedSize;
	FileSize _freeSize;
	bool     _isNetworkMount;
	bool     _isReadOnly;

    };	// class FilesystemItem

}	// namespace QDirStat

#endif	// FilesystemsWindow_h
