/*
 *   File name: DirTree.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <sys/stat.h>

#include <QDir>
#include <QFileInfo>

#include "DirTree.h"
#include "Attic.h"
#include "DirTreeCache.h"
#include "DirTreeFilter.h"
#include "DotEntry.h"
#include "ExcludeRules.h"
#include "FileInfo.h"
#include "FileInfoIterator.h"
#include "FileInfoSet.h"
#include "FormatUtil.h"
#include "MountPoints.h"
#include "PkgFilter.h"
#include "PkgReader.h"
#include "SysUtil.h"
#include "Logger.h"
#include "Exception.h"


#define VERBOSE_EXCLUDE_RULES 0


using namespace QDirStat;


namespace
{
    /**
     * Obtain information about the URL specified and create a new FileInfo
     * or a DirInfo (whatever is appropriate) from that information. Use
     * FileInfo::isDirInfo() to find out which.
     *
     * If the underlying syscall fails, this throws a SysCallException if
     * 'doThrow' is 'true', and it just returns 0 if it is 'false'.
     **/
    FileInfo * stat( const QString & url,
		     DirTree       * tree,
		     DirInfo       * parent )
    {
	struct stat statInfo;
	// logDebug() << "url: \"" << url << "\"" << Qt::endl;

	if ( lstat( url.toUtf8(), &statInfo ) == 0 ) // lstat() OK
	{
	    QString name = url;

	    if ( parent && parent != tree->root() )
		name = SysUtil::baseName( url );

	    if ( S_ISDIR( statInfo.st_mode ) )	// directory
	    {
		DirInfo * dir = new DirInfo( parent, tree, name, statInfo );
		CHECK_NEW( dir );

		if ( parent )
		{
		    parent->insertChild( dir );

		    // Get the real parent for comparing device numbers, in case we're in an Attic
		    if ( parent->isAttic() )
			parent = parent->parent();
		    if ( parent &&
			 parent != tree->root() &&
			 !parent->isPkgInfo() &&
			 !parent->isFromCache() &&
			 dir->device() != parent->device() )
		    {
			logDebug() << dir << " is a mount point under " << parent << Qt::endl;
			dir->setMountPoint();
		    }
		}

		return dir;
	    }
	    else				// not directory
	    {
		FileInfo * file = new FileInfo( parent, tree, name, statInfo );
		CHECK_NEW( file );

		if ( parent )
		    parent->insertChild( file );

		return file;
	    }
	}
	else // lstat() failed
	{
	    THROW( SysCallFailedException( "lstat", url ) );
	}

	return nullptr;
    }


    /**
     * Move all items from the attic to the normal children list.
     **/
    void unatticAll( DirInfo * dir )
    {
	if ( dir->attic() )
	{
	    //logDebug() << "Moving all attic children to the normal children list for " << dir << Qt::endl;
	    dir->takeAllChildren( dir->attic() );
	    dir->deleteEmptyAttic();
	    dir->recalc();
	}

	for ( FileInfoIterator it( dir ); *it; ++it )
	{
	    if ( (*it)->isDirInfo() )
		unatticAll( (*it)->toDirInfo() );
	}
    }


    /**
     * Recurse through the tree from 'dir' on and move any ignored items to
     * the attic on the same level.
     **/
    void moveIgnoredToAttic( DirInfo * dir )
    {
	if ( !dir )
	    return;

	const FileInfoList ignoredChildren = [ dir ]()
	{
	    FileInfoList ignoredChildren;

	    for ( FileInfoIterator it( dir ); *it; ++it )
	    {
		FileInfo * child = *it;
		if ( child->isIgnored() )
		    ignoredChildren << child;
		else
		    moveIgnoredToAttic( child->toDirInfo() );
	    }

	    return ignoredChildren;
	}();

	for ( FileInfo * child : ignoredChildren )
	{
	    //logDebug() << "Moving ignored " << child << " to attic" << Qt::endl;
	    dir->moveToAttic( child );

	    if ( child->isDirInfo() )
		unatticAll( child->toDirInfo() );
	}

	if ( !ignoredChildren.isEmpty() )
	{
	    dir->recalc();

	    if ( dir->attic() )
		dir->attic()->recalc();
	}
    }


    /**
     * Recurse through the tree from 'dir' on and ignore any empty dirs
     * (i.e. dirs without any unignored non-directory child) that are not
     * ignored yet.
     **/
    void ignoreEmptyDirs( DirInfo * dir )
    {
	FileInfo * child = dir->firstChild();
	FileInfoList ignoredChildren;

	while ( child )
	{
	    if ( !child->isIgnored() && child->isDirInfo() )
	    {
		DirInfo * subDir = child->toDirInfo();

		if ( subDir->totalUnignoredItems() == 0 )
		{
		    //logDebug() << "Ignoring empty subdir " << subDir << Qt::endl;
		    subDir->setIgnored( true );
		}

		ignoreEmptyDirs( subDir );
	    }

	    child = child->next();
	}
    }


    /**
     * Recurse through the tree from 'dir' on and ignore any empty dirs
     **/
    void createLocalDirReadJob( DirTree * tree, FileInfo * item )
    {
	LocalDirReadJob * job = new LocalDirReadJob( tree, item->toDirInfo(), false );
	CHECK_NEW( job );
	tree->addJob( job );
    }

} // namespace


DirTree::DirTree():
    QObject (),
    _root { new DirInfo( this ) }
{
    CHECK_NEW( _root );

    connect( &_jobQueue, &DirReadJobQueue::finished,
	     this,       &DirTree::sendFinished );

    connect( this,       &DirTree::deletingChild,
	     &_jobQueue, &DirReadJobQueue::deletingChildNotify );
}


DirTree::~DirTree()
{
    delete _root;
    delete _excludeRules;
    delete _tmpExcludeRules;

    clearFilters();
}

/*
void DirTree::setRoot( DirInfo *newRoot )
{
    if ( _root )
    {
	emit deletingChild( _root );
	delete _root;
	emit childrenDeleted();
    }

    _root = newRoot;

    FileInfo * realRoot = firstToplevel();
    _url = realRoot ? realRoot->url() : "";
}
*/

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

    if ( _root )
    {
	emit clearing();
	_root->clear();
	emit cleared();
    }

    _isBusy           = false;
    _blocksPerCluster = -1;
}


void DirTree::reset()
{
    clearTmpExcludeRules();
    clearFilters();
}


void DirTree::startReading( const QString & rawUrl )
{
    const QFileInfo fileInfo( rawUrl );
    const QString canonicalPath = fileInfo.canonicalFilePath();
    _url = canonicalPath.isEmpty() ? fileInfo.absoluteFilePath() : canonicalPath;
    //logDebug() << "rawUrl: \"" << rawUrl << "\"" << Qt::endl;
    logInfo() << "   url: \"" << _url	 << "\"" << Qt::endl;

    const MountPoint * mountPoint = MountPoints::findNearestMountPoint( _url );

    logInfo() << "device: " << ( mountPoint ? mountPoint->device() : "" ) << Qt::endl;

    sendStartingReading();

    FileInfo * item = stat( _url, this, _root );
    if ( item ) // should always be an item, will throw if there is an error
    {
	childAddedNotify( item );

	if ( item->isDirInfo() )
	    createLocalDirReadJob( this, item );
	else
	    sendFinished();

	emit readJobFinished( _root );
    }
}


void DirTree::refresh( const FileInfoSet & refreshSet )
{
    if ( !_root )
	return;

    // Make a list of items that are still accessible in the real world
    FileInfoSet items;
    for ( FileInfo * item : refreshSet.invalidRemoved() )
    {
	// Check the item is still accessible on disk
	// Pseudo-dirs (shouldn't be here) will implicitly fail the check
	struct stat statInfo;
	while ( lstat( item->url().toUtf8(), &statInfo ) != 0 )
	{
	    if ( item == _root || item->parent() == _root )
	    {
		// just try a full refresh, it will throw if even that isn't accessible any more
		//logDebug() << item->parent() << " " << _root << Qt::endl;
		refresh( item->toDirInfo() );
		return;
	    }

	    item = item->parent();
	}

	// Add an item that we have full access to
	items << item;
    }

    // Refresh the subtrees that we have left
    for ( FileInfo * item : items.normalized() )
    {
	// Need to check the magic number here again because a previous
	// iteration step might have made the item invalid already
	if ( item && item->checkMagicNumber() )
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

    if ( subtree == _root || subtree->parent() == _root )	// Refresh all (from first toplevel)
    {
	// Get the url to refresh before we clear the tree
	const QString url = firstToplevel()->url();
//	emit clearing(); // for the selection model
	clearSubtree( _root );
	startReading( QDir::cleanPath( url ) );
    }
    else	// Refresh subtree
    {
	// A full startingReading signal would reset all the tree branches to level 1
	emit startingRefresh();
	_isBusy = true;

	//logDebug() << "Refreshing subtree " << subtree << Qt::endl;

	//  Make copies of some key information before it is deleted
	const QString url = subtree->url();
	DirInfo * parent = subtree->parent();
	if ( parent->isAttic() )
	    parent = parent->parent();

	deleteSubtree( subtree );

	// Recreate the deleted subtree
	FileInfo * item = stat( url, this, parent );
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
	recalc( _root );
	ignoreEmptyDirs( _root );
	recalc( _root );
	if ( _root->firstChild() )
            moveIgnoredToAttic( _root->firstChild()->toDirInfo() );
	recalc( _root );
    }
}


void DirTree::childAddedNotify( FileInfo * newChild )
{
    if ( !haveClusterSize() )
        detectClusterSize( newChild );

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

    if ( child == _root )
	_root = nullptr;
}


void DirTree::deleteSubtree( DirInfo * subtree )
{
    // Make a FileInfoSet to use in the signal to DirTreeModel
    FileInfoSet subtrees;
    subtrees << subtree;

    emit deletingChildren( subtree->parent(), subtrees );
    deleteChild( subtree );
    emit childrenDeleted();
}


void DirTree::deleteSubtrees( const FileInfoSet & subtrees )
{
    // Don't do anything if a read is in progress or gets started
    if ( _isBusy )
	return;

//    if ( subtrees.contains( _root ) )
//	emit clearing();

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

    // Loop over the parents, deleting the children of each in one group
    const auto & constMap = parentMap;
    const auto parents = constMap.uniqueKeys();
    for ( DirInfo * parent : parents )
    {
	FileInfoSet children;
	const auto range = constMap.equal_range( parent );
	for ( auto it = range.first; it != range.second; ++it )
	    children << it.value();

	emit deletingChildren( parent, children );

	for ( FileInfo * child : children )
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


FileInfo * DirTree::locate( const QString & url, bool findPseudoDirs ) const
{
    if ( !_root )
	return nullptr;

    FileInfo * topItem = _root->firstChild();

    if ( topItem && topItem->isPkgInfo() && topItem->url() == url )
	return topItem;

    return _root->locate( url, findPseudoDirs );
}


bool DirTree::writeCache( const QString & cacheFileName )
{
    CacheWriter writer( cacheFileName, this );
    return writer.ok();
}


bool DirTree::readCache( const QString & cacheFileName )
{
    CacheReadJob * readJob = new CacheReadJob( this, cacheFileName );
    CHECK_NEW( readJob );

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
    PkgReader reader( this, pkgFilter );
//    reader.read( this, pkgFilter );
}


void DirTree::setExcludeRules()
{
    delete _excludeRules;
    _excludeRules = new ExcludeRules();
    CHECK_NEW( _excludeRules );
}


void DirTree::setTmpExcludeRules( const ExcludeRules * newTmpRules )
{
    delete _tmpExcludeRules;

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

    _tmpExcludeRules = newTmpRules;
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
    for ( const DirTreeFilter * filter : _filters )
    {
	if ( filter->ignore( path ) )
	    return true;
    }

    return false;
}


void DirTree::recalc( DirInfo * dir )
{
    FileInfo * child = dir->firstChild();

    while ( child )
    {
	if ( child->isDirInfo() )
	    recalc( child->toDirInfo() );

	child = child->next();
    }

    if ( dir->dotEntry() )
	recalc( dir->dotEntry() );

    if ( dir->attic() )
	recalc( dir->attic() );

    dir->recalc();
}


void DirTree::detectClusterSize( const FileInfo * item )
{
    if ( item &&
         item->isFile()     &&
         item->blocks() > 1 &&          // 1..512 bytes fits into an NTFS fragment
         item->size()   < 2 * STD_BLOCK_SIZE )
    {
        _blocksPerCluster = item->blocks();

        logInfo() << "Cluster size: " << _blocksPerCluster << " blocks ("
                  << formatSize( clusterSize() ) << ")" << Qt::endl;
//        logDebug() << "Derived from " << item << " " << formatSize( item->rawByteSize() )
//                   << " (allocated: " << formatSize( item->rawAllocatedSize() ) << ")"
//                   << Qt::endl;
    }
}


void DirTree::setIgnoreHardLinks( bool ignore )
{
    if ( ignore )
	logInfo() << "Ignoring hard links" << Qt::endl;

    _ignoreHardLinks = ignore;
}
