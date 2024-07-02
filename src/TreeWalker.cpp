/*
 *   File name: TreeWalker.cpp
 *   Summary:   QDirStat helper class to walk a FileInfo tree
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "TreeWalker.h"
#include "FileInfo.h"
#include "FileSizeStats.h"
#include "FileMTimeStats.h"
#include "SysUtil.h"


#define MAX_RESULTS              200
#define MAX_FIND_FILES_RESULTS  1000


using namespace QDirStat;


namespace
{
    /**
     * Calculate a data value threshold from a set of PercentileStats from
     * an upper percentile up to the maximum value (P100).
     **/
    double upperPercentileThreshold( PercentileStats & stats )
    {
        if ( stats.size() <= 100               ) return stats.percentile( 80 );
        if ( stats.size() <= MAX_RESULTS * 10  ) return stats.percentile( 90 );
        if ( stats.size() <= MAX_RESULTS * 20  ) return stats.percentile( 95 );
        if ( stats.size() <= MAX_RESULTS * 100 ) return stats.percentile( 99 );

        return stats.at( stats.size() - MAX_RESULTS );
    }


    /**
     * Calculate a data value threshold from a set of PercentileStats from
     * an the minimum value (P0) to a lower percentile.
     **/
    double lowerPercentileThreshold( PercentileStats & stats )
    {
        if ( stats.size() <= 100               ) return stats.percentile( 20 );
        if ( stats.size() <= MAX_RESULTS * 10  ) return stats.percentile( 10 );
        if ( stats.size() <= MAX_RESULTS * 20  ) return stats.percentile(  5 );
        if ( stats.size() <= MAX_RESULTS * 100 ) return stats.percentile(  1 );

        return stats.at( MAX_RESULTS );
    }

} // namespace


void LargestFilesTreeWalker::prepare( FileInfo * subtree )
{
    TreeWalker::prepare( subtree );
    FileSizeStats stats( subtree );

    // Implicit conversion from double, only valid up to 2^53
    _threshold = ( FileSize )upperPercentileThreshold( stats );
}


bool LargestFilesTreeWalker::check( FileInfo * item )
{
    return item && item->isFile() && item->size() >= _threshold;
}


void NewFilesTreeWalker::prepare( FileInfo * subtree )
{
    TreeWalker::prepare( subtree );
    FileMTimeStats stats( subtree );

    // Implicit conversion from double, only valid up to 2^53
    _threshold = ( time_t )upperPercentileThreshold( stats );
}


bool NewFilesTreeWalker::check( FileInfo * item )
{
    return item && item->isFile() && item->mtime() >= _threshold;
}


void OldFilesTreeWalker::prepare( FileInfo * subtree )
{
    TreeWalker::prepare( subtree );
    FileMTimeStats stats( subtree );

    // Implicit conversion from double, only valid up to 2^53
    _threshold = ( time_t )lowerPercentileThreshold( stats );
}


bool OldFilesTreeWalker::check( FileInfo * item )
{
    return item && item->isFile() && item->mtime() <= _threshold;
}


bool HardLinkedFilesTreeWalker::check( FileInfo * item )
{
    return item && item->isFile() && item->links() > 1;
}


bool BrokenSymLinksTreeWalker::check( FileInfo * item )
{
    return item && item->isSymLink() && item->isBrokenSymLink();
}


bool SparseFilesTreeWalker::check( FileInfo * item )
{
    return item && item->isFile() && item->isSparseFile();
}


bool FilesFromYearTreeWalker::check( FileInfo * item )
{
    return item && item->isFile() && item->yearAndMonth().year == _year;
}


bool FilesFromMonthTreeWalker::check( FileInfo * item )
{
    if ( !item || !item->isFile() )
        return false;

    const auto yearAndMonth = item->yearAndMonth();
    return yearAndMonth.year == _year && yearAndMonth.month == _month;
}


void FindFilesTreeWalker::prepare( FileInfo * subtree )
{
    TreeWalker::prepare( subtree );
    _count = 0;
}


bool FindFilesTreeWalker::check( FileInfo * item )
{
    if ( _count >= MAX_FIND_FILES_RESULTS )
    {
        setOverflow();
        return false;
    }

    if ( !item )
        return false;

    if ( ( !_filter.findDirs()     || !item->isDir()     ) &&
         ( !_filter.findFiles()    || !item->isFile()    ) &&
         ( !_filter.findSymLinks() || !item->isSymLink() ) &&
         ( !_filter.findPkgs()     || !item->isPkgInfo() ) )
    {
        return false;
    }

    const bool match = _filter.matches( item->name() );
    if ( match )
        ++_count;

    return match;
}
