/*
 *   File name: FileSizeStats.cpp
 *   Summary:   Statistics classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "FileSizeStats.h"
#include "FileInfoIterator.h"
#include "Wildcard.h"


using namespace QDirStat;


FileSizeStats::FileSizeStats( FileInfo * subtree, bool excludeSymlinks ):
    PercentileStats{}
{
    if ( !subtree || !subtree->checkMagicNumber() )
        return;

    reserve( subtree->totalNonDirItems() );
    collect( subtree, excludeSymlinks );
    sort();
}


FileSizeStats::FileSizeStats( const FileInfo * subtree, const WildcardCategory & wildcardCategory ):
    PercentileStats{}
{
    if ( !subtree || !subtree->checkMagicNumber() )
        return;

    collect( subtree, wildcardCategory );
    sort();
}


void FileSizeStats::collect( const FileInfo * subtree, bool excludeSymlinks )
{
    if ( ( !excludeSymlinks && subtree->isSymlink() ) || subtree->isFile() )
        append( subtree->size() );

    for ( DotEntryIterator it{ subtree }; *it; ++it )
        collect( *it, excludeSymlinks );
}


void FileSizeStats::collect( const FileInfo * subtree, const WildcardCategory & wildcardCategory )
{
    if ( wildcardCategory.matches( subtree ) )
        append( subtree->size() );

    for ( DotEntryIterator it{ subtree }; *it; ++it )
        collect( *it, wildcardCategory );
}
