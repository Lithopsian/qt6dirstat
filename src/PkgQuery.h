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

#include <QCache>
#include <QVector>

#include "PkgInfo.h" // PkgInfoList


namespace QDirStat
{
    class GlobalFileListCache;
    class PkgManager;

    typedef QVector<PkgManager *>    PkgManagerList;
    typedef QCache<QString, QString> PkgManagerCache;

    /**
     * Singleton class for simple queries to the system's package manager.
     * Only accessed by the public static functions.
     *
     * A list of all available package managers is initialised in the instance
     * constructor and a single package manager is determined as "primary".
     * The primary package manager is expected to be the one which provides
     * the majority of packages on the system, including its own program files.
     * The primary package manager is determined asynchronously using an
     * external process, and will support generating a file list cache.  It is
     * used to query package files, either using a cache of all packaged files
     * or by running multiple external process queries on individual packages,
     * although additional packages may be located using the other package
     * managers.  This class will block attempts to access a primary package
     * manager until the processes checking for one have completed.
     *
     * Finding the package that owns a file is done by looping through the
     * list of all package managers.  A cache of owning packages is maintained
     * because finding an owning package is an expensive process that requires
     * executing at least one external process command.
     **/
    class PkgQuery final
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
	 * Create the singleton instance.  The constructor will construct a
	 * list of available package managers and initiate external processes
	 * to identify a primary package manager.
	 **/
	static void init()
	    { instance(); }

	/**
	 * Return the elapsed query time that will trigger a warning message.
	 * This is currently only used by rpm.
	 **/
	static int pkgListWarningSecs()
	    { return instance()->_pkgListWarningSecs; }

	/**
	 * Return the process timeout for owning package commands.
	 **/
	static int owningPkgTimeoutSecs()
	    { return instance()->_owningPkgTimeoutSecs; }

	/**
	 * Return 'true' if any of the supported package managers was found.
	 **/
	static bool foundSupportedPkgManager()
	    { return !instance()->_pkgManagers.isEmpty(); }

	/**
	 * Return the primary package manager if there is one or 0 if not.
	 **/
	static const PkgManager * primaryPkgManager()
	    { return instance()->getPrimary(); }

	/**
	 * Set the primary package manager to 'pkgManager'.
	 **/
	static void setPrimaryPkgManager( const PkgManager * pkgManager )
	    { instance()->_primaryPkgManager = pkgManager; }

	/**
	 * Return the owning package of a file if it is in the internal cache,
	 * otherwise return 0.
	 **/
	static const QString * cachedOwningPkg( const QString & path )
	    { return instance()->_cache[ path ]; }

	/**
	 * Return the owning package of a file or directory with full path
	 * 'path' or an empty string if it is not owned by any package.
	 *
	 * Note that '_cache' is not searched.
	 **/
	static const QString * owningPkg( const QString & path )
	    { return instance()->getOwningPkg( path ); }

	/**
	 * Return the list of installed packages.
	 *
	 * Ownership of the list elements is transferred to the caller.
	 **/
	static PkgInfoList installedPkg()
	    { return instance()->getInstalledPkg(); }

	/**
	 * Return the list of all package files, from all package managers.
	 * Files are only found from package managers which support
	 * creating a file list cache.  The list is returned as a QSet, so
	 * the filenames are unique and can be located very quickly.
	 * Ownership is transferred to the caller.
	 **/
	static GlobalFileListCache * fileList()
	    { return instance()->getFileList(); }

	/**
	 * Delete all objects from the owning package cache.
	 **/
	static void clearOwningPkgCache()
	    { return instance()->_cache.clear(); }


    protected:

	/**
	 * Return the primary package manager if there is one or 0 if not.
	 **/
	const PkgManager * getPrimary() const;

	/**
	 * Return the owning package of a file or directory with full path
	 * 'path' or an empty string if it is not owned by any package.
	 *
	 * Note that '_cache' is not searched, but the result is inserted
	 * into the cache.
	 **/
	const QString * getOwningPkg( const QString & path );

	/**
	 * Return the list of installed packages.
	 *
	 * Ownership of the list elements is transferred to the caller.
	 **/
	PkgInfoList getInstalledPkg() const;

	/**
	 * Return a list of all package files.  This is found using a
	 * PkgFileListCache from each package manager that supports it, with
	 * the resulting file lists merged into a single QSet.
	 **/
	GlobalFileListCache * getFileList() const;


    private:

	int _pkgListWarningSecs;
	int _owningPkgTimeoutSecs;

	const PkgManager * _primaryPkgManager{ nullptr };
	PkgManagerList     _pkgManagers; // primary and secondary package managers found
	PkgManagerCache    _cache; // mapping of paths and package names

    };	// class PkgQuery

}	// namespace QDirStat

#endif	// PkgQuery_h
