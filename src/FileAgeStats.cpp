/*
 *   File name: FileAgeStats.cpp
 *   Summary:   Statistics classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <algorithm>    // std::sort()
#include <QDate>

#include "FileAgeStats.h"
#include "Exception.h"
#include "FileInfo.h"
#include "FileInfoIterator.h"


using namespace QDirStat;


FileAgeStats::FileAgeStats( const FileInfo * subtree ):
    _thisYear { static_cast<short>( QDate::currentDate().year() ) },
    _thisMonth { static_cast<short>( QDate::currentDate().month() ) }
{
    CHECK_PTR( subtree );

    initMonthStats( thisYear() );
    initMonthStats( lastYear() );

    collect( subtree );
}


void FileAgeStats::initMonthStats( short year )
{
    for ( int month = 1; month <= 12; month++ )
    {
        YearStats * stats = monthStats( year, month );
        if ( stats )
            *stats = YearStats( year, month );
    }
}


void FileAgeStats::collect( const FileInfo * subtree )
{
    collectRecursive( subtree );
    calcPercentages();
    collectYears();
}


void FileAgeStats::collectRecursive( const FileInfo * dir )
{
    if ( ! dir )
        return;

    for ( FileInfoIterator it( dir ); *it; ++it )
    {
        const FileInfo * item = *it;

        if ( item && item->isFile() )
        {
            const auto  yearAndMonth = item->yearAndMonth();
            const short year         = yearAndMonth.year;
            const short month        = yearAndMonth.month;

            YearStats & yearStats = _yearStats[ year ];
            yearStats.year = year;
            yearStats.filesCount++;
            yearStats.size += item->size();

            YearStats * thisMonthStats = monthStats( year, month );
            if ( thisMonthStats )
            {
                thisMonthStats->filesCount++;
                thisMonthStats->size += item->size();
            }
        }

        if ( item->hasChildren() )
            collectRecursive( item );
    }
}


void FileAgeStats::calcPercentages()
{
    // Sum up the totals over all years
    int totalFilesCount      = 0;
    FileSize totalFilesSize  = 0LL;

    for ( const YearStats & stats : asConst( _yearStats ) )
    {
        totalFilesCount += stats.filesCount;
        totalFilesSize  += stats.size;
    }

    for ( YearStats & stats : _yearStats )
    {
        if ( totalFilesCount > 0 )
            stats.filesPercent = ( 100.0f * stats.filesCount ) / totalFilesCount;

        if ( totalFilesSize > 0 )
            stats.sizePercent = ( 100.0f * stats.size ) / totalFilesSize;
    }

    calcMonthPercentages( thisYear() );
    calcMonthPercentages( lastYear() );
}


void FileAgeStats::calcMonthPercentages( short year )
{
    const YearStats * thisYearStats = yearStats( year );
    if ( !thisYearStats )
        return;

    for ( int month = 1; month <= 12; month++ )
    {
        YearStats * stats = monthStats( year, month );
        if ( stats )
        {
            if ( thisYearStats->filesCount > 0 )
                stats->filesPercent = ( 100.0f * stats->filesCount ) / thisYearStats->filesCount;

            if ( thisYearStats->size > 0 )
                stats->sizePercent = ( 100.0f * stats->size ) / thisYearStats->size;
        }
    }
}


void FileAgeStats::collectYears()
{
    _yearsList = _yearStats.keys();
    std::sort( _yearsList.begin(), _yearsList.end() );
}


YearStats * FileAgeStats::yearStats( short year )
{
    if ( _yearStats.contains( year ) )
        return &( _yearStats[ year ] );

    return nullptr;
}


YearStats * FileAgeStats::monthStats( short year, short month )
{
    if ( month >= 1 && month <= 12 )
    {
        if ( year == thisYear() && month <= thisMonth() )
            return &( _thisYearMonthStats[ month - 1 ] );

        if ( year == lastYear() )
            return &( _lastYearMonthStats[ month - 1 ] );
    }

    return nullptr;
}
