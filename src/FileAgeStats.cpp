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


FileAgeStats::FileAgeStats( const FileInfo * subtree ):
    _thisYear{ static_cast<short>( QDate::currentDate().year() ) },
    _thisMonth{ static_cast<short>( QDate::currentDate().month() ) }
{
    if ( subtree && subtree->checkMagicNumber() )
        collectRecursive( subtree );
}


void FileAgeStats::collectRecursive( const FileInfo * dir )
{
    for ( DotEntryIterator it{ dir }; *it; ++it )
    {
        const FileInfo * item = *it;

        if ( item->hasChildren() )
        {
            collectRecursive( item );
        }
        else if ( item->isFileOrSymlink() )
        {
            ++_totalCount;
            _totalSize += item->size();

            const auto  yearAndMonth = item->yearAndMonth();
            const short year         = yearAndMonth.year;
            const short month        = yearAndMonth.month;

            YearMonthStats & yearStats = _yearStats[ year ];
            ++yearStats.count;
            yearStats.size += item->size();

            YearMonthStats & monthStats = _monthStats[ yearMonthHash( year, month ) ];
            ++monthStats.count;
            monthStats.size += item->size();
        }
    }
}
