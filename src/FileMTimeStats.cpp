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
        reserve( subtree->totalFiles() );
        collect( subtree );
        sort();
    }
}


void FileMTimeStats::collect( FileInfo * subtree )
{
    // Disregard symlinks, block devices and other special files
    if ( subtree->isFile() )
        append( subtree->mtime() );

    for ( FileInfoIterator it( subtree ); *it; ++it )
        collect( *it );
}
