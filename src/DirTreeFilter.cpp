/*
 *   File name: DirTreeFilter.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QRegularExpression>

#include "DirTreeFilter.h"
#include "Logger.h"
#include "PkgFileListCache.h"
#include "PkgQuery.h"


#define VERBOSE_MATCH 0


using namespace QDirStat;


const DirTreeFilter * DirTreePatternFilter::create( const QString & pattern )
{
    if ( pattern.isEmpty() )
	return nullptr;

    if ( pattern.startsWith( "*."_L1 ) )
    {
	// Remove the leading "*"
	const QString suffix = QString{ pattern }.remove( 0, 1 );

	// Use a suffix filter if the suffix doesn't contain wildcard characters
	if ( Wildcard::isWildcard( suffix ) )
	    return new DirTreeSuffixFilter{ suffix };
    }

    // Create a more general pattern filter if the pattern wasn't suitable for simple suffix matching
    return new DirTreePatternFilter{ pattern };
}


bool DirTreePatternFilter::ignore( const QString & path ) const
{
    const bool match = _wildcard.exactMatch( path );

#if VERBOSE_MATCH
    if ( match )
	logDebug() << "Ignoring " << path << " by pattern filter " << _wildcard.pattern() << Qt::endl;
#endif

    return match;
}




bool DirTreeSuffixFilter::ignore( const QString & path ) const
{
    const bool match = path.endsWith( _suffix );

#if VERBOSE_MATCH
    if ( match )
	logDebug() << "Ignoring " << path << " by suffix filter *" << _suffix << Qt::endl;
#endif

    return match;
}




DirTreePkgFilter::DirTreePkgFilter():
    DirTreeFilter{},
    _fileListCache{ PkgQuery::fileList() }
{
    logInfo() << _fileListCache->size() << " files in filter" << Qt::endl;
}


bool DirTreePkgFilter::ignore( const QString & path ) const
{
    if ( !_fileListCache )
	return false;

    return _fileListCache->containsFile( path );
}
