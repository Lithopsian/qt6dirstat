/*
 *   File name: DirReadJob.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef DirReadJob_h
#define DirReadJob_h

#include <QTextStream>
#include <QTimer>


namespace QDirStat
{
    class DirInfo;
    class DirTree;
    class CacheReader;
    class DirReadJobQueue;
    class FileInfo;

    /**
     * A directory read job that can be queued. This is mainly to prevent
     * buffer thrashing because of too many directories opened at the same time
     * because of simultaneous reads or even system resource consumption
     * (directory handles in this case).
     *
     * Objects of this kind are transient by nature: They live only as long as
     * the job is queued or executed. When it is done, the data is contained in
     * the corresponding DirInfo subtree of the corresponding DirTree.
     *
     * For each entry automatically a FileInfo or DirInfo will be created and
     * added to the parent DirInfo. For each directory a new DirReadJob will be
     * created and added to the DirTree's job queue.
     *
     * This class is not intended to be used directly.  Derive your own class from
     * it or use one of LocalDirReadJob or CacheReadJob.  Derived classes should
     * implement at least one of read() or startReading().
     *
     * @short Abstract base class for directory reading.
     **/
    class DirReadJob
    {
    public:

	/**
	 * Constructor
	 *
	 * This does not read anything yet. Call read() for that.
	 **/
	DirReadJob( DirTree * tree, DirInfo * dir );

	/**
	 * Destructor
	 **/
	virtual ~DirReadJob();

	/**
	 * Suppress copy and assignment constructors (this is not a QObject)
	 **/
	DirReadJob( const DirReadJob & ) = delete;
	DirReadJob & operator=( const DirReadJob & ) = delete;

	/**
	 * Read the next couple of items from the directory.
	 * Call finished() when there is nothing more to read.
	 *
	 * Derived classes should overwrite this method or startReading().
	 * This default implementation calls startReading() if it has not been
	 * called yet.
	 **/
	virtual void read();

	/**
	 * Returns the corresponding DirInfo item.
	 * Caution: this may be 0.
	 **/
	DirInfo * dir() const { return _dir; }

	/**
	 * Set the corresponding DirInfo item.
	 **/
	void setDir( DirInfo * dir ) { _dir = dir; }

	/**
	 * Return the corresponding DirTree.
	 **/
	DirTree * tree() const { return _tree; }

	/**
	 * Return the job queue this job is in or 0 if it isn't queued.
	 **/
	DirReadJobQueue * queue() const { return _queue; }

	/**
	 * Set the job queue this job is in.
	 **/
	void setQueue( DirReadJobQueue * queue ) { _queue = queue; }


    protected:

	/**
	 * Initialize reading.
	 *
	 * Derived classes should overwrite this method or read().
	 **/
	virtual void startReading() {}

	/**
	 * Notification that a new child has been added.
	 *
	 * Derived classes are required to call this whenever a new child is
	 * added so this notification can be passed up to the DirTree which in
	 * turn emits a corresponding signal.
	 **/
	void childAdded( FileInfo * newChild );

	/**
	 * Notification that a child is about to be deleted.
	 *
	 * Derived classes are required to call this just before a child is
	 * deleted so this notification can be passed up to the DirTree which
	 * in turn emits a corresponding signal.
	 *
	 * Derived classes are not required to handle child deletion at all,
	 * but if they do, calling this method is required.
	 **/
//	void deletingChild( FileInfo * deletedChild );

	/**
	 * Send job finished notification to the associated tree.
	 * This will delete this job.
	 **/
	void finished();


    private:

	DirTree         *  _tree;
	DirInfo         * _dir;
	DirReadJobQueue * _queue	{ nullptr };
	bool              _started	{ false };

    };	// class DirReadJob


    /**
     * Implementation of the abstract DirReadJob class that reads a local
     * directory.
     *
     * This will use lstat() system calls rather than KDE's network transparent
     * directory services since lstat() unlike the KDE services can obtain
     * information about the device (i.e. filesystem) a file or directory
     * resides on. This is important if you wish to limit directory scans to
     * one filesystem - which is desirable when that one filesystem runs
     * out of space.
     *
     * @short Directory reader that reads one local directory.
     **/
    class LocalDirReadJob: public DirReadJob
    {
    public:
	/**
	 * Constructor.
	 **/
	LocalDirReadJob( DirTree * tree, DirInfo * dir, bool applyFileChildExcludeRules );

	/**
	 * Return 'true' if any exclude rules matching against any direct file
	 * child should be applied. This is generally useful only for
	 * second-level read jobs, not for the starting point of a directory
	 * scan, so it is easily possible to continue reading at an excluded
	 * directory.
	 *
	 * The default is 'false'.
	 **/
	bool applyFileChildExcludeRules() const { return _applyFileChildExcludeRules; }


    protected:

	/**
	 * Read the directory. Prior to this nothing happens.
	 *
	 * Inherited and reimplemented from DirReadJob.
	 **/
	void startReading() override;

	/**
	 * Process one subdirectory entry.
	 **/
	void processSubDir( const QString & entryName,
			    DirInfo       * subDir );

	/**
	 * Return 'true' if 'entryName' should be ignored.
	 **/
	bool checkIgnoreFilters( const QString & entryName ) const;

	/**
	 * Read a cache file that was picked up along the way:
	 *
	 * If one of the non-directory entries of this directory was
	 * ".qdirstat.cache.gz", open it, and if the toplevel entry in that
	 * file matches the current path, read all the cache contents, kill all
	 * pending read jobs for subdirectories of this directory and return
	 * 'true'. In that case, the current read job is finished and deleted
	 * (!), control needs to be returned to the caller, and using any data
	 * members of this object is no longer safe (since they have just been
	 * deleted).
	 *
	 * In all other cases, consider that entry as a plain file and return
	 * 'false'.
	 **/
	bool readCacheFile( const QString & cacheFileName );

	/**
	 * Handle an error during lstat() of a directory entry.
	 **/
	void handleLstatError( const QString & entryName );

	/**
	 * Exclude the directory of this read job after it is almost completely
	 * read. This is used when checking for exclude rules matching direct
	 * file children of a directory.
	 *
	 * The main purpose of having this as a separate function is to have a
	 * clear backtrace if it segfaults.
	 **/
	void excludeDirLate();

	/**
	 * Return the full name with path of an entry of this directory.
	 **/
	QString fullName( const QString & entryName ) const;

	/**
	 * Return 'true' if the current filesystem is NTFS, only
	 * checking once and then returning a cached value.
	 **/
	bool isNtfs() { return _checkedForNtfs ? _isNtfs : checkForNtfs(); }

	/**
	 * Checks if the current filesystem is NTFS and returns the result.
	 * The result is then cached for subsequent queries.
	 **/
	bool checkForNtfs();


    private:

	//
	// Data members
	//

	QString _dirName;
	bool    _applyFileChildExcludeRules;
	bool    _checkedForNtfs	{ false };
	bool    _isNtfs		{ false };

    };	// LocalDirReadJob



    class CacheReadJob: public DirReadJob
    {

    public:

	/**
	 * Constructor that reads the file contents into an empty tree.
	 **/
	CacheReadJob( DirTree       * tree,
		      const QString & cacheFileName );

	/**
	 * Constructor that checks that the cache file contents match the
	 * given toplevel.
	 **/
	CacheReadJob( DirTree       * tree,
		      DirInfo       * dir,
		      DirInfo       * parent,
		      const QString & cacheFileName );

	/**
	 * Destructor.
	 **/
	~CacheReadJob() override;

	/**
	 * Start reading the cache. Prior to this nothing happens.
	 *
	 * Inherited and reimplemented from DirReadJob.
	 **/
	void read() override;

	/**
	 * Return the associated cache reader.
	 **/
	CacheReader * reader() const { return _reader; }


    private:

	/**
	 * Initializations common for all constructors.
	 **/
	void init();


	CacheReader * _reader;

    };	// class CacheReadJob



    /**
     * Queue for read jobs
     *
     * Handles time-sliced reading automatically.
     **/
    class DirReadJobQueue: public QObject
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	DirReadJobQueue();

	/**
	 * Destructor.
	 **/
	~DirReadJobQueue() override;

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
	int count() const   { return _queue.count() + _blocked.count(); }

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

	QList<DirReadJob *> _queue;
	QList<DirReadJob *> _blocked;
	QTimer              _timer;

    };	// class DirReadJobQueue


    /**
     * Human-readable output of a DirReadJob in a debug stream.
     **/
    inline QTextStream & operator<< ( QTextStream & str, DirReadJob * job )
    {
	if ( job )
	{
	    CacheReadJob * cacheReadJob = dynamic_cast<CacheReadJob *>( job );
	    QString jobType = cacheReadJob ? "CacheReadJob" : "DirReadJob";
	    str << "<" << jobType << " " << job->dir() << ">";
	}
	else
	    str << "<NULL DirReadJob *>";

	return str;
    }

}	// namespace QDirStat


#endif // ifndef DirReadJob_h
