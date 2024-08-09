/*
 *   File name: PercentileStats.cpp
 *   Summary:   Statistics classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <algorithm> // std::sort()
#include <cmath>     // cbrt(), floor()

#include "PercentileStats.h"
#include "Exception.h"
#include "FormatUtil.h"


#define VERBOSE_LOGGING 0


using namespace QDirStat;


void PercentileStats::sort()
{
#if VERBOSE_LOGGING
    logDebug() << "Sorting " << size() << " elements" << Qt::endl;
#endif

    std::sort( begin(), end() );

#if VERBOSE_LOGGING
    logDebug() << "Sorting done." << Qt::endl;
#endif
}


PercentileBoundary PercentileStats::quantile( int order, int number ) const
{
    if ( isEmpty() )
	return 0;

    if ( order < 2 || order > 100 )
	THROW( Exception{ QString{ "Invalid quantile order %1" }.arg( order ) } );

    if ( number > order )
	THROW( Exception{ QString{ "Invalid quantile #%1 for %2-quantile" }.arg( number ).arg( order ) } );

    if ( number == 0 )
	return first();

    if ( number == order )
	return last();

    // Calculate the data point position for the number and order
    const double          pos    = 1.0 * size() * number / order;
    const int             index  = std::floor( pos ); // floor because pos 1 is list index 0
    const PercentileValue result = at( index );

    // If the boundary is on an element, return its value
    if ( pos != index )
	return result;

    // The boundary is between two elements, return the average of their values
    return ( result + at( index - 1 ) ) / 2;

}


void PercentileStats::calculatePercentiles()
{
    _percentiles.clear(); // just in case anyone calls this more than once

    // Store all the percentile boundaries
    for ( int i=0; i <= 100; ++i )
	_percentiles.append( percentile( i ) );

    _percentileSums = PercentileList( 101 ); // new list, 101 items, all zero

    // Iterate all the data points - should be in order, so add to each percentile in turn
    int currentPercentile = 1;
    auto it = _percentiles.cbegin() + 1;
    for ( PercentileValue value : *this )
    {
	// Have we gone past this percentile upper boundary?
	while ( value > *it && currentPercentile < 100 )
	{
	    ++currentPercentile;
	    ++it;
	}

	_percentileSums[ currentPercentile ] += value;
    }

    _cumulativeSums.clear(); // just in case anyone calls this more than once

    // Cumulative totals calculated in a separate iteration for clarity
    PercentileValue runningTotal = 0;
    for ( PercentileValue percentileSum : asConst( _percentileSums ) )
    {
	runningTotal += percentileSum;
	_cumulativeSums.append( runningTotal );
    }

#if VERBOSE_LOGGING
    for ( int i=0; i < _percentileSums.size(); ++i )
    {
	logDebug() << "sum[ "     << i << " ] : " << formatSize( _percentileSums[ i ] ) << Qt::endl;
	logDebug() << "cum_sum[ " << i << " ] : " << formatSize( _cumulativeSums[ i ] ) << Qt::endl;
    }
#endif
}


PercentileValue PercentileStats::percentileBoundary( int index ) const
{
    CHECK_PERCENTILE_INDEX( index );

    return _percentiles.isEmpty() ? 0 : _percentiles[ index ];
}


PercentileValue PercentileStats::percentileSum( int index ) const
{
    CHECK_PERCENTILE_INDEX( index );

    return _percentileSums.isEmpty() ? 0 : _percentileSums[ index ];
}


PercentileValue PercentileStats::cumulativeSum( int index ) const
{
    CHECK_PERCENTILE_INDEX( index );

    return _cumulativeSums.isEmpty() ? 0 : _cumulativeSums[ index ];
}


void PercentileStats::fillBuckets( int bucketCount, int startPercentile, int endPercentile )
{
    CHECK_PERCENTILE_INDEX( startPercentile );
    CHECK_PERCENTILE_INDEX( endPercentile   );

    if ( startPercentile >= endPercentile )
        THROW( Exception{ "startPercentile must be less than endPercentile" } );

    if ( bucketCount < 1 )
        THROW( Exception{ QString{ "Invalid bucket count %1" }.arg( bucketCount ) } );

    // Create a new list for bucketCount, filled with zeroes
    _buckets = BucketList( bucketCount );

    if ( isEmpty() || _percentiles.isEmpty() )
        return;

    // Remember the validated start and end percentiles that match this bucket list
    _bucketsStart = _percentiles[ startPercentile ];
    _bucketsEnd   = _percentiles[ endPercentile   ];

    const PercentileBoundary bucketWidth = ( _bucketsEnd - _bucketsStart ) / bucketCount;

#if VERBOSE_LOGGING
    logDebug() << "startPercentile: " << startPercentile
               << " endPercentile: " << endPercentile
               << " startVal: " << formatSize( _bucketsStart )
               << " endVal: " << formatSize( _bucketsEnd )
               << " bucketWidth: " << formatSize( bucketWidth )
               << Qt::endl;
#endif

    // Skip to the first percentile that we're using
    auto it = cbegin();
    while ( it != cend() && *it < _bucketsStart )
	++it;

    // Fill buckets up to the last requested percentile
    int index = 0;
    PercentileBoundary nextBucket = _bucketsStart + bucketWidth;
    while ( it != cend() && *it <= _bucketsEnd )
    {
	// Increment the bucket index when we hit the next bucket boundary
	while ( *it > nextBucket )
	{
	    index = qMin( index + 1, bucketCount - 1 ); // avoid rounding errors tipping us past the last bucket
	    nextBucket += bucketWidth;
	}

        ++_buckets[ index ];

	++it;
    }
}


int PercentileStats::bestBucketCount( FileCount n, int max )
{
    if ( n < 2 )
	return 1;

    // Using the "Rice Rule" which gives more reasonable values for the numbers
    // we are likely to encounter in the context of filesystems
    const int result = qRound( 2 * std::cbrt( n ) );

    // Enforce an upper limit so each histogram bar remains wide enough
    // for tooltips and to be seen
    if ( result > max )
    {
#if VERBOSE_LOGGING
	logInfo() << "Limiting bucket count to " << max
		  << " instead of " << result
		  << Qt::endl;
#endif

	return max;
    }

    return result;
}


FileCount PercentileStats::bucket( int index ) const
{
    CHECK_INDEX( index, 0, bucketCount() - 1 );

    return _buckets[ index ];
}
