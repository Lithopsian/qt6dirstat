/*
 *   File name: DPkgPkgManager.cpp
 *   Summary:   Dpkg package manager support for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef DpkgPkgManager_h
#define DpkgPkgManager_h

#include "PkgManager.h"


namespace QDirStat
{
    /**
     * Interface to 'dpkg' for all Debian-based Linux distros.
     *
     * Remember that 'apt' is based on 'dpkg' and 'dpkg' already does the
     * simple things needed here, so there is no need to create a specialized
     * version for 'apt' or any higher-level dpkg-based package managers.
     *
     * The commands used are:
     * dpkg-query --show to return a list of all packages
     * dpkg -S * to return all files in all packages
     * dpkg-query --listfiles to return a list of files in a given package
     *
     * The normal output from dpkg-query --listfiles is one pathname per line.
     *
     * The normal output from dpkg -S is a package, colon, space, file pathname
     *
     * gdb: /usr/bin/gdb
     *
     * There may be more than one package per line, separated by commas:
     *
     * libc6:amd64, libc6:i386: /usr/share/doc/libc6
     *
     * The path portion of the pathname (may be the whole pathname for a directory entry) can be
     * a symbolic link on any given file system and these are resolved to the real path on this
     * file system. The file portion of the pathname (when it isn't a directory) may be a symbolic
     * link and this won't be resolved.
     *
     * Pathological cases: File diversion (See man dpkg-divert).
     *
     * File diversions in dpkg -S are shown by either 2 or 3 lines per file.
     * File diversion for the file that is diverted away from has 3 lines
     * dpkg -S /bin/sh	-->
     *
     * diversion by dash from: /bin/sh
     * diversion by dash to: /bin/sh.distrib
     * dash: /bin/sh
     *
     * The package shown in the first line is the "owner" of the file, but it often does not
     * appear in the file list for that package (eg. a symlink is created by script or the
     * package is just "making room" for an external provider), and it will then not appear on
     * the third line.  The package shown in the third line is only the owner if
     * it is the same package shown in the first line.
     *
     * Query output for the file that is the result of a diversion has only 2 lines
     * dpkg -S /bin/sh.distrib	-->
     * diversion by dash from: /bin/sh
     * diversion by dash to: /bin/sh.distrib
     *
     * The file /bin/sh belongs to the package dash.  The file /bin/sh.distrib belongs
     * to some other package, which can pnly be found by further querying against /bin/sh, although
     * in this case there is no original owner and the file sh.distrib does not (yet) exist.
     * diversion by dash from: /bin/sh
     * diversion by dash to: /bin/sh.distrib
     *
     * For example:
     * diversion by glx-diversions from: /usr/lib/x86_64-linux-gnu/libGL.so
     * diversion by glx-diversions to: /usr/lib/mesa-diverted/x86_64-linux-gnu/libGL.so
     * libgl-dev:amd64: /usr/lib/x86_64-linux-gnu/libGL.so
     *
     * Here, the file libGL.so provided by the package libgl-dev is located at
     * /usr/lib/mesa-diverted/x86_64-linux-gnu/libGL.so and the file now at
     * /usr/lib/x86_64-linux-gnu/libGL.so is provided by glx-diversions although in this
     * case only a symlink is provided to /etc/alternatives.
     *
     * Both the original owning package (possibly more than one) and the diverting package may
     * appear on the third line.  Here the package on all three lines is the current owner
     * of /usr/bin/firefox and has renamed any file previously at that location (or even
     * installed later) to firefox.real.  The other package on the third line (possibly more than
     * one) is the original owner of /usr/bin/firefox, now at firefox.real.
     * diversion by firefox-esr from: /usr/bin/firefox
     * diversion by firefox-esr to: /usr/bin/firefox.real
     * firefox, firefox-esr: /usr/bin/firefox
     *
     * The last case is a local diversion, where the file is not diverted by a package:
     * local diversion from: /usr/share/doc/lm-sensors/vid
     * local diversion to: /usr/share/doc/lm-sensors/vid.distrib
     * lm-sensors: /usr/share/doc/lm-sensors/vid
     *
     * Here, the file vid provided by the package lm-sensors is now located at
     * /usr/share/doc/lm-sensors/vid.distrib and the one at /usr/share/doc/lm-sensors/vid
     * is provided by means other than a package.
     *
     * The text returned by dpkg-query --listfiles or dpkg -L is different:
     * diverted by ...
     * package diverts others to: ...
     * locally diverted to: ...
     *
     * These strings are all localised and may be different in non-English locales.
     * The locale must be set to C-UTF-8 for reliable results.
     *
     * A more efficient solution might be to read the file /var/lib/dpkg/diversions which has
     * plain lines showing just the original and renamed file paths and the diverting package.
     * However, the location of that file is configurable and there will typically be only a handful
     * of diversions on a computer (although some rename multiple files, such as glx-diversions).
     **/
    class DpkgPkgManager final : public PkgManager
    {
    public:

	/**
	 * Return the command for the dpkg manager program.
	 **/
	static const char * dpkgCommand()
	    { return "/usr/bin/dpkg"; }

	/**
	 * Return the name of this package manager.
	 *
	 * Implemented from PkgManager.
	 **/
	QLatin1String name() const override { return QLatin1String{ "dpkg" }; }

	/**
	 * Return the owning package of a file or directory with full path
	 * 'path' or an empty string if it is not owned by any package.
	 *
	 * Implemented from PkgManager.
	 *
	 * This basically executes this command:
	 *
	 *   /usr/bin/dpkg -S ${path}
	 *
	 * The command is run against the full path, hopeing for an exact match.
	 * If that fails, it is run against only the filename in case there
	 * are symlinks in the direct path.
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
	 * Return the command for querying dpkg for files and directories.
	 **/
	static const char * dpkgQueryCommand() { return "/usr/bin/dpkg-query"; }

	/**
	 * Return the command for getting the list of files and directories
	 * owned by an individual package, or querying a sequence of up to
	 * about 300 packages one by one.
	 *
	 * Reimplemented from PkgManager.
	 **/
	PkgCommand fileListCommand( const PkgInfo * pkg ) const override
	    { return PkgCommand{ dpkgQueryCommand(), QStringList{ "--listfiles", queryName( pkg ) } }; }

	/**
	 * Parse the output of the file list command and return a list
	 * of file names.
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
	    { return PkgCommand{ DpkgPkgManager::dpkgCommand(), { "-S", dpkgCommand() } }; }

	/**
	 * Returns a regular expression string to test whether the output of a
	 * process from isPrimaryCommand() matches that expected if dpkg is
	 * the primary package manager.
	 *
	 * Implemented from PkgManager.
	 **/
	QString isPrimaryRegExp() const override { return "^dpkg:.*"; };

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

    };	// class DpkgPkgManager

}	// namespace QDirStat

#endif	// ifndef DpkgPkgManager
