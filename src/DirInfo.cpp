/*
 *   File name: DirInfo.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "DirInfo.h"
#include "Attic.h"
#include "DirTree.h"
#include "Exception.h"
#include "FileInfoIterator.h"
#include "FileInfoSorter.h"
#include "FormatUtil.h"


// How many times the standard deviation from the average is considered dominant
#define DOMINANCE_FACTOR                         5
#define DOMINANCE_MIN_PERCENT                    3.0f
#define DOMINANCE_MAX_PERCENT                   70.0f
#define DOMINANCE_ITEM_COUNT                    30
#define VERBOSE_DOMINANCE_CHECK                  0
#define DIRECT_CHILDREN_COUNT_SANITY_CHECK       0


using namespace QDirStat;


namespace
{
    [[gnu::unused]] void dumpChildrenList( const FileInfo     * parent,
                                           const FileInfoList & children )
    {
	logDebug() << "Children of " << parent << Qt::endl;

	for ( int i=0; i < children.size(); ++i )
	    logDebug() << "    #" << i << ": " << children.at(i) << Qt::endl;
    }

} // namespace


DirInfo::DirInfo( DirInfo       * parent,
                  DirTree       * tree,
                  const QString & name ):
    FileInfo{ parent, tree, name },
    _isMountPoint{ false },
    _isExcluded{ false },
    _summaryDirty{ false },
    _locked{ false },
    _touched{ false },
    _fromCache{ false },
    _readState{ DirFinished }
{
    initCounts();
}


DirInfo::DirInfo( DirInfo           * parent,
                  DirTree           * tree,
                  const QString     & name,
                  const struct stat & statInfo ):
    FileInfo{ parent, tree, name, statInfo },
    _isMountPoint{ false },
    _isExcluded{ false },
    _summaryDirty{ false },
    _locked{ false },
    _touched{ false },
    _fromCache{ false },
    _readState{ DirQueued }
{
    initCounts();
    addDotEntry();
}


DirInfo::DirInfo( DirInfo       * parent,
                  DirTree       * tree,
                  const QString & name,
                  mode_t          mode,
                  FileSize        size,
                  FileSize        allocatedSize,
                  bool            fromCache,
                  bool            withUidGidPerm,
                  uid_t           uid,
                  gid_t           gid,
                  time_t          mtime ):
    FileInfo{ parent,
              tree,
              name,
              mode,
              size,
              allocatedSize,
              withUidGidPerm,
              uid,
              gid,
              mtime },
    _isMountPoint{ false },
    _isExcluded{ false },
    _summaryDirty{ false },
    _locked{ false },
    _touched{ false },
    _fromCache{ fromCache },
    _readState{ DirQueued }
{
    initCounts();
    addDotEntry();
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
    _childCount          = 0;
    _errSubDirs          = 0;
    _latestMtime         = mtime();
    _oldestFileMtime     = 0;
}


void DirInfo::clear()
{
    // If there are no children of any kind, no need to even mark as dirty,
    // otherwise cleaning up empty dot entries, etc. drastically slows down reads
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


void DirInfo::addDotEntry()
{
    if ( !_dotEntry )
    {
	// logDebug() << "Creating dot entry for " << this << Qt::endl;

	_dotEntry = new DotEntry{ tree(), this };
	++_childCount;
    }
}

/*
void DirInfo::deleteEmptyDotEntry()
{
    if ( !_dotEntry->firstChild() && !_dotEntry->hasAtticChildren() )
    {
	delete _dotEntry;
	_dotEntry = nullptr;

	--_childCount;
    }
}
*/


void DirInfo::ensureAttic()
{
    if ( !_attic )
    {
	// logDebug() << "Creating attic for " << this << Qt::endl;
	_attic = new Attic{ tree(), this };
	++_childCount;
    }
}


void DirInfo::deleteEmptyAttic()
{
    if ( _attic && _attic->isEmpty() )
    {
	delete _attic;
	_attic = nullptr;

	dropSortCache();
	_summaryDirty = true;
    }
}


void DirInfo::moveToAttic( FileInfo * child )
{
    unlinkChild( child );

    if ( child->isDotEntry() )
    {
	// Just throw the subtree in the attic, it will be marked dirty anyway
	ensureAttic();
	child->setParent( _attic );
	_attic->_dotEntry = child->toDotEntry();
    }
    else
    {
	// addToAttic() can add subtrees but won't calculate the summary correctly
	addToAttic( child );
    }

    // unlinkChild() marks all ancestors as dirty, but not the attic
    _attic->_summaryDirty = true;
    _attic->dropSortCache();
}


void DirInfo::moveAllFromAttic()
{
    //logDebug() << "Moving all attic children to the normal children list for " << this << Qt::endl;
    takeAllChildren( _attic );
    deleteEmptyAttic();

    // The attic children summary totals won't be included here, so everything has to be re-counted
    markAsDirty();
}


bool DirInfo::hasAtticChildren() const
{
    return _attic && _attic->hasChildren();
}


void DirInfo::recalc()
{
    //logDebug() << this << " " << pkgInfoParent() << " " << isPkgInfo() << Qt::endl;

    initCounts();

    // Loop through the children including the dot entry;
    // the attic is handled separately
    for ( DotEntryIterator it{ this }; *it; ++it )
    {
	// Count the child and add all its sub-totals
	++_childCount;
	_totalSize           += it->totalSize();
	_totalAllocatedSize  += it->totalAllocatedSize();
	_totalBlocks         += it->totalBlocks();
	_totalItems          += it->totalItems();
	_totalSubDirs        += it->totalSubDirs();
	_errSubDirs          += it->errSubDirs();
	_totalFiles          += it->totalFiles();
	_totalIgnoredItems   += it->totalIgnoredItems();
	_totalUnignoredItems += it->totalUnignoredItems();

	// Dot entries are iterated but don't count the dot entry itself
	if ( !it->isDotEntry() )
	    ++_totalItems;

	if ( it->isDir() )
	{
	    // Count this as a sub-directory
	    ++_totalSubDirs;

	    if ( it->readError() )
		++_errSubDirs;
	}
	else
	{
	    // Only add non-directories to the un/ignored counts
	    if ( it->isIgnored() )
		++_totalIgnoredItems;
	    else
		++_totalUnignoredItems;

	    // Only count regular files in _totalFiles
	    if ( it->isFile() )
		++_totalFiles;
	}

	time_t childLatestMtime = it->latestMtime();
	if ( childLatestMtime > _latestMtime )
	    _latestMtime = childLatestMtime;

	time_t childOldestFileMTime = it->oldestFileMtime();
	if ( childOldestFileMTime > 0 )
	{
	    if ( _oldestFileMtime == 0 || childOldestFileMTime < _oldestFileMtime )
		_oldestFileMtime = childOldestFileMTime;
	}
    }

    // Only copy ignored and error counts from ignored items to non-ignored parents
    if ( _attic )
    {
	_totalIgnoredItems += _attic->totalIgnoredItems();
	_errSubDirs        += _attic->errSubDirs();
	++_childCount;
    }

    _summaryDirty = false;
#if 0
    if ( parent() && parent() == tree()->root() )
	logDebug() << _attic << " " << _totalIgnoredItems << " " << _totalUnignoredItems << " " << _totalItems << " " << _totalSubDirs << Qt::endl;
#endif
}


FileSize DirInfo::totalSize()
{
    ensureClean();
    return _totalSize;
}


FileSize DirInfo::totalAllocatedSize()
{
    ensureClean();
    return _totalAllocatedSize;
}


FileSize DirInfo::totalBlocks()
{
    ensureClean();
    return _totalBlocks;
}


FileCount DirInfo::totalItems()
{
    ensureClean();
    return _totalItems;
}


FileCount DirInfo::totalSubDirs()
{
    ensureClean();
    return _totalSubDirs;
}


FileCount DirInfo::totalFiles()
{
    ensureClean();
    return _totalFiles;
}


FileCount DirInfo::totalIgnoredItems()
{
    ensureClean();
    return _totalIgnoredItems;
}


FileCount DirInfo::totalUnignoredItems()
{
    ensureClean();
    return _totalUnignoredItems;
}


FileCount DirInfo::errSubDirs()
{
    ensureClean();
    return _errSubDirs;
}


time_t DirInfo::latestMtime()
{
    ensureClean();
    return _latestMtime;
}


time_t DirInfo::oldestFileMtime()
{
    ensureClean();
    return _oldestFileMtime;
}


DirSize DirInfo::childCount()
{
    ensureClean();
    return _childCount;
}

/*
DirSize DirInfo::countChildren()
{
    // logDebug() << this << Qt::endl;

    _childCount = 0;

    _childCount += std::count_if( begin( this ), end( this ), []( auto ) { return true; } );

    if ( _dotEntry )
	++_childCount;

    if ( _attic )
	++_childCount;

    return _childCount;
}
*/

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

    /**
     * If there is a dot entry, store any non-directory items in it.
     * If the child is a directory or there is no dot entry (hopefully
     * because there are only file children) then store the child
     * directly in the DirInfo node.
     *
     * Note that this test automatically causes inserts to the dot entry
     * to be done directly because _dotEntry is always 0 for a DotEntry
     * itself.
     **/
    if ( _dotEntry && !newChild->isDir() )
    {
	// semi-recursive just to avoid repeating the same code here
	_dotEntry->insertChild( newChild );
    }
    else
    {
	newChild->setNext( _firstChild );
	_firstChild = newChild;
	newChild->setParent( this );

	childAdded( newChild ); // update summaries
    }
}


void DirInfo::addToAttic( FileInfo * newChild )
{
    CHECK_PTR( newChild );

    newChild->setIgnored( true );

    Attic * attic = [ this, newChild ]()
    {
	if ( !newChild->isDir() && _dotEntry )
	{
	    _dotEntry->ensureAttic();
	    return _dotEntry->_attic;
	}

	ensureAttic();
	return _attic;
    }();

    attic->insertChild( newChild );
}


void DirInfo::childAdded( FileInfo * newChild )
{
    //logDebug() << this << " " << newChild << " " << _summaryDirty << Qt::endl;

    // No point updating obsolete data - it will have be recalc()-ulated from scratch
    if ( !_summaryDirty )
    {
	// Only count non-directory items for un/ignored items
	// Empty directories will be handled separately when the tree is finalised
	if ( !newChild->isDir() )
	{
	    if ( newChild->isIgnored() )
		++_totalIgnoredItems;
	    else
		++_totalUnignoredItems;
	}

	// Don't propagate the other counts from ignored items to non-ignored ancestors
	if ( !newChild->isIgnored() || isIgnored() || isAttic() )
	{
	    // Watch for overflows at the top-level directory which should have the biggest numbers
	    if ( parent() && parent() == tree()->root() )
	    {
		if ( _totalItems == FileCountMax )
		    THROW( TooManyFilesException{} );

		if ( _totalSize          > FileSizeMax - newChild->size() ||
		     _totalAllocatedSize > FileSizeMax - newChild->allocatedSize() )
		    THROW( FilesystemTooBigException{} );
	    }

	    if ( newChild->mtime() > _latestMtime )
		_latestMtime = newChild->mtime();

	    _totalSize          += newChild->size();
	    _totalAllocatedSize += newChild->allocatedSize();
	    _totalBlocks        += newChild->blocks();
	    ++_totalItems;

	    if ( newChild->parent() == this )
	    {
		// Test for directory overflow (can't currently happen as DirSizeMax == FileCountMax)
//		if ( _childCount == DirSizeMax )
//		    THROW( Exception{ QString{ "more than %1 files in a directory" }.arg( DirSizeMax ) } );

		++_childCount;
	    }

	    if ( newChild->isDir() )
	    {
		++_totalSubDirs;
	    }
	    else if ( newChild->isFile() )
	    {
		++_totalFiles;

		const time_t childOldestFileMTime = newChild->oldestFileMtime();
		if ( childOldestFileMTime > 0 )
		{
		    if ( _oldestFileMtime == 0 || childOldestFileMTime < _oldestFileMtime )
			_oldestFileMtime = childOldestFileMTime;
		}
	    }
	}
    }

    // Don't drop the sort cache if we are reading because we haven't affected that sort order
//    if ( _sortInfo && _sortInfo->_sortedCol != ReadJobsCol )
	dropSortCache();

    // Propagate the new child totals up the tree
    if ( parent() )
	parent()->childAdded( newChild );
#if 0
    if ( parent() && parent() == tree()->root() )
	logDebug() << _summaryDirty << " " << _errSubDirs << " " << _totalIgnoredItems << " " << _totalUnignoredItems << " " << _totalItems << " " << _totalSubDirs << " " << newChild << Qt::endl;
#endif
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
    markAsDirty(); // recurses up the tree

    if ( deletedChild == _dotEntry )
    {
	//logDebug() << "Unlinking (ie. deleting) dot entry " << deletedChild << Qt::endl;
	_dotEntry = nullptr;
	return;
    }

    if ( deletedChild == _attic )
    {
	//logDebug() << "Unlinking (ie. deleting) attic " << deletedChild << Qt::endl;
	_attic = nullptr;
	return;
    }

    if ( deletedChild == _firstChild )
    {
	// logDebug() << "Unlinking first child " << deletedChild << Qt::endl;
	_firstChild = deletedChild->next();
	return;
    }

    auto compare = [ deletedChild ]( FileInfo * item ) { return item->next() == deletedChild; };
    auto it = std::find_if( begin( this ), end( this ), compare );
    if ( *it )
    {
	// logDebug() << "Unlinking " << deletedChild << Qt::endl;
	it->setNext( deletedChild->next() );
	return;
    }

    logError() << "Couldn't unlink " << deletedChild << " from " << this << " children list" << Qt::endl;
}


void DirInfo::readJobAdded()
{
    ++_pendingReadJobs;

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
	++_errSubDirs;

    if ( parent() )
	parent()->readJobFinished( dir );
}


void DirInfo::readJobAborted()
{
    _readState = DirAborted;

    if ( parent() )
	parent()->readJobAborted();
}


QLatin1String DirInfo::sizePrefix() const
{
    switch ( _readState )
    {
	case DirError:
	case DirAborted:
	case DirPermissionDenied:
	    return "> "_L1;

	case DirFinished:
//	case DirCached:
	    if ( _errSubDirs > 0 )
		return "> "_L1;
	    break;

	case DirQueued:
	case DirReading:
	case DirOnRequestOnly:
	    break;
	// No 'default' branch so the compiler can catch unhandled enum values
    }

    return QLatin1String{};
}


void DirInfo::finalizeLocal()
{
    cleanupDotEntries();
    cleanupAttics();
    checkIgnored();
}


void DirInfo::finalizeAll()
{
    for ( DirInfoIterator it{ this }; *it; ++it )
	it->finalizeAll();

    // Optimization: as long as this directory is not finalized yet, it does
    // (very likely) have a dot entry and thus all direct children are
    // subdirectories, not plain files, so we don't need to bother checking
    // plain file children as well - so do finalizeLocal() only after all
    // children are processed. If this step were the first, directories that
    // don't have any subdirectories finalizeLocal() would immediately get
    // all their plain file children reparented to themselves, so they would
    // need to be processed in the loop, too.

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

	// Reparent the dot entry's attic children to this item's attic
	if ( _dotEntry->hasAtticChildren() )
	{
	    ensureAttic();
	    _attic->takeAllChildren( _dotEntry->attic() );
	    _attic->_summaryDirty = true;
	    _attic->dropSortCache();
	}
    }

    // Delete the dot entry if it is now empty.
    //
    // This also takes care of dot entries that were just disowned because
    // they had no siblings (i.e. there are no subdirectories on this level)
    if ( !_dotEntry->firstChild() && !_dotEntry->hasAtticChildren() )
    {
	delete _dotEntry;
	_dotEntry = nullptr;

//	--_childCount;
	dropSortCache();
	_summaryDirty = true;
    }
}


void DirInfo::cleanupAttics()
{
    if ( _dotEntry )
	_dotEntry->cleanupAttics();

    if ( _attic )
    {
	_attic->finalizeLocal();
	deleteEmptyAttic();
    }
}


void DirInfo::checkIgnored()
{
    // finalizeLocal won't call this for dot entries, so call them from here
    if ( _dotEntry )
	_dotEntry->checkIgnored();

    // Display all directories as ignored that have any ignored items, but no
    // items that are not ignored.
    const bool wasIgnored = isIgnored();
    const bool newIgnored = totalIgnoredItems() > 0 && totalUnignoredItems() == 0;
    setIgnored( newIgnored );

    if ( wasIgnored != newIgnored)
	_summaryDirty = true;

    // Empty directories have no ignored items so haven't been set to ignored yet
    if ( newIgnored )
    {
	// Any children must have totalUnignoredItems == 0, so ignore them all
	for ( DotEntryIterator it{ this }; *it; ++it )
	    it->setIgnored( true );
    }

    // Cascade the 'ignored' status up the tree
    if ( !isDotEntry() && parent() )
	parent()->checkIgnored();
}


const DirSortInfo * DirInfo::newSortInfo( DataColumn sortCol, Qt::SortOrder sortOrder )
{
    // Clea old sorted children lists and create new ones
    dropSortCaches(); // recursive to all ancestors
    _sortInfo = new DirSortInfo{ this, sortCol, sortOrder };

    return _sortInfo;
}


void DirInfo::dropSortCaches()
{
    // If this dir didn't have any sort cache, there won't be any in the subtree
    if ( !_sortInfo )
	return;

    // logDebug() << "Dropping sort cache for " << this << Qt::endl;

    //  Dot entries don't have dir children (or dot entries) that could have a sort cache
    if ( !isDotEntry() )
    {
	for ( DirInfoIterator it{ this }; *it; ++it )
	    it->dropSortCaches();

	if ( _dotEntry )
	    _dotEntry->dropSortCaches();
    }

    if ( _attic )
	_attic->dropSortCaches();

    dropSortCache();
}


const DirInfo * DirInfo::findNearestMountPoint() const
{
    const DirInfo * dir = this;
    while ( dir->parent() != tree()->root() && !dir->isMountPoint() )
	dir = dir->parent();

    return dir;
}


void DirInfo::takeAllChildren( DirInfo * oldParent )
{
    FileInfo * child = oldParent->firstChild();
    if ( child )
    {
	// logDebug() << "Reparenting all children of " << oldParent << " to " << this << Qt::endl;

	while ( child->next() )
	{
	    child->setParent( this );
	    child = child->next();
	}

	child->setParent( this );
	child->setNext( _firstChild );

	_firstChild = oldParent->firstChild();
	oldParent->setFirstChild( nullptr );

	// Recalcs are taken care of by the callers
	// Need to recalc(), but oldParent will be deleted and other ancestors are still correct
//	oldParent->_summaryDirty = true;
//	_summaryDirty = true;
//	dropSortCache();
    }
}


void DirInfo::finishReading( DirReadState readState )
{
    setReadState( readState );
    finalizeLocal();
    tree()->sendReadJobFinished( this );
}




DirSortInfo::DirSortInfo( DirInfo       * parent,
                          DataColumn      sortCol,
                          Qt::SortOrder   sortOrder ):
    _sortedCol{ sortCol },
    _sortedOrder{ sortOrder }
{
    // Make space for all the children, including a dot entry and attic
    _sortedChildren.reserve( parent->childCount() );

    // Add the children and any dot entry
    for ( DotEntryIterator it{ parent }; *it; ++it )
	_sortedChildren.append( *it );

    // logDebug() << "Sorting children of " << this << " by " << sortCol << Qt::endl;

    // Do secondary sorting by NameCol (always in ascending order)
    if ( sortCol != NameCol )
    {
	const auto sorter = FileInfoSorter( NameCol, Qt::AscendingOrder );
	std::stable_sort( _sortedChildren.begin(), _sortedChildren.end(), sorter );
    }

    // Primary sorting as requested
    const auto sorter = FileInfoSorter( sortCol, sortOrder );
    std::stable_sort( _sortedChildren.begin(), _sortedChildren.end(), sorter );

    // Add any attic, always last whatever the sort order
    if ( parent->attic() )
	_sortedChildren.append( parent->attic() );

#if DIRECT_CHILDREN_COUNT_SANITY_CHECK
    if ( _sortedChildren.size() != parent->childCount() )
    {
	dumpChildrenList( parent, _sortedChildren );

	THROW( Exception{ QString{ "_childCount of %1 corrupted; is %2, should be %3" }
	                  .arg( parent->debugUrl(), parent->childCount(), _sortedChildren.size() ) } );
    }
#endif

    // Store the sort order number for each item directly on the FileInfo object
    DirSize childNumber = 0;
    for ( FileInfo * item : asConst( _sortedChildren ) )
	item->setRowNumber( childNumber++ );
}


DirSize DirSortInfo::findDominantChildren()
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

	const DirSize count = qMin( _sortedChildren.size(), DOMINANCE_ITEM_COUNT );

	// Declare that only one child (ie. 100%) doesn't count as dominant
	if ( count < 2 )
	    return 0;

	const float medianPercent    = _sortedChildren.at( count / 2 )->subtreeAllocatedPercent();
	const float dominancePercent = DOMINANCE_FACTOR * medianPercent;
	const float threshold        = qBound( DOMINANCE_MIN_PERCENT, dominancePercent, DOMINANCE_MAX_PERCENT );

#if VERBOSE_DOMINANCE_CHECK
	logDebug() << this
	           << "  median: "    << formatPercent( medianPercent )
	           << "  threshold: " << formatPercent( threshold )
	           << Qt::endl;
#endif

	// Return the child number of the first child after the dominance threshold
	for ( FileInfo * child : asConst( _sortedChildren ) )
	{
	    if ( child->subtreeAllocatedPercent() < threshold )
		return child->rowNumber();
	}

	// Should never get here, children can't all be dominant
	return 0;
    }();

    return _firstNonDominantChild;
}
