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

#include <unistd.h>	// access(), getuid(), geteduid(), readlink()
#include <sys/types.h>	// uid_t

#include <QProcess>
#include <QString>
#include <QRegularExpression>


// Override these before #include

#ifndef LOG_COMMANDS
#  define LOG_COMMANDS true
#endif

#ifndef LOG_OUTPUT
#  define LOG_OUTPUT false
#endif

#ifndef COMMAND_TIMEOUT_SEC
#  define COMMAND_TIMEOUT_SEC 15
#endif


namespace QDirStat
{
    /**
     * System utility functions
     **/
    namespace SysUtil
    {
	/**
	 * Try running a command and compare it against an expected result.
	 * Return 'true' if ok, 'false' if not.
	 *
	 * Log the command that is executed if 'logCommand' is 'true',
	 * log the command's output if 'logOutput' is 'true'.
	 **/
	bool tryRunCommand( const QString & commandLine,
			    const QString & expectedResult,
			    bool            logCommand = LOG_COMMANDS,
			    bool            logOutput  = LOG_OUTPUT );

	/**
	 * Run a command line and return its output. If exitCode_ret is
	 * non-null, return the command's exit code in exitCode_ret.
	 *
	 * Log the command that is executed if 'logCommand' is 'true',
	 * log the command's output if 'logOutput' is 'true'.
	 *
	 * If the command exits with a non-zero exit code, both the command and
	 * the output are logged anyway unless 'ignoreErrCode' is 'true'.
	 *
	 * NOTICE 1: This uses a very basic command line parser; it simply
	 * splits the command up wherever whitespace might occur. If any of
	 * the arguments (no matter how sophisticated they might be quoted)
	 * possibly contains any whitespace, this is unsafe; in that case, use
	 * the overloaded version instead that accepts a QStringList as
	 * arguments.
	 *
	 * NOTICE 2: This does not start a shell with that command, it runs the
	 * command directly, so only binaries can be executed, no shell scripts
	 * or scripts of other interpreted languages. If that is desired, wrap
	 * the command into "/bin/sh -c".
	 **/
	QString runCommand( const QString & commandLine,
			    int           * exitCode_ret  = 0,
			    int             timeout_sec   = COMMAND_TIMEOUT_SEC,
			    bool            logCommand    = LOG_COMMANDS,
			    bool            logOutput     = LOG_OUTPUT,
			    bool            ignoreErrCode = false );

	/**
	 * Run a command with arguments 'args' and return its output. If
	 * exitCode_ret is non-null, return the command's exit code in
	 * exitCode_ret.
	 *
	 * Use this version to avoid any side effects due to command line
	 * parsing.
	 *
	 * Log the command that is executed if 'logCommand' is 'true',
	 * log the command's output if 'logOutput' is 'true'.
	 *
	 * If the command exits with a non-zero exit code, both the command and
	 * the output are logged anyway unless 'ignoreErrCode' is 'true'.
	 *
	 * NOTICE: This does not start a shell with that command, it runs the
	 * command directly, so only binaries can be executed, no shell scripts
	 * or scripts of other interpreted languages. If that is desired, use
	 * "/bin/sh" as the command, "-c" as the first argument and the command
	 * line to be executed as the second. Beware of shell quoting quirks!
	 **/
	QString runCommand( const QString     & command,
			    const QStringList & args,
			    int               * exitCode_ret  = 0,
			    int                 timeout_sec   = COMMAND_TIMEOUT_SEC,
			    bool                logCommand    = LOG_COMMANDS,
			    bool                logOutput     = LOG_OUTPUT,
			    bool                ignoreErrCode = false );

	/**
	 * Return 'true' if the specified command is available and executable.
	 **/
	inline static bool haveCommand( const QString & command )
	    { return access( command.toUtf8(), X_OK ) == 0; }

	/**
	 * Open a URL in the desktop's default browser (using the
	 * /usr/bin/xdg-open command).
	 **/
//	void openInBrowser( const QString & url ); // replaced by DesktopServices::openUrl()

	/**
	 * Check if this program runs with root privileges, i.e. with effective
	 * user ID 0.
	 **/
	inline static bool runningAsRoot()
	    { return geteuid() == 0; }

	/**
	 * Check if this program runs with 'sudo'.
	 **/
	inline static bool runningWithSudo()
	    { return !QProcessEnvironment::systemEnvironment().value( "SUDO_USER", QString() ).isEmpty(); }

	/**
	 * Check if this program runs as the real root user, with root
	 * permissions, but not with 'sudo'.
	 **/
	inline static bool runningAsTrueRoot()
	    { return runningAsRoot() && !runningWithSudo(); }

	/**
	 * Return the home directory of the user with the specified user ID.
	 **/
	QString homeDir( uid_t uid );

        /**
         * Return 'true' if a symbolic link is broken, i.e. the (first level)
         * target of the symlink does not exist in the filesystem.
         **/
//        bool isBrokenSymLink( const QString & path );

        /**
         * Read the (first level) target of a symbolic link.
         * Unlike readLink( const QString & ) above, this does not make any
         * assumptions of name encoding in the filessytem; it just uses bytes.
         *
         * This is a more user-friendly version of readlink(2).
         *
         * This returns an empty QByteArray if 'path' is not a symlink.
         **/
        QByteArray readLink( const QByteArray & path );

        /**
         * Read the (first level) target of a symbolic link, assuming UTF-8
         * encoding of names in the filesystem.
         * This is a more user-friendly version of readlink(2).
         *
         * This returns an empty QByteArray if 'path' is not a symlink.
         **/
        inline static QByteArray readLink( const QString & path )
	    { return readLink( path.toUtf8() ); }

        /**
         * Return the (first level) target of a symbolic link, i.e. the path
         * that the link points to. That target may again be a symlink;
         * this function does not follow multiple levels of symlinks.
         *
         * If 'path' is not a symlink, this returns an empty string.
         *
         * This function assumes UTF-8 encoding of names in the filesystem.
         **/
        inline static QString symLinkTarget( const QString & path )
	    { return QString::fromUtf8( readLink( path ) ); }

	/**
	 * Return the last pathname component of a file name.
	 *
	 * Examples:
	 *
	 *	   "/home/bob/foo.txt"	-> "foo.txt"
	 *	   "foo.txt"		-> "foo.txt"
	 *	   "/usr/bin"		-> "bin"
	 *	   "/usr/bin/"		-> "bin"
	 *
	 * Notice that FileInfo also has a member function baseName().
	 **/
	QString baseName( const QString & fileName );

	/**
	 * Return the user name of the owner.
	 **/
	QString userName( uid_t uid );

	/**
	 * Return the group name of the owner.
	 **/
	QString groupName( gid_t gid );

	/**
	 * Return a string with all occurrences of a single quote escaped.  For
	 * shells, this means effectively closing the first part of the string
	 * with a single quote, inserting a backslash to escape the single quote,
	 * then opening the remaining part of the string with another single quote.
	 *
	 * Thus, 'Don't do this' becomes 'Don'\''t do this'.
	 *
	 * Note that the backslash itself must be escaped inside the C++ string
	 * or it would be interpreted as escaping the follopwing character.
	 **/
	inline static QString escaped( const QString & unescaped )
	    { return QString( unescaped ).replace( '\'', QLatin1String( "'\\''" ) ); }

	/**
	 * Return a string in single quotes.
	 **/
	inline static QString quoted( const QString & unquoted )
	    { return '\'' + unquoted + '\''; }

    }	// namespace SysUtil

}	// namespace QDirStat


#endif // SysUtil_h
