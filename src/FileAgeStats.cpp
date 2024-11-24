/*
 *   File name: FileAgeStats.cpp
 *   Summary:   Statistics classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QDate>

#include "FileAgeStats.h"
#include "FileInfoIterator.h"


using namespace QDirStat;


namespace
{
    /**
     * Recurse through all file elements in the subtree 'dir' and calculate the
     * stats for that subtree.
     **/
    void collectRecursive( FileCount       & totalCount,
                           FileSize        & totalSize,
                           YearStatsHash   & yearStats,
                           MonthStatsHash  & monthStats,
                           const FileInfo  * dir )
    {
        for ( DotEntryIterator it{ dir }; *it; ++it )
        {
            const FileInfo * item = *it;

            if ( item->hasChildren() )
            {
                collectRecursive( totalCount, totalSize, yearStats, monthStats, item );
            }
            else if ( item->isFileOrSymlink() )
            {
                ++totalCount;
                totalSize += item->size();

                const auto  yearAndMonth = item->yearAndMonth();
                const short year         = yearAndMonth.year;
                const short month        = yearAndMonth.month;

                YearMonthStats & yearStat = yearStats[ year ];
                ++yearStat.count;
                yearStat.size += item->size();

                YearMonthStats & monthStat = monthStats[ FileAgeStats::yearMonthHash( year, month ) ];
                ++monthStat.count;
                monthStat.size += item->size();
            }
        }
    }

}


FileAgeStats::FileAgeStats( const FileInfo * subtree ):
    _thisYear{ static_cast<short>( QDate::currentDate().year() ) },
    _thisMonth{ static_cast<short>( QDate::currentDate().month() ) }
{
    if ( subtree && subtree->checkMagicNumber() )
        collectRecursive( _totalCount, _totalSize, _yearStats, _monthStats, subtree );
}
