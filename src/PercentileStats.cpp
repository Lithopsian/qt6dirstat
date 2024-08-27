/*
 *   File name: PercentileStats.cpp
 *   Summary:   Statistics classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <algorithm> // std::sort()

#include <QtMath> // qCeil

#include "PercentileStats.h"
#include "Exception.h"


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
    // Try to validate everything so the calculation will be valid
    if ( isEmpty() )
	return 0;

    if ( order < 2 || order > maxPercentile() )
	THROW( Exception{ QString{ "Invalid quantile order %1" }.arg( order ) } );

    if ( number > order )
	THROW( Exception{ QString{ "Invalid quantile #%1 for %2-quantile" }.arg( number ).arg( order ) } );

    // Calculate the data point rank for the number and order (C=1 algorithm, rank 1 is list index 0)
    const double indexRank = ( size() - 1.0 ) * number / order;

    // Separate the rank into its base integer to index the list and fraction part for interpolation
    const int    index  = std::floor( indexRank );
    const double modulo = indexRank - index;

    // Get the value at 'index' and the next to interpolate with if necessary
    PercentileBoundary result = at( index );
    if ( modulo )
	result += modulo * ( at( index + 1 ) - result );

    return result;

}


void PercentileStats::calculatePercentiles()
{
    _percentiles.clear(); // just in case anyone calls this more than once

    // Store all the percentile boundaries
    for ( int i = minPercentile(); i <= maxPercentile(); ++i )
	_percentiles.append( percentile( i ) );

    _percentileSums = PercentileList( maxPercentile() + 1 ); // new list, 101 items, all zero

    // Iterate all the data points - should be in order, so add to each percentile in turn
    int currentPercentile = 1;
    auto it = _percentiles.cbegin() + 1;
    for ( PercentileValue value : *this )
    {
	// Have we gone past this percentile upper boundary?
	while ( value > *it && currentPercentile < maxPercentile() )
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
	logDebug() << "sum["     << i << "] : " << QLocale{}.toString( _percentileSums[ i ] ) << Qt::endl;
	logDebug() << "cum_sum[" << i << "] : " << QLocale{}.toString( _cumulativeSums[ i ] ) << Qt::endl;
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
               << " startVal: " << QLocale{}.toString( static_cast<double>( _bucketsStart ) )
               << " endVal: " << QLocale{}.toString( static_cast<double>( _bucketsEnd ) )
               << " bucketWidth: " << QLocale{}.toString( static_cast<double>( bucketWidth ) )
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
    // Use the Rice Rule which gives reasonable values for the numbers
    // we are likely to encounter in the context of filesystems
    const int bucketCount = qCeil( 2 * std::cbrt( n ) );

    // Enforce an upper limit so each histogram bar remains wide enough
    // for tooltips and to be seen
    if ( bucketCount > max )
    {
#if VERBOSE_LOGGING
	logInfo() << "Limiting bucket count to " << max
	          << " instead of " << bucketCount
	          << Qt::endl;
#endif

	return max;
    }

    // Don't return zero, bad things will happen
    return bucketCount > 0 ? bucketCount : 1;
}


FileCount PercentileStats::bucket( int index ) const
{
    CHECK_INDEX( index, 0, bucketCount() - 1 );

    return _buckets[ index ];
}
