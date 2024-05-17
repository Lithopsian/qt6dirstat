/*
 *   File name: SysUtil.cpp
 *   Summary:   System utility functions for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#define DONT_DEPRECATE_STRERROR // for Logger.h

#include <errno.h>
#include <pwd.h>	// getpwuid()
#include <grp.h>	// getgrgid()
#include <limits.h>     // PATH_MAX
#include <sys/stat.h>   // lstat()
#include <sys/types.h>

#include "SysUtil.h"
#include "Exception.h"
#include "Logger.h"


using namespace QDirStat;


bool SysUtil::tryRunCommand( const QString & commandLine,
			     const QString & expectedResult,
			     bool            logCommand,
			     bool            logOutput )
{
    int exitCode = -1;
    QString output = runCommand( commandLine, &exitCode,
				 COMMAND_TIMEOUT_SEC, logCommand, logOutput,
				 true ); // ignoreErrCode

    if ( exitCode != 0 )
    {
	//logDebug() << "Exit code: " << exitCode << " command line: \"" << commandLine << "\"" << Qt::endl;
	return false;
    }

    const bool expected = QRegularExpression( expectedResult ).match( output ).hasMatch();

    return expected;
}


QString SysUtil::runCommand( const QString & commandLine,
			     int           * exitCode_ret,
			     int             timeout_sec,
			     bool            logCommand,
			     bool            logOutput,
			     bool            ignoreErrCode )
{
    if ( exitCode_ret )
	*exitCode_ret = -1;

    QStringList args = commandLine.split( QRegularExpression( "\\s+" ) );

    if ( args.size() < 1 )
    {
	logError() << "Bad command line: \"" << commandLine << "\"" << Qt::endl;
	return "ERROR: Bad command line";
    }

    const QString command = args.takeFirst();

    return runCommand( command, args, exitCode_ret,
		       timeout_sec, logCommand, logOutput, ignoreErrCode );
}


QString SysUtil::runCommand( const QString     & command,
			     const QStringList & args,
			     int               * exitCode_ret,
			     int                 timeout_sec,
			     bool                logCommand,
			     bool                logOutput,
			     bool                ignoreErrCode )
{
    if ( exitCode_ret )
	*exitCode_ret = -1;

    if ( !haveCommand( command ) )
    {
	//logInfo() << "Command not found: " << command << Qt::endl;
	return "ERROR: Command not found";
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert( "LANG", "C" ); // Prevent output in translated languages

    QProcess process;
    process.setProgram( command );
    process.setArguments( args );
    process.setProcessEnvironment( env );
    process.setProcessChannelMode( QProcess::MergedChannels ); // combine stdout and stderr

    if ( logCommand )
	logDebug() << command << " " << args.join( ' ' ) << Qt::endl;

    process.start();
    const bool success = process.waitForFinished( timeout_sec * 1000 );
    QString output = QString::fromUtf8( process.readAll() );

    if ( success )
    {
	if ( process.exitStatus() == QProcess::NormalExit )
	{
	    if ( exitCode_ret )
		*exitCode_ret = process.exitCode();

	    if ( !ignoreErrCode && process.exitCode() )
	    {
		logError() << "Command exited with exit code "
			   << process.exitCode() << ": "
			   << command << "\" args: " << args
			   << Qt::endl;
	    }
	}
	else
	{
	    logError() << "Command crashed: \"" << command << "\" args: " << args << Qt::endl;
	    output = "ERROR: Command crashed\n\n" + output;
	}
    }
    else
    {
	logError() << "Timeout or crash: \"" << command << "\" args: " << args << Qt::endl;
	output = "ERROR: Timeout or crash\n\n" + output;
    }

    if ( logOutput || ( process.exitCode() != 0 && !ignoreErrCode ) )
    {
        if ( output.contains( '\n' ) )
            logDebug() << "Output: \n" << output << Qt::endl;
        else
            logDebug() << "Output: \"" << output.trimmed() << "\"" << Qt::endl;
    }

    return output;
}

/*
void SysUtil::openInBrowser( const QString & url )
{
    logDebug() << "Opening URL " << url << Qt::endl;

    QProcess::startDetached( "/usr/bin/xdg-open", { url } );
}
*/
/*
bool SysUtil::isBrokenSymLink( const QString & path )
{
    const QByteArray target = readLink( path );

    if ( target.size() == 0 )   // path is not a symlink
        return false;           // so it's also not a broken symlink


    // Start from the symlink's parent directory

    QStringList pathSegments = path.split( '/', Qt::SkipEmptyParts );
    pathSegments.removeLast(); // We already know it's a symlink, not a directory
    const QString parentPath = QString( path.startsWith( '/' ) ? '/' : QString() ) + pathSegments.join( '/' );
    const DirSaver dir( parentPath );

    // We can't use access() here since that would follow symlinks.
    // Let's use lstat() instead.

    struct stat statBuf;
    const int statResult = lstat( target, &statBuf );

    if ( statResult == 0 )      // lstat() successful?
    {
        return false;           // -> the symlink is not broken.
    }
    else                        // lstat() failed
    {
        if ( errno == EACCES )  // permission denied for one of the dirs in target
        {
            logWarning() << "Permission denied for one of the directories"
                         << " in symlink target " << QString::fromUtf8( target )
                         << " of symlink " << path
                         << Qt::endl;

            return false;       // We don't know if the symlink is broken
        }
        else
        {
            logWarning() << "Broken symlink " << path
                         << " errno: " << strerror( errno )
                         << Qt::endl;
            return true;
        }
    }
}
*/

QByteArray SysUtil::readLink( const QByteArray & path )
{
#ifndef PATH_MAX
    const int maxLen = PATH_MAX;
#else
    // no max path, just pick a big number that isn't too out of control
    const int maxLen = 1024 * 1024;
#endif

    // 99+% of symlinks will fit in a small buffer
    QByteArray buf( 256, Qt::Uninitialized );

    ssize_t len = ::readlink( path, buf.data(), buf.size() );
    while ( len == buf.size() ) {
        // readlink(2) will fill our buffer and not necessarily terminate with NUL;
        if ( buf.size() >= maxLen )
            return QByteArray();

        // double the size and try again
        buf.resize( buf.size() * 2 );
        len = ::readlink( path, buf.data(), buf.size() );
    }

    if (len == -1)
        return QByteArray();

    buf.resize( len );
    return buf;
}


QString SysUtil::baseName( const QString & fileName )
{
    //logDebug() << fileName << Qt::endl;
    const QStringList segments = fileName.split( '/', Qt::SkipEmptyParts );
    if ( !segments.isEmpty() )
	return segments.last();

    return QString();
}


QString SysUtil::userName( uid_t uid )
{
    const struct passwd * pw = getpwuid( uid );
    if ( pw )
	return pw->pw_name;

    return QString::number( uid );
}


QString SysUtil::groupName( gid_t gid )
{
    const struct group * grp = getgrgid( gid );
    if ( grp )
	return grp->gr_name;

    return QString::number( gid );
}


QString SysUtil::homeDir( uid_t uid )
{
    const struct passwd * pw = getpwuid( uid );
    if ( pw )
	return QString::fromUtf8( pw->pw_dir );

    return QString();
}
