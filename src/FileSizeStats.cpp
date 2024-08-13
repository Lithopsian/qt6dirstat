/*
 *   File name: FileSizeStats.cpp
 *   Summary:   Statistics classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "FileSizeStats.h"
#include "Exception.h"
#include "FileInfoIterator.h"
#include "FormatUtil.h"


using namespace QDirStat;


FileSizeStats::FileSizeStats( FileInfo * subtree ):
    PercentileStats{}
{
    CHECK_PTR( subtree );

    // Avoid reallocations for potentially millions of list appends
    reserve( subtree->totalFiles() );
    collect( subtree );
    sort();
}


FileSizeStats::FileSizeStats( const FileInfo * subtree, const QString & suffix ):
    PercentileStats{}
{
    CHECK_PTR( subtree );

    collect( subtree, suffix );
    sort();
}


void FileSizeStats::collect( const FileInfo * subtree )
{
    if ( subtree->isFile() )
        append( subtree->size() );

    for ( DotEntryIterator it{ subtree }; *it; ++it )
        collect( *it );
}


void FileSizeStats::collect( const FileInfo * subtree, const QString & suffix )
{
    if ( subtree->isFile() && subtree->name().toLower().endsWith( suffix ) )
        append( subtree->size() );

    for ( DotEntryIterator it{ subtree }; *it; ++it )
        collect( *it, suffix );
}
