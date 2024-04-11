/*
 *   File name: PercentileStats.cpp
 *   Summary:	Statistics classes for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <math.h>	// ceil()
#include <algorithm>    // std::sort()

#include "PercentileStats.h"
#include "Exception.h"
#include "FormatUtil.h"
#include "Logger.h"


#define VERBOSE_SORT_THRESHOLD	50000


using namespace QDirStat;


void PercentileStats::sort()
{
    if ( size() > VERBOSE_SORT_THRESHOLD )
	logDebug() << "Sorting " << size() << " elements" << Qt::endl;

    std::sort( begin(), end() );
    _sorted = true;

    if ( size() > VERBOSE_SORT_THRESHOLD )
	logDebug() << "Sorting done." << Qt::endl;
}

/*
qreal PercentileStats::median()
{
    if ( isEmpty() )
	return 0;

    if ( !_sorted )
	sort();

    const int centerPos = size() / 2;

    // Since we are doing integer division, the center is already rounded down
    // if there is an odd number of data items, so don't use the usual -1 to
    // compensate for the index of the first data element being 0, not 1: if
    // size() is 5, we get _data[2] which is the center of
    // [0, 1, 2, 3, 4].
    const qreal result = at( centerPos );

    if ( size() % 2 == 0 ) // Even number of items
    {
	// Use the average of the two center items. We already accounted for
	// the upper one with the above integer division, so now we need to
	// account for the lower one: If size() is 6, we already got
	// _data[3], and now we need to average this with _data[2] of
	// [0, 1, 2, 3, 4, 5].
	return ( result + at( centerPos - 1 ) ) / 2.0;
    }

    return result;
}


qreal PercentileStats::average()
{
    if ( isEmpty() )
	return 0.0;

    const qreal sum = std::accumulate( cbegin(), cend(), 0.0 );
    const int count = size();

    return sum / count;
}


qreal PercentileStats::min()
{
    if ( isEmpty() )
	return 0.0;

    if ( !_sorted )
	sort();

    return first();
}


qreal PercentileStats::max()
{
    if ( isEmpty() )
	return 0.0;

    if ( !_sorted )
	sort();

    return last();
}
*/

qreal PercentileStats::quantile( int order, int number )
{
    if ( isEmpty() )
	return 0.0;

    if ( number > order )
    {
	QString msg = QString( "Cannot determine quantile #%1 for %2-quantile" ).arg( number ).arg( order );
	THROW( Exception( msg ) );
    }

    if ( order < 2 )
    {
	QString msg = QString( "Invalid quantile order %1" ).arg( order );
	THROW( Exception( msg ) );
    }

    if ( !_sorted )
	sort();

    if ( number == 0 )
	return first();

    if ( number == order )
	return last();

    // Same as in median(): The integer division already cut off any non-zero
    // decimal place, so don't subtract 1 to compensate for starting _data with
    // index 0.
    const int pos = ( size() * number ) / order;
    const qreal result = at( pos );

    // As in median: we hit between two elements, so use the average between them
    if ( ( size() * number ) % order == 0 )
	return ( result + at( pos - 1 ) ) / 2.0;

    return result;
}


void PercentileStats::calculatePercentiles()
{
    if ( !_sorted )
	sort();

    _percentiles.clear();
    _percentiles.reserve( 101 );

    for ( int i=0; i <= 100; ++i )
	_percentiles << percentile( i );

    _percentileSums.clear();
    _percentileSums = QRealList( 101 );

    const qreal percentileSize = size() / 100.0;

    for ( int i=0; i < size(); ++i )
    {
	const int percentile = qMax( 1.0, ceil( i / percentileSize ) );
	_percentileSums[ percentile ] += at(i);
    }

    _cumulativeSums.clear();
    _cumulativeSums.reserve( _percentileSums.size() );

    qreal runningTotal = 0.0;

    for ( int i=0; i < _percentileSums.size(); i++ )
    {
	runningTotal	 += _percentileSums.at(i);
	_cumulativeSums.append( runningTotal );
    }

#if 0
    for ( int i=0; i < _percentileSums.size(); ++i )
    {
	logDebug() << "sum[ "	  << i << " ] : " << formatSize( _percentileSums[i] ) << Qt::endl;
	logDebug() << "cum_sum[ " << i << " ] : " << formatSize( _cumulativeSums[i] ) << Qt::endl;
    }
#endif
}


void PercentileStats::fillBuckets( int bucketCount,
                                   int startPercentile,
                                   int endPercentile )
{
    CHECK_INDEX( startPercentile, 0, 100 );
    CHECK_INDEX( endPercentile,   0, 100 );

    if ( startPercentile >= endPercentile )
        THROW( Exception( "startPercentile must be less than endPercentile" ) );

    if ( bucketCount < 1 )
        THROW( Exception( QString( "Invalid bucket count %1" ).arg( bucketCount ) ) );

    _buckets = QRealList( bucketCount );

    if ( isEmpty() )
        return;

    qreal startVal = _percentiles[ startPercentile ];
    qreal endVal   = _percentiles[ endPercentile   ];
    qreal bucketWidth = ( endVal - startVal ) / bucketCount;

#if 0
    logDebug() << "startPercentile: " << startPercentile
               << " endPercentile: " << endPercentile
               << " startVal: " << formatSize( startVal )
               << " endVal: " << formatSize( endVal )
               << " bucketWidth: " << formatSize( bucketWidth )
               << Qt::endl;
#endif

    // Skip to the first percentile that we're using
    auto it = cbegin();
    while ( it != cend() && *it < startVal )
	++it;

    // Fill buckets up to the last requested percentile
    int index = 0;
    qreal nextBucket = startVal + bucketWidth;
    while ( it != cend() && *it <= endVal )
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
