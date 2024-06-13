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
#include "PkgManager.h"


#define VERBOSE_MATCH 0


using namespace QDirStat;


const DirTreeFilter * DirTreePatternFilter::create( const QString & pattern )
{
    if ( pattern.isEmpty() )
	return nullptr;

    if ( pattern.startsWith( QLatin1String( "*." ) ) )
    {
	// Remove the leading "*"
	const QString suffix = QString( pattern ).remove( 0, 1 );

	// Use a suffix filter if the suffix contains only "word" characters
	if ( QRegularExpression( "\\A\\.(?:\\w+)\\z" ).match( suffix ).hasMatch() )
	    return new DirTreeSuffixFilter( suffix );
    }

    // Create a more general pattern filter if the pattern wasn't suitable for suffix matching
    return new DirTreePatternFilter( pattern );
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




DirTreePkgFilter::DirTreePkgFilter( const PkgManager * pkgManager ):
    DirTreeFilter (),
    _fileListCache { pkgManager->createFileListCache( PkgFileListCache::LookupGlobal ) }
{
    //logInfo() << "Creating file list cache with " << pkgManager->name() << Qt::endl;
    //logInfo() << "Done." << Qt::endl;
}


bool DirTreePkgFilter::ignore( const QString & path ) const
{
    if ( !_fileListCache )
	return false;

    return _fileListCache->containsFile( path );
}
