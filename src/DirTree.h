/*
 *   File name: DirTree.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef DirTree_h
#define DirTree_h

#include <memory>

#include <QTimer>
#include <QVector>

#include "Typedefs.h"   // FileSize


namespace QDirStat
{
    class DirInfo;
    class DirReadJob;
    class FileInfo;
    class FileInfoSet;
    class ExcludeRules;
    class DirTreeFilter;
    class PkgFilter;

    typedef QVector<DirReadJob *> DirReadJobList;


    /**
     * Queue for read jobs
     *
     * Handles time-sliced reading automatically.
     **/
    class DirReadJobQueue final : public QObject
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	DirReadJobQueue():
	    QObject {}
	{
	    connect( &_timer, &QTimer::timeout,
	             this,    &DirReadJobQueue::timeSlicedRead );
	}

	/**
	 * Destructor.
	 **/
	~DirReadJobQueue() override
	    { clear(); }

	/**
	 * Add a job to the end of the queue. Begin time-sliced reading if not
	 * in progress yet.
	 **/
	void enqueue( DirReadJob * job );

	/**
	 * Get the head of the queue (the next job that is due for processing).
	 **/
	DirReadJob * head() const { return _queue.first();}

	/**
	 * Count the number of pending jobs in the queue.
	 **/
	FileCount count() const { return _queue.count() + _blocked.count(); }

	/**
	 * Check if the queue is empty.
	 **/
	bool isEmpty() const { return _queue.isEmpty() && _blocked.isEmpty(); }

	/**
	 * Add a job to the list of blocked jobs: Jobs that are not yet ready
	 * yet, e.g. because they are waiting for results from an external
	 * process.
	 **/
	void addBlocked( DirReadJob * job ) { _blocked.append( job ); }

	/**
	 * Notification that a job that was blocked is now ready to be
	 * scheduled, so it will be taken out of the list of blocked jobs and
	 * added to the end of the queue.
	 **/
	void unblock( DirReadJob * job );

	/**
	 * Clear the queue: Remove all pending jobs from the queue and destroy
	 * them.
	 **/
	void clear();

	/**
	 * Abort all jobs in the queue.
	 **/
	void abort();

	/**
	 * Delete all jobs for a subtree, except 'exceptJob'.
	 **/
	void killSubtree( DirInfo * subtree, const DirReadJob * exceptJob = nullptr );

	/**
	 * Notification that a job is finished.
	 * This takes that job out of the queue and deletes it.
	 * Read jobs are required to call this when they are finished.
	 **/
	void jobFinishedNotify( DirReadJob * job );


    signals:

	/**
	 * Emitted when job reading starts, i.e. when a new job is inserted
	 * into a queue that was empty
	 **/
	void startingReading();

	/**
	 * Emitted when reading is finished, i.e. when the last read job of the
	 * queue is finished.
	 **/
	void finished();


    public slots:

	/**
	 * Notification that a child node is about to be deleted from the
	 * outside (i.e., not from this ReadJobQueue), e.g. because of cleanup
	 * actions. This will remove all pending directory read jobs for that
	 * subtree from the job queue.
	 **/
	void deletingChildNotify( FileInfo * child );


    protected slots:

	/**
	 * Time-sliced work procedure to be performed while the application is
	 * in the main loop: Read some directory entries, but relinquish
	 * control back to the application so it can maintain some
	 * responsiveness. This method uses a timer of minimal duration to
	 * activate itself as soon as there are no more user events to
	 * process. Call this only once directly after inserting a read job
	 * into the job queue.
	 **/
	void timeSlicedRead();


    private:

	DirReadJobList _queue;
	DirReadJobList _blocked;
	QTimer         _timer;

    };	// class DirReadJobQueue



    /**
     * This class provides some infrastructure as well as global data for a
     * directory tree. It acts as the glue that holds things together: the root
     * item from which to descend into the subtrees, the read queue and some
     * global policies (like whether or not to cross filesystems while reading
     * directories).
     *
     * Note that this class uses a "pseudo root" to better conform with Qt's
     * notion of tree views and the corresponding data models: they use an
     * invisible root item to support multiple toplevel items.
     **/
    class DirTree final : public QObject
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 *
	 * Remember to call startReading() after the constructor and
	 * setting up connections.
	 **/
	DirTree( QObject * parent );

	/**
	 * Destructor.
	 **/
	~DirTree() override;

	/**
	 * Actually start reading.  The given path is converted to a directory:
	 * if it is a symlink then the link is followed; if that is not a
	 * directory then the parent directory is read.
	 *
	 * It's not very pretty that this is required as a separate method, but
	 * this cannot simply be done in the constructor: we need to give the
	 * caller a chance to set up Qt signal connections, and for this the
	 * constructor must return before any signals are sent, i.e. before
	 * anything is read.
	 **/
	void startReading( const QString & path );

	/**
	 * Forcefully stop a running read process.
	 **/
	void abortReading();

	/**
	 * Refresh a number of subtrees.  If any of these are no longer accessible,
	 * the item's parent will be used instead (and so on, recursively).  The
	 * set of accessible subtrees will then be purged of invalid items and
	 * normalised (items with ancestors in the set will be removed).
	 **/
	void refresh( const FileInfoSet & refreshSet );

	/**
	 * Delete a directory: so no check is made for an empty dot entry parent.
	 * The model is notified with a list of the children (just one) being
	 * deleted.
	 **/
	void deleteSubtree( DirInfo * subtree );

	/**
	 * Delete a list of subtrees.  These are grouped by parent, and each group
	 * of children with the same parent is notified to the model as a single
	 * list. This complex approach is because beginRemoveRows() is rather slow,
	 * especially for large directories.  Calling it tens or hundreds of times
	 * locks the program for many seconds or more.  Now only truly pathological
	 * selections of items with hundreds of distinct parents are slow enough to
	 * be an issue.
	 *
	 * Additionally, after the children of each parent have been deleted, check
	 * if it is now an empty dot entry and delete it as well.
	 **/
	void deleteSubtrees( const FileInfoSet & subtrees );

	/**
	 * Delete all children of a subtree, but leave the subtree inself
	 * intact.
	 **/
	void clearSubtree( DirInfo * subtree );

	/**
	 * Finalize the complete tree after all read jobs are done.
	 **/
	void finalizeTree();

	/**
	 * Return the URL of this tree.
	 **/
	const QString & url() const { return _url; }

	/**
	 * Set the URL of this tree.
	 **/
	void setUrl( const QString & url ) { _url = url; }

	/**
	 * Return the root item of this tree. Note that this is a pseudo root
	 * that does not really correspond to a filesystem object.
	 **/
	DirInfo * root() const { return _root.get(); }

	/**
	 * Return a special printable url for the root item of this tree.
	 **/
	QLatin1String rootDebugUrl() const { return "<root>"_L1; }

	/**
	 * Return the first toplevel item of this tree (that is, the first
	 * child of the invisible root item) or 0 if there is none.  There
	 * is normally only one child of the root item and it is nearly
	 * always a directory (eg. "/" ).
	 **/
	FileInfo * firstToplevel() const;

	/**
	 * Return 'true' if 'item' is the (visible) root item.
	 **/
//	bool isToplevel( const FileInfo * item ) const;

	/**
	 * Return 'true' if 'item' is the (invisible) root item.
	 **/
	bool isRoot( const DirInfo * dir ) const { return dir && dir == root(); }

	/**
	 * Clear all items of this tree.  This should only be called from
	 * DirTreeModel unless you have some other special way of ensuring that
	 * the model is aware that the tree is being emptied.
	 **/
	void prepare();

	/**
	 * Clear all items of this tree.  This should only be called from
	 * DirTreeModel unless you have some other special way of ensuring that
	 * the model is aware that the tree is being emptied.
	 **/
	void clear();

	/**
	 * Clear all exclude rules and filters of this tree.  The items themselves
	 * are not cleared since this can only be done safely by the model.
	 **/
	void reset() { clearTmpExcludeRules(); clearFilters(); }

	/**
	 * Locate a child somewhere in the tree whose URL (i.e. complete path)
	 * matches the URL passed. Returns 0 if there is no such child.
	 *
	 * Note that this is a very expensive operation since the entire tree is
	 * searched recursively.
	 **/
	FileInfo * locate( const QString & url ) const;

	/**
	 * Add a new directory read job to the queue.
	 **/
	void addJob( DirReadJob * job )
	    { _jobQueue.enqueue( job ); }

	/**
	 * Add a new directory read job to the list of blocked jobs. A job may
	 * be blocked because it may be waiting for an external process to
	 * finish.
	 **/
	void addBlockedJob( DirReadJob * job )
	    { _jobQueue.addBlocked( job ); }

	/**
	 * Unblock a previously blocked read job so it is scheduled along with
	 * the other pending jobs.
	 **/
	void unblock( DirReadJob * job )
	    { _jobQueue.unblock( job ); }

	/**
	 * Returns whether reads should cross filesystem boundaries.
	 *
	 * Note that this can only be avoided with local directories where the
	 * device number can be obtained.
	 *
	 * This flag is initialised from a configuration setting, but may be
	 * modified from the Open dialog or Unpkg dialog.
	 **/
	bool crossFilesystems() const { return _crossFilesystems; }

	/**
	 * Set or unset the "cross filesystems" flag.
	 **/
	void setCrossFilesystems( bool crossFilesystems )
	    { _crossFilesystems = crossFilesystems; }

	/**
	 * Notification that a child has been added.
	 *
	 * Directory read jobs are required to call this for each child added
	 * so the tree can emit the corresponding childAdded() signal.
	 **/
	void childAddedNotify( FileInfo * newChild );

	/**
	 * Send a startingReading() signal.
	 **/
	void sendStartingReading();

	/**
	 * Send a aborted() signal.
	 **/
//	void sendAborted();

	/**
	 * Send a startingReading( DirInfo * ) signal.
	 **/
//	void sendStartingReading( DirInfo * dir );

	/**
	 * Send a readJobFinished( DirInfo * ) signal.
	 **/
	void sendReadJobFinished( DirInfo * dir );

	/**
	 * Returns 'true' if directory reading is in progress in this tree.
	 **/
	bool isBusy() const { return _isBusy; }

	/**
	 * Write the complete tree to a cache file.
	 *
	 * Returns true if OK, false if there was an error.
	 **/
	bool writeCache( const QString & cacheFileName );

	/**
	 * Read a cache file.
	 *
	 * Returns true if OK, false if there was an error.
	 **/
	bool readCache( const QString & cacheFileName );

	/**
	 * Read installed packages that match the specified PkgFilter and their
	 * file lists from the system's package manager(s).
	 **/
	void readPkg( const PkgFilter & pkgFilter );

	/**
	 * Return exclude rules specific to this tree (as opposed to the global
	 * ones stored in the ExcludeRules singleton) or 0 if there are none.
	 **/
	const ExcludeRules * excludeRules() const { return _excludeRules.get(); }

	/**
	 * Return exclude rules specific to this tree (as opposed to the global
	 * ones stored in the ExcludeRules singleton) or 0 if there are none.
	 **/
//	const ExcludeRules * tmpExcludeRules() const { return _tmpExcludeRules.get(); }

	/**
	 * Set exclude rules from the settings file.  The tree will create its
	 * own ExcludeRules object.  These rules will be applied to all regular
	 * directory reads and unpackaged views, but not to packaged file views.
	 **/
	void setExcludeRules();

	/**
	 * Set exclude rules specific to this tree. They are additional rules
	 * to the ones in the ExcludeRules singleton. This can be used for
	 * temporary exclude rules that are not to be written to the config
	 * file.
	 *
	 * The DirTree takes over ownership of this object and will delete it
	 * when appropriate (i.e. in its destructor or when new ExcludeRules
	 * are set with this function). Call this with 0 to remove the existing
	 * exclude rules.
	 **/
	void setTmpExcludeRules( const ExcludeRules * newTmpRules );

	/**
	 * Return 'true' if 'entryName' matches an exclude rule of the
	 * ExcludeRule singleton or a temporary exclude rule of the DirTree.
	 **/
	bool matchesExcludeRule( const QString & fullName, const QString & entryName ) const;

	/**
	 * Return 'true' if any chiuldren of the given directory are matched
	 * by an exclude rule of that type.
	 **/
	bool matchesDirectChildren( const DirInfo * dir ) const;

	/**
	 * Add a filter to ignore files during directory reading.
	 *
	 * The DirTree takes over ownership of the filter object and will
	 * delete it when appropriate.
	 **/
	void addFilter( const DirTreeFilter * filter )
	    { if ( filter ) _filters << filter; }

	/**
	 * Clear all filters.
	 **/
	void clearFilters();

	/**
	 * Iterate over all filters and return 'true' if any of them wants a
	 * filesystem object to be ignored during directory reading, 'false'
	 * if not.
	 **/
	bool checkIgnoreFilters( const QString & path ) const;

	/**
	 * Return 'true' if there is any filter, 'false' if not.
	 **/
	bool hasFilters() const { return !_filters.isEmpty(); }

	/**
	 * Returns whether a valid cluster size has been determined yet
	 * for this tree.
	 **/
	bool haveClusterSize() const { return _blocksPerCluster > 0; }

	/**
	 * Return the number of 512-bytes blocks per cluster.
	 *
	 * This may be 0 if no small file (< 512 bytes) was found in this tree
	 * yet.
	 **/
//	int blocksPerCluster() const { return _blocksPerCluster; }

	/**
	 * Return the cluster size of this tree, i.e. the disk space allocation
	 * unit. No non-zero file can have an allocated size smaller than this.
	 *
	 * This may be 0 if no small file (< 512 bytes) was found in this tree
	 * yet.
	 *
	 * FileSize (64 bits) is overkill, but the maximum cluster size for some
	 * filesystems is approaching this value (eg. 256M for ext4) so just for
	 * safety.
	 **/
	FileSize clusterSize() const { return _blocksPerCluster * STD_BLOCK_SIZE; }

	/**
	 * Set the policy for how hard links are handled: by default, for files
	 * with multiple hard links, the total size is distributed among each
	 * individual hard link for that file. So a file with a size of 4 kB
	 * and 4 hard links reports 1 kB to its parent directory.  This avoids
	 * counting the same disk allocation multiple times.
	 *
	 * When this flag is set to 'true', it will report the full 4 kB each
	 * time, so all 4 hard links together will now add up to 16 kB. While
	 * this is probably a very bad idea if those links are all in the same
	 * directory (or subtree), it might be useful if there are several
	 * separate subtrees that all share hard links between each other, but
	 * not within the same subtree. Some backup systems use this strategy
	 * to save disk space.
	 *
	 * This flag will be read from the config file from the outside
	 * (DirTreeModel) and set from there using this function.
	 **/
	void setIgnoreHardLinks( bool ignore );

	/**
	 * Return the current hard links accounting policy.
	 **/
	bool ignoreHardLinks() const { return _ignoreHardLinks; }

	/**
	 * Set whether to trust the number of hard links reported for an NTFS
	 * file.  If not, then the hard links count is always set to 1; hard
	 * links will not be reported and total disk allocations will be shown
	 * too high.
	 *
	 * The ntfs-3g legacy driver can report excessive hard link counts if
	 * it is not configured with the posix_nlink option.  This is
	 * increasingly less common, so this option now defaults to true.
	 *
	 * This flag will be read from the config file from the outside
	 * (DirTreeModel) and set from there using this function.
	 **/
	void setTrustNtfsHardLinks( bool trust )
	    { _trustNtfsHardLinks = trust; }

	/**
	 * Return whether to trust hatd link counts reported by NTFS.
	 **/
	bool trustNtfsHardLinks() const { return _trustNtfsHardLinks; }

	/**
	 * Check if going from 'parent' to 'child' would cross a filesystem
	 * boundary. This take Btrfs subvolumes into account.
	 **/
	static bool crossingFilesystems( const DirInfo * parent, const DirInfo * child );


    signals:

	/**
	 * Emitted when a child has been added.
	 *
	 * Currently nobody uses this signal.
	 **/
//	void childAdded( FileInfo * newChild );

	/**
	 * Emitted when a child is about to be deleted.
	 **/
	void deletingChild( FileInfo * deletedChild );

	/**
	 * Emitted when several children, with the same parent, are
	 * about to be deleted.  This will be followed by a childrenDeleted()
	 * signal after the children have been deleted.  A deletingChild()
	 * signal will also be generated for each individual child.
	 **/
	void deletingChildren( DirInfo * parent, const FileInfoSet & deletedChildren );

	/**
	 * Emitted after a set of children have been deleted following the
	 * deletingChildren() signal.
	 **/
	void childrenDeleted();

	/**
	 * Emitted when the tree is about to be cleared.
	 **/
	void clearing();

	/**
	 * Emitted after the tree has been cleared.
	 **/
	void cleared();

	/**
	 * Emitted when a subtree is about to be cleared, i.e. all its children
	 * will be deleted (but not the subtree node itself).
	 **/
	void clearingSubtree( DirInfo * subtree );

	/**
	 * Emitted when clearing a subtree is finished.
	 **/
	void subtreeCleared();

	/**
	 * Emitted when reading is started.
	 **/
	void startingReading();

	/**
	 * Emitted when a refresh has started reading (or is about to).
	 **/
	void startingRefresh();

	/**
	 * Emitted when reading this directory tree is completely and
	 * successfully finished.
	 **/
	void finished();

	/**
	 * Emitted when reading this directory tree has been aborted.
	 **/
	void aborted();

	/**
	 * Emitted when reading the specified directory has been finished.
	 * This is sent AFTER finalizeLocal( DirInfo * dir ).
	 **/
	void readJobFinished( DirInfo * dir );


    public slots:

	/**
	 * Notification that all jobs in the job queue are finished.
	 * This will finalize the tree and emit the finished() signal.
	 **/
	void sendFinished();


    protected:

	/**
	 * Refresh a subtree, i.e. read its contents from disk again.
	 *
	 * All children of the old subtree will be deleted and rebuilt from
	 * scratch, i.e. all pointers to elements within this subtree will
	 * become invalid (a subtreeDeleted() signal will be emitted to notify
	 * about that fact).
	 *
	 * If the tree root (an item with no parent) is passed, then a full
	 * re-read of the tree will be done.
	 **/
	void refresh( DirInfo * subtree );

	/**
	 * Delete a child from the tree.  A signal is emitted for this child
	 * specifically, used by SelectionModel, but not deletingChildren()
	 * signals for the data model.  Either the caller should notify the model
	 * or this should only be called when the model is not yet aware it even
	 * has children.
	 **/
	void deleteChild( FileInfo * child );

	/**
	 * Clear all temporary exclude rules.
	 **/
	void clearTmpExcludeRules() { setTmpExcludeRules( nullptr ); }


    private:

	std::unique_ptr<DirInfo>            _root;
	std::unique_ptr<const ExcludeRules> _excludeRules;
	std::unique_ptr<const ExcludeRules> _tmpExcludeRules;

	QString                        _url;
	DirReadJobQueue                _jobQueue;
	QVector<const DirTreeFilter *> _filters;

	bool _crossFilesystems{ false };
	bool _isBusy{ false };
	bool _ignoreHardLinks{ false };
	bool _trustNtfsHardLinks{ true };
	int  _blocksPerCluster{ -1 };

    };	// class DirTree

}	// namespace QDirStat

#endif	// ifndef DirTree_h
