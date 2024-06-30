/*
 *   File name: FileMTimeStats.cpp
 *   Summary:   Statistics classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "FileMTimeStats.h"
#include "DirTree.h"
#include "FileInfo.h"
#include "FileInfoIterator.h"


using namespace QDirStat;


FileMTimeStats::FileMTimeStats( FileInfo * subtree ):
    PercentileStats {}
{
    if ( subtree )
    {
        collect( subtree );
        sort();
    }
}


void FileMTimeStats::collect( FileInfo * subtree )
{
    if ( isEmpty() )
        reserve( subtree->totalFiles() );

    if ( subtree->isFile() )
        append( subtree->mtime() );

    for ( FileInfoIterator it( subtree ); *it; ++it )
    {
        FileInfo * item = *it;

        // Disregard symlinks, block devices and other special files
        if ( item->hasChildren() )
            collect( item );
        else if ( item->isFile() )
            append( item->mtime() );
    }
}
