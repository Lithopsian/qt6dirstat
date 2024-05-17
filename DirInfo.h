/*
 *   File name: DirInfo.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef DirInfo_h
#define DirInfo_h

#include "FileInfo.h"
#include "DataColumns.h"


namespace QDirStat
{
    class DirSortInfo;
    class DirTree;
    class DotEntry;

    /**
     * Small class to contain information about sorted children of a
     * DirInfo object.  Relatively few DirInfo objects will generally
     * have sorted children, they will only be generated when the model
     * needs to display the children in the tree.
     **/
    class DirSortInfo
    {
	friend class DirInfo;

	/**
	 * Constructor, from a DirInfo parent item and sort order.  The
	 * _sortedChildren list is populated with the direct children plus
	 * any Attic.  This information is currently only used by DirTreeModel
	 * and is optimized for speed due to the numerous lookups required.
	 * The row numbers of each child are stored on the FileInfo object
	 * when the children are sorted.
	 **/
	DirSortInfo( DirInfo       * parent,
	             DataColumn      sortCol,
		     Qt::SortOrder   sortOrder );

	/**
	 * Return a pointer to the dominant children list, creating it if
	 * necessary.
	 **/
	int firstNonDominantChild()
	    { return _firstNonDominantChild < 0 ? findDominantChildren() : _firstNonDominantChild; }

	/**
	 * Create a dominant children list, and populate it as appropriate.  The created
	 * list may be empty if no children are dominant.
	 **/
	int findDominantChildren();


	// Data members

	DataColumn    _sortedCol;
	Qt::SortOrder _sortedOrder;
	FileInfoList  _sortedChildren;
	int           _firstNonDominantChild { -1 };

    };	// class DirSortInfo


    /**
     * A more specialized version of FileInfo: This class can manage
     * children. The base class (FileInfo) has only stubs for the respective
     * methods to integrate seamlessly with the abstraction of a file /
     * directory tree; this class fills those stubs with life.
     *
     * @short directory item within a DirTree.
     **/
    class DirInfo: public FileInfo
    {
    public:

	/**
	 * Constructor from a stat buffer (i.e. based on an lstat() call).
	 **/
	DirInfo( DirInfo           * parent,
		 DirTree           * tree,
		 const QString     & name,
		 const struct stat & statInfo );

	/**
	 * Constructor from the raw fields, used by the cache reader.
	 **/
	DirInfo( DirInfo       * parent,
		 DirTree       * tree,
		 const QString & name,
		 mode_t          mode,
		 FileSize        size,
		 FileSize        allocatedSize,
		 bool            withUidGidPerm,
		 uid_t           uid,
		 gid_t           gid,
		 time_t	         mtime );

	/**
	 * Constructor from the bare necessary fields to create a dummy tree.
	 * Used by the Mime categorizer config page to create a dummy treemap
	 * and by DirReadJob when stat fails.
	 **/
	DirInfo( DirInfo       * parent,
		 DirTree       * tree,
		 const QString & name,
		 mode_t          mode,
		 FileSize        size ):
	    DirInfo ( parent, tree, name, mode, size, size, false, 0, 0, 0 )
	{}

	/**
	 * This constructor does not initially create a dot entry.  If that
	 * is desired, you can always use ensureDotEntry() later.  This is
	 * used for pseudo-directories.
	 **/
	DirInfo( DirInfo       * parent,
		 DirTree       * tree,
		 const QString & name );

	/**
	 * Constructor from just a tree.  Will have no parent, no name, and
	 * no dot entry.  It is used to initialise the root when a new tree
	 * is created.
	 **/
	DirInfo( DirTree * tree ):
	    DirInfo ( nullptr, tree, "" )
	{}

	/**
	 * Destructor.
	 **/
	~DirInfo() override { clear(); }

	/**
	 * Returns the number of hard links, always zero for DirInfo objects.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	nlink_t links() const override { return 0; }

	/**
	 * Returns the total size in bytes of this subtree.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	FileSize totalSize() override;

	/**
	 * Returns the total allocated size in bytes of this subtree.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	FileSize totalAllocatedSize() override;

	/**
	 * Returns the total size in blocks of this subtree.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	FileSize totalBlocks() override;

	/**
	 * Returns the total number of children in this subtree, excluding this
	 * item.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	int totalItems() override;

	/**
	 * Returns the total number of subdirectories in this subtree,
	 * excluding this item. Dot entries and "." or ".." are not counted.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	int totalSubDirs() override;

	/**
	 * Returns the total number of plain file children in this subtree,
	 * excluding this item.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	int totalFiles() override;

	/**
	 * Returns the total number of non-directory items in this subtree,
	 * excluding this item.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
//	int totalNonDirItems() override { return totalItems() - totalSubDirs(); }

	/**
	 * Returns the total number of ignored (non-directory!) items in this
	 * subtree, excluding this item.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	int totalIgnoredItems() override;

	/**
	 * Returns the total number of not ignored (non-directory!) items in
	 * this subtree, excluding this item.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	int totalUnignoredItems() override;

	/**
	 * Returns the total number of direct children of this directory.
	 *
	 * If this directory has a dot entry, the dot entry itself is counted,
	 * but not the file children of the dot entry.
	 *
	 * This method uses a cached value whenever possible, so it is
	 * considerably faster than the unconditional countDirectChildren()
	 * method.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	int directChildrenCount() override;

	/**
	 * Returns the number of subdirectories below this item that could not
	 * be read (typically due to insufficient permissions).
	 *
	 * Notice that this does NOT include this item if it is a directory
	 * that could not be read.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	int errSubDirCount() override;

	/**
	 * Returns the latest modification time of this subtree.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	time_t latestMtime() override;

	/**
	 * Returns the oldest modification time of any file in this subtree.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	time_t oldestFileMtime() override;

	/**
	 * Returns 'true' if this had been excluded while reading.
	 **/
	bool isExcluded() const override { return _isExcluded; }

	/**
	 * Set the 'excluded' status.
	 **/
	void setExcluded( bool excl = true ) override { _isExcluded = excl; }

	/**
	 * Returns whether or not this is a mount point.
	 *
	 * This will return 'false' only if this information can be obtained at
	 * all, i.e. if local directory reading methods are used.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	bool isMountPoint() const  override { return _isMountPoint; }

	/**
	 * Sets the mount point state, i.e. whether or not this is a mount
	 * point.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	void setMountPoint( bool isMountPoint = true ) override { _isMountPoint = isMountPoint; }

	/**
	 * Find the nearest parent that is a mount point or 0 if there is
	 * none. This may return this DirInfo itself.
	 **/
	const DirInfo * findNearestMountPoint() const;

	/**
	 * Returns true if this subtree is finished reading.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	bool isFinished() const override { return !isBusy(); }

	/**
	 * Returns true if this subtree is busy, i.e. it is not finished
	 * reading yet.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	bool isBusy() const override;

	/**
	 * Returns the number of pending read jobs in this subtree. When this
	 * number reaches zero, the entire subtree is done.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	int pendingReadJobs() const override { return _pendingReadJobs; }

	/**
	 * Delete the dot entry if it is empty, i.e. it does not have any
	 * children or its attic (if it has one) is also empty. The dot entry's
	 * attic is implicitly deleted along with it.
	 **/
	void deleteEmptyDotEntry();

	/**
	 * Delete the attic if it is empty.
	 **/
	void deleteEmptyAttic();

	/**
	 * Returns the first child of this item or 0 if there is none.
	 * Use the child's next() method to get the next child.
	 **/
	FileInfo * firstChild() const override { return _firstChild; }

	/**
	 * Insert a child into the children list.
	 *
	 * The order of children in this list is absolutely undefined;
	 * don't rely on any implementation-specific order.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	void insertChild( FileInfo * newChild ) override;

	/**
	 * Add a child to the attic. This is very much like insertChild(), but
	 * it inserts the child into the appropriate attic instead (and sets
	 * its 'ignored' flag: The dot entry's attic if there is a dot entry
	 * and the new child is not a directory, and the directory's attic
	 * otherwise.
	 **/
	void addToAttic( FileInfo * newChild );

	/**
	 * Move a child to the attic, i.e. remove it from the normal children
	 * and move it to the attic instead.
	 **/
	void moveToAttic( FileInfo * newChild );

	/**
	 * Get the "Dot Entry" for this node if there is one (or 0 otherwise):
	 * This is a pseudo entry that directory nodes use to store
	 * non-directory children separately from directories. This way the end
	 * user can easily tell which summary fields belong to the directory
	 * itself and which are the accumulated values of the entire subtree.
	 **/
	DotEntry * dotEntry() const override { return _dotEntry; }

	/**
	 * Return the "Attic" entry for this node if there is one (or 0
	 * otherwise): This is a pseudo entry that directory nodes use to store
	 * ignored files and directories separately from the normal tree
	 * hierarchy.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	Attic * attic() const override { return _attic; }

	/**
	 * Notification that a child has been added somewhere in the subtree.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	void childAdded( FileInfo * newChild );

	/**
	 * Remove a child from the children list.
	 *
	 * IMPORTANT: This MUST be called just prior to deleting a FileInfo
	 * object. Regrettably, this cannot simply be moved to the destructor:
	 * important parts of the object might already be destroyed
	 * (e.g., the virtual table - no more virtual methods).
	 **/
	void unlinkChild( FileInfo * deletedChild );

	/**
	 * Notification that a child is about to be deleted somewhere in the
	 * subtree.
	 *
	 * It is currently only necessary to call unlinkChild() when deleting
	 * a FileInfo object and this function is redundant.
	 **/
//	void deletingChild( FileInfo * deletedChild );

	/**
	 * Notification of a new directory read job somewhere in the subtree.
	 **/
	void readJobAdded();

	/**
	 * Notification of a finished directory read job for 'dir'.
	 * This is cascaded upward in the tree.
	 **/
	void readJobFinished( DirInfo * dir );

	/**
	 * Notification of an aborted directory read job for 'dir'.
	 **/
	void readJobAborted( DirInfo * dir );

	/**
	 * Finalize this directory level after reading it is completed. This
	 * does _not_ mean that reading all subdirectories is completed as
	 * well!
	 *
	 * Clean up unneeded dot entries.
	 **/
	void finalizeLocal();

	/**
	 * Recursively finalize all directories from here on -
	 * call finalizeLocal() recursively.
	 **/
	virtual void finalizeAll();

	/**
	 * Get the current state of the directory reading process:
	 *
	 *    DirQueued		    waiting in the directory read queue
	 *    DirReading	    reading in progress
	 *    DirFinished	    reading finished and OK
	 *    DirAborted	    reading aborted upon user request
	 *    DirError		    error while reading
	 *    DirPermissionDenied   insufficient permissions
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	DirReadState readState() const override { return _readState; }

	/**
	 * Check if readState() is anything that indicates an error reading the
	 * directory. This returns 'true' for DirError or DirPermissionDenied,
	 * 'false' otherwise.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	bool readError() const override;

	/**
	 * Return a prefix for the total size (and similar accumulated fields)
	 * of this item: ">" if there might be more, i.e. if a subdirectory
	 * could not be read or if reading was aborted, an empty string
	 * otherwise.
	 *
	 * Note that this implementation also returns an empty string as long
	 * as this subtree is busy, i.e. reading is not finished: The ">"
	 * prefix should be something special to catch the user's attention,
	 * not something omnipresent that is commonly ignored.
	 *
	 * Reimplemented from FileInfo.
	 **/
	QString sizePrefix() const override;

	/**
	 * Set the state of the directory reading process.
	 * See readState() for details.
	 **/
	void setReadState( DirReadState newReadState );

	/**
	 * Return the sorted list of children from the sort info structure.
	 **/
	const FileInfoList & sortedChildren( DataColumn sortCol, Qt::SortOrder sortOrder )
	    { return sortInfo( sortCol, sortOrder )->_sortedChildren; }

	/**
	 * Return the numeric position of the given child pointer in the
	 * sorted children list.  This is defined here so DirTreeModel can use
	 * it inline.  The first call ensures that the row number on FileInfo
	 * is valid for the requested sort order, also inline unless a new
	 * sort is required.
	 **/
	int childNumber( DataColumn sortCol, Qt::SortOrder sortOrder, const FileInfo * child )
	    { sortInfo( sortCol, sortOrder );  return child->rowNumber(); }

	/**
	 * Check if this directory is locked. This is purely a user lock
	 * that can be used by the application. The DirInfo does not care
	 * about it at all.
	 **/
	bool isLocked() const { return _locked; }

	/**
	 * Set the user lock.
	 **/
	void lock() { _locked = true; }

	/**
	 * Unlock the user lock.
	 **/
	void unlock() { _locked = false; }

	/**
	 * Recursively delete all children, including the dot entry.
	 **/
	void clear();

	/**
	 * Reset to the same status like just after construction in preparation
	 * of refreshing the tree from this point on:
	 *
	 * Delete all children if there are any, delete the dot entry's
	 * children if there are any, restore the dot entry if it was removed
	 * (e.g. in finalizeLocal()), set the read state to DirQueued.
	 **/
	virtual void reset();

	/**
	 * Mark this directory as 'touched'. Item models can use this to keep
	 * track of which items were ever used by a connected view to minimize
	 * any update events: If no information about an item was ever
	 * requested by the view, it is not necessary to tell it that that that
	 * data is now outdated.
	 **/
	void touch() { _touched = true; }

	/**
	 * Check the 'touched' flag.
	 **/
	bool isTouched() const { return _touched; }

	/**
	 * Recursively clear the 'touched' flag.
	 **/
	void clearTouched() { _touched = false; }

	/**
	 * Sets a flag that this is the root directory of a cache file read.
	 **/
	virtual bool isFromCache() const { return _fromCache; }

	/**
	 * Sets a flag that this is the root directory of a cache file read.
	 **/
	void setFromCache() { _fromCache = true; }

	/**
	 * Returns true if this is a DirInfo object.
	 *
	 * Don't confuse this with isDir() which tells whether or not this is a
	 * disk directory! Both should return the same, but you'll never know -
	 * better be safe than sorry!  Derived classes such as PkgInfo and DotEntry
	 * are implicitly isDirInfo(), but not isDir().
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	bool isDirInfo() const override { return true; }

	/**
	 * Take all children from 'oldParent' and move them to this DirInfo.
	 **/
	void takeAllChildren( DirInfo * oldParent );

	/**
	 * Recursively recalculate the summary fields when they are dirty.
	 *
	 * This is a _very_ expensive operation since the entire subtree may
	 * recursively be traversed.
	 **/
	void recalc();

	/**
	 * Return 'true' if this child is a dominant one among its siblings,
	 * i.e. if its total size is much larger than the other items on the
	 * same level.  This won't normally be called unless be called unless
	 * the sort information has already been generated.
	 **/
	bool isDominantChild( FileInfo * child )
	    { return _sortInfo ? child->rowNumber() < _sortInfo->firstNonDominantChild() : false; }

	/**
	 * Finish reading the directory: Set the specified read state, send
	 * signals and finalize the directory (clean up dot entries etc.).
	 **/
	void finishReading( DirReadState readState );

	/**
	 * Mark this object and all its ancestors as dirty and drop their sort
	 * caches.
	 **/
	void markAsDirty();


    protected:

	/**
	 * Set all the counters to values ready to start counting (generally zero).
	 **/
	void initCounts();

	/**
	 * Set this entry's first child.
	 * Use this method only if you know exactly what you are doing.
	 **/
	void setFirstChild( FileInfo * newfirstChild )
	    { _firstChild = newfirstChild; }

	/**
	 * Return the dot entry for this node. If it doesn't have one yet,
	 * create it first.
	 **/
	DotEntry * ensureDotEntry();

	/**
	 * Return the attic for this node. If it doesn't have one yet, create
	 * it first.
	 **/
	Attic * ensureAttic();

	/**
	 * Return 'true' if there is an attic and it has any children.
	 **/
	bool hasAtticChildren() const;

	/**
	 * Return a structure containing a list of children sorted by 'sortCol'
	 * and in 'sortOrder' (Qt::AscendingOrder or Qt::DescendingOrder).  The
	 * structure also includes a mapping of FileInfo pointers to the
	 * numeric order in the list, for rapid lookup in DirTreeModel.
	 *
	 * A list of dominant children by size can also be included, but is
	 * generated in a separate function call.
	 *
	 * This might return cached information if all parameters are the same
	 * as for the last call to this function, and there were no children
	 * added or removed in the meantime.
	 **/
	const DirSortInfo * sortInfo( DataColumn sortCol, Qt::SortOrder sortOrder )
	    { return sorted( sortCol, sortOrder ) ? _sortInfo : newSortInfo( sortCol, sortOrder ); }

	/**
	 * Replaces any existing sort information with new data for the given sort
	 * column and order and returns a pointer to the DirSortInfo structure.
	 **/
	const DirSortInfo * newSortInfo( DataColumn sortCol, Qt::SortOrder sortOrder );

	/**
	 * Return whether the current sort data, if any, matches the given
	 * sort column and order.
	 **/
	bool sorted( DataColumn sortCol, Qt::SortOrder sortOrder ) const
	    { return _sortInfo && sortCol == _sortInfo->_sortedCol && sortOrder == _sortInfo->_sortedOrder; }

	/**
	 * Drop all cached information about children sorting for this object and
	 * all its descendants.
	 **/
	void dropSortCaches();

	/**
	 * Drop all cached information about children sorting.
	 **/
	void dropSortCache();

	/**
	 * Count the direct children unconditionally and update
	 * _directChildrenCount.
	 **/
	int countDirectChildren();

	/**
	 * Check the 'ignored' state of this item and set the '_isIgnored' flag
	 * accordingly.
	 **/
	virtual void checkIgnored();

	/**
	 * Set any empty subdir children to ignored. This affects only direct
	 * children.
	 **/
	void ignoreEmptySubDirs();

	/**
	 * Clean up unneeded / undesired dot entries:
	 * Delete dot entries that don't have any children,
	 * reparent dot entry children to the "real" (parent) directory if
	 * there are not subdirectory siblings at the level of the dot entry.
	 **/
	virtual void cleanupDotEntries();

	/**
	 * Clean up unneeded attics: Delete attic entries that don't have any
	 * children.
	 **/
	void cleanupAttics();


    private:

	//
	// Data members
	//

	int            _pendingReadJobs	{ 0 };

	bool           _isMountPoint:1;		// flag: is this a mount point?
	bool           _isExcluded:1;		// flag: was this directory excluded?
	bool           _summaryDirty:1;		// dirty flag for the cached values
	bool           _locked:1;		// app lock
	bool           _touched:1;		// app 'touch' flag
	bool           _fromCache:1;		// is this the root of a cache file read

	// Children management
	FileInfo     * _firstChild	{ nullptr };	// pointer to the first child
	DotEntry     * _dotEntry	{ nullptr };	// pseudo entry to hold non-dir children
	Attic        * _attic		{ nullptr };	// pseudo entry to hold ignored children
	DirSortInfo  * _sortInfo	{ nullptr };	// sorted children lists

	// Summary data, not always current as indicated by the _summaryDirty flag
	DirReadState   _readState;
	int            _totalItems;
	int            _totalSubDirs;
	int            _totalFiles;
	int            _totalIgnoredItems;
	int            _totalUnignoredItems;
	int            _directChildrenCount;
	int            _errSubDirCount;
	FileSize       _totalSize;
	FileSize       _totalAllocatedSize;
	FileSize       _totalBlocks;
	time_t         _latestMtime;
	time_t         _oldestFileMtime;

    };	// class DirInfo

}	// namespace QDirStat


#endif // ifndef DirInfo_h

