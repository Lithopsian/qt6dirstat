/*
 *   File name: DirTreePatternFilter.cpp
 *   Summary:	Support classes for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */

#include <QRegularExpression>

#include "DirTreePatternFilter.h"
#include "Exception.h"
#include "Logger.h"

#define VERBOSE_MATCH 0


using namespace QDirStat;


DirTreeFilter * DirTreePatternFilter::create( const QString & pattern )
{
    if ( pattern.isEmpty() )
	return nullptr;

    DirTreeFilter * filter = 0;

    if ( pattern.startsWith( "*." ) )
    {
	QString suffix = pattern;
	suffix.remove( 0, 2 ); // Remove the leading "*."

	if ( QRegularExpression( "\\A(?:\\w+)\\z" ).match( suffix ).hasMatch() )
	    filter = new DirTreeSuffixFilter( suffix );
    }

    if ( ! filter )
	filter = new DirTreePatternFilter( pattern );

    CHECK_NEW( filter );
    return filter;
}


bool DirTreePatternFilter::ignore( const QString & path ) const
{
    bool match = _wildcard.exactMatch( path );

#if VERBOSE_MATCH
    if ( match )
	logDebug() << "Ignoring " << path << " by pattern filter " << _wildcard.pattern() << Qt::endl;
#endif

    return match;
}





bool DirTreeSuffixFilter::ignore( const QString & path ) const
{
    bool match = path.endsWith( _suffix );

#if VERBOSE_MATCH
    if ( match )
	logDebug() << "Ignoring " << path << " by suffix filter *" << _suffix << Qt::endl;
#endif

    return match;
}
