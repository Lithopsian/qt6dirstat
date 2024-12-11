/*
 *   File name: PkgFileListCache.h
 *   Summary:   Package manager support for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef PkgFileListCache_h
#define PkgFileListCache_h

#include <QMultiHash>
#include <QSet>


namespace QDirStat
{
    class PkgManager;

    /**
     * Cache class for package file lists.
     *
     * This is useful when file lists for many packages need to be fetched;
     * some package managers (not all!) have a command to return all file
     * lists for all packages at once with one single command which is
     * typically much faster than invoking a separate external command for each
     * installed package.
     *
     * Use PkgManager::createFileListCache() to create and fill such a cache.
     **/
    class PkgFileListCache final : public QMultiHash<QString, QString>
    {
    public:

	/**
	 * Constructor.
	 **/
	PkgFileListCache( const PkgManager * pkgManager ):
	    _pkgManager{ pkgManager }
	{}

	/**
	 * Return the package manager parent of this cache.
	 **/
	const PkgManager * pkgManager() const { return _pkgManager; }

	/**
	 * Add one file for one package.
	 **/
	void add( const QString & pkgName, const QString & fileName )
	    { insert( pkgName, fileName ); }

	/**
	 * Return 'true' if the cache contains any information about a package,
	 * 'false' if not.
	 **/
	bool containsPkg( const QString & pkgName ) const { return contains( pkgName ); }

	/**
	 * Return the sorted file list for a package.  Sorting is not
	 * essential, but it does make package tree generation in PkgReader
	 * faster.
	 **/
	QStringList fileList( const QString & pkgName ) const { return values( pkgName ); }


    private:

	const PkgManager * _pkgManager;

    };	// class PkgFileListCache



    /**
     * Class to contain a global (unique) list of filenames from all packages.
     *
     * This is a fairly trivial sub-class of QSet<QString> with convenience
     * functions for adding filenames from a PkgFileListCache object and checking
     * whether a given filename path is in the set.
     *
     * This is constructed by PkgQuery and used by DirTreePkgFilter for
     * unpackaged queries.
     **/
    class GlobalFileListCache final : public QSet<QString>
    {
    public:
	void add( const PkgFileListCache & pkgFileListCache )
	    { for ( const QString & filename : pkgFileListCache ) insert( filename ); }

	bool containsFile( const QString & path ) const { return contains( path ); }
    };

}	// namespace QDirStat

#endif	// PkgFileListCache_h
