/*
 *   File name: FileInfoIterator.cpp
 *   Summary:   Support classes for QDirStat - DirTree iterator classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "FileInfoIterator.h"
#include "FileInfoSorter.h"


using namespace QDirStat;


BySizeIterator::BySizeIterator( const FileInfo * parent )
{
    _sortedChildren.reserve( parent->childCountConst() );

    for ( DotEntryIterator it{ parent }; *it; ++it )
    {
	_sortedChildren << *it;
	_totalSize += it->itemTotalSize();
    }

    auto sorter = FileInfoSorter( SizeCol, Qt::DescendingOrder );
    std::stable_sort( _sortedChildren.begin(), _sortedChildren.end(), sorter );

    _currentIt = _sortedChildren.cbegin();
}
