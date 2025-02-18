/*
 *   File name: PercentileStats.h
 *   Summary:   Statistics classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef PercentileStats_h
#define PercentileStats_h

#include <algorithm> // std::max, std::max_element, std::sort
#include <cmath>     // cbrt(), ceil(), floor(), log2(), exp2()

#include <QVector>


namespace QDirStat
{
    typedef qint32                      PercentileCount; // can hold FileCount
    typedef qint64                      PercentileValue; // can hold FileSize or time_t
    typedef long double                 PercentileBoundary; // can hold a fractional PercentileValue
    typedef QVector<PercentileBoundary> Percentiles;
    typedef QVector<PercentileCount>    PercentileCountList;
    typedef QVector<PercentileValue>    PercentileValueList;
    typedef QVector<PercentileBoundary> Buckets;


    /**
     * Base class for percentile-related statistics calculation.
     *
     * This class is currently only used to store FileSize (64-bit integer)
     * and time_t (usually 64-bit) integers.  The collected data is held
     * as qint64 64-bit signed integers, capable of holding either FileSize
     * (aka qint64) or 64-bit time_t values.
     *
     * Five lists of values can be calculated and stored for reference:
     * - _percentiles with the boundaries between each percentile;
     * - _percentileCounts with the cumulative sum of all entries up to percentile;
     * - _percentileSums with the cumulative sum of size values up to a percentile;
     * - _buckets with start values for each bucket;
     * - _bucketCounts with counts of data entries in a bucket.
     *
     * The first three lists are generated by calling calculatePercentiles().
     * Values are treated as being in a particular percentile if they are no
     * larger than the cutoff for that percentile and greater than the cutoff
     * for the previous percentile.
     *
     * Percentile boundaries are stored as long double floating point numbers
     * because they may lie between individual 64-bit integer data points.
     * External users of the percentile boundaries (FileSizeStatsWindow,
     * HistogramView, and TreeWalker) only need integers corresponding to
     * their data values, but floating point values are needed for the bucket
     * filling algorithm (although arguably long double precision is overkill
     * for this).
     *
     * The buckets lists are generated by calling fillBuckets() with the
     * desired number of buckets and a start percentile and an end percentile.
     * A sensible number of buckets for a particular dataset can be found by
     * calling bestBucketCount().
     *
     * Derived classes have to populate the percentile and bucket lists
     * explicitly as they are not needed in all cases and so are not
     * automatically filled.
     **/
    class PercentileStats : public PercentileValueList
    {
    public:

	/**
	 * Return the minimum and maximum valid percentile values, as well
	 * as the percentiles for corresponding to the median and quartiles.
	 * These values are trivial but defining them here helps to avoid
	 * indexing errors and avoids scattering hard-coded values throughout
	 * the code.
	 *
	 * Note that these are unsigned to help negative values being used,
	 * and short so they can be assigned to a standard signed int or
	 * float without narrowing.
	 **/
	constexpr static unsigned short minPercentile() { return 0;                   }
	constexpr static unsigned short maxPercentile() { return 100;                 }
	constexpr static unsigned short median()        { return maxPercentile() / 2; }
	constexpr static unsigned short quartile1()     { return maxPercentile() / 4; }
	constexpr static unsigned short quartile3()     { return quartile1()     * 3; }

	/**
	 * Return the values for the minimum (ie. 0) and maximum (ie. 100)
	 * percentiles, and for the median and quartiles.
	 *
	 * Note that these functions return pre-calculated percentile boundary
	 * values from the _percentiles list (rounded down).  They require the
	 * percentiles list to be fully populated and will throw an exception
	 * if they are not.
	 **/
	PercentileValue minValue()    const { return percentileValue( minPercentile() ); }
	PercentileValue maxValue()    const { return percentileValue( maxPercentile() ); }
	PercentileValue medianValue() const { return percentileValue( median()        ); }
	PercentileValue q1Value()     const { return percentileValue( quartile1()     ); }
	PercentileValue q3Value()     const { return percentileValue( quartile3()     ); }

	/**
	 * Calculate a percentile directly, without creating or using
	 * _percentiles.  Treewalker uses this for one-off queries and it
	 * is used for populating the _percentiles list.
	 **/
	PercentileBoundary percentile( int number ) const
	    { return quantile( maxPercentile(), number ); }

	/**
	 * Calculates the percentiles list and sums for this set of data.  Not
	 * done automatically because not all users need this (ie. Treewalker).
	 * The lists it generates are already sorted.
	 *
	 * This is normally only called once per set of collected stats, but is
	 * written to operate safely if called again.
	 **/
	void calculatePercentiles();

	/**
	 * Returns a particular percentile boundary as an integer
	 * value.  For most users, this is more convenient and
	 * matches their integer data values.  The same definition
	 * applies: a data point is a member of a given percentile
	 * if it is no larger than the boundary for that percentile
	 * and greater than the boundary for the previous percentile.
	 * To keep that definition, the floating point boundary
	 * is rounded down to an integer.
	 *
	 * Note that this function needs the percentiles lists to be
	 * fully populated and will always return 0 if it is not. It
	 * will throw if 'index' is not in the range of 0 to 100.
	 **/
	PercentileValue percentileValue( int index ) const
	{
	    validatePercentileIndex( index );
	    return std::floor( _percentiles[ index ] );
	}

	/**
	 * Returns a particular percentile sum, for the given 'index':
	 * - percentileCount() returns the number of stats entries in
	 * that percentile;
	 * - cumulativeCount() returns the number of stats entries in
	 * percentiles from o up to 'index';
	 * - percentileSum() returns a sum of the values of all entries
	 * in that percentile;
	 * - cumulativeSum() returns a sum of the values of all entries
	 * in percentiles from 0 up to 'index';
	 * - percentileSum() returns a sum of the values of all
	 * entries in percentiles from 'startIndex' to 'endIndex'
	 * inclusive.
	 *
	 * The overloads with two indexes return the sum between those two
	 * percentiles.
	 *
	 * Note that these functions need the percentiles lists to be
	 * fully populated and will always return 0 if they are not.  If
	 * any index is out of the range 0 to 100, the functions will throw.
	 **/
	PercentileCount percentileCount( int index ) const
	{
	    validatePercentileIndex( index );
	    return index == 0 || _percentileCounts.isEmpty() ? 0 : percentileCountDiff( index, index-1 );
	}
	PercentileCount cumulativeCount( int index ) const
	{
	    validatePercentileIndex( index );
	    return _percentileCounts.isEmpty() ? 0 : _percentileCounts[ index ];
	}
	PercentileValue percentileSum( int index ) const
	{
	    validatePercentileIndex( index );
	    return index == 0 || _percentileSums.isEmpty() ? 0 : percentileSumDiff( index, index-1 );
	}
	PercentileValue cumulativeSum( int index ) const
	{
	    validatePercentileIndex( index );
	    return _percentileSums.isEmpty() ? 0 : _percentileSums[ index ];
	}
	PercentileCount percentileCount( int startIndex, int endIndex ) const
	{
	    validateIndexRange( startIndex, endIndex );
	    return _percentileCounts.isEmpty() ? 0 : percentileCountDiff( endIndex, startIndex );
	}
	PercentileValue percentileSum( int startIndex, int endIndex ) const
	{
	    validateIndexRange( startIndex, endIndex );
	    return _percentileSums.isEmpty() ? 0 : percentileSumDiff( endIndex, startIndex );
	}

	/**
	 * Validate 'startIndex' and 'endIndex'; they must both be between
	 * 0 and 100, and the end must be higher then the start.  This
	 * function will throw if it encounters an invalid value.
	 **/
	static void validateIndexRange( int startIndex, int endIndex );

	/**
	 * Fill 'bucketCount' buckets with values from 'startPercentile'
	 * to 'endPercentile'. Each bucket contains the number of data
	 * points (not their value!).
	 *
	 * The size of each bucket is determined to fit the range of
	 * values requested and 'bucketCount'.  If 'logWidths' is true
	 * then the width of each bucket increases by the same factor
	 * from the previous bucket.  Otherwise the buckets all have
	 * the same width.  Log bucket widths can be useful with inverse
	 * exponential datasets such as common file size distributions;
	 * they give finer resolution of the small file sizes instead of
	 * lumping everything from 0 to several MB in the first bucket.
	 **/
	void fillBuckets( bool logWidths, int bucketCount, int startPercentile, int endPercentile );

	/**
	 * Calculate the best bucket count according to the Rice Rule for
	 * 'n' data points.  The number of buckets is limited to 'max' for
	 * on-screen display.
	 *
	 * See also https://en.wikipedia.org/wiki/Histogram
	 **/
	static int bestBucketCount( PercentileCount n, double max )
	    { return std::min( std::ceil( 2 * std::cbrt( n ) ), max ); }

	/**
	 * Return the number of buckets for the current list of bucket counts.
	 *
	 * Note that converting size() to int will be narrowing in Qt6, but
	 * the actual list size is restricted to an int, and in practice much
	 * less than that.
	 **/
	int bucketsCount() const { return _bucketCounts.size(); }

	/**
	 * Return the exact span of bytes that are included in bucket
	 * 'index'.
	 **/
	PercentileBoundary bucketWidth( int index ) const
	{
	    validateBucketIndex( index );
	    return _buckets[ index+1 ] - _buckets[ index ];
	}

	/**
	 * Return the start value of bucket 'index'.  This is rounded
	 * up from the (possibly) fractional bucket start value in
	 * the _buckets list so that it indicates the smallest integer
	 * value that would be collected in that bucket, although not
	 * necessarily the actual smallest value in the bucket.
	 *
	 * rawBucketStart() returns an unrounded value.
	 **/
	PercentileValue bucketStart( int index ) const
	{
	    validateBucketIndex( index );
	    return std::ceil( _buckets[ index ] );
	}
	PercentileBoundary rawBucketStart( int index ) const
	{
	    validateBucketIndex( index );
	    return _buckets[ index ];
	}

	/**
	 * Return the end value of bucket 'index'.  This is rounded
	 * up from the (possibly) fractional start value for the next
	 * bucket and then decremented by one so that it indicates the
	 * largest integer value that would be included in the bucket,
	 * although not necessarily the actual largest value in the
	 * bucket.
	 *
	 * The bucket end calculated in this way may be 1 byte smaller
	 * than the bucket start, when the bucket does not span an
	 * integer value and so there are no data points in the bucket.
	 * In the special case of a single bucket, with the same start
	 * and end values, force the rounded end value to be the same
	 * as the start value.
	 *
	 * rawBucketEnd() returns the start of the next bucket,
	 * unrounded.
	 **/
	PercentileValue bucketEnd( int index ) const
	{
	    validateBucketIndex( index );
	    return std::ceil( _bucketCounts.size() == 1 ? _buckets[ index ] : _buckets[ index + 1 ] - 1 );
	}
	PercentileBoundary rawBucketEnd( int index ) const
	{
	    validateBucketIndex( index );
	    return _buckets[ index + 1 ];
	}

	/**
	 * Return the number of data points in bucket 'index'.
	 **/
	PercentileCount bucketCount( int index ) const
	{
	    validateBucketIndex( index );
	    return _bucketCounts[ index ];
	}

	/**
	 * Return the highest number of data points currently in a
	 * single bucket.
	 **/
	PercentileCount lowestBucketCount() const
	    { return *std::min_element( _bucketCounts.begin(), _bucketCounts.end() ); }
	PercentileCount highestBucketCount() const
	    { return *std::max_element( _bucketCounts.begin(), _bucketCounts.end() ); }

	/**
	 * Return a value representing the difference between the largest
	 * and some of the smaller buckets.  This is the ratio between
	 * the largest bucket count and the 85th-percentile (15th smallest)
	 * bucket count.  If the reference count happens to be 0, 1 is used
	 * instead.
	 **/
	double skewness() const;

	/**
	 * If 'value' is 0, then return 0.  If 'value' is 1, then return 0.5.
	 * Otherwise return the base-2 logarithm of 'value'.   This avoids
	 * problems with log2(0) or with huge negative logarithms for very
	 * small values.
	 **/
	static PercentileBoundary log2( PercentileBoundary value )
	    { return value > 2 ? std::log2( value ) : value / 2; }


    protected:

	/**
	 * Sort the collected data in ascending order.  This class does not
	 * know if all the data that has been added to the list has been sorted,
	 * so it is vital that sort() is called after all collect() calls are
	 * complete, before the data is analysed.
	 **/
	void sort();

	/**
	 * Calculate a quantile: find the quantile 'number' of order 'order'.
	 *
	 * The median is quantile( 2, 1 ), the minimum is quantile( 2, 0 ), the
	 * maximum is quantile( 2, 2 ). The first quartile is quantile( 4, 1 ),
	 * the first percentile is quantile( 100, 1 ), and the last percentile
	 * is quantile( 100, 100 ).
	 *
	 * The method for this calculation is to find the rank of the
	 * requested quantile and interpolate between the values at the list
	 * positions either side of that rank (eg. rank 75.5 would interpolate
	 * between the values at list positions 75 and 76).
	 *
	 * Rank is defined here as 'number' / 'order' * (size() - 1) (rank is
	 * formally defined as one larger than this but this list starts at index
	 * 0).  This corresponds to the "C = 1" or R7 interpolation and is
	 * convenient because it is valid and safe for the entire range of
	 * percentiles and data points.  For example, the "C = 0" method, which
	 * may be preferred on strictly statistical grounds, has the formula
	 * 'number' / 'order' * (size() + 1), but this formula can only be used
	 * for percentiles in the range 1/(size() + 1) to size()/(size() + 1) and
	 * the rank must be explicitly clamped to 1 or size() outside those limits.  The
	 * differences between most of these interpolation methods are small and
	 * for large data sets they are negligible.
	 **/
	PercentileBoundary quantile( int order, int number ) const;

	/**
	 * Return the difference between two percentile counts or sums.
	 * This can be used to obtain the count or sum for a range of
	 * percentiles.
	 *
	 * These functions make no checks on the validity of the
	 * indexes or lists.
	 **/
	PercentileCount percentileCountDiff( int endIndex, int startIndex ) const
	    { return _percentileCounts[ endIndex ] - _percentileCounts[ startIndex ]; }
	PercentileValue percentileSumDiff( int endIndex, int startIndex ) const
	    { return _percentileSums[ endIndex ] - _percentileSums[ startIndex ]; }

	/**
	 * Validate that 'index' is within the allowed range for
	 * a percentiles index.  This will throw an exception
	 * if 'index' is less than 0 or more than 100.
	 **/
	static void validatePercentileIndex( int index );

	/**
	 * Validate that 'index' is within the allowed range for
	 * the current buckets list.  This will throw an exception
	 * if 'index' is less than 0 or more than the size of the
	 * list.
	 **/
	void validateBucketIndex( int index ) const;


    private:

	Percentiles         _percentiles;
	PercentileCountList _percentileCounts;
	PercentileValueList _percentileSums;

	Buckets             _buckets;
	PercentileCountList _bucketCounts;

    };	// class PercentileStats

}	// namespace QDirStat

#endif	// ifndef PercentileStats_h
