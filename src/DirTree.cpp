/*
 *   File name: DirTree.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QDir>
#include <QFileInfo>

#include "DirTree.h"
#include "Attic.h"
#include "DirTreeCache.h"
#include "DirTreeFilter.h"
#include "Exception.h"
#include "ExcludeRules.h"
#include "FileInfoIterator.h"
#include "FileInfoSet.h"
#include "FormatUtil.h"
#include "MountPoints.h"
#include "PkgFilter.h"
#include "PkgReader.h"
#include "SysUtil.h"


#define VERBOSE_EXCLUDE_RULES 0


using namespace QDirStat;


namespace
{
    /**
     * For NTFS mounts, log a message about how hard links are being handled
     **/
    void processMount( const MountPoint * mountPoint, bool trustNtfsHardLinks )
    {
	// Log a message for NTFS mounts about how hard links are being handled
	if ( mountPoint->isNtfs() )
	{
	    if ( trustNtfsHardLinks )
		logInfo() << "Trusting NTFS hard link counts: edit settings file to change" << Qt::endl;
	    else
		logWarning() << "Not trusting NTFS hard link counts: always assume 1" << Qt::endl;
	}
    }


    /**
     * Obtain information about the URL specified and create a new FileInfo
     * or a DirInfo (whatever is appropriate) from that information. Use
     * FileInfo::isDirInfo() to find out which.
     *
     * If the underlying syscall fails, this throws a SysCallException.
     **/
    FileInfo * createItem( const QString & url, DirTree * tree, DirInfo * parent )
    {
	// logDebug() << "url: \"" << url << '"' << Qt::endl;

	struct stat statInfo;
	if ( SysUtil::stat( url, statInfo ) != 0 ) // fstatat() failed
	    THROW( ( SysCallFailedException{ "fstatat", url } ) );

	const bool isRoot = parent == tree->root();
	const QString name = isRoot ? url : SysUtil::baseName( url );

	if ( !S_ISDIR( statInfo.st_mode ) ) // not directory
	{
	    FileInfo * file = new FileInfo{ parent, tree, name, statInfo };
	    parent->insertChild( file );

	    return file;
	}

	DirInfo * dir = new DirInfo{ parent, tree, name, statInfo };
	parent->insertChild( dir );

	if ( !isRoot && !parent->isPkgInfo() && DirTree::crossingFilesystems( parent, dir ) )
	    dir->setMountPoint();

	return dir;
    }

#if 0
    /**
     * Recursively force a complete recalculation of all sums.
     *
     * Redundant now, fingers crossed.
     **/
    void recalc( DirInfo * dir )
    {
	for ( DirInfoIterator it{ dir }; *it; ++it )
	    recalc( *it );

	if ( dir->dotEntry() )
	    recalc( dir->dotEntry() );

	if ( dir->attic() )
	    recalc( dir->attic() );

	dir->recalc();
    }
#endif

    /**
     * Find the ancestor of an ignored FileInfo item that is the
     * direct child of the attic.  Only direct children of an attic
     * can be unattic'd.
     **/
    FileInfo * findAtticChild( FileInfo * item )
    {
	for ( FileInfo * toplevel = item; toplevel->parent(); toplevel = toplevel->parent() )
	{
	    if ( toplevel->parent()->isAttic() )
		return toplevel;
	}

	return nullptr;
    }


    /**
     * Move one ignored item out of the attic.  This is done before
     * refreshing to ensure that unignored items (after the refresh)
     * aren't stranded in the attic.  The whole subtree from the
     * attic level down has to be unattic'd.  It, or at least anything
     * that is still ignored after the refresh, will get moved back
     * again by finalizeTree().
     **/
    void unatticOne( DirTree * tree, FileInfo * atticChild )
    {
	Attic * attic = atticChild->parent()->toAttic();
	DirInfo * parent = attic->parent();
	if ( parent ) // an attic should always have a parent
	{
	    emit tree->deletingChildren( attic, FileInfoSet{ atticChild } );
	    attic->unlinkChild( atticChild );
	    emit tree->childrenDeleted();
	    atticChild->setIgnored( false );

	    // Pretend to be clearing all the children, whether we delete anything or not
	    // This will keep the model away while we mess with the tree
	    emit tree->clearingSubtree( parent );
	    parent->deleteEmptyAttic();
	    emit tree->subtreeCleared();

	    // Notify the model of all the parent rows as if they were new
	    parent->insertChild( atticChild );
	    emit tree->readJobFinished( parent );
	}
    }


    /**
     * Move all items from any attics below a directory into the attic parent and
     * remove the emptied attics.  This is done when a directory has been moved
     * into an attic, so any attics within it are redundant.
     **/
    void unatticAll( FileInfo * item )
    {
	if ( !item->isDirInfo() )
	    return;

	DirInfo * dir = item->toDirInfo();
	if ( dir->attic() )
	    dir->moveAllFromAttic();

	for ( DotEntryIterator it{ item }; it != nullptr ; ++it )
	    unatticAll( *it );
    }


    /**
     * Recurse through the tree from 'dir' on and move any ignored items to
     * the attic on the same level.
     **/
    void moveIgnoredToAttic( DirInfo * dir )
    {
	if ( !dir )
	    return;

	DotEntryIterator it{ dir };
	while ( *it )
	{
	    FileInfo * child = *it;

	    ++it; // before we re-home that child

	    if ( child->isIgnored() )
	    {
		// Move ignored items to an attic (created if necessary) at this level
		dir->moveToAttic( child );

		// Empty and delete any attics that have been moved under this attic
		unatticAll( child );
	    }
	    else // stop recursing when we've found an ignored item
	    {
		moveIgnoredToAttic( child->toDirInfo() ); // recurse away
	    }
	}
    }


    /**
     * Recurse through the tree from 'dir' on and ignore any empty dirs
     * (i.e. dirs without any unignored non-directory child) that are not
     * ignored yet.
     **/
    void ignoreEmptyDirs( DirInfo * dir )
    {
	for ( auto item : dir )
	{
	    if ( !item->isIgnored() && item->isDirInfo() )
	    {
		DirInfo * subDir = item->toDirInfo();

		if ( subDir->totalUnignoredItems() == 0 )
		{
		    //logDebug() << "Ignoring empty subdir " << subDir << Qt::endl;
		    subDir->setIgnored( true );
		}

		ignoreEmptyDirs( subDir );
	    }
	}
    }


    /**
     * Recurse through the tree from 'dir' on and ignore any empty dirs
     **/
    void createLocalDirReadJob( DirTree * tree, FileInfo * item )
    {
	tree->addJob( new LocalDirReadJob{ tree, item->toDirInfo(), false } );
    }

} // namespace


DirTree::DirTree( QObject * parent ):
    QObject{ parent },
    _root{ new DirInfo{ this } }
{
    setExcludeRules();

    connect( &_jobQueue, &DirReadJobQueue::finished,
             this,       &DirTree::sendFinished );

    connect( this,       &DirTree::deletingChild,
             &_jobQueue, &DirReadJobQueue::deletingChildNotify );
}


DirTree::~DirTree()
{
    clearFilters();
}


FileInfo * DirTree::firstToplevel() const
{
    if ( !_root )
	return nullptr;

    FileInfo * result = _root->firstChild();

    if ( !result )
	result = _root->attic();

    if ( !result )
	result = _root->dotEntry();

    return result;
}

/*
bool DirTree::isToplevel( const FileInfo * item ) const
{
    return item && isRoot( item->parent() );
}
*/

void DirTree::clear()
{
    _jobQueue.clear();

    _url.clear();
    if ( _root )
    {
	emit clearing();
	_root->clear();
	emit cleared();
    }

    _isBusy           = false;
    _blocksPerCluster = -1;
}


void DirTree::startReading( const QString & rawUrl )
{
    //logDebug() << "rawUrl: \"" << rawUrl << '"' << Qt::endl;

    _url = [ &rawUrl ]()
    {
	const QFileInfo fileInfo{ rawUrl };

	if ( fileInfo.isDir() ) // return the input path, just canonicalised
	    return fileInfo.canonicalFilePath();

	if ( fileInfo.exists() ) // return the parent directory for any existing non-directory
	    return fileInfo.canonicalPath();

	if ( fileInfo.isSymLink() ) // symlink target doesn't exist, return symlink parent directory
	    return QFileInfo{ fileInfo.absolutePath() }.canonicalFilePath();

	return fileInfo.absoluteFilePath(); // return nonexistent input file which should throw
    }();

    // Find the nearest ancestor that is a mount point and log its details
    const MountPoint * mountPoint = MountPoints::findNearestMountPoint( _url );
    logInfo() << "url:    " << _url << Qt::endl;
    logInfo() << "device: " << ( mountPoint ? mountPoint->device() : QString{} ) << Qt::endl;
    processMount( mountPoint, _trustNtfsHardLinks );

    sendStartingReading();

    FileInfo * item = createItem( _url, this, root() );
    if ( item ) // should always be an item, will throw if there is an error
    {
	childAddedNotify( item );

	if ( item->isDirInfo() )
	    createLocalDirReadJob( this, item );
	else
	    sendFinished();

	emit readJobFinished( root() );
    }
}


void DirTree::refresh( const FileInfoSet & refreshSet )
{
    if ( !_root )
	return;

    // Make a list of items that are still accessible in the real world
    FileInfoSet items;
    for ( FileInfo * item : refreshSet )
    {
	// During a refresh, some items may already have been deleted
	if ( item && item->checkMagicNumber() )
	{
	    // Check the item is still accessible on disk
	    // Pseudo-dirs (shouldn't be here) will implicitly fail the check
	    while ( faccessat( AT_FDCWD, item->url().toUtf8(), F_OK, AT_SYMLINK_NOFOLLOW ) != 0 )
	    {
		if ( item == root() || item->parent() == root() )
		{
		    // just try a full refresh, it will throw if even that isn't accessible any more
		    //logDebug() << item->parent() << " " << _root << Qt::endl;
		    refresh( item->toDirInfo() );
		    return;
		}

		// Desperately try the parent of items that no longer exist
		item = item->parent();
	    }

	    // Add an item that we have full access to
	    items << item;
	}
    }

    // Refresh the subtrees that we have left
    const auto normalizedItems = items.normalized();
    for ( FileInfo * item : normalizedItems )
    {
	// Need to check the magic number here again because a previous
	// iteration step might have made the item invalid already
	if ( item->checkMagicNumber() )
	{
	    if ( item->isDirInfo() )
		refresh( item->toDirInfo() );
	    else if ( item->parent() )
		refresh( item->parent() );
	}
    }
}


void DirTree::refresh( DirInfo * subtree )
{
    if ( subtree->isPseudoDir() )
	subtree = subtree->parent();

    if ( subtree == root() || subtree->parent() == root() )
    {
	// Refresh all (from first toplevel)
	clearSubtree( root() ); // clears the contents, but not _url, filters, etc.
	startReading( QDir::cleanPath( _url ) );
    }
    else // refresh subtree with a parent
    {
	// If this is an NTFS mountpoint, log a message about it
	const MountPoint * mountPoint = MountPoints::findByPath( subtree->path() );
	if ( mountPoint )
	    processMount( mountPoint, _trustNtfsHardLinks );

	// A full startingReading signal would reset all the tree branches to level 1
	emit startingRefresh();
	_isBusy = true;

	//logDebug() << "Refreshing subtree " << subtree << Qt::endl;

	// Take ignored subtrees out of the attic before refreshing
	// This takes care of items that become unignored and empty attics
	if ( subtree->isIgnored() )
	    unatticOne( this, findAtticChild( subtree ) );

	// Make copies of some key information before the objects are deleted
	const QString url = subtree->url();
	DirInfo * parent = subtree->parent(); // the parent can't be an attic now

	deleteSubtree( subtree );

	// Recreate the deleted subtree
	FileInfo * item = createItem( url, this, parent );
	if ( item ) // should always be an item, stat() will throw if there is an error
	{
	    childAddedNotify( item );
	    createLocalDirReadJob( this, item );
	    emit readJobFinished( parent );
	}
    }
}


void DirTree::abortReading()
{
    if ( _jobQueue.isEmpty() )
	return;

    _jobQueue.abort();

    _isBusy = false;
    emit aborted();
}


void DirTree::finalizeTree()
{
    if ( _root && hasFilters() )
    {
	ignoreEmptyDirs( root() );
	if ( _root->firstChild() )
	    moveIgnoredToAttic( _root->firstChild()->toDirInfo() );
    }
}


void DirTree::childAddedNotify( FileInfo * newChild )
{
    if ( !haveClusterSize() && newChild && newChild->fileWithOneCluster() )
    {
	_blocksPerCluster = newChild->blocks();

	logInfo() << "Cluster size: " << _blocksPerCluster << " blocks ("
	          << formatSize( clusterSize() ) << ")" << Qt::endl;
//	logDebug() << "Derived from " << newChild << " " << formatSize( newChild->rawByteSize() )
//	           << " (allocated: " << formatSize( newChild->rawAllocatedSize() ) << ")"
//	           << Qt::endl;
    }

//    emit childAdded( newChild ); // nobody listening for this

//    if ( newChild->dotEntry() )
//	emit childAdded( newChild->dotEntry() );
}


void DirTree::deleteChild( FileInfo * child )
{
    //logDebug() << "Deleting " << child << Qt::endl;

    // Send notification to anybody interested (e.g. SelectionModel)
    emit deletingChild( child );

    DirInfo * parent = child->parent();

    // Give the parent of the child to be deleted a chance to unlink the child
    // from its children list and take care of internal summary fields
    if ( parent )
	parent->unlinkChild( child );

    delete child;
}


void DirTree::deleteSubtree( DirInfo * subtree )
{
    emit deletingChildren( subtree->parent(), FileInfoSet{ subtree } );
    deleteChild( subtree );
    emit childrenDeleted();
}


void DirTree::deleteSubtrees( const FileInfoSet & subtrees )
{
    // Don't do anything if a read is in progress or gets started
    if ( _isBusy )
	return;

    // Create a map to group the items by parent
    QMultiMap<DirInfo *, FileInfo *>parentMap;
    for ( FileInfo * subtree : subtrees )
    {
	// Check if the item has already been deleted, by us or someone else
	if ( subtree->checkMagicNumber() )
	{
	    DirInfo * parent = subtree->parent();
	    if ( parent )
		parentMap.insert( parent, subtree );
	}
    }

    // Loop over the parents
    const auto parents = parentMap.uniqueKeys();
    for ( DirInfo * parent : parents )
    {
	// Collect all the children of this parent and delete them together
	const FileInfoSet childrenOfOneParent = [ parent, &parentMap ]()
	{
	    FileInfoSet children;
	    const auto range = asConst( parentMap ).equal_range( parent );
	    for ( auto it = range.first; it != range.second; ++it )
		children << it.value();
	    return children;
	}();

	emit deletingChildren( parent, childrenOfOneParent );
	for ( FileInfo * child : childrenOfOneParent )
	    deleteChild( child );
	emit childrenDeleted();

	// If that was the last child of a dot entry
	if ( parent && parent->isDotEntry() && !parent->hasChildren() && parent->parent()->isFinished() )
	{
	    //logDebug() << "Removing empty dot entry " << parent << Qt::endl;
	    deleteSubtree( parent );
	}
    }
}


void DirTree::clearSubtree( DirInfo * subtree )
{
    if ( subtree->hasChildren() )
    {
	emit clearingSubtree( subtree );
	subtree->clear();
	emit subtreeCleared();
    }
}


void DirTree::sendStartingReading()
{
    _isBusy = true;
    emit startingReading();
}


void DirTree::sendFinished()
{
    finalizeTree();
    _isBusy = false;
    emit finished();
}

/*
void DirTree::sendAborted()
{
    _isBusy = false;
    emit aborted();
}


void DirTree::sendStartingReading( DirInfo * dir )
{
    emit dirStartingReading( dir );
}
*/

void DirTree::sendReadJobFinished( DirInfo * dir )
{
    // logDebug() << dir << Qt::endl;
    emit readJobFinished( dir );
}


FileInfo * DirTree::locate( const QString & url ) const
{
    // Search from the top of the tree
    if ( _root )
	return _root->locate( url );

    // Should never get here, there is always _root
    return nullptr;
}


bool DirTree::writeCache( const QString & cacheFileName )
{
    CacheWriter writer{ cacheFileName, this };
    return writer.ok();
}


bool DirTree::readCache( const QString & cacheFileName )
{
    CacheReadJob * readJob = new CacheReadJob{ this, cacheFileName };

    if ( !readJob->reader() )
    {
	delete readJob;
//	emit aborted();
	return false;
    }

    sendStartingReading();
    addJob( readJob );
    return true;
}


void DirTree::readPkg( const PkgFilter & pkgFilter )
{
    _url = pkgFilter.url();

    sendStartingReading();

    // logDebug() << "Reading " << pkgFilter << Qt::endl;
    PkgReader reader{ this, pkgFilter };
}


void DirTree::setExcludeRules()
{
    _excludeRules.reset( new ExcludeRules{} );
}


void DirTree::setTmpExcludeRules( const ExcludeRules * newTmpRules )
{
#if VERBOSE_EXCLUDE_RULES
    if ( newTmpRules )
    {
	logDebug() << "New tmp exclude rules:" << Qt::endl;

	for ( const ExcludeRule * excludeRule : *newTmpRules )
	    logDebug() << excludeRule << Qt::endl;
    }
    else
    {
	logDebug() << "Clearing tmp exclude rules" << Qt::endl;
    }
#endif

    _tmpExcludeRules.reset( newTmpRules );
}


bool DirTree::matchesExcludeRule( const QString & fullName, const QString & entryName ) const
{
    if ( _excludeRules && _excludeRules->match( fullName, entryName ) )
	return true;

    if ( _tmpExcludeRules && _tmpExcludeRules->match( fullName, entryName ) )
	return true;

    return false;
}


bool DirTree::matchesDirectChildren( const DirInfo * dir ) const
{
    if ( _excludeRules && _excludeRules->matchDirectChildren( dir ) )
	return true;

    return false;
}


void DirTree::clearFilters()
{
    qDeleteAll( _filters );
    _filters.clear();
}


bool DirTree::checkIgnoreFilters( const QString & path ) const
{
    for ( const DirTreeFilter * filter : asConst( _filters ) )
    {
	if ( filter->ignore( path ) )
	    return true;
    }

    return false;
}


void DirTree::setIgnoreHardLinks( bool ignore )
{
    if ( ignore )
	logInfo() << "Ignoring hard links" << Qt::endl;

    _ignoreHardLinks = ignore;
}


bool DirTree::crossingFilesystems( const DirInfo * parent, const DirInfo * child )
{
    /**
     * Return the device name that 'dir' is on if it's a mount point.
     * This uses MountPoints which reads /proc/mounts.
     **/
    const auto device = []( const DirInfo * dir ) { return MountPoints::device( dir->url() ); };

    // If the device numbers match then we're definitely not crossing
    if ( parent->device() == child->device() )
	return false;

    // See if there is an entry in the mountpoint list for 'child'
    const QString childDevice  = device( child );
    if ( childDevice.isEmpty() )
	return false;

    // Compare to the parent device name to eliminate mountpoints on the same device (eg. Btrfs sub-volumes)
    const QString parentDevice = device( parent->findNearestMountPoint() );
    const bool crossing = !parentDevice.isEmpty() && parentDevice != childDevice;
    if ( crossing )
	logInfo() << "Filesystem boundary at mount point " << child << " on device " << childDevice << Qt::endl;
    else
	logInfo() << "Mount point " << child << " is still on the same device " << childDevice << Qt::endl;

    return crossing;
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
	    _timer.start( 0 );
	}
    }
}


void DirReadJobQueue::clear()
{
    qDeleteAll( _queue );
    _queue.clear();
    _queue.squeeze();

    qDeleteAll( _blocked );
    _blocked.clear();
    _blocked.squeeze();
}


void DirReadJobQueue::abort()
{
    for ( const DirReadJob * job : asConst( _queue ) )
    {
	if ( job->dir() )
	    job->dir()->readJobAborted();
    }

    for ( const DirReadJob * job : asConst( _blocked ) )
    {
	if ( job->dir() )
	    job->dir()->readJobAborted();
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


void DirReadJobQueue::jobFinishedNotify( DirReadJob * job )
{
    if ( job )
    {
	// Get rid of the old (finished) job.
	_queue.removeOne( job );
	delete job;
    }

    if ( _queue.isEmpty() && _blocked.isEmpty() )
    {
	// The timer will fire again and then stop itself
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

    /**
     * Delete all jobs within 'subtree' from the given queue, except 'exceptJob'.
     **/
    const auto killQueue = [ subtree, exceptJob ]( DirReadJobList & queue )
    {
	DirReadJobList newQueue;
	for ( DirReadJob * job : asConst( queue ) )
	{
	    if ( job->dir() && job->dir()->isInSubtree( subtree ) && ( !exceptJob || job != exceptJob ) )
		delete job;
	    else
		newQueue << job;
	}
	newQueue.swap( queue );
    };

    killQueue( _queue );
    killQueue( _blocked );
}


void DirReadJobQueue::unblock( DirReadJob * job )
{
    _blocked.removeAll( job );
    enqueue( job );

//    if ( _blocked.isEmpty() )
//	logDebug() << "No more jobs waiting for external processes" << Qt::endl;
}
