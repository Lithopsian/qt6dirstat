/*
 *   File name: PkgQuery.h
 *   Summary:   Package manager query support for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef PkgQuery_h
#define PkgQuery_h

#include <QString>
#include <QCache>

#include "PkgInfo.h"


namespace QDirStat
{
    class PkgManager;

    /**
     * Singleton class for simple queries to the system's package manager.
     * Only normally accessed by the public static functions.
     **/
    class PkgQuery
    {
	/**
	 * Constructor. For internal use only; use the static methods instead.
	 **/
	PkgQuery();

	/**
	 * Destructor.
	 **/
	~PkgQuery();

	/**
	 * Suppress copy and assignment constructors (this is a singleton)
	 **/
	PkgQuery( const PkgQuery & ) = delete;
	PkgQuery & operator=( const PkgQuery & ) = delete;

	/**
	 * Return the singleton instance of this class.
	 **/
	static PkgQuery * instance();


    public:

	/**
	 * Return the owning package of a file or directory with full path
	 * 'path' or an empty string if it is not owned by any package.
	 **/
	static QString owningPkg( const QString & path )
	    { return instance()->getOwningPackage( path ); }

	/**
	 * Return 'true' if any of the supported package managers was found.
	 **/
	static bool foundSupportedPkgManager()
	    { return ! instance()->_pkgManagers.isEmpty(); }

	/**
	 * Return the (first) primary package manager if there is one or 0 if
	 * not.
	 **/
	static const PkgManager * primaryPkgManager()
	    { return instance()->_pkgManagers.isEmpty() ? nullptr : instance()->_pkgManagers.first(); }

	/**
	 * Return 'true' if any of the package managers has support for getting
	 * the list of installed packages.
	 **/
	static bool haveGetInstalledPkgSupport()
	    { return instance()->checkGetInstalledPkgSupport(); }

	/**
	 * Return the list of installed packages.
	 *
	 * Ownership of the list elements is transferred to the caller.
	 **/
	static PkgInfoList installedPkg()
	    { return instance()->getInstalledPkg(); }

	/**
	 * Return 'true' if any of the package managers has support for getting
	 * the the file list for a package.
	 **/
	static bool haveFileListSupport()
	    { return instance()->checkFileListSupport(); }

	/**
	 * Return the list of files and directories owned by a package.
	 **/
	static QStringList fileList( const PkgInfo * pkg )
	    { return instance()->getFileList( pkg ); }


    protected:

	/**
	 * Return the owning package of a file or directory with full path
	 * 'path' or an empty string if it is not owned by any package.
	 *
	 * The result is also inserted into the cache.
	 **/
	QString getOwningPackage( const QString & path );

	/**
	 * Return the list of installed packages.
	 *
	 * Ownership of the list elements is transferred to the caller.
	 **/
	PkgInfoList getInstalledPkg() const;

	/**
	 * Return the list of files and directories owned by a package.
	 **/
	QStringList getFileList( const PkgInfo * pkg ) const;

	/**
	 * Return 'true' if any of the package managers has support for getting
	 * the list of installed packages.
	 **/
	bool checkGetInstalledPkgSupport() const;

	/**
	 * Return 'true' if any of the package managers has support for getting
	 * the the file list for a package.
	 **/
	bool checkFileListSupport() const;

	/**
	 * Check which supported package managers are available and add them to
	 * the internal list.
	 **/
	void checkPkgManagers();

	/**
	 * Check if a package manager is available; add it to one of the
	 * internal lists if it is, or delete it if not.
	 **/
	void checkPkgManager( const PkgManager * pkgManager );


    private:

	// Data members

	QVector<const PkgManager *> _pkgManagers; // primary and secondary package managers found

	QCache<QString, QString>  _cache; // mapping of paths and package names

    }; // class PkgQuery

} // namespace QDirStat


#endif // PkgQuery_h
