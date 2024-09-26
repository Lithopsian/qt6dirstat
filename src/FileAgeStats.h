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

#include "Typedefs.h" // FileSize


namespace QDirStat
{
    class FileInfo;
    class YearStats;

    typedef QHash<short, YearStats> YearStatsHash;
    typedef QList<short>            YearsList;


    /**
     * File modification year / month statistics for one year or one month.
     **/
    struct YearStats
    {
        short    year;                     // 1970 (time_t 0 seconds) - 32767
        short    month;                    // 1-12 or 0 for the complete year
        float    filesPercent{ 0.0f };     // 0.0 .. 100.0
        int      filesCount{ 0 };
        float    sizePercent{ 0.0f };      // 0.0 .. 100.0
        FileSize size{ 0LL };

        YearStats( short yr = 0, short mn = 0 ):
            year{ yr },
            month{ mn }
        {}

    };  // struct YearStats


    /**
     * Class for calculating and storing file age statistics, i.e. statistics
     * about the years of the last modification times of files in a subtree.
     **/
    class FileAgeStats
    {
    public:

        /**
         * Constructor.  Collects data for the given subtree.
         **/
        FileAgeStats( const FileInfo * subtree );

        /**
         * Return a sorted list of the years where files with that modification
         * year were found after collecting data.
         **/
        const YearsList & years() const { return _yearsList; }

        /**
         * Return year statistics for the specified year or 0 if there are
         * none.
         **/
        YearStats * yearStats( short year )
                { return _yearStats.contains( year ) ? &( _yearStats[ year ] ) : nullptr; }

        /**
         * Return the month statistics for the specified year and month
         * or 0 if there are none.
         *
         * Month statistics are only available for this year and the last year.
         **/
        YearStats * monthStats( short year, short month );

        /**
         * Return 'true' if month statistics are available for the specified
         * year.
         *
         * Month statistics are only available for this year and the last year.
         **/
        bool monthStatsAvailableFor( short year ) const { return year == thisYear() || year == lastYear(); }

        /**
         * Return the current year.
         **/
        short thisYear() const { return _thisYear; }

        /**
         * Return the current month (1-12).
         **/
        short thisMonth() const { return _thisMonth; }

        /**
         * Return the year before the current year.
         **/
        short lastYear() const { return _thisYear - 1; }


    protected:

        /**
         * Initialise all month stats for this or the last year.
         **/
        void initMonthStats( short year );

        /**
         * Recurse through all file elements in the subtree and calculate the
         * data for that subtree.
         **/
        void collectRecursive( const FileInfo * subtree );

        /**
         * Sum up the totals over all years and calculate the percentages for
         * each year
         **/
        void calcPercentages();

        /**
         * Calculate the monthly percentages for all 12 months in one year.
         **/
        void calcMonthPercentages( short year );

        /**
         * Fill the _yearsList with all the years in the _yearStats hash and
         * sort the list.
         **/
        void collectYears();


    private:

        YearStatsHash   _yearStats;
        YearsList       _yearsList;

        YearStats       _thisYearMonthStats[ 12 ];
        YearStats       _lastYearMonthStats[ 12 ];

        short           _thisYear;
        short           _thisMonth;

    };  // class FileAgesStats

}       // namespace QDirStat

#endif  // FileAgeStats_h
