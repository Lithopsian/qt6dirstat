/*
 *   File name: FileInfo.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef FileInfo_h
#define FileInfo_h

#include <sys/types.h>
#include <sys/stat.h>

#include <QFileInfo>
#include <QModelIndex>
#include <QTextStream>
#include <QtMath> // qCeil()

#include "Typedefs.h" // FileSize


#define FileInfoMagic 4242


namespace QDirStat
{
    class Attic;
    class DirInfo;
    class DirTree;
    class DotEntry;
    class PkgInfo;

    /**
     * Status of a directory read job.
     **/
    enum DirReadState
    {
	DirQueued,		// Waiting in the directory read queue
	DirReading,		// Reading in progress
	DirFinished,		// Reading finished and OK
	DirOnRequestOnly,	// Will be read upon explicit request only (mount points)
//	DirCached,		// Content was read from a cache, obsolete
	DirAborted,		// Reading aborted upon user request
	DirPermissionDenied,	// Insufficient permissions for reading
	DirError		// Error while reading
    };

    /**
     * The most basic building block of a DirTree:
     *
     * Information about one single directory entry. This is the type of info
     * typically obtained by stat() / lstat() or similar calls.
     *
     * This class is tuned for size rather than speed: A typical Linux system
     * easily has 150,000+ filesystem objects, and at least one entry of this
     * sort is required for each of them.
     *
     * This class provides stubs for children management; those stubs are all
     * default implementations that don't really deal with children. Derived
     * classes need to implement real methods for those cases.
     **/
    class FileInfo
    {
    public:

	/**
	 * Constructor from raw data values.  Used by the cache reader and as
	 * a delegate by some other constructors.
	 **/
	FileInfo( DirInfo       * parent,
		  DirTree       * tree,
		  const QString & filename,
		  mode_t          mode,
		  FileSize        size,
		  FileSize        allocatedSize,
		  bool            withUidGidPerm,
		  uid_t           uid,
		  gid_t           gid,
		  time_t          mtime,
		  bool            isSparseFile,
		  FileSize        blocks,
		  nlink_t         links ):
	    _name { filename },
	    _parent { parent },
	    _tree { tree },
	    _isLocalFile { true },
	    _isSparseFile { isSparseFile },
	    _isIgnored { false },
	    _hasUidGidPerm { withUidGidPerm },
	    _device { 0 },
	    _mode { mode },
	    _links { links },
	    _uid { uid },
	    _gid { gid },
	    _size { size },
	    _blocks { blocks },
	    _allocatedSize { allocatedSize },
	    _mtime { mtime }
	{}

	/**
	 * Constructor from the bare necessary fields.  This is used by the
	 * Mime categorizer config page to create dummy entries in an example tree.
	 *
	 **/
	FileInfo( DirInfo       * parent,
		  DirTree       * tree,
		  const QString & filename,
		  mode_t          mode,
		  FileSize        size ):
	    FileInfo ( parent, tree, filename, mode, size, size, false, 0, 0, 0, false, blocksFromSize( size ), 1 )
	{}

	/**
	 * Constructor from a more complete set of raw data.  Used to create FileInfo
	 * objects from the DirInfo constructor.
	 *
	 **/
	FileInfo( DirInfo       * parent,
		  DirTree       * tree,
		  const QString & filename,
		  mode_t          mode,
		  FileSize        size,
		  FileSize        allocatedSize,
		  bool            withUidGidPerm,
		  uid_t           uid,
		  gid_t           gid,
		  time_t          mtime ):
	    FileInfo ( parent,
		       tree,
		       filename,
		       mode,
		       size,
		       allocatedSize,
		       withUidGidPerm,
		       uid,
		       gid,
		       mtime,
		       false,
		       blocksFromSize( size ),
		       1 )
	{}

	/**
	 * Constructor from just the parent, tree, and name.  This is used to create
	 * PkgInfo and pseudo-directories and to create dummy nodes in some error
	 * situations.
	 **/
	FileInfo( DirInfo       * parent,
		  DirTree       * tree,
		  const QString & filename ):
	    FileInfo ( parent, tree, filename, 0, 0, 0, false, 0, 0, 0, false, 0, 1 )
	{}

	/**
	 * Constructor from a stat buffer (i.e. based on an lstat() call).  It is
	 * expected that this will be used for all "real" files.
	 **/
	FileInfo( DirInfo           * parent,
		  DirTree           * tree,
		  const QString     & filename,
		  const struct stat & statInfo );

	/**
	 * Suppress copy and assignment constructors (this is not a QObject)
	 **/
	FileInfo( const FileInfo & ) = delete;
	FileInfo & operator=( const FileInfo & ) = delete;

	/**
	 * Destructor.
	 *
	 * The destructor should also take care of unlinking this object from
	 * its parent's children list, but regrettably that just doesn't work: At
	 * this point (within the destructor) parts of the object are already
	 * destroyed, e.g., the virtual table - virtual methods don't work any
	 * more. Thus, somebody from outside must call unlinkChild() just prior
	 * to the actual "delete".
	 **/
	virtual ~FileInfo();

	/**
	 * Check with the magic number if this object is valid.
	 * Return 'true' if it is valid, 'false' if invalid.
	 *
	 * Notice that this is intentionally not a virtual function to avoid
	 * a segfault via the vptr if it is not valid.
	 **/
	bool checkMagicNumber() const { return _magic == FileInfoMagic; }

	/**
	 * Returns whether or not this is a local file (protocol "file:").
	 * It might as well be a remote file ("ftp:", "smb:" etc.).
	 **/
	bool isLocalFile() const { return _isLocalFile; }

	/**
	 * Returns the file or directory name without path, i.e. only the last
	 * path name component (i.e. "printcap" rather than "/etc/printcap").
	 *
	 * If a directory scan doesn't begin at the root directory and this is
	 * the top entry of this directory scan, it will also contain the base
	 * path, i.e. "/usr/share/man" rather than just "man" if a scan was
	 * requested for "/usr/share/man". Notice, however, that the entry for
	 * "/usr/share/man/man1" will only return "man1" in this example.
	 **/
	const QString & name() const { return _name; }

	/**
	 * Set the (display) name for this object.
	 *
	 * This is useful if a package is installed in multiple versions or
	 * for multiple architectures; in that case, it is advisable to use the
	 * base name plus either the version or the architecture or both.
	 **/
	void setName( const QString & newName ) { _name = newName; }

	/**
	 * Returns the base name of this object, i.e. the last path component,
	 * even if this is a toplevel item.
	 **/
	QString baseName() const;

	/**
	 * Returns the full URL of this object with full path.
	 *
	 * This is a (somewhat) expensive operation since it will recurse up
	 * to the top of the tree.
	 **/
	virtual QString url() const;

	/**
	 * Returns the full path of this object. Unlike url(), this never has a
	 * protocol prefix or a part that identifies the package this belongs
	 * to. This is the path that can be used to find this object in the
	 * filesystem.
	 *
	 * url()  might return	"Pkg:/chromium-browser/usr/lib/chromium/foo.z"
	 * path() returns just	"/usr/lib/chromium/foo.z"
	 *
	 * Like url(), this is somewhat expensive since it recurses up the
	 * tree, but it stops when a PkgInfo node is found there.
	 **/
	QString path() const;

	/**
	 * Very much like FileInfo::url(), but with "/<Files>" appended if this
	 * is a dot entry. Useful for debugging.
	 *
	 * Note that the QTextStream operator<< outputs exactly this.
	 **/
	QString debugUrl() const;

	/**
	 * Returns the major and minor device numbers of the device this file
	 * resides on or 0 if this is a remote file (or a "simulated" FileInfo
	 * object such as from a cache read).
	 **/
	dev_t device() const { return _device; }

	/**
	 * Return the row number for this item within its parent's sorted
	 * children.  Note that it may not be valid if the children haven't
	 * been sorted yet or if the sort order is obsolete.
	 **/
	int rowNumber() const { return _rowNumber; }

	/**
	 * Return the row number for this item within its parent's sorted
	 * children.  Note that it may not be valid if the children haven't
	 * been sorted yet or if the sort order is obsolete.
	 **/
	void setRowNumber( int rowNumber ) { _rowNumber = rowNumber; }

	/**
	 * The file permissions and object type as returned by lstat().
	 * You might want to use the respective convenience methods instead:
	 * isDir(), isFile(), ...
	 *
	 * See also symbolicPermissions(), octalPermissions()
	 **/
	mode_t mode() const { return _mode; }

	/**
	 * The number of hard links to this file. Relevant for size summaries
	 * to avoid counting one file several times.
	 **/
	virtual nlink_t links() const { return _links; }

	/**
	 * User ID of the owner.
	 *
	 * Notice that this might be undefined (zero will be stored, but it
	 * it doesn't mean 'root') if this tree branch was read
	 * from an old-format cache file. Check that with hasUid().
	 **/
	uid_t uid() const { return _uid; }

	/**
	 * Return the user name of the owner.
	 *
	 * If this tree branch was read from an old-format cache file, this
	 * returns an empty string.
	 **/
	QString userName() const;

	/**
	 * Return 'true' if this FileInfo has a UID (user ID).
	 **/
	bool hasUid() const { return _hasUidGidPerm; }

	/**
	 * Group ID of the owner.
	 *
	 * Notice that this might be undefined (zero will be stored, but it
	 * it doesn't mean 'root') if this tree branch was read
	 * from an old-format cache file. Check that with hasGid().
	 **/
	gid_t gid() const { return _gid; }

	/**
	 * Return the group name of the owner.
	 *
	 * If this tree branch was read from an old-format cache file, this
	 * returns an empty string.
	 **/
	QString groupName() const;

	/**
	 * Return 'true' if this FileInfo has a GID (group ID).
	 **/
	bool hasGid() const { return _hasUidGidPerm; }

	/**
	 * Return 'true' if this FileInfo has valid permissions in the mode.
	 **/
	bool hasPerm() const { return _hasUidGidPerm; }

	/**
	 * File permissions formatted like in "ls -l", i.e. "-rwxrwxrwx",
	 * "drwxrwxrwx"
	 **/
	QString symbolicPermissions() const;

	/**
	 * File permissions formatted as octal number (like used by the "chmod"
	 * command, i.e. "0644"
	 **/
	QString octalPermissions() const;

	/**
	 * The file size, taking into account multiple links for plain files or
	 * the true allocated size for sparse files. For plain files with
	 * multiple links this will be size/no_links, for sparse files it is
	 * the number of bytes actually allocated.
	 **/
	FileSize size() const;

	/**
	 * The number of blocks, calculated from the (usually allocated) size of
	 * the file.  Some file systems may not report allocations in complete
	 * blocks, so round up to the number of blocks required to hold the
	 * allocation.
	 *
	 **/
	static int blocksFromSize( FileSize allocatedSize )
	    { return qCeil( (float)allocatedSize / STD_BLOCK_SIZE ); }

	/**
	 * The file size in bytes without taking multiple hard links into
	 * account.
	 **/
	FileSize rawByteSize() const { return _size; }

	/**
	 * The number of bytes actually allocated on the filesystem, taking
	 * multiple hard links (for plain files) into account.
	 *
	 * Usually this will be more than size() since the last few bytes of a
	 * file usually consume an additional cluster on the filesystem.
	 *
	 * In the case of sparse files, however, this might as well be
	 * considerably less than size() - this means that this file has
	 * "holes", i.e. large portions filled with zeros. This is typical for
	 * large core dumps for example. The only way to create such a file is
	 * to lseek() far ahead of the previous file size and then writing
	 * data. Most filesystem utilities disregard the fact that files are
	 * sparse files and simply allocate the holes as well, thus greatly
	 * increasing the disk space consumption of such a file. Only a few
	 * filesystem utilities like "cp", "rsync", "tar" have options to handle
	 * this more graciously - and usually only when specifically requested.
	 **/
	FileSize allocatedSize() const;

	/**
	 * The ratio of size() / allocatedSize() in percent.
	 **/
	int usedPercent() const
	    { return _allocatedSize > 0 && _size > 0 ? qRound( 100.0 * size() / allocatedSize() ) : 100; }

	/**
	 * The allocated size without taking multiple hard links into account.
	 *
	 * If the filesystem can properly report the number of disk blocks
	 * used, this is the same as blocks() * 512.
	 **/
	FileSize rawAllocatedSize() const { return _allocatedSize; }

	/**
	 * The file size in 512 byte blocks.
	 **/
	FileSize blocks() const { return _blocks; }

	/**
	 * The modification time of the file (not the inode).
	 **/
	time_t mtime() const { return _mtime; }

	/**
	 * Returns the year and month derived from the file mtime.
	 **/
	 QPair<short, short> yearAndMonth() const;

	//
	// Directory-related methods that should be implemented by
	// derived classes.
	//

	/**
	 * Returns the total size in bytes of this subtree.
	 *
	 * Derived classes that have children should overwrite this.
	 **/
	virtual FileSize totalSize() { return size(); }

	/**
	 * Returns the total allocated size in bytes of this subtree.
	 *
	 * Derived classes that have children should overwrite this.
	 **/
	virtual FileSize totalAllocatedSize() { return allocatedSize(); }

	/**
	 * Returns the total size in blocks of this subtree.
	 *
	 * Derived classes that have children should overwrite this.
	 **/
	virtual FileSize totalBlocks() { return _blocks; }

	/**
	 * Returns the total number of children in this subtree, excluding this
	 * item.
	 *
	 * Derived classes that have children should overwrite this.
	 **/
	virtual int totalItems() { return 0; }

	/**
	 * Returns the total number of subdirectories in this subtree,
	 * excluding this item. Dot entries and "." or ".." are not counted.
	 *
	 * Derived classes that have children should overwrite this.
	 **/
	virtual int totalSubDirs() { return 0; }

	/**
	 * Returns the total number of plain file children in this subtree,
	 * excluding this item.
	 *
	 * Derived classes that have children should overwrite this.
	 **/
	virtual int totalFiles() { return 0; }

	/**
	 * Returns the total number of non-directory items in this subtree,
	 * excluding this item.
	 *
	 * Derived classes that have children should overwrite this.
	 **/
//	virtual int totalNonDirItems() { return 0; }

	/**
	 * Returns the total number of ignored (non-directory!) items in this
	 * subtree, excluding this item.
	 *
	 * Derived classes that have children should overwrite this.
	 **/
	virtual int totalIgnoredItems() { return 0; }

	/**
	 * Returns the total number of not ignored (non-directory!) items in
	 * this subtree, excluding this item.
	 *
	 * Derived classes that have children should overwrite this.
	 **/
	virtual int totalUnignoredItems() { return 0; }

	/**
	 * Returns the total number of direct children of this item.
	 *
	 * Derived classes that have children should overwrite this.
	 **/
	virtual int directChildrenCount() { return 0; }

	/**
	 * Returns the number of subdirectories below this item that could not
	 * be read (typically due to insufficient permissions).
	 *
	 * Notice that this does NOT include this item if it is a directory
	 * that could not be read.
	 *
	 * Derived classes that have children should overwrite this.
	 **/
	virtual int errSubDirCount() { return 0; }

	/**
	 * Returns the latest modification time of this subtree.
	 *
	 * Derived classes that have children should overwrite this.
	 **/
	virtual time_t latestMtime() { return _mtime; }

	/**
	 * Returns the oldest modification time of any file in this subtree or
	 * 0 if there is no file.
	 *
	 * Derived classes that have children should overwrite this.
	 **/
	virtual time_t oldestFileMtime() { return isFile() ? _mtime : 0; }

	/**
	 * Return the percentage of this subtree in regard to its parent
	 * (0.0..100.0). Return a negative value if for any reason this cannot
	 * be calculated or it would not make any sense.
	 **/
	float subtreePercent();

	/**
	 * Return the percentage of this subtree's allocated size in regard to
	 * its parent's allocated size.  (0.0..100.0). Return a negative value
	 * if for any reason this cannot be calculated or it would not make any
	 * sense.
	 **/
	float subtreeAllocatedPercent();

	/**
	 * Returns 'true' if this had been excluded while reading.
	 *
	 * Derived classes may want to overwrite this.
	 **/
	virtual bool isExcluded() const { return false; }

	/**
	 * Set the 'excluded' status.
	 *
	 * Only DirInfo objects are excluded, so the default implementation
	 * silently ignores the value passed here and does nothing.
	 *
	 * Derived classes may want to overwrite this.
	 **/
	virtual void setExcluded( bool ) {}

	/**
	 * Returns whether or not this is a mount point.  Always false for a
	 * file.
	 *
	 * Derived classes may want to overwrite this.
	 **/
	virtual bool isMountPoint() const  { return false; }

	/**
	 * Sets the mount point state, i.e. whether or not this is a mount
	 * point.
	 *
	 * This default implementation silently ignores the value passed and
	 * does nothing.
	 *
	 * Derived classes may want to overwrite this.
	 **/
	virtual void setMountPoint( bool ) {}

	/**
	 * Returns true if this subtree is finished reading.  Files have no
	 * children and are always finished as soon as they are constructed,
	 * so the default implementation always returns 'true'; derived
	 * classes should overwrite this.
	 **/
	virtual bool isFinished() const { return true; }

	/**
	 * Returns true if this subtree is busy, i.e. it is not finished
	 * reading yet.
	 *
	 * This default implementation always returns 'false';
	 * derived classes should overwrite this.
	 **/
	virtual bool isBusy() const { return false; }

	/**
	 * Returns the number of pending read jobs in this subtree. When this
	 * number reaches zero, the entire subtree is done.
	 *
	 * Derived classes that have children should overwrite this.
	 **/
	virtual int pendingReadJobs() const { return 0; }

	/**
	 * Return 'true' if the filesystem can report block sizes.
	 *
	 * This is determined heuristically from the nearest DirInfo parent: If
	 * it has blocks() > 0 and size() > 0, we can safely assume that the
	 * filesystem does report the number of blocks.
	 **/
	bool filesystemCanReportBlocks() const;

	/**
	 * Return 'true' if this is a dominant item among its siblings, i.e. if
	 * its total size is much larger than the other items on the same level.
	 *
	 * This forwards the query to the parent, if there is one.
	 **/
	bool isDominant();

 	//
	// Tree management
	//

	/**
	 * Returns a pointer to the DirTree this entry belongs to.
	 **/
	DirTree * tree() const { return _tree; }

	/**
	 * Set the parent DirTree for this object.
	 **/
	void setTree( DirTree * tree ) { _tree = tree; }

	/**
	 * Returns a pointer to this entry's parent entry or 0 if there is
	 * none.
	 **/
	DirInfo * parent() const { return _parent; }

	/**
	 * Set the "parent" pointer.
	 **/
	void setParent( DirInfo * newParent ) { _parent = newParent; }

	/**
	 * Returns a pointer to the next entry on the same level, or 0 if
	 * if there is none.
	 **/
	FileInfo * next() const { return _next; }

	/**
	 * Set the "next" pointer.
	 **/
	void setNext( FileInfo * newNext ) { _next = newNext; }

	/**
	 * Returns the first child of this item or 0 if there is none.
	 * Use the child's next() method to get the next child.
	 *
	 * This default implementation always returns 0.
	 **/
	virtual FileInfo * firstChild() const { return nullptr; }

	/**
	 * Set this entry's first child.
	 *
	 * This default implementation does nothing.
	 * Derived classes might want to overwrite this.
	 **/
//	virtual void setFirstChild( FileInfo * ) {}

	/**
	 * Returns true if this entry has any children.
	 **/
	virtual bool hasChildren() const { return firstChild() || dotEntry(); }

	/**
	 * Returns true if this entry has no children.
	 **/
	bool isEmpty() const { return !hasChildren(); }

	/**
	 * Returns true if this entry is in subtree 'subtree', i.e. if this is
	 * a child or grandchild etc. of 'subtree'.
	 **/
	bool isInSubtree( const FileInfo * subtree ) const;

	/**
	 * Locate a child somewhere in this subtree whose URL (i.e. complete
	 * path) matches the URL passed. Returns 0 if there is no such child.
	 *
	 * This is a very expensive operation since the entire subtree is
	 * searched recursively.
	 *
	 * Derived classes might or might not wish to overwrite this method;
	 * it's only advisable to do so if a derived class comes up with a
	 * different method than brute-force searching all children.
	 *
	 * 'findPseudoDirs' specifies if locating pseudo directories like "dot
	 * entries" (".../<Files>") or "attics" (".../<Ignored>") is desired.
	 **/
	virtual FileInfo * locate( const QString & url, bool findPseudoDirs );

	/**
	 * Insert a child into the children list.
	 *
	 * The order of children in this list is absolutely undefined; don't
	 * rely on any implementation-specific order.
	 *
	 * This default implementation does nothing and this should be
	 * overridden by a derived class.
	 **/
	virtual void insertChild( FileInfo * ) {}

	/**
	 * Return the "Dot Entry" for this node if there is one (or 0
	 * otherwise): This is a pseudo entry that directory nodes use to store
	 * non-directory children separately from directories. This way the end
	 * user can easily tell which summary fields belong to the directory
	 * itself and which are the accumulated values of the entire subtree.
	 *
	 * This default implementation always returns 0.
	 **/
	virtual DotEntry * dotEntry() const { return nullptr; }

	/**
	 * Set a "Dot Entry". This makes sense for directories only.
	 *
	 * This default implementation does nothing and this should be
	 * overridden by a derived class.
	 *
	 * Currently unused.
	 **/
//	virtual void setDotEntry( FileInfo * ) {}

	/**
	 * Return 'true' if this is a pseudo directory: A "dot entry" or an
	 * "attic".
	 **/
	bool isPseudoDir() const { return isDotEntry() || isAttic(); }

	/**
	 * Returns true if this is a "Dot Entry".
	 * See dotEntry() for details.
	 *
	 * This default implementation always returns false.
	 **/
	virtual bool isDotEntry() const { return false; }

	/**
	 * (Translated) user-visible string for a "Dot Entry" ("<Files>").
	 **/
	static QString dotEntryName() { return QObject::tr( "<Files>" ); }

	/**
	 * Return the "Attic" entry for this node if there is one (or 0
	 * otherwise): This is a pseudo entry that directory nodes use to store
	 * ignored files and directories separately from the normal tree
	 * hierarchy.
	 *
	 * This default implementation always returns 0.
	 **/
	virtual Attic * attic() const { return nullptr; }

	/**
	 * Check if this is an attic entry where ignored files and directories
	 * are stored.
	 *
	 * This default implementation always returns false.
	 **/
	virtual bool isAttic() const { return false; }

	/**
	 * (Translated) user-visible string for the "Attic" ("<Ignored>").
	 **/
	static QString atticName() { return QObject::tr( "<Ignored>" ); }

	/**
	 * Returns the tree level (depth) of this item. The topmost (invisible)
	 * level is 0 and the visible root is 1.
	 *
	 * This is a (somewhat) expensive operation since it will recurse up
	 * to the top of the tree.
	 **/
	int treeLevel() const;

	/**
	 * Notification that a child has been added somewhere in the subtree.
	 *
	 * This default implementation does nothing.
	 **/
//	virtual void childAdded( FileInfo * ) {}

	/**
	 * Remove a child from the children list.
	 *
	 * IMPORTANT: This MUST be called just prior to deleting an object of
	 * this class. Regrettably, this cannot simply be moved to the
	 * destructor: Important parts of the object might already be destroyed
	 * (e.g., the virtual table - no more virtual methods).
	 *
	 * This is currently not a virtual function and there is just a plain
	 * function DirInfo::unlinkChild.
	 **/
//	virtual void unlinkChild( FileInfo * ) {}

	/**
	 * Notification that a child is about to be deleted somewhere in the
	 * subtree.  The default implementation has no children and so this
	 * does nothing.
	 *
	 * Derived classes that can handle children should overwrite this.
	 **/
//	virtual void deletingChild( FileInfo * ) {}

	/**
	 * Get the current state of the directory reading process.
	 *
	 * Files are always finsihed as soon as they are constructed, so the
	 * default implementation always returns DirFinished.
	 * Derived classes should overwrite this.
	 **/
	virtual DirReadState readState() const { return DirFinished; }

	/**
	 * Check if readState() is anything that indicates an error reading the
	 * directory, i.e. DirError or DirPermissionDenied.
	 *
	 * Files have been read successfully if they are constructed at all, so
	 * the default implementation always returns 'false'.
	 * Derived classes should overwrite this.
	 **/
	virtual bool readError() const { return false; }

	/**
	 * Return a prefix for the total size (and similar accumulated fields)
	 * of this item: ">" if there might be more, i.e. if a subdirectory
	 * could not be read or if reading was aborted, an empty string
	 * otherwise.
	 *
	 * This default implementation returns an empty string. Derived classes
	 * that can handle child items should reimplement this.
	 **/
	virtual QString sizePrefix() const { return ""; }

	/**
	 * Returns true if this is a DirInfo object.
	 *
	 * Don't confuse this with isDir() which tells whether or not this is a
	 * disk directory! Both should return the same, but you'll never know -
	 * better be safe than sorry!
	 *
	 * This default implementation always returns 'false'. Derived classes
	 * (in particular, those derived from DirInfo) should overwrite this.
	 **/
	virtual bool isDirInfo() const { return false; }

	/**
	 * Returns true if this is a PkgInfo object.
	 *
	 * This default implementation always returns 'false'. Derived classes
	 * (in particular, those derived from PkgInfo) should overwrite this.
	 **/
	virtual bool isPkgInfo() const { return false; }

	/**
	 * Try to convert this to a DirInfo pointer. This returns null if this
	 * is not a DirInfo.
	 **/
	DirInfo * toDirInfo();

	/**
	 * Try to convert this to a DirInfo pointer. This returns null if this
	 * is not a DirInfo.
	 **/
	DotEntry * toDotEntry();

	/**
	 * Try to convert this to a DirInfo pointer. This returns null if this
	 * is not a DirInfo.
	 **/
	Attic * toAttic();

	/**
	 * Try to convert this to a PkgInfo pointer. This returns null if this
	 * is not a DirInfo.
	 **/
	PkgInfo * toPkgInfo();

	/**
	 * Returns true if this is a sparse file, i.e. if this file has
	 * actually fewer disk blocks allocated than its byte size would call
	 * for.
	 *
	 * This is a cheap operation since it relies on a cached flag that is
	 * calculated in the constructor rather than doing repeated
	 * calculations and comparisons.
	 *
	 * Please note that size() already takes this into account.
	 **/
	bool isSparseFile() const { return _isSparseFile; }

	/**
	 * Returns true if this FileInfo was ignored by some rule (e.g. in the
	 * "unpackaged files" view).
	 **/
	bool isIgnored() const { return _isIgnored; }

	/**
	 * Set the "ignored" flag. Notice that this only sets the flag; it does
	 * not reparent the FileInfo or anything like that.
	 **/
	void setIgnored( bool ignored ) { _isIgnored = ignored; }

	/**
	 * Return the nearest PkgInfo parent or 0 if there is none.
	 **/
	PkgInfo * pkgInfoParent() const;

	//
	// File type / mode convenience methods.
	// These are simply shortcuts to the respective macros from
	// <sys/stat.h>.
	//

	/**
	 * Returns true if this is a directory.
	 **/
	bool isDir()         const { return S_ISDIR ( _mode ) ? true : false; }

	/**
	 * Returns true if this is a regular file.
	 **/
	bool isFile()        const { return S_ISREG ( _mode ) ? true : false; }

	/**
	 * Returns true if this is a symbolic link.
	 **/
	bool isSymLink()     const { return S_ISLNK ( _mode ) ? true : false; }

	/**
	 * Returns true if this is a (block or character) device.
	 **/
	bool isDevice()      const { return S_ISBLK ( _mode ) || S_ISCHR( _mode ) ? true : false; }

	/**
	 * Returns true if this is a block device.
	 **/
	bool isBlockDevice() const { return S_ISBLK ( _mode ) ? true : false; }

	/**
	 * Returns true if this is a block device.
	 **/
	bool isCharDevice()  const { return S_ISCHR ( _mode ) ? true : false; }

	/**
	 * Returns true if this is a FIFO.
	 **/
	bool isFifo()        const { return S_ISFIFO( _mode ) ? true : false; }

	/**
	 * Returns true if this is a socket.
	 **/
	bool isSocket()      const { return S_ISSOCK( _mode ) ? true : false; }

	/**
	 * Returns true if this is a "special" file, i.e. a (block or character)
	 * device, a FIFO (named pipe) or a socket.
	 **/
	bool isSpecial()     const { return  S_ISBLK ( _mode ) ||
	                                     S_ISCHR ( _mode ) ||
	                                     S_ISFIFO( _mode ) ||
	                                     S_ISSOCK( _mode ) ? true : false; }

	/**
	 * Returns true if this is a symlink, but the (direct) link target does
	 * not exist. This does NOT check multiple symlink indirections,
	 * i.e. it does not check if the target is also a symlink if the target
	 * of that also exists.
	 **/
	bool isBrokenSymLink() const
	    { return isSymLink() ? !QFileInfo( QFileInfo( path() ).symLinkTarget() ).exists() : false; }

	/**
	 * Return the (direct) target path if this is a symlink. This does not
	 * follow multiple symlink indirections, only the direct target.
	 *
	 * If this is not a symlink, an empty string is returned.
	 **/
	QString symLinkTarget() const;


    private:

	//
	// Data members.
	//

	// Keep this short in order to use as little memory as possible -
	// there will be a _lot_ of entries of this kind!
	QString    _name;		// the file name (without path!)

	DirInfo  * _parent;		// pointer to the parent (DirInfo) item
	FileInfo * _next { nullptr };	// pointer to the next child in the same parent
	DirTree  * _tree;		// pointer to the parent tree

	int        _rowNumber { 0 };	// order of this child when the children are sorted
	short      _magic { FileInfoMagic };	// magic number to detect if this object is valid

	bool       _isLocalFile   :1;	// flag: local or remote file?
	bool       _isSparseFile  :1;	// flag: sparse file (file with "holes")?
	bool       _isIgnored     :1;	// flag: ignored by rule?
	bool       _hasUidGidPerm :1;	// flag: was this constructed with uid/guid/ and permissions
	dev_t      _device;		// device this object resides on
	mode_t     _mode;		// file permissions + object type
	nlink_t    _links;		// number of links
	uid_t      _uid;		// User ID of owner
	gid_t      _gid;		// Group ID of owner
	FileSize   _size;		// size in bytes
	FileSize   _blocks;		// 512 bytes blocks
	FileSize   _allocatedSize;	// allocated size in bytes
	time_t     _mtime;		// modification time

    };	// class FileInfo



    typedef QVector<FileInfo *> FileInfoList;



    //----------------------------------------------------------------------
    //			       Static Functions
    //----------------------------------------------------------------------

    /**
     * Print the debugUrl() of a FileInfo in a debug stream.
     **/
    inline QTextStream & operator<< ( QTextStream & stream, const FileInfo * info )
    {
	if ( info )
	{
	    if ( info->checkMagicNumber() )
		stream << info->debugUrl();
	    else
		stream << "<INVALID FileInfo *>";
	}
	else
	    stream << "<NULL FileInfo *>";

	return stream;
    }


    /**
     * Print a QModelIndex of this model in text form to a debug stream.
     **/
    inline QTextStream & operator<<( QTextStream & stream, const QModelIndex & index )
    {
	if ( ! index.isValid() )
	    stream << "<Invalid QModelIndex>";
	else
	{
	    FileInfo * item = static_cast<FileInfo *>( index.internalPointer() );
	    stream << "<QModelIndex row: " << index.row()
		   << " col: " << index.column();

	    if ( item && !item->checkMagicNumber() )
		stream << " <INVALID FileInfo *>";
	    else
		stream << " " << item;

	    stream << " >";
	}

	return stream;
    }


}	// namespace QDirStat


#endif // ifndef FileInfo_h

