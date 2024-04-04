/*
 *   File name: DirTree.cpp
 *   Summary:	Support classes for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
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


DirTree::DirTree():
    QObject (),
    _root { new DirInfo( this ) }
{
    CHECK_NEW( _root );

    connect( & _jobQueue, &DirReadJobQueue::finished,
	     this,	  &DirTree::sendFinished );

    connect( this,	  &DirTree::deletingChild,
	     & _jobQueue, &DirReadJobQueue::deletingChildNotify );
}


DirTree::~DirTree()
{
    _beingDestroyed = true;

    delete _root;
    delete _excludeRules;
    delete _tmpExcludeRules;

    clearFilters();
}


void DirTree::setRoot( DirInfo *newRoot )
{
    if ( _root )
    {
	emit deletingChild( _root );
	delete _root;
	emit childDeleted();
    }

    _root = newRoot;

    FileInfo * realRoot = firstToplevel();
    _url = realRoot ? realRoot->url() : "";
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


bool DirTree::isTopLevel( FileInfo *item ) const
{
    return item && item->parent() && !item->parent()->parent();
}


void DirTree::clear()
{
    _jobQueue.clear();

    if ( _root )
    {
	emit clearing();
	_root->clear();
    }

    _isBusy           = false;
    _haveClusterSize  = false;
    _blocksPerCluster = 0;
    _device.clear();
}


void DirTree::reset()
{
    clear();
    clearTmpExcludeRules();
    clearFilters();
}


void DirTree::startReading( const QString & rawUrl )
{
    QFileInfo fileInfo( rawUrl );
    const QString canonicalPath = fileInfo.canonicalFilePath(); // fix me, broken link handling
    _url = canonicalPath.isEmpty() ? fileInfo.absoluteFilePath() : canonicalPath;
    //logDebug() << "rawUrl: \"" << rawUrl << "\"" << Qt::endl;
    logInfo() << "   url: \"" << _url	 << "\"" << Qt::endl;

    MountPoint * mountPoint = MountPoints::findNearestMountPoint( _url );
    _device = mountPoint ? mountPoint->device() : "";
    logInfo() << "device: " << _device << Qt::endl;

    if ( _root->hasChildren() )
	clear();

    sendStartingReading();

    FileInfo * item = stat( _url, this, _root );
    CHECK_PTR( item );

    if ( item )
    {
	childAddedNotify( item );

	if ( item->isDirInfo() )
	{
	    LocalDirReadJob * job = new LocalDirReadJob( this, item->toDirInfo(), false );
	    CHECK_NEW( job );
	    addJob( job );
	}
	else
	{
	    sendFinished();
	}

	emit readJobFinished( _root );
    }
    else	// stat() failed
    {
	logWarning() << "stat(" << _url << ") failed" << Qt::endl;
	_isBusy = false;
	emit finished();
    }
}


void DirTree::refresh( const FileInfoSet & refreshSet )
{
    for ( FileInfo * item : refreshSet.invalidRemoved().normalized() )
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
    if ( !_root )
	return;

    if ( !subtree->checkMagicNumber() )
    {
	// Not using CHECK_MAGIC() here which would throw an exception since
	// this might easily happen after cleanup actions with multi selection
	// if one selected item is in the subtree of another, and that parent
	// was already refreshed.

	logWarning() << "Item is no longer valid - not refreshing subtree" << Qt::endl;
	return;
    }

    if ( subtree->isDotEntry() )
	subtree = subtree->parent();

    if ( !subtree || !subtree->parent() )	// Refresh all (from first toplevel)
    {
	try
	{
	    startReading( QDir::cleanPath( firstToplevel()->url() ) );
	}
	catch ( const SysCallFailedException & ex )
	{
	    CAUGHT( ex );
	}
    }
    else	// Refresh subtree
    {
	// logDebug() << "Refreshing subtree " << subtree << Qt::endl;

	clearSubtree( subtree );

	subtree->reset();
	subtree->setExcluded( false );
	subtree->setReadState( DirReading );

	// A full startingReading signal would reset all the tree branches to level 1
	emit startingRefresh();
	_isBusy = true;

	LocalDirReadJob * job = new LocalDirReadJob( this, subtree, false );
	CHECK_NEW( job);
	addJob( job );
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

/*
void DirTree::slotFinished()
{
    sendFinished();
}
*/

void DirTree::childAddedNotify( FileInfo * newChild )
{
    //logDebug() << Qt::endl;
    if ( !_haveClusterSize )
        detectClusterSize( newChild );

    emit childAdded( newChild );

    if ( newChild->dotEntry() )
	emit childAdded( newChild->dotEntry() );
}


void DirTree::deletingChildNotify( FileInfo * deletedChild )
{
    logDebug() << "Deleting child " << deletedChild << Qt::endl;
    emit deletingChild( deletedChild );

    if ( deletedChild == _root )
	_root = nullptr;
}

/*
void DirTree::childDeletedNotify()
{
    emit childDeleted();
}
*/

void DirTree::deleteSubtree( FileInfo *subtree )
{
    //logDebug() << "Deleting subtree " << subtree << Qt::endl;
    DirInfo * parent = subtree->parent();

    // Send notification to anybody interested (e.g., to attached views)
    deletingChildNotify( subtree );

    // If this was the last child of a dot entry
    if ( parent && parent->isDotEntry() && !parent->hasChildren() )
    {
	// Get rid of that now empty and useless dot entry
	if ( parent->parent() )
	{
	    if ( parent->parent()->isFinished() )
	    {
		// logDebug() << "Removing empty dot entry " << parent << Qt::endl;

		deletingChildNotify( parent );
		parent->parent()->deleteEmptyDotEntry();

		delete parent;
		parent = nullptr;
	    }
	}
	else	// no parent - this should never happen (?)
	{
	    logError() << "Internal error: NOT killing dot entry without parent: " << parent << Qt::endl;

	    // Better leave that dot entry alone - we shouldn't have come
	    // here in the first place. Who knows what will happen if this
	    // thing is deleted now?!
	    //
	    // Intentionally NOT calling:
	    //     delete parent;
	}
    }

    if ( parent )
    {
	// Give the parent of the child to be deleted a chance to unlink the
	// child from its children list and take care of internal summary
	// fields
	parent->deletingChild( subtree );
    }

    delete subtree;

    if ( subtree == _root )
	_root = nullptr;

    emit childDeleted();
}


void DirTree::clearSubtree( DirInfo * subtree )
{
    if ( subtree->hasChildren() )
    {
	emit clearingSubtree( subtree );
	subtree->clear();
	emit subtreeCleared( subtree );
    }
}


void DirTree::addJob( DirReadJob * job )
{
    _jobQueue.enqueue( job );
}


void DirTree::addBlockedJob( DirReadJob * job )
{
    _jobQueue.addBlocked( job );
}


void DirTree::unblock( DirReadJob * job )
{
    _jobQueue.unblock( job );
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
    sendStartingReading();

    CacheReadJob * readJob = new CacheReadJob( this, cacheFileName );
    if ( !readJob->reader() )
    {
	delete readJob;
//	emit aborted();
	return false;
    }

    addJob( readJob );
    return true;
}

/*
void DirTree::clearAndReadCache( const QString & cacheFileName )
{
    clear();
    readCache( cacheFileName, true );
}
*/

void DirTree::readPkg( const PkgFilter & pkgFilter )
{
    clear();
    _url = pkgFilter.url();

    sendStartingReading();

    // logDebug() << "Reading " << pkgFilter << Qt::endl;
    PkgReader reader( this );
    reader.read( pkgFilter );
}


void DirTree::setExcludeRules()
{
    delete _excludeRules;
    _excludeRules = new ExcludeRules();
    CHECK_NEW( _excludeRules );
}


void DirTree::setTmpExcludeRules( ExcludeRules * newTmpRules )
{
    delete _tmpExcludeRules;

#if VERBOSE_EXCLUDE_RULES
    if ( newTmpRules )
    {
	logDebug() << "New tmp exclude rules:" << Qt::endl;

	for ( ExcludeRuleListIterator it = newTmpRules->cbegin(); it != newTmpRules->cend(); ++it )
	    logDebug() << *it << Qt::endl;
    }
    else
    {
	logDebug() << "Clearing tmp exclude rules" << Qt::endl;
    }
#endif

    _tmpExcludeRules = newTmpRules;
}


void DirTree::clearFilters()
{
    qDeleteAll( _filters );
    _filters.clear();
}


bool DirTree::checkIgnoreFilters( const QString & path )
{
    for ( const DirTreeFilter * filter : _filters )
    {
	if ( filter->ignore( path ) )
	    return true;
    }

    return false;
}


void DirTree::moveIgnoredToAttic( DirInfo * dir )
{
    if ( !dir )
	return;

    if ( dir->totalIgnoredItems() == 0 && dir->totalUnignoredItems() > 0 )
	return;

    // Not using FileInfoIterator because we don't want to iterate over the dot
    // entry as well, just the normal children.

    FileInfo * child = dir->firstChild();
    FileInfoList ignoredChildren;

    while ( child )
    {
	if ( child->isIgnored() )
	    // Don't move the child right here, otherwise the iteration breaks
	    ignoredChildren << child;
	else
	    moveIgnoredToAttic( child->toDirInfo() );

	child = child->next();
    }


    for ( FileInfo * child : ignoredChildren )
    {
	// logDebug() << "Moving ignored " << child << " to attic" << Qt::endl;
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


void DirTree::ignoreEmptyDirs( DirInfo * dir )
{
    CHECK_PTR( dir );

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
	    else
	    {
		ignoreEmptyDirs( subDir );
	    }
	}

	child = child->next();
    }
}


void DirTree::unatticAll( DirInfo * dir )
{
    CHECK_PTR( dir );

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


void DirTree::recalc( DirInfo * dir )
{
    CHECK_PTR( dir );

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


void DirTree::detectClusterSize( FileInfo * item )
{
    if ( item &&
         item->isFile()     &&
         item->blocks() > 1 &&          // 1..512 bytes fits into an NTFS fragment
         item->size()   < 2 * STD_BLOCK_SIZE )
    {
        _blocksPerCluster = item->blocks();
        _haveClusterSize  = true;

        logInfo() << "Cluster size: " << _blocksPerCluster << " blocks ("
                  << formatSize( clusterSize() ) << ")" << Qt::endl;
//        logDebug() << "Derived from " << item << " " << formatSize( item->rawByteSize() )
//                   << " (allocated: " << formatSize( item->rawAllocatedSize() ) << ")"
//                   << Qt::endl;
    }
}


FileInfo * DirTree::stat( const QString & url,
				  DirTree	* tree,
				  DirInfo	* parent,
				  bool		  doThrow )
{
    struct stat statInfo;
    // logDebug() << "url: \"" << url << "\"" << Qt::endl;

    if ( lstat( url.toUtf8(), &statInfo ) == 0 ) // lstat() OK
    {
	QString name = url;

	if ( parent && parent != tree->root() )
	    name = SysUtil::baseName( url );
//	{
//	    const QStringList components = url.split( "/", Qt::SkipEmptyParts );
//	    name = components.last();
//	}

	if ( S_ISDIR( statInfo.st_mode ) )	// directory?
	{
	    DirInfo * dir = new DirInfo( parent, tree, name, &statInfo );
	    CHECK_NEW( dir );

	    if ( parent )
	    {
		parent->insertChild( dir );

		if ( !tree->isTopLevel( dir ) && !parent->isPkgInfo() && dir->device() != parent->device() )
		{
		    logDebug() << dir << " is a mount point" << Qt::endl;
		    dir->setMountPoint();
		}
	    }

	    return dir;
	}
	else					// no directory
	{
	    FileInfo * file = new FileInfo( parent, tree, name, &statInfo );
	    CHECK_NEW( file );

	    if ( parent )
		parent->insertChild( file );

	    return file;
	}
    }
    else // lstat() failed
    {
	if ( doThrow )
	    THROW( SysCallFailedException( "lstat", url ) );

	return nullptr;
    }
}


void DirTree::setIgnoreHardLinks( bool ignore )
{
    if ( ignore )
	logInfo() << "Ignoring hard links" << Qt::endl;

    _ignoreHardLinks = ignore;
}