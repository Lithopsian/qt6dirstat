/*
 *   File name: PacManPkgManager.h
 *   Summary:   PacMan package manager support for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef PacManPkgManager_h
#define PacManPkgManager_h

#include "PkgManager.h"


namespace QDirStat
{
    /**
     * Interface to 'pacman' for Manjaro / Arch Linux.
     **/
    class PacManPkgManager final : public PkgManager
    {
    public:

	/**
	 * Return the name of this package manager.
	 *
	 * Implemented from PkgManager.
	 **/
	QLatin1String name() const override { return "pacman"_L1; }

	/**
	 * Check if RPM is active on the currently running system.
	 *
	 * Implemented from PkgManager.
	 **/
	bool isPrimaryPkgManager() const override;

	/**
	 * Check if the rpm command is available on the currently running
	 * system.
	 *
	 * Implemented from PkgManager.
	 **/
	bool isAvailable() const override;

	/**
	 * Return the owning package of a file or directory with full path
	 * 'path' or an empty string if it is not owned by any package.
	 *
	 * Implemented from PkgManager.
	 *
	 * This basically executes this command:
	 *
	 *   /usr/bin/pacman -Qo ${path}
	 **/
	QString owningPkg( const QString & path ) const override;

	//-----------------------------------------------------------------
	//                    Optional Features
	//-----------------------------------------------------------------

	/**
	 * Return 'true' if this package manager supports getting the list of
	 * installed packages.
	 *
	 * Reimplemented from PkgManager.
	 **/
	bool supportsGetInstalledPkg() const override { return true; }

	/**
	 * Return the list of installed packages.
	 *
	 * Ownership of the list elements is transferred to the caller.
	 *
	 * Reimplemented from PkgManager.
	 **/
	PkgInfoList installedPkg() const override;

	/**
	 * Return 'true' if this package manager supports getting the file list
	 * for a package.
	 *
	 * Reimplemented from PkgManager.
	 **/
	bool supportsFileList() const override { return true; }

	/**
	 * Return the command for querying pacman for files and directories.
	 **/
	static const char * pacmanCommand() { return "/usr/bin/pacman"; }

	/**
	 * Return the command for getting the list of files and directories
	 * owned by a package.
	 *
	 * Reimplemented from PkgManager.
	 **/
	PkgCommand fileListCommand( const PkgInfo * pkg ) const override
	    { return PkgCommand{ pacmanCommand(), QStringList{ "-Qlq", pkg->baseName() } }; }

	/**
	 * Parse the output of the file list command.
	 *
	 * Reimplemented from PkgManager.
	 **/
	QStringList parseFileList( const QString & output ) const override
	    { return output.split( u'\n' ); }

    };	// class PacManPkgManager

}	// namespace QDirStat

#endif	// PacManPkgManager_h
