/*
 *   File name: DirReadJob.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <dirent.h>
#include <fcntl.h>      // AT_ constants (fstatat() flags)
#include <unistd.h>     // R_OK, X_OK
#include <stdio.h>

#include <QMutableListIterator>

#include "DirReadJob.h"
#include "DirTree.h"
#include "DirTreeCache.h"
#include "DirInfo.h"
#include "ExcludeRules.h"
#include "MountPoints.h"
#include "Exception.h"
#include "Logger.h"


#define DONT_TRUST_NTFS_HARD_LINKS      1
#define VERBOSE_NTFS_HARD_LINKS         0


using namespace QDirStat;


namespace
{
    /**
     * Return the device name where 'dir' is on if it's a mount point.
     * This uses MountPoints which reads /proc/mounts.
     **/
    QString device( const DirInfo * dir )
    {
	return MountPoints::device( dir->url() );
    }


    /**
     * Check if going from 'parent' to 'child' would cross a filesystem
     * boundary. This take Btrfs subvolumes into account.
     **/
    bool crossingFilesystems( DirTree * tree, DirInfo * parent, DirInfo * child )
    {
	if ( parent->device() == child->device() )
	    return false;

	const QString childDevice = device( child );
	QString parentDevice      = device( parent->findNearestMountPoint() );
	if ( parentDevice.isEmpty() )
	    parentDevice = tree->device();

	// Not safe to assume that empty devices indicate filesystem crossing.
	// Calling something a mountpoint when it isn't causes findNearestMountPoint()
	// to return null and things then crash.
	const bool crossing = !parentDevice.isEmpty() && !childDevice.isEmpty() && parentDevice != childDevice;
	if ( crossing )
	    logInfo() << "Filesystem boundary at mount point " << child << " on device " << childDevice << Qt::endl;
	else
	    logInfo() << "Mount point " << child << " is still on the same device " << childDevice << Qt::endl;

	return crossing;
    }


    /**
     * Check if we really should cross into a mounted filesystem; don't do
     * it if this is a system mount, a bind mount, a filesystem mounted
     * multiple times, or a network mount (NFS / Samba).
     **/
    bool shouldCrossIntoFilesystem( const DirInfo * dir )
    {
	const MountPoint * mountPoint = MountPoints::findByPath( dir->url() );

	if ( !mountPoint )
	{
	    logError() << "Can't find mount point for " << dir->url() << Qt::endl;

	    return false;
	}

	const bool doCross = !mountPoint->isSystemMount()  &&	//  /dev, /proc, /sys, ...
			     !mountPoint->isDuplicate()    &&	//  bind mount or multiple mounted
			     !mountPoint->isNetworkMount();	//  NFS or CIFS (Samba)

//        logDebug() << ( doCross ? "Reading" : "Not reading" )
//	             << " mounted filesystem " << mountPoint->path() << Qt::endl;

	return doCross;
    }


    /**
     * Delete all jobs from the given queue, except 'exceptJob'.
     **/
    int killQueue( DirInfo * subtree, QList<DirReadJob *> &queue, const DirReadJob * exceptJob )
    {
	int count = 0;

	QMutableListIterator<DirReadJob *> it( queue );
	while ( it.hasNext() )
	{
	    DirReadJob * job = it.next();

	    if ( exceptJob && job == exceptJob )
	    {
		//logDebug() << "NOT killing " << job << Qt::endl;
	    }
	    else if ( job->dir() && job->dir()->isInSubtree( subtree ) )
	    {
		//logDebug() << "Killing " << job << Qt::endl;
		it.remove();
		delete job;
		++count;
	    }
	}

	return count;
    }

} // namespace


DirReadJob::DirReadJob( DirTree * tree,
			DirInfo * dir  ):
    _tree { tree },
    _dir { dir }
{
    if ( _dir )
	_dir->readJobAdded();
}


DirReadJob::~DirReadJob()
{
    if ( !_tree->beingDestroyed() )
    {
	// Only do this if the tree is not in the process of being destroyed;
	// otherwise all FileInfo / DirInfo pointers pointing into that tree
	// may already be invalid. And even if they are not, it is now
	// pointless to do all the housekeeping stuff to finalize the read job:
	// We'd be beautifying the tree content that is now being destroyed.
	//
	// https://github.com/shundhammer/qdirstat/issues/122

	if ( _dir )
	    _dir->readJobFinished( _dir );
    }
}


/**
 * Default implementation - derived classes should overwrite this method or
 * startReading() (or both).
 **/

void DirReadJob::read()
{
    if ( !_started )
    {
	_started = true;
	startReading();

	// Don't do anything after startReading() - startReading() might call
	// finished() which in turn makes the queue destroy this object
    }
}


void DirReadJob::finished()
{
    if ( _queue )
	_queue->jobFinishedNotify( this );
    else
	logError() << "No job queue for " << _dir << Qt::endl;
}


void DirReadJob::childAdded( FileInfo *newChild )
{
    _tree->childAddedNotify( newChild );
}


void DirReadJob::deletingChild( FileInfo * deletedChild )
{
    _tree->deletingChildNotify( deletedChild );
}






LocalDirReadJob::LocalDirReadJob( DirTree * tree,
				  DirInfo * dir,
				  bool applyFileChildExcludeRules ):
    DirReadJob ( tree, dir ),
    _applyFileChildExcludeRules { applyFileChildExcludeRules }
{
    if ( dir )
	_dirName = dir->url();
}


void LocalDirReadJob::startReading()
{
    static bool _warnedAboutNtfsHardLinks = false;


    struct dirent * entry;
    struct stat     statInfo;
    const QString   defaultCacheName = DEFAULT_CACHE_NAME;
    DIR           * diskDir;

    // logDebug() << dir() << Qt::endl;

    bool ok = true;

    if ( access( _dirName.toUtf8(), X_OK | R_OK ) != 0 )
    {
	ok = false;
	//logWarning() << "No permission to read directory " << _dirName << Qt::endl;
	dir()->finishReading( DirPermissionDenied );
    }

    if ( ok )
    {
	diskDir = ::opendir( _dirName.toUtf8() );

	if ( !diskDir )
	{
	    logWarning() << "opendir(" << _dirName << ") failed" << Qt::endl;
	    ok = false;
	    // opendir() doesn't set 'errno' according to POSIX	 :-(
	    dir()->finishReading( DirError );
	}
    }

    if ( ok )
    {
	dir()->setReadState( DirReading );
	int dirFd = dirfd( diskDir );

#ifdef AT_NO_AUTOMOUNT
	const int flags = AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT;
#else
	const int flags = AT_SYMLINK_NOFOLLOW;
#endif

	// QMultiMap (just like QMap) guarantees sort order by keys, so we are
	// now iterating over the directory entries by i-number order. Most
	// filesystems will benefit from that since they store i-nodes sorted
	// by i-number on disk, so (at least with rotational disks) seek times
	// are minimized by this strategy.
	//
	// We need a QMultiMap, not just a map: If a file has multiple hard links
	// in the same directory, a QMap would store only one of them, all others
	// would go missing in the DirTree.
	QMultiMap<ino_t, QString> entryMap;

	while ( ( entry = readdir( diskDir ) ) )
	{
	    const QString entryName = QString::fromUtf8( entry->d_name );

	    if ( entryName != "."  && entryName != ".."   )
		entryMap.insert( entry->d_ino, entryName );
	}

	for ( const QString & entryName : entryMap )
	{
	    if ( fstatat( dirFd, entryName.toUtf8(), &statInfo, flags ) == 0 )	// OK?
	    {
		if ( S_ISDIR( statInfo.st_mode ) )	// directory child?
		{
		    DirInfo *subDir = new DirInfo( dir(), tree(), entryName, statInfo );
		    CHECK_NEW( subDir );

		    processSubDir( entryName, subDir );

		}
		else  // non-directory child
		{
		    if ( entryName == defaultCacheName )	// .qdirstat.cache.gz found?
		    {
			logDebug() << "Found cache file " << defaultCacheName << Qt::endl;

			// Try to read the cache file. If that was successful and the toplevel
			// path in that cache file matches the path of the directory we are
			// reading right now, the directory is finished reading, the read job
			// (this object) was just deleted, and we may no longer access any
			// member variables; just return.

			if ( readCacheFile( entryName ) )
			    return;
		    }

#if DONT_TRUST_NTFS_HARD_LINKS

                    if ( statInfo.st_nlink > 1 && isNtfs() )
                    {
                        // NTFS seems to return bogus hard link counts; use 1 instead.
                        // See  https://github.com/shundhammer/qdirstat/issues/88

#if !VERBOSE_NTFS_HARD_LINKS
                        if ( !_warnedAboutNtfsHardLinks )
#endif
                        {
                            logWarning() << "Not trusting NTFS with hard links: \""
                                         << dir()->url() << "/" << entryName
                                         << "\" links: " << statInfo.st_nlink
                                         << " -> resetting to 1"
                                         << Qt::endl;
                            _warnedAboutNtfsHardLinks = true;
                        }

                        statInfo.st_nlink = 1;
                    }
#endif
		    FileInfo * child = new FileInfo( dir(), tree(), entryName, statInfo );
		    CHECK_NEW( child );

		    if ( checkIgnoreFilters( entryName ) )
			dir()->addToAttic( child );
		    else
			dir()->insertChild( child );

		    childAdded( child );
		}
	    }
	    else  // lstat() error
	    {
		handleLstatError( entryName );
	    }
	}

	closedir( diskDir );

	//
	// Check all entries against exclude rules that match against any
	// direct non-directory entry.  Don't do this check for the top-level
	// directory.  This is only relevant to the main set of exclude rules;
	// the temporary rules cannot include this type of rule.
	//
	// Doing this now is a performance optimization: This could also be
	// done immediately after each entry is read, but that would mean
	// iterating over all exclude rules for every single directory entry,
	// even if there are no exclude rules that match against any
	// files, so it would be a general performance penalty.
	//
	// Doing this after all entries are read means more cleanup if any
	// exclude rule does match, but that is the exceptional case; if there
	// are no such rules to begin with, the match function returns 'false'
	// immediately, so the performance impact is minimal.

	const bool excludeLate = _applyFileChildExcludeRules &&
				 tree()->excludeRules()->matchDirectChildren( dir() );
	if ( excludeLate )
	    excludeDirLate();

	dir()->finishReading( excludeLate ? DirOnRequestOnly : DirFinished );
    }


    finished();
    // Don't add anything after finished() since this deletes this job!
}


void LocalDirReadJob::processSubDir( const QString & entryName, DirInfo * subDir )
{
    dir()->insertChild( subDir );
    childAdded( subDir );

    if ( matchesExcludeRule( entryName ) )
    {
	// Don't read children of excluded directories, just mark them
	subDir->setExcluded();
	subDir->finishReading( DirOnRequestOnly );
    }
    else if ( !crossingFilesystems( tree(), dir(), subDir ) ) // normal case
    {
	LocalDirReadJob * job = new LocalDirReadJob( tree(), subDir, true );
	CHECK_NEW( job );
	tree()->addJob( job );
    }
    else	    // The subdirectory we just found is a mount point.
    {
	subDir->setMountPoint();

	if ( tree()->crossFilesystems() && shouldCrossIntoFilesystem( subDir ) )
	{
	    LocalDirReadJob * job = new LocalDirReadJob( tree(), subDir, true );
	    CHECK_NEW( job );
	    tree()->addJob( job );
	}
	else
	{
	    subDir->finishReading( DirOnRequestOnly );
	}
    }
}


bool LocalDirReadJob::matchesExcludeRule( const QString & entryName ) const
{
    const QString full = fullName( entryName );

    if ( tree()->excludeRules() && tree()->excludeRules()->match( full, entryName ) )
	return true;

    if ( tree()->tmpExcludeRules() && tree()->tmpExcludeRules()->match( full, entryName ) )
	return true;

    return false;
}


bool LocalDirReadJob::checkIgnoreFilters( const QString & entryName ) const
{
    if ( !tree()->hasFilters() )
	return false;

    return tree()->checkIgnoreFilters( fullName( entryName ) );
}


bool LocalDirReadJob::readCacheFile( const QString & cacheFileName )
{
    const QString cacheFullName = fullName( cacheFileName );
    const bool isToplevel = tree()->isTopLevel( dir() );
    DirInfo * parent = isToplevel ? nullptr : dir()->parent();

    CacheReadJob * cacheReadJob = new CacheReadJob( tree(), dir(), parent, cacheFullName );
    CHECK_NEW( cacheReadJob );

    if ( cacheReadJob->reader() ) // Does this cache file match this directory?
    {
	logDebug() << "Using cache file " << cacheFullName << " for " << _dirName << Qt::endl;

	DirTree * treeLocal = tree();	// Copy data members to local variables:
	DirInfo * dirLocal  = dir();		// This object might be deleted soon by killSubtree()

	if ( isToplevel )
	{
	    //logDebug() << "Clearing complete tree" << Qt::endl;

	    treeLocal->clear();

	    // Since this clears the tree and thus the job queue and thus
	    // deletes this read job, it is important not to do anything after
	    // this point that might access any member variables or even just
	    // uses any virtual method.
	    treeLocal->sendStartingReading();
	}
	else
	{
	    //logDebug() << "Clearing subtree" << dir() << Qt::endl;

	    dirLocal->parent()->setReadState( DirReading );

	    // Clean up partially read directory content
	    queue()->killSubtree( dirLocal, cacheReadJob );	// Will delete this job as well!
	    // All data members of this object are invalid from here on!

	    treeLocal->deleteSubtree( dirLocal );
	}

	treeLocal->addJob( cacheReadJob );     // The job queue will assume ownership of cacheReadJob

	return true;
    }
    else
    {
	logWarning() << "NOT using cache file " << cacheFullName
		     << " for " << _dirName
		     << Qt::endl;

	delete cacheReadJob;

	return false;
    }
}


void LocalDirReadJob::excludeDirLate()
{
    logDebug() << "Excluding dir " << dir() << Qt::endl;

    // Kill all queued jobs for this dir except this one
    queue()->killSubtree( dir(), this );

    tree()->clearSubtree( dir() );
    dir()->setExcluded();
}


void LocalDirReadJob::handleLstatError( const QString & entryName )
{
    logWarning() << "lstat(" << fullName( entryName ) << ") failed: " << formatErrno() << Qt::endl;

    /*
     * Not much we can do when lstat() didn't work; let's at
     * least create an (almost empty) entry as a placeholder.
     */
    DirInfo *child = new DirInfo( dir(), tree(), entryName );
    CHECK_NEW( child );
    child->finalizeLocal();
    child->setReadState( DirError );
    dir()->insertChild( child );
    childAdded( child );
}


QString LocalDirReadJob::fullName( const QString & entryName ) const
{
    const QString dirName = _dirName == "/" ? "" : _dirName;  // Avoid leading // when in root dir

    return dirName + "/" + entryName;
}


bool LocalDirReadJob::checkForNtfs()
{
    _isNtfs         = false;
    _checkedForNtfs = true;

    if ( !_dirName.isEmpty() )
    {
	MountPoint * mountPoint = MountPoints::findNearestMountPoint( _dirName );
	_isNtfs = mountPoint && mountPoint->isNtfs();
    }

    return _isNtfs;
}





CacheReadJob::CacheReadJob( DirTree       * tree,
			    DirInfo       * dir,
			    DirInfo       * parent,
			    const QString & cacheFileName ):
    DirReadJob ( tree, parent ),
    _reader { new CacheReader( cacheFileName, tree, dir, parent ) }
{
    CHECK_NEW( _reader );

    init();
}


CacheReadJob::CacheReadJob( DirTree       * tree,
			    const QString & cacheFileName ):
    DirReadJob ( tree, nullptr ),
    _reader { new CacheReader( cacheFileName, tree ) }
{
    CHECK_NEW( _reader );

    init();
}


void CacheReadJob::init()
{
    if ( !_reader->ok() )
    {
	delete _reader;
	_reader = nullptr;
    }
}


CacheReadJob::~CacheReadJob()
{
    delete _reader;
}


void CacheReadJob::read()
{
    if ( !_reader )
    {
	finished();
	return;
    }

    //logDebug() << "Reading 1000 cache lines" << Qt::endl;
    _reader->read( 1000 );

    if ( _reader->eof() || !_reader->ok() )
    {
	//logDebug() << "Cache reading finished - ok: " << _reader->ok() << Qt::endl;
	finished();
    }
}





DirReadJobQueue::DirReadJobQueue():
    QObject ()
{
    connect( &_timer, &QTimer::timeout,
	     this,    &DirReadJobQueue::timeSlicedRead );
}


DirReadJobQueue::~DirReadJobQueue()
{
    clear();
}


void DirReadJobQueue::enqueue( DirReadJob * job )
{
    if ( job )
    {
	_queue.append( job );
	job->setQueue( this );

	if ( !_timer.isActive() )
	{
	    // logDebug() << "First job queued" << Qt::endl;
//	    emit startingReading();
	    _timer.start( 0 );
	}
    }
}

/*
DirReadJob * DirReadJobQueue::dequeue()
{
    DirReadJob * job = _queue.takeFirst();

    if ( job )
	job->setQueue( nullptr );

    return job;
}
*/

void DirReadJobQueue::clear()
{
    qDeleteAll( _queue );
    qDeleteAll( _blocked );
    _queue.clear();
    _blocked.clear();
}


void DirReadJobQueue::abort()
{
    for ( DirReadJob * job : _queue )
    {
	if ( job->dir() )
	    job->dir()->readJobAborted( job->dir() );
    }

    for ( DirReadJob * job : _blocked )
    {
	if ( job->dir() )
	    job->dir()->readJobAborted( job->dir() );
    }

    clear();
}


void DirReadJobQueue::timeSlicedRead()
{
    if ( _queue.isEmpty() )
	_timer.stop();
    else
	_queue.first()->read();
}


void DirReadJobQueue::jobFinishedNotify( DirReadJob *job )
{
    if ( job )
    {
	// Get rid of the old (finished) job.
	_queue.removeOne( job );
	delete job;
    }

    // The timer will start a new job when it fires.
    if ( _queue.isEmpty() && _blocked.isEmpty() )	// No new job available - we're done.
    {
	logDebug() << "No more jobs - finishing" << Qt::endl;
	emit finished();
    }
}


void DirReadJobQueue::deletingChildNotify( FileInfo * child )
{
    if ( child && child->isDirInfo() )
    {
	logDebug() << "Killing all pending read jobs for " << child << Qt::endl;
	killSubtree( child->toDirInfo() );
    }
}


void DirReadJobQueue::killSubtree( DirInfo * subtree, const DirReadJob * exceptJob )
{
    if ( !subtree )
	return;

    killQueue( subtree, _queue,   exceptJob );
    killQueue( subtree, _blocked, exceptJob );

    //const int count = killQueue( subtree, _queue, exceptJob ) + killQueue( subtree, _blocked, exceptJob );
    //logDebug() << "Killed " << count << " read jobs for " << subtree << Qt::endl;
}


void DirReadJobQueue::unblock( DirReadJob * job )
{
    _blocked.removeAll( job );
    enqueue( job );

    if ( _blocked.isEmpty() )
	logDebug() << "No more jobs waiting for external processes" << Qt::endl;
}
