/*
 *   File name: PkgFileListCache.cpp
 *   Summary:   Package manager support for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "PkgFileListCache.h"
#include "Exception.h"


#define CHECK_LOOKUP_TYPE(wanted)					  \
do {									  \
    if ( ( _lookupType & (wanted) ) != (wanted) )			  \
	THROW( Exception{ "Cache not set up for this type of lookup" } ); \
} while ( false )


using namespace QDirStat;



QStringList PkgFileListCache::fileList( const QString & pkgName ) const
{
    CHECK_LOOKUP_TYPE( LookupByPkg );

    QStringList fileList = _pkgFileNames.values( pkgName );
    fileList.sort();

    return fileList;
}


bool PkgFileListCache::containsPkg( const QString & pkgName ) const
{
    CHECK_LOOKUP_TYPE( LookupByPkg );

    return _pkgFileNames.contains( pkgName );
}


bool PkgFileListCache::containsFile( const QString & fileName ) const
{
    CHECK_LOOKUP_TYPE( LookupGlobal );

    return _fileNames.contains( fileName );
}


void PkgFileListCache::remove( const QString & pkgName )
{
    CHECK_LOOKUP_TYPE( LookupByPkg );

    _pkgFileNames.remove( pkgName );
}


void PkgFileListCache::add( const QString & pkgName, const QString & fileName )
{
    if ( _lookupType & LookupByPkg )
	_pkgFileNames.insert( pkgName, fileName );

    if ( _lookupType & LookupGlobal )
	_fileNames.insert( fileName );
}
