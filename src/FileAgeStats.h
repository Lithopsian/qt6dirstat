/*
 *   File name: FileAgeStats.h
 *   Summary:	Statistics classes for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#ifndef FileAgeStats_h
#define FileAgeStats_h

#include <QHash>
#include <QList>

#include "FileSize.h"


namespace QDirStat
{
    class FileInfo;
    class YearStats;

    typedef QHash<short, YearStats>     YearStatsHash;
    typedef QList<short>                YearsList;


    /**
     * File modification year / month statistics for one year or one month.
     **/
    class YearStats
    {
    public:

	short		year;           // 1970-2037 (time_t range)
        short           month;          // 1-12 or 0 for the  complete year
	int		filesCount;
	float		filesPercent;	// 0.0 .. 100.0
	FileSize	size;
	float		sizePercent;	// 0.0 .. 100.0

	YearStats( short yr = 0, short mn = 0 ):
            year { yr },
            month { mn },
            filesCount { 0 },
            filesPercent { 0.0 },
            size { 0 },
            sizePercent { 0.0 }
        {}

    };  // class YearStats


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
        YearStats * yearStats( short year );

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
        static short thisYear();

        /**
         * Return the current month (1-12).
         **/
        static short thisMonth();

        /**
         * Return the year before the current year.
         **/
        static short lastYear();


    protected:

        /**
         * Recurse through all file elements in the subtree and calculate the
         * data for that subtree.
         **/
        void collect( const FileInfo * subtree );

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

        //
        // Data Members
        //

        YearStatsHash   _yearStats;
        YearsList       _yearsList;

        YearStats       _thisYearMonthStats[ 12 ];
        YearStats       _lastYearMonthStats[ 12 ];

    };  // class FileAgesStats

}       // namespace QDirStat


#endif  // FileAgeStats_h
