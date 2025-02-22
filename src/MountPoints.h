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

#include <memory>

#include <QStringList>
#include <QTextStream>

#if QT_VERSION < QT_VERSION_CHECK( 5, 4, 0 )
#  define HAVE_Q_STORAGE_INFO 0
#else
#  define HAVE_Q_STORAGE_INFO 1
#  include <QStorageInfo>
#endif

#include "Typedefs.h" // FileSize, _L1


namespace QDirStat
{
    /**
     * Helper class to represent one mount point of a Linux/Unix filesystem.
     **/
    class MountPoint final
    {
    public:

	/**
	 * Constructor.
	 **/
	MountPoint( const QString & device,
	            const QString & path,
	            const QString & filesystemType,
	            const QString & mountOptions ):
	    _device{ device },
	    _path{ path },
	    _filesystemType{ filesystemType },
	    _mountOptions{ mountOptions.split( ',' ) }
	{}

	/**
	 * Return the device that is mounted, something like "/dev/sda3",
	 * "/dev/mapper/crypto", "nas:/share/work".
	 **/
	const QString & device() const { return _device; }

	/**
	 * Return the path the device is mounted at.
	 **/
	const QString & path() const { return _path; }

	/**
	 * Get or set the filesystem type as a string (eg. "ext4", "btrfs",
	 * or "none").
	 **/
	const QString & filesystemType() const { return _filesystemType; }
	void setFilesystemType( const QString & fsType ) { _filesystemType = fsType; }

	/**
	 * Return the individual mount options as a list of strings (eg.
	 * { "rw", "nosuid", "nodev", "relatime", "rsize=32768" }.
	 **/
	const QStringList & mountOptions() const { return _mountOptions; }

	/**
	 * Return the mount options as one comma-separated string.  (eg.
	 * "rw, nosuid, nodev, relatime, rsize=32768").
	 **/
//	QString mountOptionsStr() const { return _mountOptions.join( u',' ); }

	/**
	 * Return 'true' if the filesystem is a "normal mount point: that
	 * is, not a system mount, duplicate mount, autofs mount, or Snap
	 * package.
	 **/
	bool isNormalMountPoint()
	    { return !isSystemMount() && !isDuplicate() && !isUnmountedAutofs() && !isSnapPackage(); }

	/**
	 * Return 'true' if the filesystem is mounted read-only.
	 **/
	bool isReadOnly() const { return _mountOptions.contains( "ro"_L1 ); }

	/**
	 * Return 'true' if the filesystem type of this mount point is "btrfs".
	 **/
	bool isBtrfs() const { return _filesystemType.toLower() == "btrfs"_L1; }

	/**
	 * Return 'true' if the filesystem type of this mount point starts with
	 * "ntfs".
	 **/
	bool isNtfs() const { return _filesystemType.toLower().startsWith( "ntfs"_L1 ); }

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
	bool isAutofs() const { return _filesystemType.toLower() == "autofs"_L1; }

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
	    { return _path.startsWith( "/snap"_L1 ) && _filesystemType.toLower() == "squashfs"_L1; }

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
	 * Qt before 5.4 does not have QStorageInfo, and statfs()
	 * is Linux-specific (not POSIX).
	 **/
	FileSize totalSize()       { return -1LL; }
	FileSize usedSize()        { return -1LL; }
	FileSize reservedSize()    { return -1LL; }
	FileSize freeSizeForUser() { return -1LL; }
	FileSize freeSizeForRoot() { return -1LL; }
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
	bool        _isDuplicate{ false };

    };	// class MountPoint


    typedef QMap<QString, MountPoint *> MountPointMap; // QMap so the mountpoints are ordered


    /**
     * Singleton class to access the current mount points.  Access through
     * the public static methods.
     *
     * The class is instantiated when it is first accessed and the map
     * of mount points is populated.
     **/
    class MountPoints final : public MountPointMap
    {
	/**
	 * Constructor. Not for public use. Use the static methods instead.
	 **/
	MountPoints()
	    { populate(); }

	/**
	 * Destructor.
	 **/
	~MountPoints()
	    { clear(); }

	/**
	 * Suppress copy and assignment constructors (this is a singleton)
	 **/
	MountPoints( const MountPoints & ) = delete;
	MountPoints & operator=( const MountPoints & ) = delete;

	/**
	 * Return the singleton object for this class. The first use will
	 * create the singleton. Most of the static methods access the
	 * singleton, so the first call to those static methods will
	 * create the singleton.
	 **/
	static MountPoints * instance();

	/**
	 * Initialise the member variables and mount point map.
	 **/
	void init()
	    { clear(); populate(); }

	/**
	 * Clear the map and delete all the mountpoints.
	 **/
	void clear()
	    { qDeleteAll( *this ); MountPointMap::clear(); }


    public:

	/**
	 * Return the mount point for 'path' if there is one or 0 if there is
	 * not. Ownership of the returned object is not transferred to the
	 * caller, i.e. the caller should not delete it. The pointer remains
	 * valid until the next call to clear().
	 **/
	static const MountPoint * findByPath( const QString & path )
	    { return instance()->value( path, nullptr ); }

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
	 * Return 'true' if any mount point has filesystem type starting with
	 * "ntfs".
	 **/
	static bool hasNtfs()
	    { return instance()->_hasNtfs; };

	/**
	 * Return begin and end iterators for the mount point map.
	 **/
	static MountPointMap::const_iterator cbegin() { return instance()->MountPointMap::cbegin(); }
	static MountPointMap::const_iterator cend()   { return instance()->MountPointMap::cend();   }

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
	 *
	 * This invalidates ALL MountPoint pointers!
	 **/
	static void reload()
	    { instance()->init(); }


    protected:

	/**
	 * Ensure the mount points are populated with the content of
	 * /proc/mounts, falling back to /etc/mtab if /proc/mounts cannot be
	 * read.
	 **/
	void populate();

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
	void readStorageInfo();
#endif

	/**
	 * Add a mount point to the map.
	 **/
	void add( MountPoint * mountPoint )
	    { insert( mountPoint->path(), mountPoint ); }


    private:

	bool _hasNtfs;

    };	// class MountPoints



    /**
     * Iterator for MountPoints.  The constructor accepts a flag which
     * indicates whether to iterator through all mount points, or just
     * "normal" ones.
     *
     * Note that this iterator follows the standard QDirStat format,
     * but is not STL-compliant and can't be used in range for loops
     * or algorithms, not least because the sinlgeton object is not
     * public.
     **/
    class MountPointIterator final
    {
    public:

	MountPointIterator( bool all, bool duplicates = false ):
	    _all{ all },
	    _duplicates{ duplicates },
	    _end{ MountPoints::cend() },
	    _current{ find( MountPoints::cbegin() ) }
	{}

	MountPoint * operator*() const { return _current == _end ? nullptr : _current.value(); }
	MountPoint * operator->() const { return _current.value(); }

	MountPointIterator & operator++() { _current = find( ++_current ); return *this; }
	MountPointIterator operator++(int) { auto tmp = *this; operator++(); return tmp; }


    protected:

	/**
	 * Find the next mount point that is "normal", starting
	 * from 'item', or any mount point if '_all' is true,
	 * or a duplicate mount point if '_duplicates' is true.
	 *
	 * _all includes system mounts, duplicate mounts, unmounted autofs
	 * mount-points, and snap packages.
	 * _duplicates can be used when _all is false but it is desired to
	 * include bind mounts, which Trash needs.
	 **/
	MountPointMap::const_iterator find( MountPointMap::const_iterator current )
	{
	    while ( current != _end &&
	            !_all && !current.value()->isNormalMountPoint() &&
		    ( !_duplicates || !current.value()->isDuplicate() ) )
		++current;
	    return current;
	}


    private:

	bool _all;
	bool _duplicates;

	MountPointMap::const_iterator _end;
	MountPointMap::const_iterator _current;

    };	// class MountPointIterator



    inline QTextStream & operator<<( QTextStream & stream, MountPoint * mp )
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

}	// namespace QDirStat

#endif	// MountPoints_h
