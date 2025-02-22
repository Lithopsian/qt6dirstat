/*
 *   File name: PkgManager.h
 *   Summary:   Package manager support for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef PkgManager_h
#define PkgManager_h

#include <QProcess>

#include "PkgInfo.h" // PkgInfoList


namespace QDirStat
{
    class PkgFileListCache;


    /**
     * Simple pair structure for passing a program name and list of arguments,
     * typically to be used for running an extermal process.
     **/
    struct PkgCommand
    {
	const char * program;
	QStringList args;
	bool isEmpty() const { return !program; }
    };


    /**
     * Abstract base class for all package managers.
     *
     * Concrete implementation classes:
     *	DpkgPkgManager
     *	RpmPkgManager
     *	PacManPkgManager
     **/
    class PkgManager
    {
    public:

	/**
	 * Constructor.
	 **/
	PkgManager() = default;

	/**
	 * Destructor.
	 **/
	virtual ~PkgManager() = default;

	/**
	 * Return the name of this package manager.
	 **/
	virtual QLatin1String name() const = 0;

	/**
	 * Start a process to check whether this is a primary or secondary
	 * package manager.  If the relevant program does not exist, then no
	 * process if created, bo entries are created in PkgQuery, and this
	 * function returns false.
	 *
	 * The process consists of a command that will test if the package
	 * manager binary is provided by a package in the package manager
	 * itself.  Derived classes are required to implement functions to
	 * return relevant the program and arguments, and a regular expression
	 * to test for the expected output.
	 *
	 * If the regular expression matches, then this is considered to be a
	 * primary package manager.  The (probably just one ) primary package
	 * manager is used to create a cache of packages and filenames that can
	 * be for packaged and unpackaged queries.
	 *
	 * If the program exists, but the output does not match, then it is
	 * considered to be a secondary package manager.  Secondary package
	 * managers are queried for lists of packages, owning packages, etc.
	 *
	 * When the process finishes, a signal handler will add this package
	 * to the relevant list in PkgQuery and '_process' will be deleted.
	 **/
	bool check();

	/**
	 * Wait until the primary package manager checking process has
	 * finished, or the timeout is exceeded.  Returns whether the process
	 * finishes successfully. If '_process' is null, for example it was
	 * never set or has been reset by the finished() signal handler, then
	 * true is returned immediately.
	 **/
	bool waitForPrimary()
	    { return _process ? _process->waitForFinished() : true; }

	/**
	 * Return the owning package of a file or directory with full path
	 * 'path' or an empty string if it is not owned by any package.
	 **/
	virtual QString owningPkg( const QString & path ) const = 0;

	/**
	 * Return the list of installed packages.
	 *
	 * Ownership of the list elements is transferred to the caller.
	 *
	 * This is an optional feature; a package manager that implements this
	 * should also return 'true' in supportsGetInstalledPkg().
	 *
	 * The default implementation returns nothing.
	 **/
	virtual PkgInfoList installedPkg() const { return PkgInfoList{}; }

	/**
	 * Return the command and arguments for getting the list of files and
	 * directories owned by a package, as a PkgCommand struct.
	 *
	 * This is an optional feature; a package manager that implements this
	 * should also return 'true' in supportsGetFileList().
	 *
	 * The default implementation returns nothing.
	 **/
	virtual PkgCommand fileListCommand( const PkgInfo * ) const
	    { return PkgCommand{ nullptr, {} }; };

	/**
	 * Parse the output of the file list command and return a list of
	 * strings, each of which is a file name.
	 *
	 * The default implementation does nothing.
	 **/
	virtual QStringList parseFileList( const QString & ) const
	    { return QStringList{}; }

	/**
	 * Create a file list cache with the specified lookup type for all
	 * installed packages. This is an expensive operation.  Must be
	 * reimplemented by derived classes.
	 *
	 * This is a best-effort approach; the cache might still not contain
	 * all desired packages. Check with PkgFileListCache::contains() and
	 * use PkgManager::fileListCommand() as a fallback.
	 *
	 * Ownership of the cache is transferred to the caller; make sure to
	 * delete it when you are done with it.
	 **/
	virtual PkgFileListCache * createFileListCache() const
	    { return nullptr; }

	/**
	 * Return a name suitable for detailed queries for 'pkg'.
	 */
	virtual QString queryName( const PkgInfo * pkg ) const
	    { return pkg->name(); }


    protected:

	/**
	 * Return a command to determine whether this is an installed primary
	 * or secondary package manager.  Derived classes must implement this.
	 **/
	virtual PkgCommand isPrimaryCommand() const = 0;

	/**
	 * Return a regular expression to test the output of isPrimaryCommand()
	 * to determine if it is an installed primary package manager.  Derived
	 * classes must implement this.
	 **/
	virtual QString isPrimaryRegExp() const = 0;

	/**
	 * Return 'true' if this package manager supports getting the list of
	 * installed packages.  Implies that the function installedPkg() is
	 * defined
	 *
	 * The default implementation returns 'false'.
	 **/
	virtual bool supportsGetInstalledPkg() const { return false; }

	/**
	 * Return 'true' if this package manager supports getting the file list
	 * for a package; implies that the functions fileListCommand() and
	 * parseFileList() are defined.
	 *
	 * The default implementation returns 'false'.
	 *
	 ** See also supportsFileListCache().
	 **/
	virtual bool supportsFileList() const { return false; }

	/**
	 * Return 'true' if this package manager supports building a file list
	 * cache for getting all file lists for all packages: implies that the
	 * function createFileListCache() is defined.
	 *
	 * The default implementation returns 'false'.
	 **/
	virtual bool supportsFileListCache() const { return false; }


    private:

	QProcess * _process{ nullptr };

    };	// class PkgManager

}	// namespace QDirStat

#endif	// PkgManager_h
