/*
 *   File name: DirReadJob.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <dirent.h> // opendir(), etc
#include <fcntl.h>  // AT_ constants (fstatat() flags)
#include <unistd.h> // access(), R_OK, X_OK

#include "DirReadJob.h"
#include "DirTree.h"
#include "DirTreeCache.h"
#include "DirInfo.h"
#include "Logger.h"
#include "MountPoints.h"


#define DONT_TRUST_NTFS_HARD_LINKS      1
#define VERBOSE_NTFS_HARD_LINKS         0


using namespace QDirStat;


namespace
{
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

//        logDebug() << ( doCross ? "Reading" : "Not reading" )
//	             << " mounted filesystem " << mountPoint->path() << Qt::endl;

	return doCross;
    }


    /**
     * Handle an error during fstatat() of a directory entry.
     **/
    void handleStatError( const QString & entryName, const QString & fullName, DirInfo * dir, DirTree * tree )
    {
	logWarning() << "lstat(" << fullName << ") failed: " << formatErrno() << Qt::endl;

	/*
	 * Not much we can do when lstat() didn't work; let's at
	 * least create an (almost empty) entry as a placeholder.
	 */
	DirInfo * child = new DirInfo{ dir, tree, entryName };
	child->finalizeLocal();
	child->setReadState( DirError );
	dir->insertChild( child );
	tree->childAddedNotify( child );
    }


    /**
     * Exclude the directory of this read job after it is almost completely
     * read. This is used when checking for exclude rules matching direct
     * file children of a directory.
     *
     * The main purpose of having this as a separate function is to have a
     * clear backtrace if it segfaults.
     **/
    void excludeDirLate( DirReadJobQueue * queue, DirTree * tree, DirInfo * dir, LocalDirReadJob * except )
    {
	logDebug() << "Excluding dir " << dir << Qt::endl;

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
    static bool _warnedAboutNtfsHardLinks = false;

    DIR * diskDir = nullptr;

    // logDebug() << dir() << Qt::endl;

    if ( access( _dirName.toUtf8(), X_OK | R_OK ) != 0 )
    {
	switch ( errno )
	{
	    case EACCES:
		//logWarning() << "No permission to read directory " << _dirName << Qt::endl;
		dir()->finishReading( DirPermissionDenied );
		break;

	    default:
		const QString msg{ "Unable to read directory %1 (errno=%2)" };
		logWarning() << msg.arg( _dirName ).arg( errno ) << Qt::endl;
		dir()->finishReading( DirError );
		break;
	}

    }
    else
    {
	diskDir = ::opendir( _dirName.toUtf8() );

	if ( !diskDir )
	{
	    logWarning() << "opendir(" << _dirName << ") failed" << Qt::endl;
	    dir()->finishReading( DirError );
	}
    }

    if ( diskDir )
    {
	dir()->setReadState( DirReading );

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

	int dirFd = dirfd( diskDir );
	struct dirent * entry;
	while ( ( entry = readdir( diskDir ) ) )
	{
	    const QString entryName = QString::fromUtf8( entry->d_name );
	    if ( entryName != "."_L1 && entryName != ".."_L1 )
		entryMap.insert( entry->d_ino, entryName );
	}

#ifdef AT_NO_AUTOMOUNT
	const int flags = AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT;
#else
	const int flags = AT_SYMLINK_NOFOLLOW;
#endif
	for ( const QString & entryName : asConst( entryMap ) )
	{
	    struct stat statInfo;
	    if ( fstatat( dirFd, entryName.toUtf8(), &statInfo, flags ) == 0 )	// OK
	    {
		if ( S_ISDIR( statInfo.st_mode ) )	// directory child
		{
		    processSubDir( entryName, new DirInfo{ dir(), tree(), entryName, statInfo } );
		}
		else  // non-directory child
		{
		    if ( entryName == QLatin1String{ DEFAULT_CACHE_NAME } )	// .qdirstat.cache.gz found
		    {
			logDebug() << "Found cache file " << DEFAULT_CACHE_NAME << Qt::endl;

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
#if !VERBOSE_NTFS_HARD_LINKS
                        // NTFS seems to return bogus hard link counts; use 1 instead.
                        // See  https://github.com/shundhammer/qdirstat/issues/88
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
		    FileInfo * child = new FileInfo{ dir(), tree(), entryName, statInfo };

		    if ( tree()->checkIgnoreFilters( fullName( entryName ) ) )
			dir()->addToAttic( child );
		    else
			dir()->insertChild( child );

		    tree()->childAddedNotify( child );
		}
	    }
	    else  // fstatat() error
	    {
		handleStatError( entryName, fullName( entryName ), dir(), tree() );
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
    }

    finished();
    // Don't add anything after finished() since this deletes this job!
}


void LocalDirReadJob::processSubDir( const QString & entryName, DirInfo * subDir )
{
    dir()->insertChild( subDir );
    tree()->childAddedNotify( subDir );

    if ( tree()->matchesExcludeRule( fullName( entryName ), entryName ) )
    {
	// Don't read children of excluded directories, just mark them
	subDir->setExcluded();
	subDir->finishReading( DirOnRequestOnly );
    }
    else if ( !DirTree::crossingFilesystems( dir(), subDir ) ) // normal case
    {
	tree()->addJob( new LocalDirReadJob{ tree(), subDir, true } );
    }
    else // The subdirectory we just found is a mount point.
    {
	subDir->setMountPoint();

	if ( tree()->crossFilesystems() && shouldCrossIntoFilesystem( subDir ) )
	    tree()->addJob( new LocalDirReadJob{ tree(), subDir, true } );
	else
	    subDir->finishReading( DirOnRequestOnly );
    }
}


bool LocalDirReadJob::readCacheFile( const QString & cacheFileName )
{
    const QString cacheFullName = fullName( cacheFileName );
    const bool isToplevel = dir() && tree()->root() == dir()->parent();
    DirInfo * parent = isToplevel ? nullptr : dir()->parent();

    CacheReadJob * cacheReadJob = new CacheReadJob{ tree(), dir(), parent, cacheFullName };

    if ( cacheReadJob->reader() ) // Does this cache file match this directory?
    {
	logDebug() << "Using cache file " << cacheFullName << " for " << _dirName << Qt::endl;

	DirTree * treeLocal = tree();	// Copy data members to local variables:
	DirInfo * dirLocal  = dir();	// this object might be deleted soon by killSubtree()

	if ( isToplevel )
	{
	    //logDebug() << "Clearing complete tree" << Qt::endl;

	    // Since this clears the tree and thus the job queue and thus
	    // deletes this read job, it is important not to do anything after
	    // this point that might access any member variables or even just
	    // uses any virtual method.
	    treeLocal->clear();
	    treeLocal->sendStartingReading();
	}
	else
	{
	    //logDebug() << "Deleting subtree " << dirLocal << Qt::endl;

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




QString LocalDirReadJob::fullName( const QString & entryName ) const
{
    // Avoid leading // when in root dir
    if ( _dirName == "/"_L1 )
	return '/' % entryName;

    return _dirName % '/' % entryName;
}


bool LocalDirReadJob::checkForNtfs()
{
    _checkedForNtfs = true;

    if ( !MountPoints::hasNtfs() )
    {
	_isNtfs = false;
    }
    else if ( !_dirName.isEmpty() )
    {
	const MountPoint * mountPoint = MountPoints::findNearestMountPoint( _dirName );
	_isNtfs = mountPoint && mountPoint->isNtfs();
    }

    return _isNtfs;
}




CacheReadJob::CacheReadJob( DirTree       * tree,
                            const QString & cacheFileName ):
    DirReadJob{ tree, nullptr },
    _reader{ new CacheReader{ cacheFileName, tree } }
{
    init();
}


CacheReadJob::CacheReadJob( DirTree       * tree,
                            DirInfo       * dir,
                            DirInfo       * parent,
                            const QString & cacheFileName ):
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
