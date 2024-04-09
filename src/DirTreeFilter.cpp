/*
 *   File name: DirTreeFilter.cpp
 *   Summary:	Support classes for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */

#include <QRegularExpression>

#include "DirTreeFilter.h"
#include "Exception.h"
#include "Logger.h"
#include "PkgManager.h"


#define VERBOSE_MATCH 0


using namespace QDirStat;


const DirTreeFilter * DirTreePatternFilter::create( const QString & pattern )
{
    if ( pattern.isEmpty() )
	return nullptr;

    if ( pattern.startsWith( "*." ) )
    {
	// Remove the leading "*."
	const QString suffix = QString( pattern ).remove( 0, 2 );

	// Use a suffix filter if the suffix contains only "word" characters
	if ( QRegularExpression( "\\A(?:\\w+)\\z" ).match( suffix ).hasMatch() )
	{
	    const DirTreeFilter * filter = new DirTreeSuffixFilter( suffix );
	    CHECK_NEW( filter );
	    return filter;
	}
    }

    // Create a more general pattern filter if the pattern wasn't suitable for suffix matching
    const DirTreeFilter * filter = new DirTreePatternFilter( pattern );
    CHECK_NEW( filter );
    return filter;
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


DirTreePkgFilter::DirTreePkgFilter( const PkgManager * pkgManager )
{
    CHECK_PTR( pkgManager );

    //logInfo() << "Creating file list cache with " << pkgManager->name() << Qt::endl;
    _fileListCache = pkgManager->createFileListCache( PkgFileListCache::LookupGlobal );
    //logInfo() << "Done." << Qt::endl;
}




DirTreePkgFilter::~DirTreePkgFilter()
{
    delete _fileListCache;
}


bool DirTreePkgFilter::ignore( const QString & path ) const
{
    if ( !_fileListCache )
	return false;

    return _fileListCache->containsFile( path );
}
