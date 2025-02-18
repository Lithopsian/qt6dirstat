/*
 *   File name: FileAgeStats.h
 *   Summary:   Statistics classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef FileAgeStats_h
#define FileAgeStats_h

#include <QHash>

#include "Typedefs.h" // FileCount, FileSize


namespace QDirStat
{
    class FileInfo;

    /**
     * File count and size statistics for one year or one month.
     *
     * Note that this struct is small enough and simple enough that
     * it should generally be passed by value.
     **/
    struct YearMonthStats
    {
        FileCount count{ 0 };
        FileSize  size { 0 };
    };  // struct YearMonthStats


    typedef QHash<short, YearMonthStats> YearStatsHash;
    typedef QHash<int,   YearMonthStats> MonthStatsHash;
    typedef QList<short>                 YearsList; // QList from QHash::keys


    /**
     * Class for calculating and storing file age statistics, i.e. statistics
     * about the years of the last modification times of files in a subtree.
     **/
    class FileAgeStats final
    {
    public:

        /**
         * Constructor.  Collects data for the given subtree.
         **/
        FileAgeStats( const FileInfo * subtree );

        /**
         * Return an unsorted list of the years where files with that
         * modification year were found after collecting data.
         **/
        YearsList years() const
            { return _yearStats.keys(); }

        /**
         * Return 'true' if year statistics are available for the specified
         * year.
         **/
        bool yearStatsAvailable( short year ) const
            { return _yearStats.contains( year ); }

        /**
         * Return year statistics for the specified year.
         *
         * If this function is called when yearStatsAvailableFor() returns
         * false, it will return a default-constructed YearMonthStats
         * struct with count and size both 0.
         **/
        YearMonthStats yearStats( short year ) const
            { return _yearStats.value( year, YearMonthStats{} ); }

        /**
         * Return the month statistics for the specified year and month.
         *
         * Note that this function will return a default-constructed
         * YearMonthStats struct if no files were collected for this
         * month, but it does not create a default-constructed entry
         * in the hash table.
         **/
        YearMonthStats monthStats( short year, short month ) const
            { return _monthStats.value( yearMonthHash( year, month ), YearMonthStats{} ); }

        /**
         * Return the current year.
         **/
        short thisYear() const { return _thisYear; }

        /**
         * Return the current month (1-12).
         **/
        short thisMonth() const { return _thisMonth; }

        /**
         * Return the percentage of 'count' or 'size' wrt the total.
         **/
        float countPercent( FileCount count ) const
            { return _totalCount == 0 ? 100 : 100 * ( 1.0 * count / _totalCount ); }
        float sizePercent( FileSize size ) const
            { return _totalSize == 0 ? 100 : 100 * ( 1.0 * size / _totalSize ); }

        /**
         * Return a unique value for the combination 'year' and 'month', to
         * be used as the hash key for MonthStats.
         **/
        static int yearMonthHash( short year, short month )
            { return year * 12 + month; }


    private:

        short          _thisYear;
        short          _thisMonth;

        FileCount      _totalCount{ 0 };
        FileSize       _totalSize{ 0 };

        YearStatsHash  _yearStats;
        MonthStatsHash _monthStats;

    };  // class FileAgesStats

}       // namespace QDirStat

#endif  // FileAgeStats_h
