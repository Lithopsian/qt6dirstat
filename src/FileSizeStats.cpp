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


namespace
{
    bool canCollect( const FileInfo * subtree, bool excludeSymLinks )
    {
        return ( !excludeSymLinks && subtree->isSymLink() ) || subtree->isFile();
    }

}


FileSizeStats::FileSizeStats( FileInfo * subtree, const QString & suffix, bool excludeSymLinks ):
    PercentileStats{}
{
    CHECK_PTR( subtree );

    if ( suffix.isEmpty() )
    {
        reserve( subtree->totalNonDirItems() );
        collect( subtree, excludeSymLinks );
    }
    else
    {
        collect( subtree, suffix, excludeSymLinks );
    }

    sort();
}


void FileSizeStats::collect( const FileInfo * subtree, bool excludeSymLinks )
{
    if ( canCollect( subtree, excludeSymLinks ) )
        append( subtree->size() );

    for ( DotEntryIterator it{ subtree }; *it; ++it )
        collect( *it, excludeSymLinks );
}


void FileSizeStats::collect( const FileInfo * subtree, const QString & suffix, bool excludeSymLinks )
{
    if ( canCollect( subtree, excludeSymLinks ) && subtree->name().endsWith( suffix ) )
        append( subtree->size() );

    for ( DotEntryIterator it{ subtree }; *it; ++it )
        collect( *it, suffix, excludeSymLinks );
}
