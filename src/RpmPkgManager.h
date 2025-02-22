/*
 *   File name: RpmPkgManager.h
 *   Summary:   RPM package manager support for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef RpmPkgManager_h
#define RpmPkgManager_h

#include "PkgManager.h"


namespace QDirStat
{
    /**
     * Interface to 'rpm' for all RPM-based Linux distros such as SUSE,
     * openSUSE, Red Hat, Fedora.
     *
     * Remember that 'zypper' and 'yum' are based on 'rpm' and 'rpm' already
     * does the simple things needed here, so there is no need to create a
     * specialized version for 'zypper' or 'yum' or any higher-level rpm-based
     * package managers.
     **/
    class RpmPkgManager final : public PkgManager
    {
    public:

	/**
	 * Constructor.
	 **/
	RpmPkgManager();

	/**
	 * Return the name of this package manager.
	 *
	 * Implemented from PkgManager.
	 **/
	QLatin1String name() const override { return "rpm"_L1; }

	/**
	 * Return the owning package of a file or directory with full path
	 * 'path' or an empty string if it is not owned by any package.
	 *
	 * Implemented from PkgManager.
	 *
	 * This basically executes this command:
	 *
	 *   /usr/bin/rpm -qf ${path}
	 **/
	QString owningPkg( const QString & path ) const override;

	/**
	 * Return the list of installed packages.
	 *
	 * Ownership of the list elements is transferred to the caller.
	 *
	 * Reimplemented from PkgManager.
	 **/
	PkgInfoList installedPkg() const override;

	/**
	 * Return the command for getting the list of files and directories
	 * owned by a package.
	 *
	 * Reimplemented from PkgManager.
	 **/
	PkgCommand fileListCommand( const PkgInfo * pkg ) const override
	    { return PkgCommand{ _rpmCommand, QStringList{ "-ql", queryName( pkg ) } }; }

	/**
	 * Parse the output of the file list command.
	 *
	 * Reimplemented from PkgManager.
	 **/
	QStringList parseFileList( const QString & output ) const override;

	/**
	 * Create a file list cache with the specified lookup type for all
	 * installed packages. This is an expensive operation.
	 *
	 * This is a best-effort approach; the cache might still not contain
	 * all desired packages. Check with PkgFileListCache::contains() and
	 * use PkgManager::fileList() as a fallback.
	 *
	 * Ownership of the cache is transferred to the caller; make sure to
	 * delete it when you are done with it.
	 *
	 * Reimplemented from PkgManager.
	 **/
	PkgFileListCache * createFileListCache() const override;

	/**
	 * Return a name suitable for a detailed queries for 'pkg'.
	 *
	 * This might include the architecture and the version if this is a
	 * multi-arch or multi-version package.
	 *
	 * Reimplemented from PkgManager.
	 **/
	QString queryName( const PkgInfo * pkg ) const override;


    protected:

	/**
	 * Return the program and arguments for a command to test if this is a
	 * primary package manager.
	 *
	 * Implemented from PkgManager.
	 **/
	PkgCommand isPrimaryCommand() const override
	    { return PkgCommand{ _rpmCommand, { "-qf", _rpmCommand } }; }

	/**
	 * Returns a regular expression string to test whether the output of a
	 * process from isPrimaryCommand() matches that expected if rpm is the
	 * primary package manager.
	 *
	 * Implemented from PkgManager.
	 **/
	QString isPrimaryRegExp() const override { return "^rpm.*"; };

	/**
	 * Return 'true' if this package manager supports getting the list of
	 * installed packages.
	 *
	 * Reimplemented from PkgManager.
	 **/
	bool supportsGetInstalledPkg() const override { return true; }

	/**
	 * Return 'true' if this package manager supports getting the file list
	 * for a package.
	 *
	 * Reimplemented from PkgManager.
	 **/
	bool supportsFileList() const override { return true; }

	/**
	 * Return 'true' if this package manager supports building a file list
	 * cache for getting all file lists for all packages.
	 *
	 * Reimplemented from PkgManager.
	 **/
	bool supportsFileListCache() const override { return true; }


    private:

	const char * _rpmCommand;

    };	// class RpmPkgManager

}	// namespace QDirStat

#endif	// RpmPkgManager_h
