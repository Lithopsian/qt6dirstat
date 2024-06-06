/*
 *   File name: MountPoints.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef MountPoints_h
#define MountPoints_h

#include "memory"

#include <QList>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include "Typedefs.h" // FileSize, MountPointList


#if QT_VERSION < QT_VERSION_CHECK( 5, 4, 0 )
#  define HAVE_Q_STORAGE_INFO 0
typedef void * QStorageInfo;
#else
#  define HAVE_Q_STORAGE_INFO 1
#  include <QStorageInfo>
#endif


namespace QDirStat
{

    /**
     * Helper class to represent one mount point of a Linux/Unix filesystem.
     **/
    class MountPoint
    {
    public:

	/**
	 * Constructor.
	 **/
	MountPoint( const QString & device,
		    const QString & path,
		    const QString & filesystemType,
		    const QString & mountOptions ):
	    _device { device },
	    _path { path },
	    _filesystemType { filesystemType },
	    _mountOptions { mountOptions.split( ',' ) }
	{}

	/**
	 * Return the device that is mounted, something like "/dev/sda3",
	 * "/dev/mapper/crypto", "nas:/share/work".
	 **/
	const QString & device() const { return _device; }

	/**
	 * Return the path where the device is mounted to.
	 **/
	const QString & path() const { return _path; }

	/**
	 * Return the filesystem type as string ("ext4", "btrfs", "none").
	 **/
	const QString & filesystemType() const { return _filesystemType; }

	/**
	 * Return the individual mount options as a list of strings
	 * ["rw", "nosuid", "nodev", "relatime", "rsize=32768"].
	 **/
	const QStringList & mountOptions() const { return _mountOptions; }

	/**
	 * Return the mount options as one comma-separated string.
	 **/
	QString mountOptionsStr() const { return _mountOptions.join( ',' ); }

	/**
	 * Return 'true' if the filesystem is mounted read-only.
	 **/
	bool isReadOnly() const { return _mountOptions.contains( QLatin1String( "ro" ) ); }

	/**
	 * Return 'true' if the filesystem type of this mount point is "btrfs".
	 **/
	bool isBtrfs() const { return _filesystemType.toLower() == QLatin1String( "btrfs" ); }

	/**
	 * Return 'true' if the filesystem type of this mount point starts with
	 * "ntfs".
	 **/
        bool isNtfs() const { return _filesystemType.toLower().startsWith( QLatin1String( "ntfs" ) ); }

	/**
	 * Return 'true' if this is a network filesystem like NFS or Samba
	 * (cifs).
	 **/
	bool isNetworkMount() const;

	/**
	 * Return 'true' if this is a system mount, i.e. one of the known
	 * system mount points like /dev, /proc, /sys, or if the device name
	 * does not start with a slash (e.g. cgroup, tmpfs, sysfs, ...)
	 **/
	bool isSystemMount() const;

        /**
         * Return 'true' if this is an autofs, i.e. a filesystem managed by the
         * automounter.
         **/
        bool isAutofs() const { return _filesystemType.toLower() == QLatin1String( "autofs" ); }

        /**
         * Return 'true' if this is an autofs that is not currently mounted.
         **/
        bool isUnmountedAutofs() { return isAutofs() && totalSize() == 0; }

	/**
	 * Return 'true' if this is a duplicate mount, i.e. either a bind mount
	 * or a filesystem that was mounted multiple times.
	 **/
	bool isDuplicate() const { return _isDuplicate; }

        /**
         * Return 'true' if this is a snap package, i.e. it is a squashfs
         * mounted below /snap.
         **/
        bool isSnapPackage() const
	    { return _path.startsWith( QLatin1String( "/snap" ) ) &&
		     _filesystemType.toLower() == QLatin1String( "squashfs" ); }

	/**
	 * Set the 'duplicate' flag. This should only be set while /proc/mounts
	 * or /etc/mtab is being read.
	 **/
	void setDuplicate( bool dup = true ) { _isDuplicate = dup; }

	/**
	 * Return 'true' if size information for this mount point is available.
	 * This may depend on the build OS and the Qt version.
	 **/
#if HAVE_Q_STORAGE_INFO
	bool hasSizeInfo() const { return true; }
#else
	bool hasSizeInfo() const { return false; }
#endif

#if HAVE_Q_STORAGE_INFO
	/**
	 * Total size of the filesystem of this mount point.
	 * This returns -1 if no size information is available.
	 **/
	FileSize totalSize()
	    { return storageInfo().bytesTotal(); }

	/**
	 * Total used size of the filesystem of this mount point.
	 * This returns -1 if no size information is available.
	 **/
	FileSize usedSize()
	    { return storageInfo().bytesTotal() - storageInfo().bytesFree(); }

	/**
	 * Reserved size for root for the filesystem of this mount point.
	 * This returns -1 if no size information is available.
	 **/
	FileSize reservedSize()
	    { return storageInfo().bytesFree() - storageInfo().bytesAvailable(); }

	/**
	 * Available free size of this filesystem for non-privileged users.
	 * This returns -1 if no size information is available.
	 **/
	FileSize freeSizeForUser()
	    { return storageInfo().bytesAvailable(); }

	/**
	 * Available free size of this filesystem for privileged users.
	 * This returns -1 if no size information is available.
	 **/
	FileSize freeSizeForRoot()
	    { return storageInfo().bytesFree(); }

#else
	/**
	 *  Qt before 5.4 does not have QStorageInfo, and statfs()
	 * is Linux-specific (not POSIX).
	 **/
	FileSize MountPoint::totalSize()       { return -1; }
	FileSize MountPoint::usedSize()        { return -1; }
	FileSize MountPoint::reservedSize()    { return -1; }
	FileSize MountPoint::freeSizeForUser() { return -1; }
	FileSize MountPoint::freeSizeForRoot() { return -1; }
#endif

    private:

#if HAVE_Q_STORAGE_INFO
	/**
	 * Lazy access to the QStorageInfo for this mount.  Returns a reference to
	 * the QStorageInfo object.
	 **/
	const QStorageInfo & storageInfo();


	std::unique_ptr<const QStorageInfo> _storageInfo;
#endif

	QString     _device;
	QString     _path;
	QString     _filesystemType;
	QStringList _mountOptions;
	bool        _isDuplicate { false };

    }; // class MountPoint


    /**
     * Singleton class to access the current mount points.
     **/
    class MountPoints
    {
	/**
	 * Constructor. Not for public use. Use the static methods instead.
	 **/
	MountPoints();

	/**
	 * Destructor.
	 **/
	~MountPoints();

	/**
	 * Suppress copy and assignment constructors (this is a singleton)
	 **/
	MountPoints( const MountPoints & ) = delete;
	MountPoints & operator=( const MountPoints & ) = delete;

	/**
	 * Return the singleton object for this class. The first use will
	 * create the singleton. Most of the static methods access
	 * the singleton, so the first call to those static
	 * methods will create the singleton.
	 **/
	static MountPoints * instance();

	/**
	 * Initialise the member variables.
	 **/
	void init();

	/**
	 * Clear the data structures used internally.
	 **/
	void clear();


    public:

	/**
	 * Return the mount point for 'path' if there is one or 0 if there is
	 * not. Ownership of the returned object is not transferred to the
	 * caller, i.e. the caller should not delete it. The pointer remains
	 * valid until the next call to clear().
	 **/
	static const MountPoint * findByPath( const QString & path );

	/**
	 * Find the nearest mount point upwards in the directory hierarchy
	 * starting from 'path'. 'path' itself might be that mount point.
	 * Ownership of the returned object is not transferred to the caller.
	 *
	 * This might return 0 if none of the files containing mount
	 * information (/proc/mounts, /etc/mtab) could be read.
	 **/
	static const MountPoint * findNearestMountPoint( const QString & path );

	/**
	 * Return the device name where 'dir' is on if it's a mount point.
	 * This uses MountPoints which reads /proc/mounts.
	 **/
	static QString device( const QString & url );

	/**
	 * Return whether the givfe url corresponds to a duplicate mount point.
	 **/
	static bool isDuplicate( const QString & url );

	/**
	 * Return 'true' if any mount point has filesystem type "btrfs".
	 **/
	static bool hasBtrfs();

	/**
	 * Return a list of "normal" mount points, i.e. those that are not
	 * system mounts, bind mounts or duplicate mounts.
	 *
	 * The result is sorted by the order in which the filesystems were
	 * mounted (the same as in /proc/mounts or in /etc/mtab).
	 **/
	static MountPointList normalMountPoints();

	/**
	 * Return a list of "normal" mount points, i.e. those that are not
	 * system mounts, bind mounts or duplicate mounts.
	 *
	 * The result is sorted by the order in which the filesystems were
	 * mounted (the same as in /proc/mounts or in /etc/mtab).
	 **/
	static const MountPointList & allMountPoints() { return instance()->_mountPointList; }

	/**
	 * Return 'true' if size information for mount points is available.
	 * This may depend on the build OS and the Qt version.
	 **/
#if HAVE_Q_STORAGE_INFO
	static bool hasSizeInfo() { return true; }
#else
	static bool hasSizeInfo() { return false; }
#endif

	/**
	 * Clear all information and reload it from disk.
	 * NOTICE: This invalidates ALL MountPoint pointers!
	 **/
	static void reload() { instance()->init(); instance()->ensurePopulated(); }


    protected:

	/**
	 * Ensure the mount points are populated with the content of
	 * /proc/mounts, falling back to /etc/mtab if /proc/mounts cannot be
	 * read.
	 **/
	void ensurePopulated();

	/**
	 * Read 'filename' (in /proc/mounts or /etc/mnt syntax) and populate
	 * the mount points with the content. Return 'true' on success, 'false'
	 * on failure.
	 **/
	bool read( const QString & filename );

#if HAVE_Q_STORAGE_INFO
	/**
	 * Fallback method if neither /proc/mounts nor /etc/mtab is available:
	 * Try using QStorageInfo. Return 'true' if any mount point was found.
	 **/
	bool readStorageInfo();
#endif

	/**
	 * Post-process a mount point and add it to the internal list and map.
	 **/
	void postProcess( MountPoint * mountPoint );

	/**
	 * Add a mount point to the internal list and map.
	 **/
	void add( MountPoint * mountPoint );

	/**
	 * Check if any of the mount points has filesystem type "btrfs".
	 **/
	bool checkForBtrfs();

	/**
	 * Try to check with the external "lsblk" command (if available) what
	 * block devices use NTFS and populate _ntfsDevices with them.
	 **/
	void findNtfsDevices();

	/**
	 * Return 'true' if 'device' is mounted.
	 **/
	bool isDeviceMounted( const QString & device ) const;

	/**
	 * Dump all current mount points to the log. This does not call
	 * ensurePopulated() first.
	 **/
//	static void dump();


    private:

	//
	// Data members
	//

	MountPointList                     _mountPointList;
	QHash<QString, const MountPoint *> _mountPointMap;
	QStringList                        _ntfsDevices;
	bool                               _isPopulated;
	bool                               _hasBtrfs;
	bool                               _checkedForBtrfs;

    }; // class MountPoints


    inline QTextStream & operator<< ( QTextStream & stream, MountPoint * mp )
    {
	if ( mp )
	{
	    stream << "<mount point for " << mp->device()
		   << " at " << mp->path()
		   << " type " << mp->filesystemType()
		   << ">";
	}
	else
	    stream << "<NULL MountPoint*>";

	return stream;
    }

} // namespace QDirStat


#endif // MountPoints_h
