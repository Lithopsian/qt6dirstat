/*
 *   File name: PacManPkgManager.h
 *   Summary:	PacMan package manager support for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#ifndef PacManPkgManager_h
#define PacManPkgManager_h

#include <QString>

#include "PkgManager.h"


namespace QDirStat
{
    /**
     * Interface to 'pacman' for Manjaro / Arch Linux.
     **/
    class PacManPkgManager: public PkgManager
    {
    public:

	/**
	 * Return the name of this package manager.
	 *
	 * Implemented from PkgManager.
	 **/
	virtual QString name() const { return "pacman"; }

	/**
	 * Check if RPM is active on the currently running system.
	 *
	 * Implemented from PkgManager.
	 **/
	virtual bool isPrimaryPkgManager() const override;

	/**
	 * Check if the rpm command is available on the currently running
	 * system.
	 *
	 * Implemented from PkgManager.
	 **/
	virtual bool isAvailable() const override;

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
	virtual QString owningPkg( const QString & path ) const override;


        //-----------------------------------------------------------------
        //                     Optional Features
        //-----------------------------------------------------------------

        /**
         * Return 'true' if this package manager supports getting the list of
         * installed packages.
         *
	 * Reimplemented from PkgManager.
         **/
        virtual bool supportsGetInstalledPkg() const override
            { return true; }

        /**
         * Return the list of installed packages.
         *
         * Ownership of the list elements is transferred to the caller.
         *
	 * Reimplemented from PkgManager.
         **/
        virtual PkgInfoList installedPkg() const override;

        /**
         * Return 'true' if this package manager supports getting the file list
         * for a package.
         *
	 * Reimplemented from PkgManager.
         **/
        virtual bool supportsFileList() const override
            { return true; }

        /**
         * Return the command for getting the list of files and directories
         * owned by a package.
         *
	 * Reimplemented from PkgManager.
         **/
        virtual QString fileListCommand( const PkgInfo * pkg ) const override
	    { return QString( "/usr/bin/pacman -Qlq %1" ).arg( pkg->baseName() ); }

        /**
         * Parse the output of the file list command.
         *
	 * Reimplemented from PkgManager.
         **/
        virtual QStringList parseFileList( const QString & output ) const override
	    { return output.split( "\n" ); }


    protected:

        /**
         * Parse a package list as output by "dpkg-query --show --showformat".
         **/
        PkgInfoList parsePkgList( const QString & output ) const;

    }; // class PacManPkgManager

} // namespace QDirStat


#endif // PacManPkgManager_h