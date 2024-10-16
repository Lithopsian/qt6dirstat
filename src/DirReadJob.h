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

#include <memory>

#include <QString>
#include <QTextStream>


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

	DirTree         * _tree;
	DirInfo         * _dir;
	DirReadJobQueue * _queue{ nullptr };
	bool              _started{ false };

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
	bool    _checkedForNtfs{ false };
	bool    _isNtfs{ false };

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
	 * Start reading the cache. Prior to this nothing happens.
	 *
	 * Inherited and reimplemented from DirReadJob.
	 **/
	void read() override;

	/**
	 * Return the associated cache reader.
	 **/
	CacheReader * reader() const { return _reader.get(); }


    protected:

	/**
	 * Initializations common for all constructors.
	 **/
	void init();


    private:

	std::unique_ptr<CacheReader> _reader;

    };	// class CacheReadJob



    /**
     * Human-readable output of a DirReadJob in a debug stream.
     **/
    inline QTextStream & operator<<( QTextStream & str, DirReadJob * job )
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

#endif	// ifndef DirReadJob_h
