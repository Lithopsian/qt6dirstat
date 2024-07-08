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
#include "DotEntry.h"
#include "Exception.h"
#include "ExcludeRules.h"
#include "FileInfo.h"
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
	// logDebug() << "url: \"" << url << '"' << Qt::endl;

	if ( lstat( url.toUtf8(), &statInfo ) == 0 ) // lstat() OK
	{
	    QString name = url;

	    if ( parent && parent != tree->root() )
		name = SysUtil::baseName( url );

	    if ( S_ISDIR( statInfo.st_mode ) ) // directory
	    {
		DirInfo * dir = new DirInfo( parent, tree, name, statInfo );

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
	    else // not directory
	    {
		FileInfo * file = new FileInfo( parent, tree, name, statInfo );

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

#if 0
    /**
     * Recursively force a complete recalculation of all sums.
     *
     * Redundant now, fingers crossed.
     **/
    void recalc( DirInfo * dir )
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
#endif

    /**
     * Move all items from any attics below a directory into the attic parent and
     * remove the emptied attics.  This is done when a directory has been moved
     * into an attic, so any attics within it are redundant.
     **/
    void unatticAll( DirInfo * dir )
    {
	if ( dir->attic() )
	{
	    //logDebug() << "Moving all attic children to the normal children list for " << dir << Qt::endl;
	    dir->takeAllChildren( dir->attic() );
	    dir->deleteEmptyAttic();
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
	    dir->unlinkChild( child );
	    dir->addToAttic( child );

	    if ( child->isDirInfo() )
		unatticAll( child->toDirInfo() );
	}

	if ( !ignoredChildren.isEmpty() )
	{
	    // Recalc the attic to capture error counts in the moved children
	    // childAdded() doesn't expect the child to already have error counts
	    dir->attic()->recalc();

	    // unlinkChild() has already marked dir and all its ancestors as dirty
	}
    }


    /**
     * Recurse through the tree from 'dir' on and ignore any empty dirs
     * (i.e. dirs without any unignored non-directory child) that are not
     * ignored yet.
     **/
    void ignoreEmptyDirs( DirInfo * dir )
    {
	for ( FileInfo * child = dir->firstChild(); child; child = child->next() )
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
	}
    }


    /**
     * Recurse through the tree from 'dir' on and ignore any empty dirs
     **/
    void createLocalDirReadJob( DirTree * tree, FileInfo * item )
    {
	tree->addJob( new LocalDirReadJob( tree, item->toDirInfo(), false ) );
    }

} // namespace


DirTree::DirTree( QObject * parent ):
    QObject { parent },
    _root { new DirInfo( this ) }
{
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


void DirTree::reset()
{
    clearTmpExcludeRules();
    clearFilters();
}


void DirTree::startReading( const QString & rawUrl )
{
    //logDebug() << "rawUrl: \"" << rawUrl << '"' << Qt::endl;

    _url = [ &rawUrl ]()
    {
	const QFileInfo fileInfo( rawUrl );

	if ( fileInfo.isDir() ) // return the input path, just canonicalised
	    return fileInfo.canonicalFilePath();

	if ( fileInfo.exists() ) // return the parent directory for any existing non-directory
	    return fileInfo.canonicalPath();

	if ( fileInfo.isSymLink() ) // symlink target doesn't exist, return symlink parent directory
	    return QFileInfo( fileInfo.absolutePath() ).canonicalFilePath();

	return fileInfo.absoluteFilePath(); // return nonexistent input file which should throw
    }();

    const MountPoint * mountPoint = MountPoints::findNearestMountPoint( _url );
    logInfo() << "url:    " << _url << Qt::endl;
    logInfo() << "device: " << ( mountPoint ? mountPoint->device() : QString() ) << Qt::endl;

    sendStartingReading();

    FileInfo * item = stat( _url, this, root() );
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
	    struct stat statInfo;
	    while ( lstat( item->url().toUtf8(), &statInfo ) != 0 )
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

    if ( subtree == root() || subtree->parent() == root() )	// Refresh all (from first toplevel)
    {
	clearSubtree( root() ); // clears the contents, but not _url, filters, etc.
	startReading( QDir::cleanPath( _url ) );
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
	ignoreEmptyDirs( root() );
	if ( _root->firstChild() )
            moveIgnoredToAttic( _root->firstChild()->toDirInfo() );
//	recalc( root() );
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


FileInfo * DirTree::locate( const QString & url, bool findPseudoDirs ) const
{
    // Search from the top of the tree
    if ( _root )
	return _root->locate( url, findPseudoDirs );

    // Should never get here, there is always _root
    return nullptr;
}


bool DirTree::writeCache( const QString & cacheFileName )
{
    CacheWriter writer( cacheFileName, this );
    return writer.ok();
}


bool DirTree::readCache( const QString & cacheFileName )
{
    CacheReadJob * readJob = new CacheReadJob( this, cacheFileName );

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
}


void DirTree::setExcludeRules()
{
    _excludeRules.reset( new ExcludeRules() );
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
