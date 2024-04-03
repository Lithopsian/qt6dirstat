/*
 *   File name: PercentileStats.h
 *   Summary:	Statistics classes for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#ifndef PercentileStats_h
#define PercentileStats_h

#include <QList>


typedef QVector<qreal> QRealList;


namespace QDirStat
{
    /**
     * Base class for percentile-related statistics calculation.
     *
     * Derived classes have to make sure to populate the list.
     *
     * Note that one data item (one qreal, i.e. one 64 bit double) is
     * stored for each file (or each matching file) in this object, so this is
     * expensive in terms of memory usage. Also, since data usually need to be
     * sorted for those calculations and sorting has at least logarithmic cost
     * O( n * log(n) ), this also has heavy performance impact.
     **/
    class PercentileStats: public QRealList
    {

    public:

	// All calculation functions below will sort the internal data first if
	// they are not sorted yet. This is why they are not const.  Note that
	// HistogramView now uses the percentile list to get these values.

	/**
	 * Calculate the median.
	 **/
//	qreal median();

	/**
	 * Calculate the arithmetic average based on the collected data.
	 **/
//	qreal average();

	/**
	 * Find the minimum value.
	 **/
//	qreal min();

	/**
	 * Find the maximum value.
	 **/
//	qreal max();

	/**
	 * Calculate a quartile.
	 **/
//	qreal quartile( int number ) { return quantile( 4, number ); }

	/**
	 * Calculate a percentile directly, without creating or using
	 * _percentileList.  Treewalker uses this for one-off queries.
	 **/
	qreal percentile( int number ) { return quantile( 100, number ); }

	/**
	 * Calculate a quantile: Find the quantile no. 'number' of order
	 * 'order'.
	 *
	 * The median is quantile( 2, 1 ), the minimum is quantile( 2, 0 ), the
	 * maximum is quantile( 2, 2 ). The first quartile is quantile( 4, 1 ),
	 * the first percentile is quantile( 100, 1 ).
	 **/
	qreal quantile( int order, int number );

	/**
	 * Calculates the percentiles list and sums for this set of data.  Not
	 * done automatically because not all users need this (ie. Treewalker).
	 * The lists it generates are already sorted.
	 **/
	void calculatePercentiles();

	/**
	 * Returns the percentiles list, indivdual, or cumulative sums.  These
	 * getters are const and assume that the lists are populated and sorted.
	 **/
	const QRealList & percentileList() const { return _percentiles;    }
	const QRealList & percentileSums() const { return _percentileSums; }
	const QRealList & cumulativeSums() const { return _cumulativeSums; }

        /**
         * Fill buckets for a histogram from 'startPercentile' to
         * 'endPercentile'.
	 *
	 * Each bucket contains the number of data points (not their value!)
	 * from one interval of bucketWidth width.
	 *
	 * The type of the buckets is qreal even though by mathematical
	 * definition it should be int, but QGraphicsView uses qreal
	 * everywhere, so this is intended to minimize the hassle converting
	 * back and forth. The bucket values can safely be converted to int
	 * with no loss of precision.
	 **/
        void fillBuckets( int bucketCount,
                          int startPercentile,
                          int endPercentile );

	/**
	 * Return the list of buckets.  Note that this is correct only for
	 * the most-recently-used start and end percentiles and bucket count.
	 *
	 * This is a relatively expensive operation so the results need
	 * to be cached somewhere, and here is as good as anywhere.
	 **/
	const QRealList & buckets() const { return _buckets; }


    protected:

	/**
	 * Sort the collected data in ascending order.
	 * This is necessary after all collect() calls.
	 *
	 * The functions accessing results like min(), max(), median(),
	 * quantile(), percentile() etc. all implicitly sort the data if they
	 * are not sorted yet.
	 **/
	void sort();


    private:

	QRealList _percentiles;
	QRealList _percentileSums;
	QRealList _cumulativeSums;
	QRealList _buckets;
	bool      _sorted         { false };
    };

}	// namespace QDirStat


#endif // ifndef PercentileStats_h

