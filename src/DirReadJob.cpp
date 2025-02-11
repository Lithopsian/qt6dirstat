/*
 *   File name: DirReadJob.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <dirent.h> // opendir(), etc

#include "DirReadJob.h"
#include "DirTree.h"
#include "DirTreeCache.h"
#include "DirInfo.h"
#include "Logger.h"
#include "MountPoints.h"
#include "SysUtil.h"


#define VERBOSE_NTFS_HARD_LINKS 0


using namespace QDirStat;


namespace
{
    /**
     * Return the full name including path of 'entryName' in directory
     * 'dirName', accounting for the leading "/".
     **/
    QString fullName( const QString & dirName, const QByteArray & entryName )
    {
	// Avoid leading // when in root dir
	if ( dirName == "/"_L1 )
	    return '/' + entryName;

	return dirName % '/' % entryName;
    }


    /**
     * Read a cache file that was found in 'dir': if one of the
     * non-directory entries of this directory was named
     * ".qdirstat.cache.gz", open it and, if the toplevel entry in that
     * file matches the current path, read all the cache contents, kill all
     * pending read jobs for subdirectories of this directory and return
     * 'true'. In that case, the current read job is finished and deleted
     * (!).
     *
     * In all other cases, consider that entry as a plain file and return
     * 'false'.
     *
     * Note that the tree, queue, and dir pointers are local copies.  This is
     * important because the parent job will be deleted by this function if
     * the directory is to be populated from the cache file.
     **/
    bool readCacheFile( DirTree         * tree,
                        DirReadJobQueue * queue,
                        DirInfo         * dir,
                        const QString   & dirName,
                        const QString   & cacheFullName )
    {
	const bool isToplevel = dir && tree->root() == dir->parent();
	DirInfo * parent = isToplevel ? nullptr : dir->parent();

	CacheReadJob * cacheReadJob = new CacheReadJob{ tree, dir, parent, cacheFullName };

	if ( cacheReadJob->reader() ) // does this cache file match this directory?
	{
	    logInfo() << "Using cache file " << cacheFullName << " for " << dirName << Qt::endl;

	    if ( isToplevel )
	    {
		//logDebug() << "Clearing complete tree" << Qt::endl;

		// Since this clears the tree and thus the job queue and thus
		// deletes this read job, it is important not to do anything after
		// this point that might access any member variables or even just
		// uses any virtual method.
		tree->clear();
		tree->sendStartingReading();
	    }
	    else
	    {
		//logDebug() << "Deleting subtree " << dirLocal << Qt::endl;

		dir->parent()->setReadState( DirReading );

		// Clean up partially read directory content
		queue->killSubtree( dir, cacheReadJob ); // will delete the parent job as well!

		tree->deleteSubtree( dir );
	    }

	    tree->addJob( cacheReadJob ); // the job queue will assume ownership of cacheReadJob

	    return true;
	}
	else
	{
	    logInfo() << "NOT using cache file " << cacheFullName << " for " << dirName << Qt::endl;

	    delete cacheReadJob;

	    return false;
	}
    }


    /**
     * Check if this directory is on an NTFS mount.  If it is then set the
     * hard link count in 'statInfo' to 1.
     *
     * The ntfs-3g driver may report an incorrect number of hard links.
     * This can be fixed by mounting with option posix_nlink although this
     * may slow down accesses to some directories.
     * See https://github.com/shundhammer/qdirstat/issues/88
     * and https://sourceforge.net/p/ntfs-3g/mailman/message/37070754/
     *
     * To work around this issue, there is a bool setting whether to trust
     * the number of hard links reported for NTFS files.  If they are not
     * trusted, then the number of hard links is always set to 1 even when
     * the driver reports more than 1.  This function is only called if the
     * config setting is to not trust NTFS hard links.
     *
     * The NTFS check is expensive (finding the nearest ancestor that is a
     * mount point is expensive), so the result of this check is cached for
     * all files in this job/directory.
     **/
    IsNtfs handleNtfsHardLinks( IsNtfs             isNtfs,
                                const QString    & dir,
#if VERBOSE_NTFS_HARD_LINKS
                                const QByteArray & name,
#else
                                const QByteArray &,
#endif
                                struct stat      & statInfo )
    {
	if ( isNtfs == NotChecked )
	{
	    if ( !MountPoints::hasNtfs() )
	    {
		isNtfs = NotNtfs;
	    }
	    else if ( !dir.isEmpty() )
	    {
		const MountPoint * mountPoint = MountPoints::findNearestMountPoint( dir );
		isNtfs = mountPoint && mountPoint->isNtfs() ? Ntfs : NotNtfs;
	    }
	}

	if ( isNtfs == Ntfs )
	{
#if VERBOSE_NTFS_HARD_LINKS
	    logWarning() << "Not trusting NTFS hard links for \"" << dir << '/' << name
	                 << "\" links: " << statInfo.st_nlink << " -> resetting to 1"
	                 << Qt::endl;
#endif

	    statInfo.st_nlink = 1;
	}

	return isNtfs;
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

	const bool doCross = !mountPoint->isSystemMount() &&	//  /dev, /proc, /sys, ...
	                     !mountPoint->isDuplicate()   &&	//  bind mount or multiple mounted
	                     !mountPoint->isNetworkMount();	//  NFS or CIFS (Samba)

//	logDebug() << ( doCross ? "Reading" : "Not reading" )
//	           << " mounted filesystem " << mountPoint->path() << Qt::endl;

	return doCross;
    }


    /**
     * Process the directory 'entryName'.  This does late exclude (match any
     * child) checking, adds a new LocalDirReadJob if not crossing to a
     * different filesystem or if crossing is configured, and finishes this
     * job.
     **/
    void processSubDir( DirTree       * tree,
                        DirInfo       * dir,
                        const QString & entryName,
                        const QString & fullName,
                        struct stat   & statInfo )
    {
	DirInfo * subDir = new DirInfo{ dir, tree, entryName, statInfo };
	dir->insertChild( subDir );
	tree->childAddedNotify( subDir );

	if ( tree->matchesExcludeRule( fullName, entryName ) )
	{
	    // Don't read children of excluded directories, just mark them
	    subDir->setExcluded();
	    subDir->finishReading( DirOnRequestOnly );
	}
	else if ( !DirTree::crossingFilesystems( dir, subDir ) ) // normal case
	{
	    tree->addJob( new LocalDirReadJob{ tree, subDir, true } );
	}
	else // The subdirectory we just found is a mount point.
	{
	    subDir->setMountPoint();

	    if ( tree->crossFilesystems() && shouldCrossIntoFilesystem( subDir ) )
		tree->addJob( new LocalDirReadJob{ tree, subDir, true } );
	    else
		subDir->finishReading( DirOnRequestOnly );
	}
    }


    /**
     * Handle an error during fstatat() of a directory entry.
     **/
    void handleStatError( const QString & entryName, const QString & fullName, DirInfo * dir, DirTree * tree )
    {
	if ( errno != EACCES )
	    logWarning() << "fstatat(" << fullName << ") failed: " << formatErrno() << Qt::endl;

	/*
	 * Not much we can do when fstatat() didn't work; just
	 * create an (almost empty) entry as a placeholder
	 */
	DirInfo * child = new DirInfo{ dir, tree, entryName };
	child->finalizeLocal();
	child->setReadError( errno == EACCES ? DirNoAccess : DirError );
	dir->insertChild( child );
	tree->childAddedNotify( child );
    }


    /**
     * Exclude the directory of this read job after it has been read.  This is
     * used when checking for exclude rules matching direct file children of a
     * directory. The check is made after all children have been read and
     * created as children of 'dir'.
     **/
    void excludeDirLate( DirReadJobQueue * queue, DirTree * tree, DirInfo * dir, LocalDirReadJob * except )
    {
	//logDebug() << "Excluding dir " << dir << Qt::endl;

	// Kill all queued jobs for this dir except 'except'
	queue->killSubtree( dir, except );

	tree->clearSubtree( dir );
	dir->setExcluded();
    }

} // namespace


DirReadJob::DirReadJob( DirTree * tree, DirInfo * dir  ):
    _tree{ tree },
    _dir{ dir }
{
    if ( _dir )
	_dir->readJobAdded();
}


DirReadJob::~DirReadJob()
{
    if ( _dir && _dir->checkMagicNumber() )
	_dir->readJobFinished( _dir );
}


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




LocalDirReadJob::LocalDirReadJob( DirTree * tree,
                                  DirInfo * dir,
                                  bool      applyFileChildExcludeRules ):
    DirReadJob{ tree, dir },
    _applyFileChildExcludeRules{ applyFileChildExcludeRules }
{
    if ( dir )
	_dirName = dir->url();
}


void LocalDirReadJob::startReading()
{
    // logDebug() << dir() << Qt::endl;

    // Directories without 'x' permission can be opened here, but stat will fail on the contents
    DIR * diskDir = opendir( _dirName.toUtf8() );
    if ( !diskDir )
    {
	switch ( errno )
	{
	    case EACCES:
		//logWarning() << "No permission to read directory " << _dirName << Qt::endl;
		dir()->finishReading( DirPermissionDenied );
		break;

	    default:
		const QString msg{ "Unable to read directory %1: %2" };
		logWarning() << msg.arg( _dirName, formatErrno() ) << Qt::endl;
		dir()->finishReading( DirError );
		break;
	}

	finished();
	// Don't add anything after finished() since this deletes this job!
	return;
    }
    int dirFd = dirfd( diskDir );

    dir()->setReadState( DirReading );

    // QMultiMap (just like QMap) guarantees sort order by keys, so we are
    // now iterating over the directory entries by i-number order. Most
    // filesystems will benefit from that since they store i-nodes sorted
    // by i-number on disk, so (at least with rotational disks) seek times
    // are minimized by this strategy.
    //
    // We need a QMultiMap, not just a map: if a file has multiple hard links
    // in the same directory, a QMap would store only one of them, all others
    // would go missing in the DirTree.
    QMultiMap<ino_t, QByteArray> entryMap;
    struct dirent * entry;
    while ( ( entry = readdir( diskDir ) ) )
    {
	const QByteArray entryName = entry->d_name;
	if ( entryName != "." && entryName != ".." )
	    entryMap.insert( entry->d_ino, entryName );
    }

    for ( const QByteArray & entryName : asConst( entryMap ) )
    {
	const QString fullEntryName = fullName( _dirName, entryName );

	struct stat statInfo;
	if ( SysUtil::stat( dirFd, entryName, statInfo ) == 0 ) // OK
	{
	    if ( S_ISDIR( statInfo.st_mode ) ) // directory child
	    {
		processSubDir( tree(), dir(), entryName, fullEntryName, statInfo );
	    }
	    else  // non-directory child
	    {
		if ( entryName == DEFAULT_CACHE_NAME ) // .qdirstat.cache.gz found
		{
		    //logDebug() << "Found cache file " << DEFAULT_CACHE_NAME << Qt::endl;

		    // Try to read the cache file. If that was successful and the toplevel
		    // path in that cache file matches the path of the directory we are
		    // reading right now, the directory is finished reading, the read job
		    // (this object) was just deleted, and we may no longer access any
		    // member variables; just return.
		    if ( readCacheFile( tree(), queue(), dir(), _dirName, fullEntryName ) )
			return;
		}

		if ( statInfo.st_nlink > 1 && !tree()->trustNtfsHardLinks() )
		    _isNtfs = handleNtfsHardLinks( _isNtfs, _dirName, entryName, statInfo );

		FileInfo * child = new FileInfo{ dir(), tree(), entryName, statInfo };

		if ( tree()->checkIgnoreFilters( fullEntryName ) )
		    dir()->addToAttic( child );
		else
		    dir()->insertChild( child );

		tree()->childAddedNotify( child );
	    }
	}
	else // fstatat() error
	{
	    handleStatError( entryName, fullEntryName, dir(), tree() );
	}
    }

    closedir( diskDir );

    // Check all entries against exclude rules that match against any
    // direct non-directory entry.  Don't do this check for the top-level
    // directory.  This is only relevant to the main set of exclude rules;
    // the temporary rules cannot include this type of rule.
    //
    // Doing this after all entries are read means more cleanup if any
    // exclude rule does match, but that is the exceptional case; if there
    // are no such rules to begin with, the match function returns 'false'
    // immediately, so the performance impact is minimal.
    const bool excludeLate = _applyFileChildExcludeRules && tree()->matchesDirectChildren( dir() );
    if ( excludeLate )
	excludeDirLate( queue(), tree(), dir(), this );

    dir()->finishReading( excludeLate ? DirOnRequestOnly : DirFinished );

    finished();
    // Don't add anything after finished() since this deletes this job!
}




CacheReadJob::CacheReadJob( DirTree * tree, const QString & cacheFileName ):
    DirReadJob{ tree, nullptr },
    _reader{ new CacheReader{ cacheFileName, tree } }
{
    init();
}


CacheReadJob::CacheReadJob( DirTree * tree, DirInfo * dir, DirInfo * parent, const QString & cacheFileName ):
    DirReadJob{ tree, parent },
    _reader{ new CacheReader{ cacheFileName, tree, dir, parent } }
{
    init();
}


void CacheReadJob::init()
{
    if ( !_reader->ok() )
	_reader.reset( nullptr );
}


void CacheReadJob::read()
{
    if ( _reader )
    {
	//logDebug() << "Reading 1000 cache lines" << Qt::endl;
	_reader->read( 1000 );
	if ( _reader->ok() && !_reader->eof() )
	    return;
    }

    finished();
}
