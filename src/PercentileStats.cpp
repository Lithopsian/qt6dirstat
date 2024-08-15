/*
 *   File name: PercentileStats.cpp
 *   Summary:   Statistics classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <algorithm> // std::sort()

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

    // Calculate the data point rank for the number and order
    const double rank   = ( size() - 1.0 ) * number / order;
    const int    index  = std::floor( rank );
    const double modulo = rank - index;

    // Get the value at 'rank' and the next to interpolate with
    const PercentileBoundary result   = at( index ); // rank 1 is list index 0
    const PercentileBoundary fraction = modulo ? at( index + 1 ) - result : 0; // qint64 stored as long double

    // This interplation will collapse to simply result if the rank is an exact integer
    return result + fraction;

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


PercentileBoundary PercentileStats::percentileBoundary( int index ) const
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
    // Validate as much as possible, although the percentiles list still might not match the stats
    CHECK_PERCENTILE_INDEX( startPercentile );
    CHECK_PERCENTILE_INDEX( endPercentile   );

    if ( startPercentile >= endPercentile )
	THROW( Exception{ "startPercentile must be less than endPercentile" } );

    if ( bucketCount < 1 )
	THROW( Exception{ QString{ "Invalid bucket count %1" }.arg( bucketCount ) } );

    // Create a new list of bucketCount zeroes, discarding any existing list
    _buckets = BucketList( bucketCount );

    // Remember the validated start and end percentiles that match this bucket list
    _bucketsStart = percentileBoundary( startPercentile );
    _bucketsEnd   = percentileBoundary( endPercentile   );

    const PercentileBoundary bucketWidth = ( _bucketsEnd - _bucketsStart ) / bucketCount;

#if VERBOSE_LOGGING
    logDebug() << "startPercentile: " << startPercentile
               << " endPercentile: " << endPercentile
               << " startVal: " << formatSize( _bucketsStart )
               << " endVal: " << formatSize( _bucketsEnd )
               << " bucketWidth: " << formatSize( bucketWidth )
               << Qt::endl;
#endif

    // Find the first data point that we want for the buckets
    auto beginIt = cbegin();
    while ( beginIt != cend() && *beginIt < _bucketsStart )
	++beginIt;

    // Fill buckets up to the last requested percentile
    int index = 0;
    PercentileBoundary bucketEnd = _bucketsStart + bucketWidth;
    for ( auto it = beginIt; it != cend() && *it <= _bucketsEnd; ++it )
    {
	// Increment the bucket index when we hit the next bucket boundary, skipping empty buckets
	while ( *it > bucketEnd )
	{
	    if ( index < bucketCount - 1 ) // avoid rounding errors tipping us past the last bucket
		++index;
	    bucketEnd += bucketWidth;
	}

	// Fill this bucket
	++_buckets[ index ];
    }
}


int PercentileStats::bestBucketCount( FileCount n, int max )
{
    if ( n < 2 )
	return 1;

    // Using the Rice Rule which gives reasonable values for the numbers
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
