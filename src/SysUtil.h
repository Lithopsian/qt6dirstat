/*
 *   File name: SysUtil.h
 *   Summary:   System utility functions for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef SysUtil_h
#define SysUtil_h

#include <fcntl.h> // AT_FDCWD, AT_EACCESS
#include <unistd.h> // faccessat(), getuid(), geteduid(), readlink()
#include <sys/stat.h> // fstatat()
#include <sys/types.h> // uid_t

#include <QProcessEnvironment>
#include <QStringBuilder>


#define COMMAND_TIMEOUT_SEC 30


namespace QDirStat
{
    /**
     * System utility functions
     **/
    namespace SysUtil
    {
	/**
	 * Return the flags to use with fstatat():
	 * - AT_SYMLINK_NOFOLLOW means that the call behaves like lstat(), that
	 * is it returns information about symbolic links, and not about the
	 * target of the link;
	 * - AT_NO_AUTOMOUNT means that directories with auto-mounts should not
	 * be mounted, and information is returned about the directory itself
	 * unless it is already mounted.  This flag did not exist before
	 * kernel 2.6.38, was ignored starting with kernel 3.1, and became the
	 * default with kernel 4.11, so almost pointless.
	 **/
	inline int statFlags()
#ifdef AT_NO_AUTOMOUNT
	    { return AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT; }
#else
	    { return AT_SYMLINK_NOFOLLOW; }
#endif

	/**
	 * Populate 'statInfo' and return whether the status code from this
	 * command.  Symlinks are not followed and, as far as is possible,
	 * auto-mounts are not mounted.
	 **/
	inline int stat( int dirFd, const char * path, struct stat & statInfo )
	    { return fstatat( dirFd, path, &statInfo, statFlags() ); }
	inline int stat( const QString & path, struct stat & statInfo )
	    { return fstatat( AT_FDCWD, path.toUtf8(), &statInfo, statFlags() ); }

	/**
	 * Return true if 'file' exists (and is visible to the current user).
	 **/
	inline bool exists( const char * file )
	    { return faccessat( AT_FDCWD, file, F_OK, AT_EACCESS | AT_SYMLINK_NOFOLLOW ) == 0; }
	inline bool exists( const QString & file )
	    { return faccessat( AT_FDCWD, file.toUtf8(), F_OK, AT_EACCESS | AT_SYMLINK_NOFOLLOW ) == 0; }

	/**
	 * Return true if the current user has access to files in 'dir'.
	 **/
	inline bool canAccess( const char * dir )
	    { return faccessat( AT_FDCWD, dir, R_OK | W_OK | X_OK, AT_EACCESS | AT_SYMLINK_NOFOLLOW ) == 0; }
	inline bool canAccess( const QString & dir )
	    { return faccessat( AT_FDCWD, dir.toUtf8(), R_OK | W_OK | X_OK, AT_EACCESS | AT_SYMLINK_NOFOLLOW ) == 0; }

	/**
	 * Return true if 'command' is available and executable (possibly at a
	 * symlink target).
	 **/
	inline bool haveCommand( const char * command )
	    { return faccessat( AT_FDCWD, command, X_OK, AT_EACCESS ) == 0; }
	inline bool haveCommand( const QString & command )
	    { return faccessat( AT_FDCWD, command.toUtf8(), X_OK, AT_EACCESS ) == 0; }

	/**
	 * Return a QProcess pointer, created from program 'command' and
	 * arguments 'args'.  The process is set to use a C environment and
	 * produced merged output.  It is not started.
	 **/
	QProcess * commandProcess( const QString & command, const QStringList & args );

	/**
	 * Run a command with arguments 'args' and return its output. If
	 * exitCode_ret is non-null, return the command's exit code in
	 * exitCode_ret.
	 *
	 * Log the command that is executed if 'logCommand' is true,
	 * log the command's output if 'logOutput' is true.
	 *
	 * If the command exits with a non-zero exit code, both the command and
	 * the output are logged anyway unless 'logError' is false.
	 *
	 * The command is not executed in a shell; the command is run directly
	 * so only binaries can be executed, no shell scripts or scripts of
	 * other interpreted languages. If that is desired, wrap the command
	 * into "/bin/sh -c".
	 **/
	QString runCommand( const QString     & program,
	                    const QStringList & args,
	                    int               * exitCode_ret = nullptr,
	                    int                 timeoutSec   = COMMAND_TIMEOUT_SEC,
	                    bool                logCommand   = true,
	                    bool                logOutput    = false,
	                    bool                logError     = true );

	/**
	 * Check if this program runs with root privileges, i.e. with effective
	 * user ID 0.
	 **/
	inline bool runningAsRoot()
	    { return geteuid() == 0; }

	/**
	 * Check if this program runs with sudo.
	 **/
	inline bool runningWithSudo()
	    { return !QProcessEnvironment::systemEnvironment().value( "SUDO_USER", QString{} ).isEmpty(); }

	/**
	 * Check if this program runs as the real root user, with root
	 * permissions, but not with sudo.
	 **/
	inline bool runningAsTrueRoot()
	    { return runningAsRoot() && !runningWithSudo(); }

	/**
	 * Return the home directory of the user with the specified user ID.
	 **/
	QString homeDir( uid_t uid );

	/**
	 * Return the (first level) target of a symbolic link, i.e. the path
	 * that the link points to. That target may again be a symlink;
	 * this function does not follow multiple levels of symlinks.
	 *
	 * If 'path' is not a symlink, this returns an empty string.
	 *
	 * This function assumes UTF-8 encoding of names in the filesystem.
	 **/
	QString symlinkTarget( const QString & path );

	/**
	 * Return the last path component of a file name.
	 *
	 * Examples:
	 *
	 *	   "/home/bob/foo.txt"	-> "foo.txt"
	 *	   "foo.txt"		-> "foo.txt"
	 *	   "/usr/bin"		-> "bin"
	 *
	 * Note that if 'fileName' ends with "/" then this function will
	 * return an empty string.  This will normally only happen for
	 * root (ie. "/").
	 **/
	QString baseName( const QString & fileName );

	/**
	 * Split a path up into its base path (everything up to the last path
	 * component) and its base name (the last path component).
	 *
	 * Both 'path_ret' and 'name_ret' are return parameters and will be
	 * modified by this function.
	 *
	 * If 'path' is root (ie. "/") 'basPath_ret' is empty and
	 * 'name_ret' = "/".  If 'path' has no "/" characters, then
	 * 'name_ret' = 'path' and 'path_ret' is empty.  Otherwise
	 * 'path_ret' contains the initial portion of 'path' up to and
	 * including the last "/" character and 'name_ret' contains the portion
	 * of 'path' after the last "/" character to the end of the string.
	 **/
	void splitPath( const QString & fileNameWithPath,
                        QString       & path_ret,   // return parameter
                        QString       & name_ret ); // return parameter

	/**
	 * Return the user name of the owner.
	 **/
	QString userName( uid_t uid );

	/**
	 * Return the group name of the owner.
	 **/
	QString groupName( gid_t gid );

	/**
	 * Return a string with all occurrences of a single quote escaped, for
	 * shells.  This means effectively closing the first part of the string
	 * with a single quote, inserting a backslash to escape the single
	 * quote, then opening the remaining part of the string with another
	 * single quote.
	 *
	 * Thus, 'Don't do this' becomes 'Don'\''t do this'.
	 *
	 * Note that the backslash itself must be escaped inside the C++ string
	 * or it would be interpreted as escaping the following character.
	 **/
	inline QString escaped( const QString & unescaped )
	    { return QString{ unescaped }.replace( u'\'', QLatin1String{ "'\\''" } ); }

	/**
	 * Return a string in single quotes, with single quotes in the string
	 * escaped.
	 **/
	inline QString shellQuoted( const QString & unquoted )
	    { return '\'' % escaped( unquoted ) % '\''; }

    }	// namespace SysUtil

}	// namespace QDirStat

#endif // SysUtil_h
