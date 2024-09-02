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
    PercentileBoundary upperPercentileThreshold( PercentileStats & stats )
    {
        if ( stats.size() <= 100               ) return stats.percentile( 80 );
        if ( stats.size() <= MAX_RESULTS * 10  ) return stats.percentile( 90 );
        if ( stats.size() <= MAX_RESULTS * 20  ) return stats.percentile( 95 );
        if ( stats.size() <= MAX_RESULTS * 100 ) return stats.percentile( 99 );

        return stats.at( stats.size() - MAX_RESULTS ); // check() for >= this value
    }


    /**
     * Calculate a data value threshold from a set of PercentileStats from
     * the minimum value (P0) to a lower percentile.
     **/
    PercentileBoundary lowerPercentileThreshold( PercentileStats & stats )
    {
        if ( stats.size() <= 100               ) return stats.percentile( 20 );
        if ( stats.size() <= MAX_RESULTS * 10  ) return stats.percentile( 10 );
        if ( stats.size() <= MAX_RESULTS * 20  ) return stats.percentile(  5 );
        if ( stats.size() <= MAX_RESULTS * 100 ) return stats.percentile(  1 );

        return stats.at( MAX_RESULTS ); // check() for <= this value
    }

} // namespace


void LargestFilesTreeWalker::prepare( FileInfo * subtree )
{
    FileSizeStats stats{ subtree };

    _threshold = std::floor( upperPercentileThreshold( stats ) );
}


bool LargestFilesTreeWalker::check( const FileInfo * item )
{
    return item && item->isFileOrSymLink() && item->size() >= _threshold;
}




void NewFilesTreeWalker::prepare( FileInfo * subtree )
{
    FileMTimeStats stats{ subtree };

    _threshold = std::floor( upperPercentileThreshold( stats ) );
}


bool NewFilesTreeWalker::check( const FileInfo * item )
{
    return item && item->isFileOrSymLink() && item->mtime() >= _threshold;
}




void OldFilesTreeWalker::prepare( FileInfo * subtree )
{
    FileMTimeStats stats{ subtree };

    _threshold = std::ceil( lowerPercentileThreshold( stats ) );
}


bool OldFilesTreeWalker::check( const FileInfo * item )
{
    return item && item->isFileOrSymLink() && item->mtime() <= _threshold;
}




bool HardLinkedFilesTreeWalker::check( const FileInfo * item )
{
    return item && item->isFile() && item->links() > 1;
}


bool BrokenSymLinksTreeWalker::check( const FileInfo * item )
{
    return item && item->isSymLink() && item->isBrokenSymLink();
}




bool SparseFilesTreeWalker::check( const FileInfo * item )
{
    return item && item->isFile() && item->isSparseFile();
}




bool FilesFromYearTreeWalker::check( const FileInfo * item )
{
    return item && item->isFileOrSymLink() && item->yearAndMonth().year == _year;
}




bool FilesFromMonthTreeWalker::check( const FileInfo * item )
{
    if ( !item || !item->isFileOrSymLink() )
        return false;

    const auto yearAndMonth = item->yearAndMonth();
    return yearAndMonth.year == _year && yearAndMonth.month == _month;
}




void FindFilesTreeWalker::prepare( FileInfo * )
{
    _count = 0;
    _overflow = false;
}


bool FindFilesTreeWalker::check( const FileInfo * item )
{
    if ( _count >= MAX_FIND_FILES_RESULTS )
    {
        _overflow = true;
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

    if ( !_filter.matches( item->name() ) )
        return false;

    ++_count;

    return true;
}
