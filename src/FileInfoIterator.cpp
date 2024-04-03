/*
 *   File name: FileInfoIterator.cpp
 *   Summary:	Support classes for QDirStat - DirTree iterator classes
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <algorithm>

#include "FileInfo.h"
#include "FileInfoIterator.h"
#include "FileInfoSorter.h"
#include "DotEntry.h"
#include "Exception.h"

using namespace QDirStat;



FileInfoIterator::FileInfoIterator( const FileInfo * parent ) :
    _parent { parent }
{
    next();
}


void FileInfoIterator::next()
{
    if ( !_directChildrenProcessed )
    {
	// Process direct children
	_current = _current ? _current->next() : _parent->firstChild();

	if ( !_current )
	{
	    _directChildrenProcessed = true;
	    next();
	}
    }
    else // _directChildrenProcessed
    {
	if ( !_dotEntryProcessed )
	{
	    // Process dot entry
	    _current = _parent->dotEntry();
	    _dotEntryProcessed = true;
	}
	else	// Dot entry already processed
	{
	    _current = nullptr;
	}
    }
}


FileInfoSortedBySizeIterator::FileInfoSortedBySizeIterator( FileInfo		* parent,
							    FileSize		( *itemTotalSize )( FileInfo * ),
							    Qt::SortOrder	sortOrder )
{
    _sortedChildren.reserve( parent->directChildrenCount() );
    for ( FileInfoIterator it( parent ); *it; ++it )
    {
	_sortedChildren << *it;

	if ( itemTotalSize )
	    _totalSize += ( *itemTotalSize )( *it );
    }

    std::stable_sort( _sortedChildren.begin(),
		      _sortedChildren.end(),
		      FileInfoSorter( SizeCol, sortOrder ) );

    _currentIt = _sortedChildren.cbegin();
}
