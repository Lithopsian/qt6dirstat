/*
 *   File name: DirInfo.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <algorithm> // std::stable_sort

#include "DirInfo.h"
#include "Attic.h"
#include "DirTree.h"
#include "DotEntry.h"
#include "Exception.h"
#include "FileInfo.h"
#include "FileInfoIterator.h"
#include "FileInfoSorter.h"
#include "FormatUtil.h"


// How many times the standard deviation from the average is considered dominant
#define DOMINANCE_FACTOR                         5.0
#define DOMINANCE_MIN_PERCENT                    3.0
#define DOMINANCE_MAX_PERCENT                   70.0
#define DOMINANCE_ITEM_COUNT                    30
#define VERBOSE_DOMINANCE_CHECK                  0
#define DIRECT_CHILDREN_COUNT_SANITY_CHECK       0


using namespace QDirStat;


namespace
{
    [[gnu::unused]] void dumpChildrenList( const FileInfo     * dir,
					   const FileInfoList & children )
    {
	logDebug() << "Children of " << dir << Qt::endl;

	for ( int i=0; i < children.size(); ++i )
	    logDebug() << "    #" << i << ": " << children.at(i) << Qt::endl;
    }

} // namespace


DirInfo::DirInfo( DirInfo       * parent,
		  DirTree       * tree,
		  const QString & name ):
    FileInfo ( parent, tree, name ),
    _isMountPoint { false },
    _isExcluded { false },
    _summaryDirty { false },
    _locked { false },
    _touched { false },
    _fromCache { false },
    _readState { DirFinished }
{
    initCounts();
}


DirInfo::DirInfo( DirInfo           * parent,
		  DirTree           * tree,
		  const QString     & name,
		  const struct stat & statInfo ):
    FileInfo ( parent,
		 tree,
		 name,
		 statInfo ),
    _isMountPoint { false },
    _isExcluded { false },
    _summaryDirty { false },
    _locked { false },
    _touched { false },
    _fromCache { false },
    _readState { DirQueued }
{
    initCounts();
    ensureDotEntry();
    _directChildrenCount++;	// One for the newly created dot entry
}


DirInfo::DirInfo( DirInfo       * parent,
		  DirTree       * tree,
		  const QString & name,
		  mode_t          mode,
		  FileSize        size,
		  FileSize        allocatedSize,
		  bool            withUidGidPerm,
		  uid_t           uid,
		  gid_t           gid,
		  time_t          mtime ):
    FileInfo ( parent,
	       tree,
	       name,
	       mode,
	       size,
	       allocatedSize,
	       withUidGidPerm,
	       uid,
	       gid,
	       mtime ),
    _isMountPoint { false },
    _isExcluded { false },
    _summaryDirty { false },
    _locked { false },
    _touched { false },
    _fromCache { false },
    _readState { DirQueued }
{
    initCounts();
    ensureDotEntry();
    _directChildrenCount++;	// One for the newly created dot entry
}


DirInfo::~DirInfo()
{
    clear();
}


void DirInfo::initCounts()
{
    // logDebug() << this << Qt::endl;

    _totalSize           = size();
    _totalAllocatedSize  = allocatedSize();
    _totalBlocks         = blocks();
    _totalItems          = 0;
    _totalSubDirs        = 0;
    _totalFiles          = 0;
    _totalIgnoredItems   = 0;
    _totalUnignoredItems = 0;
    _directChildrenCount = 0;
    _errSubDirCount      = 0;
    _latestMtime         = mtime();
    _oldestFileMtime     = 0;
}


void DirInfo::clear()
{
    // If there are no children of any kind, no need to even mark as dirty
    if ( !_firstChild && !_dotEntry && !_attic )
	return;

    // Recursively (through the destructors) delete all children
    while ( _firstChild )
    {
	FileInfo * childToDelete = _firstChild;
	_firstChild = _firstChild->next();

	delete childToDelete;
    }

    delete _dotEntry;
    _dotEntry = nullptr;

    delete _attic;
    _attic = nullptr;

    markAsDirty();
}


void DirInfo::ensureDotEntry()
{
    if ( !_dotEntry )
    {
	// logDebug() << "Creating dot entry for " << this << Qt::endl;

	_dotEntry = new DotEntry( tree(), this );
    }
}


void DirInfo::deleteEmptyDotEntry()
{
    if ( !_dotEntry->firstChild() && !_dotEntry->hasAtticChildren() )
    {
	delete _dotEntry;
	_dotEntry = nullptr;

	countDirectChildren();
    }
}


Attic * DirInfo::ensureAttic()
{
    if ( !_attic )
    {
	// logDebug() << "Creating attic for " << this << Qt::endl;

	_attic = new Attic( tree(), this );
    }

    return _attic;
}


void DirInfo::deleteEmptyAttic()
{
    if ( _attic && !_attic->firstChild() )
    {
	delete _attic;
	_attic = nullptr;
    }
}


bool DirInfo::hasAtticChildren() const
{
    return _attic && _attic->hasChildren();
}


void DirInfo::recalc()
{
    //logDebug() << this << " " << pkgInfoParent() << " " << isPkgInfo() << Qt::endl;

    initCounts();

    for ( FileInfoIterator it( this ); *it; ++it )
    {
	_directChildrenCount++;
	_totalSize           += (*it)->totalSize();
	_totalAllocatedSize  += (*it)->totalAllocatedSize();
	_totalBlocks         += (*it)->totalBlocks();
	_totalItems          += (*it)->totalItems() + 1;
	_totalSubDirs        += (*it)->totalSubDirs();
	_errSubDirCount      += (*it)->errSubDirCount();
	_totalFiles          += (*it)->totalFiles();
	_totalIgnoredItems   += (*it)->totalIgnoredItems();
	_totalUnignoredItems += (*it)->totalUnignoredItems();

	if ( (*it)->isDir() )
	{
	    _totalSubDirs++;

	    if ( (*it)->readError() )
		_errSubDirCount++;
	}
	else
	{
	    if ( (*it)->isIgnored() )
		_totalIgnoredItems++;
	    else
		_totalUnignoredItems++;

	    if ( (*it)->isFile() )
		_totalFiles++;
	}

	time_t childLatestMtime = (*it)->latestMtime();
	if ( childLatestMtime > _latestMtime )
	    _latestMtime = childLatestMtime;

	time_t childOldestFileMTime = (*it)->oldestFileMtime();
	if ( childOldestFileMTime > 0 )
	{
	    if ( _oldestFileMtime == 0 || childOldestFileMTime < _oldestFileMtime )
		_oldestFileMtime = childOldestFileMTime;
	}
    }

    if ( _attic )
    {
	_totalIgnoredItems += _attic->totalIgnoredItems();
	_errSubDirCount	   += _attic->errSubDirCount();
    }

    _summaryDirty = false;
}


FileSize DirInfo::totalSize()
{
    if ( _summaryDirty )
	recalc();

    return _totalSize;
}


FileSize DirInfo::totalAllocatedSize()
{
    if ( _summaryDirty )
	recalc();

    return _totalAllocatedSize;
}


FileSize DirInfo::totalBlocks()
{
    if ( _summaryDirty )
	recalc();

    return _totalBlocks;
}


int DirInfo::totalItems()
{
    if ( _summaryDirty )
	recalc();

    return _totalItems;
}


int DirInfo::totalSubDirs()
{
    if ( _summaryDirty )
	recalc();

    return _totalSubDirs;
}


int DirInfo::totalFiles()
{
    if ( _summaryDirty )
	recalc();

    return _totalFiles;
}


int DirInfo::totalIgnoredItems()
{
    if ( _summaryDirty )
	recalc();

    return _totalIgnoredItems;
}


int DirInfo::totalUnignoredItems()
{
    if ( _summaryDirty )
	recalc();

    return _totalUnignoredItems;
}


time_t DirInfo::latestMtime()
{
    if ( _summaryDirty )
	recalc();

    return _latestMtime;
}


time_t DirInfo::oldestFileMtime()
{
    if ( _summaryDirty )
	recalc();

    return _oldestFileMtime;
}


int DirInfo::directChildrenCount()
{
    if ( _summaryDirty )
	recalc();

    return _directChildrenCount;
}


int DirInfo::countDirectChildren()
{
    // logDebug() << this << Qt::endl;

    _directChildrenCount = 0;

    for ( const FileInfo * child = _firstChild; child; child = child->next() )
	++_directChildrenCount;

    if ( _dotEntry )
	++_directChildrenCount;

    return _directChildrenCount;
}


int DirInfo::errSubDirCount()
{
    if ( _summaryDirty )
	recalc();

    return _errSubDirCount;
}


void DirInfo::setReadState( DirReadState newReadState )
{
    // "aborted" has higher priority than "finished"
    if ( _readState == DirAborted && newReadState == DirFinished )
	return;

    _readState = newReadState;
}


bool DirInfo::isFinished() const
{
    if ( _pendingReadJobs > 0 && _readState != DirAborted )
	return false;

    if ( _readState == DirReading || _readState == DirQueued )
	return false;

    return true;
}


void DirInfo::insertChild( FileInfo * newChild )
{
    CHECK_PTR( newChild );

    if ( newChild->isDir() || !_dotEntry )
    {
	/**
	 * Only directories are stored directly in pure directory nodes -
	 * unless something went terribly wrong, e.g. there is no dot entry to use.
	 * If this is a dot entry, store everything it gets directly within it.
	 *
	 * In any of those cases, insert the new child in the children list.
	 *
	 * The list is unordered so we can always prepend for performance reasons.
	 **/
	newChild->setNext( _firstChild );
	_firstChild = newChild;
	newChild->setParent( this );	// make sure the parent pointer is correct

	childAdded( newChild );		// update summaries
    }
    else
    {
	/*
	 * If the child is not a directory, don't store it directly here - use
	 * this entry's dot entry instead.
	 */
	_dotEntry->insertChild( newChild );
    }
}


void DirInfo::addToAttic( FileInfo * newChild )
{
    CHECK_PTR( newChild );

    newChild->setIgnored( true );

    if ( newChild->isDir() )
	_totalIgnoredItems += newChild->totalIgnoredItems();
    else
	_totalIgnoredItems++;

    Attic * attic = [ this, newChild ]()
    {
	if ( !newChild->isDir() && _dotEntry )
	    return _dotEntry->ensureAttic();

	return ensureAttic();
    }();

    attic->insertChild( newChild );
}


void DirInfo::childAdded( FileInfo * newChild )
{
    if ( newChild->isIgnored() )
    {
	if ( newChild->isDir() )
	    _totalIgnoredItems += newChild->totalIgnoredItems();
	else
	    _totalIgnoredItems++;
    }
    else if ( !newChild->isDir() )
    {
	_totalUnignoredItems++;
    }

    // Add ignored items to all the totals only if this directory is also
    // ignored or if this is the attic.
    if ( !newChild->isIgnored() || isIgnored() || isAttic() )
    {
	// No point updating obsolete data
	// - it will have to be calculated from scratch when it is needed
	if ( !_summaryDirty )
	{
	    if ( newChild->mtime() > _latestMtime )
		_latestMtime = newChild->mtime();

	    _totalSize          += newChild->size();
	    _totalAllocatedSize += newChild->allocatedSize();
	    _totalBlocks        += newChild->blocks();
	    _totalItems++;

	    if ( newChild->parent() == this )
		_directChildrenCount++;

	    if ( newChild->isDir() )
	    {
		_totalSubDirs++;
	    }
	    else if ( newChild->isFile() )
	    {
		_totalFiles++;

		const time_t childOldestFileMTime = newChild->oldestFileMtime();
		if ( childOldestFileMTime > 0 )
		{
		    if ( _oldestFileMtime == 0 || childOldestFileMTime < _oldestFileMtime )
			_oldestFileMtime = childOldestFileMTime;
		}
	    }
	}
    }

    if ( _sortInfo && _sortInfo->_sortedCol != ReadJobsCol )
	dropSortCache();

    if ( parent() )
	parent()->childAdded( newChild );
}


void DirInfo::markAsDirty()
{
    _summaryDirty = true;

    if ( parent() )
	parent()->markAsDirty();

    dropSortCache();
}

/*
void DirInfo::deletingChild( FileInfo * child )
{
    if ( child->parent() == this )
	unlinkChild( child );

    if ( parent() )
	parent()->deletingChild( child );
}
*/

void DirInfo::unlinkChild( FileInfo * deletedChild )
{
    dropSortCache();
    _summaryDirty = true;

    if ( deletedChild == _attic )
    {
	//logDebug() << "Unlinking (ie. deleting) attic " << deletedChild << Qt::endl;
	_attic = nullptr;
	return;
    }

    if ( deletedChild == _dotEntry )
    {
	//logDebug() << "Unlinking (ie. deleting) dot entry " << deletedChild << Qt::endl;
	_dotEntry = nullptr;
	return;
    }

    if ( deletedChild == _firstChild )
    {
	// logDebug() << "Unlinking first child " << deletedChild << Qt::endl;
	_firstChild = deletedChild->next();
	return;
    }

    for ( FileInfo * child = firstChild(); child; child = child->next() )
    {
	if ( child->next() == deletedChild )
	{
	    // logDebug() << "Unlinking " << deletedChild << Qt::endl;
	    child->setNext( deletedChild->next() );
	    return;
	}
    }

    logError() << "Couldn't unlink " << deletedChild << " from " << this << " children list" << Qt::endl;
}


void DirInfo::readJobAdded()
{
    _pendingReadJobs++;

    if ( _sortInfo && _sortInfo->_sortedCol == ReadJobsCol )
	dropSortCache();

    if ( parent() )
	parent()->readJobAdded();
}


void DirInfo::readJobFinished( DirInfo * dir )
{
    _pendingReadJobs--;

    if ( _sortInfo && _sortInfo->_sortedCol == ReadJobsCol )
	dropSortCache();

    if ( dir && dir != this && dir->readError() )
	_errSubDirCount++;

    if ( parent() )
	parent()->readJobFinished( dir );
}


void DirInfo::readJobAborted()
{
    _readState = DirAborted;

    if ( parent() )
	parent()->readJobAborted();
}


QString DirInfo::sizePrefix() const
{
    switch ( _readState )
    {
	case DirError:
	case DirAborted:
	case DirPermissionDenied:
	    return "> ";

	case DirFinished:
//	case DirCached:
	    if ( _errSubDirCount > 0 )
		return "> ";

	case DirQueued:
	case DirReading:
	case DirOnRequestOnly:
	    break;
	// No 'default' branch so the compiler can catch unhandled enum values
    }

    return "";
}


void DirInfo::finalizeLocal()
{
    cleanupDotEntries();
    cleanupAttics();
    checkIgnored();
}


void DirInfo::finalizeAll()
{
    for ( FileInfo * child = firstChild(); child; child = child->next() )
    {
	if ( child->isDirInfo() && !child->isDotEntry() )
	    child->toDirInfo()->finalizeAll();
    }

    // Optimization: as long as this directory is not finalized yet, it does
    // (very likely) have a dot entry and thus all direct children are
    // subdirectories, not plain files, so we don't need to bother checking
    // plain file children as well - so do finalizeLocal() only after all
    // children are processed. If this step were the first, for directories
    // that don't have any subdirectories finalizeLocal() would immediately
    // get all their plain file children reparented to themselves, so they
    // would need to be processed in the loop, too.

    finalizeLocal();
}


void DirInfo::cleanupDotEntries()
{
    if ( !_dotEntry )
	return;

    // Reparent dot entry children if there are no subdirectories on this level
    if ( !_firstChild && !hasAtticChildren() )
    {
	takeAllChildren( _dotEntry );

	// Reparent the dot entry's attic's children to this item's attic
	if ( _dotEntry->hasAtticChildren() )
	{
	    ensureAttic();
	    _attic->takeAllChildren( _dotEntry->attic() );
	}
    }

    // Delete the dot entry if it is now empty.
    //
    // This also affects dot entries that were just disowned because they had
    // no siblings (i.e., there are no subdirectories on this level).
    //
    // Note that this also checks if the dot entry's attic (if it has one) is
    // empty, and that its attic is deleted along with the dot entry.
    deleteEmptyDotEntry();
}


void DirInfo::cleanupAttics()
{
    if ( _dotEntry )
	_dotEntry->cleanupAttics();

    if ( _attic )
    {
	_attic->finalizeLocal();

	if ( !_attic->firstChild() && !_attic->dotEntry() )
	{
	    delete _attic;
	    _attic = nullptr;

	    dropSortCache();

	    _summaryDirty = true;
	}
    }
}


void DirInfo::checkIgnored()
{
    if ( _dotEntry )
	_dotEntry->checkIgnored();

    // Cascade the 'ignored' status up the tree:
    //
    // Display all directories as ignored that have any ignored items, but no
    // items that are not ignored.
    setIgnored( totalIgnoredItems() > 0 && totalUnignoredItems() == 0 );

    if ( isIgnored() )
	ignoreEmptySubDirs();

    if ( !isPseudoDir() && parent() )
	parent()->checkIgnored();
}


void DirInfo::ignoreEmptySubDirs()
{
    // Any children must have totalUnignoredItems == 0, so ignore them all
    for ( FileInfoIterator it( this ); *it; ++it )
    {
	// logDebug() << "Ignoring empty subdir " << (*it) << Qt::endl;
	(*it)->setIgnored( true );
	_summaryDirty = true;
    }
}


const DirSortInfo * DirInfo::newSortInfo( DataColumn sortCol, Qt::SortOrder sortOrder )
{
    // Clean old sorted children lists and create new ones
    dropSortCaches(); // recursive to all ancestors
    _sortInfo = new DirSortInfo( this, sortCol, sortOrder );

    return _sortInfo;
}


void DirInfo::dropSortCache()
{
    delete _sortInfo;
    _sortInfo = nullptr;
}


void DirInfo::dropSortCaches()
{
    // If this dir didn't have any sort cache, there won't be any in the subtree
    if ( _sortInfo )
    {
	// logDebug() << "Dropping sort cache for " << this << Qt::endl;

	//  Dot entries don't have dir children that could have a sort cache
	if ( !isDotEntry() )
	{
	    for ( FileInfo * child = _firstChild; child; child = child->next() )
	    {
		if ( child->isDirInfo() )
		    child->toDirInfo()->dropSortCaches();
	    }

	    if ( _dotEntry )
		_dotEntry->dropSortCaches();
	}

	if ( _attic )
	    _attic->dropSortCaches();
    }

    dropSortCache();
}


const DirInfo * DirInfo::findNearestMountPoint() const
{
    const DirInfo * dir = this;
    while ( dir->parent() != dir->tree()->root() && !dir->isMountPoint() )
	dir = dir->parent();

    return dir;
}


void DirInfo::takeAllChildren( DirInfo * oldParent )
{
    FileInfo * child = oldParent->firstChild();
    if ( child )
    {
	// logDebug() << "Reparenting all children of " << oldParent << " to " << this << Qt::endl;

	FileInfo * oldFirstChild = _firstChild;
	_firstChild              = child;
	FileInfo * lastChild     = child;

	oldParent->setFirstChild( nullptr );
	oldParent->recalc();

	_directChildrenCount = -1;
	_summaryDirty        = true;

	while ( child )
	{
	    child->setParent( this );
	    lastChild = child;
	    child = child->next();
	}

	lastChild->setNext( oldFirstChild );
    }
}


void DirInfo::finishReading( DirReadState readState )
{
    // logDebug() << this << Qt::endl;
    setReadState( readState );
    finalizeLocal();
    tree()->sendReadJobFinished( this );
}




DirSortInfo::DirSortInfo( DirInfo       * parent,
                          DataColumn      sortCol,
			  Qt::SortOrder   sortOrder ):
    _sortedCol { sortCol },
    _sortedOrder { sortOrder }
{
    // Generate the unsorted children list, including a dot entry
    _sortedChildren.reserve( parent->directChildrenCount() + 1 );
    for ( FileInfoIterator it( parent ); *it; ++it )
	_sortedChildren.append( *it );

    // logDebug() << "Sorting children of " << this << " by " << sortCol << Qt::endl;

    // Do secondary sorting by NameCol (always in ascending order)
    if ( sortCol != NameCol )
    {
	std::stable_sort( _sortedChildren.begin(),
			  _sortedChildren.end(),
			  FileInfoSorter( NameCol, Qt::AscendingOrder ) );
    }

    // Primary sorting as requested
    std::stable_sort( _sortedChildren.begin(),
		      _sortedChildren.end(),
		      FileInfoSorter( sortCol, sortOrder ) );

#if DIRECT_CHILDREN_COUNT_SANITY_CHECK
    // Do the sanity check before adding the attic, because it isn't in the direct children count
    if ( _sortedChildren.size() != parent->directChildrenCount() )
    {
	dumpChildrenList( parent, _sortedChildren );

	THROW( Exception( QString( "_directChildrenCount of %1 corrupted; is %2, should be %3" )
			  .arg( parent->debugUrl() )
			  .arg( parent->directChildrenCount() )
			  .arg( _sortedChildren.size() ) ) );
    }
#endif

    if ( parent->attic() )
	_sortedChildren.append( parent->attic() );

    // Store the sort order number for each item directly on the FileInfo object
    int childNumber = 0;
    for ( FileInfo * item : asConst( _sortedChildren ) )
	item->setRowNumber( childNumber++ );
}


int DirSortInfo::findDominantChildren()
{
    _firstNonDominantChild = [ this ]()
    {
	// Only if sorting by size or percent, descending
	switch ( _sortedCol )
	{
	    case PercentBarCol:
	    case PercentNumCol:
	    case SizeCol:
		break;

	    default:
		return 0;
	}

	if ( _sortedOrder != Qt::DescendingOrder )
	    return 0;

	const qreal count = qMin( _sortedChildren.size(), DOMINANCE_ITEM_COUNT );

	// Declare that only one child (ie. 100%) doesn't count as dominant
	if ( count < 2 )
	    return 0;

	const qreal medianPercent      = _sortedChildren.at( count / 2 )->subtreeAllocatedPercent();
	const qreal dominancePercent   = DOMINANCE_FACTOR * medianPercent;
	const qreal dominanceThreshold = qBound( DOMINANCE_MIN_PERCENT, dominancePercent, DOMINANCE_MAX_PERCENT );

#if VERBOSE_DOMINANCE_CHECK
	logDebug() << this
		   << "  median: "    << formatPercent( medianPercent )
		   << "  threshold: " << formatPercent( dominanceThreshold )
		   << Qt::endl;
#endif

	// Return the child number of the first child after the dominance threshold
	for ( FileInfo * child : asConst( _sortedChildren ) )
	{
	    if ( child->subtreeAllocatedPercent() < dominanceThreshold )
		return child->rowNumber();
	}

	// Should never get here, children can't all be dominant
	return 0;
    }();

    return _firstNonDominantChild;
}
