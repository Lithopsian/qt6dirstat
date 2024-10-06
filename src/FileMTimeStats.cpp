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
#include "FileInfoIterator.h"


using namespace QDirStat;


FileMTimeStats::FileMTimeStats( FileInfo * subtree ):
    PercentileStats{}
{
    if ( subtree && subtree->checkMagicNumber() )
    {
        reserve( subtree->totalNonDirItems() );
        collect( subtree );
        sort();
    }
}


void FileMTimeStats::collect( FileInfo * subtree )
{
    // Disregard block devices and other special files
    if ( subtree->isFileOrSymLink() )
        append( subtree->mtime() );

    for ( DotEntryIterator it{ subtree }; *it; ++it )
        collect( *it );
}
