/*
 *   File name: FileMTimeStats.cpp
 *   Summary:	Statistics classes for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <math.h>       // ceil()
#include <algorithm>

#include "FileMTimeStats.h"
#include "FileInfo.h"
#include "FileInfoIterator.h"
#include "DirTree.h"
#include "Exception.h"


using namespace QDirStat;


FileMTimeStats::FileMTimeStats( FileInfo * subtree ):
    PercentileStats ()
{
    if ( subtree )
    {
        collect( subtree );
        sort();
    }
}


void FileMTimeStats::collect( FileInfo * subtree )
{
    Q_CHECK_PTR( subtree );

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