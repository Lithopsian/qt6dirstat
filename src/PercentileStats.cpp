/*
 *   File name: PercentileStats.cpp
 *   Summary:   Statistics classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QtMath> // qCeil

#include "PercentileStats.h"
#include "Exception.h"
#include "FormatUtil.h" // only needed with VERBOSE_LOGGING


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

    CHECK_INDEX( order, 2, maxPercentile(), "Quantile order out of range" );

    if ( number > order )
	THROW( Exception{ QString{ "Invalid quantile #%1 for %2-quantile" }.arg( number ).arg( order ) } );

    // Calculate the data point rank for the number and order (C=1 algorithm, rank 1 is list index 0)
    const double indexRank = ( size() - 1.0 ) * number / order;

    // Separate the rank into its base integer to index the list and fraction part for interpolation
    const int    index  = std::floor( indexRank );
    const double modulo = indexRank - index;

    // Get the value at 'index' and interpolate with the next if necessary
    PercentileBoundary result = at( index );
    if ( modulo )
	result += modulo * ( at( index + 1 ) - result );

    return result;

}


void PercentileStats::calculatePercentiles()
{
    _percentiles.clear(); // just in case anyone calls this more than once

    // Calculate and store all the percentile boundaries
    for ( int i = minPercentile(); i <= maxPercentile(); ++i )
	_percentiles.append( percentile( i ) );

    // Initialise the first list entries to 0
    _percentileCounts = PercentileCountList( 1 );
    _percentileSums   = PercentileValueList( 1 );

    PercentileCount count = 0;
    PercentileValue sum   = 0;

    // Iterate the percentiles as we go along, starting at percentile 1
    auto it = _percentiles.cbegin() + 1;
    auto filledPercentile = [ this, &it, &count, &sum ]()
    {
	// Put the running totals into the lists
	_percentileCounts.append( count );
	_percentileSums.append( sum );

	// Move on to the next percentile
	++it;
    };

    // Iterate all the data points - should be in order, so add to each percentile in turn
    for ( PercentileValue value : *this )
    {
	// Have we gone past this percentile upper boundary?
	while ( value > *it )
	    filledPercentile();

	++count;
	sum += value;
    }

    // Fill trailing entries, percentiles after the last stats entry, including when no stats at all
    while ( it != _percentiles.cend() )
	filledPercentile();

#if VERBOSE_LOGGING
    for ( int i=0; i < _percentileSums.size(); ++i )
    {
	logDebug() << " boundary[" << i << "] : " << formatCount( _percentiles[ i ] ) << Qt::endl;
	logDebug() << "    count[" << i << "] : " << formatCount( percentileCount( i ) ) << Qt::endl;
	logDebug() << "  cum sum[" << i << "] : " << formatCount( percentileSum( i ) ) << Qt::endl;
	logDebug() << "cum count[" << i << "] : " << formatCount( percentileSum( i ) ) << Qt::endl;
	logDebug() << "  cum sum[" << i << "] : " << formatCount( percentileSum( i ) ) << Qt::endl;
    }
#endif
}


void PercentileStats::validateIndex( int index )
{
    CHECK_INDEX( index, minPercentile(), maxPercentile(), "Percentile index out of range" );
}


void PercentileStats::validateIndexRange( int startIndex, int endIndex )
{
    // Validate as much as possible, although the percentiles list still might not match the stats
    validateIndex( startIndex );
    validateIndex( endIndex   );

    if ( startIndex >= endIndex )
	THROW( Exception{ "startPercentile must be less than endPercentile" } );
}


void PercentileStats::fillBuckets( bool logWidths, int bucketCount, int startPercentile, int endPercentile )
{
    validateIndexRange( startPercentile, endPercentile );
    if ( bucketCount < 1 || bucketCount > _percentiles.size() )
	THROW( Exception{ QString{ "Invalid bucket count %1" }.arg( bucketCount ) } );

    // Create an empty list of boundaries and a list of bucketCount zeroes, discarding the old lists
    _buckets      = Buckets{};
    _bucketCounts = PercentileCountList( bucketCount );

    // Find the first and last values to count in the buckets
    const PercentileBoundary bucketsStart = _percentiles[ startPercentile ];
    const PercentileBoundary bucketsEnd   = _percentiles[ endPercentile   ];

    // Calculate the bucket width either as a linear increment, or a factor for log widths
    const PercentileBoundary bucketWidth = [ logWidths, bucketsStart, bucketsEnd, bucketCount ]()
    {
	// Return a linear increment for non-log widths
	if ( !logWidths )
	    return ( bucketsEnd - bucketsStart ) / bucketCount;

	// Avoid taking the log of 0 and smoothly transition to logs for higher values
	const PercentileBoundary logStart = bucketsStart > 2 ? std::log2( bucketsStart ) : bucketsStart / 2;
	const PercentileBoundary logEnd   = bucketsEnd   > 2 ? std::log2( bucketsEnd   ) : bucketsEnd   / 2;
	const PercentileBoundary logDiff = ( logEnd - logStart ) / bucketCount;
#if VERBOSE_LOGGING
        logDebug() << " logEnd: " << formatCount( logEnd )
                   << " logStart: " << formatCount( logStart )
                   << " logDiff: " << formatCount( logDiff )
                   << Qt::endl;
#endif
	return std::exp2( logDiff );
    }();

#if VERBOSE_LOGGING
    logDebug() << "startPercentile: " << startPercentile
               << " endPercentile: " << endPercentile
               << " startVal: " << formatCount( bucketsStart )
               << " endVal: " << formatCount( bucketsEnd )
               << " bucketWidth: " << formatCount( bucketWidth )
               << Qt::endl;
#endif

    // Special case: don't skip files with size equal to P0 for the first percentile/bucket
    auto beginIt = cbegin();
    if ( startPercentile > minPercentile() )
    {
	// Find the first (ie. smallest) data point that we want for the first bucket
	while ( beginIt != cend() && *beginIt <= bucketsStart )
	    ++beginIt;
    }

    // Set the first bucket boundary to use in the loop
    PercentileBoundary thisBucketStart = bucketsStart;

    // Find the start of the next (ie. second) bucket to use in the loop
    PercentileBoundary nextBucketStart = thisBucketStart;
    const auto getNextBucketStart = [ this, logWidths, bucketWidth, &nextBucketStart ]()
    {
	// Add the current bucket start value to the list and find the start of the next
	_buckets.append( nextBucketStart );
	return logWidths ? nextBucketStart * bucketWidth : nextBucketStart + bucketWidth;
    };
    nextBucketStart = getNextBucketStart();

    // A log scaling factor doesn't work on zero, unless we actually have a zero bucket increment
    if ( logWidths && nextBucketStart == 0 && bucketWidth > 1 )
	nextBucketStart = 1;

    // Fill buckets from the start we just found, up to the last requested value
    auto bucketIt = _bucketCounts.begin();
    for ( auto it = beginIt; it != cend() && *it <= bucketsEnd; ++it )
    {
	// Loop through buckets until we reach the one for this data point, skipping empty buckets
	while ( *it >= nextBucketStart && *it > thisBucketStart )
	{
	    // Handle rounding errors which could push us past the last bucket,
	    // and handle the special case of the last bucket end value
	    if ( bucketIt + 1 != _bucketCounts.end() )
	    {
		// For most buckets, just append to the boundaries and get the start of the next bucket
		++bucketIt;
		thisBucketStart = nextBucketStart;
		nextBucketStart = getNextBucketStart();
	    }
	    else
	    {
		// The bucket start calculated after the last bucket is actually the last file size needed,
		// so nudge it up by one, leaving the end of the last bucket equal to the last data point.
		++nextBucketStart;
	    }
	}

	// Add to this bucket
	( *bucketIt )++;
    }

    // Add any empty trailing buckets to the bucket boundaries list
    while ( _buckets.size() <= _bucketCounts.size() )
	nextBucketStart = getNextBucketStart();
}


int PercentileStats::bestBucketCount( PercentileCount n, int max )
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


void PercentileStats::validateBucketIndex( int index ) const
{
    CHECK_INDEX( index, 0, bucketCount() - 1, "Bucket index out of range" );
}
