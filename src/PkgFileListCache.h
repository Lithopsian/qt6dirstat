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
     * Cache class for a package file lists.
     *
     * This is useful when file lists for many packages need to be fetched;
     * some package managers (not all!) have a command to return all file
     * lists for all packages at once with one single command which is
     * typically much faster than invoking a separate external command for each
     * installed package.
     *
     * Use PkgManager::createFileListCache() to create and fill such a cache.
     **/
    class PkgFileListCache final
    {
    public:

	enum LookupType
	{
	    LookupByPkg  = 0x01,	// Will use only containsPkg()
	    LookupGlobal = 0x02,	// Will use only containsFile()
	    LookupAll    = 0xFF,	// Will use all
	};


	/**
	 * Constructor. 'lookupType' indicates what type of lookup to prepare
	 * for. This has significant impact on the memory footprint.
	 **/
	PkgFileListCache( const PkgManager * pkgManager,
	                  LookupType         lookupType ):
	    _pkgManager{ pkgManager },
	    _lookupType{ lookupType }
	{}

	/**
	 * Return the sorted file list for a package.
	 **/
	QStringList fileList( const QString & pkgName ) const;

	/**
	 * Return 'true' if the cache contains any information about a package,
	 * 'false' if not.
	 **/
	bool containsPkg( const QString & pkgName ) const;

	/**
	 * Return 'true' if the cache contains any information about a file,
	 * 'false' if not.
	 **/
	bool containsFile( const QString & fileName ) const;

	/**
	 * Return 'true' if the cache is empty, 'false' if not.
	 **/
	bool isEmpty() const
	    { return _pkgFileNames.isEmpty() && _fileNames.isEmpty(); }

	/**
	 * Remove the entries for a package from the cache.
	 **/
	void remove( const QString & pkgName );

	/**
	 * Clear the cache.
	 **/
//	void clear() { _pkgFileNames.clear(); _fileNames.clear(); }

	/**
	 * Add one file for one package.
	 **/
	void add( const QString & pkgName, const QString & fileName );

	/**
	 * Return the package manager parent of this cache.
	 **/
	const PkgManager * pkgManager() const { return _pkgManager; }

	/**
	 * Return the type of lookup this cache is set up for.
	 **/
	LookupType lookupType() const { return _lookupType; }


    private:

	const PkgManager           * _pkgManager;
	LookupType                   _lookupType;
	QMultiHash<QString, QString> _pkgFileNames;
	QSet<QString>                _fileNames;

    };	// class PkgFileListCache

}	// namespace QDirStat

#endif	// PkgFileListCache_h
