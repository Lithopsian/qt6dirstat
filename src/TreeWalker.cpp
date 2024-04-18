/*
 *   File name: TreeWalker.cpp
 *   Summary:   QDirStat helper class to walk a FileInfo tree
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "TreeWalker.h"
#include "FileSizeStats.h"
#include "FileMTimeStats.h"
#include "SysUtil.h"
#include "Logger.h"
#include "Exception.h"


#define MAX_RESULTS              200
#define MAX_FIND_FILES_RESULTS  1000


using namespace QDirStat;


namespace
{
    /**
     * Calculate a data value threshold from a set of PercentileStats from
     * an upper percentile up to the maximum value (P100).
     **/
    qreal upperPercentileThreshold( PercentileStats & stats )
    {
        const int percentile = [&stats]()
        {
            if ( stats.size() <= 100 )                return 80;
            if ( stats.size() * 0.10 <= MAX_RESULTS ) return 90;
            if ( stats.size() * 0.05 <= MAX_RESULTS ) return 95;
            if ( stats.size() * 0.01 <= MAX_RESULTS ) return 99;
            return 0;
        }();

        if ( percentile > 0 )
        {
            //logDebug() << "Threshold: " << percentile << ". percentile" << Qt::endl;
            return stats.percentile( percentile );
        }

        //logDebug() << "Threshold: " << MAX_RESULTS << " items" << Qt::endl;
        return stats.at( stats.size() - MAX_RESULTS );
    }


    /**
     * Calculate a data value threshold from a set of PercentileStats from
     * an the minimum value (P0) to a lower percentile.
     **/
    qreal lowerPercentileThreshold( PercentileStats & stats )
    {
        const int percentile = [&stats]()
        {
            if ( stats.size() <= 100 )                return 20;
            if ( stats.size() * 0.10 <= MAX_RESULTS ) return 10;
            if ( stats.size() * 0.05 <= MAX_RESULTS ) return 5;
            if ( stats.size() * 0.01 <= MAX_RESULTS ) return 1;
            return 0;
        }();

        if ( percentile > 0 )
        {
            //logDebug() << "Threshold: " << percentile << ". percentile" << Qt::endl;
            return stats.percentile( percentile );
        }

        //logDebug() << "Threshold: " << MAX_RESULTS << " items" << Qt::endl;
        return stats.at( MAX_RESULTS );
    }

} // namespace


void LargestFilesTreeWalker::prepare( FileInfo * subtree )
{
    TreeWalker::prepare( subtree );
    FileSizeStats stats( subtree );
    _threshold = ( FileSize )upperPercentileThreshold( stats );
}


void NewFilesTreeWalker::prepare( FileInfo * subtree )
{
    TreeWalker::prepare( subtree );
    FileMTimeStats stats( subtree );
    _threshold = ( time_t )upperPercentileThreshold( stats );
}


void OldFilesTreeWalker::prepare( FileInfo * subtree )
{
    TreeWalker::prepare( subtree );
    FileMTimeStats stats( subtree );
    _threshold = ( time_t )lowerPercentileThreshold( stats );
}


bool FilesFromMonthTreeWalker::check( FileInfo * item )
{
    if ( !item || !item->isFile() )
        return false;

    const auto yearAndMonth = item->yearAndMonth();
    return yearAndMonth.first  == _year && yearAndMonth.second == _month;
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

    if ( ! item )
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
