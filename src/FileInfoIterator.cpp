/*
 *   File name: FileInfoIterator.cpp
 *   Summary:   Support classes for QDirStat - DirTree iterator classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <algorithm> // std::stable_sort

#include "FileInfo.h"
#include "FileInfoIterator.h"
#include "FileInfoSorter.h"
#include "DotEntry.h"


using namespace QDirStat;


FileInfoIterator::FileInfoIterator( const FileInfo * parent ):
    _dotEntry { parent->dotEntry() },
    _current { parent->firstChild() ? parent->firstChild() : _dotEntry }
{
}


FileInfo * FileInfoIterator::next()
{
    if ( !_current )
	return nullptr;

    if ( _current->next() )
	return _current->next();

    return _current == _dotEntry ? nullptr : _dotEntry;
}




FileInfoBySizeIterator::FileInfoBySizeIterator( const FileInfo * parent )
{
    _sortedChildren.reserve( parent->directChildrenCountConst() );

    for ( FileInfoIterator it( parent ); *it; ++it )
    {
	_sortedChildren << *it;
	_totalSize += ( *it )->itemTotalSize();
    }

    std::stable_sort( _sortedChildren.begin(),
                      _sortedChildren.end(),
                      FileInfoSorter( SizeCol, Qt::DescendingOrder ) );

    _currentIt = _sortedChildren.cbegin();
}
