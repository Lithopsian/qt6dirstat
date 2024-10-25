/*
 *   File name: SysUtil.cpp
 *   Summary:   System utility functions for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <pwd.h>	// getpwuid()
#include <grp.h>	// getgrgid()
#include <sys/stat.h>   // lstat()

#include <QRegularExpression>

#include "SysUtil.h"
#include "Logger.h"


using namespace QDirStat;


bool SysUtil::tryRunCommand( const QString & commandLine,
                             const QString & expectedResult,
                             bool            logCommand,
                             bool            logOutput )
{
    int exitCode = -1;
    QString output = runCommand( commandLine,
                                 &exitCode,
                                 COMMAND_TIMEOUT_SEC,
                                 logCommand,
                                 logOutput,
                                 true ); // ignoreErrCode

    if ( exitCode != 0 )
    {
	//logDebug() << "Exit code: " << exitCode << " command line: \"" << commandLine << '"' << Qt::endl;
	return false;
    }

    const bool expected = QRegularExpression{ expectedResult }.match( output ).hasMatch();

    return expected;
}


QString SysUtil::runCommand( const QString & commandLine,
                             int           * exitCode_ret,
                             int             timeout_sec,
                             bool            logCommand,
                             bool            logOutput,
                             bool            logError )
{
    if ( exitCode_ret )
	*exitCode_ret = -1;

    QStringList args = commandLine.split( QRegularExpression{ "\\s+" } );

    if ( args.size() < 1 )
    {
	logError() << "Bad command line: \"" << commandLine << '"' << Qt::endl;
	return "ERROR: Bad command line";
    }

    const QString command = args.takeFirst();

    return runCommand( command, args, exitCode_ret, timeout_sec, logCommand, logOutput, logError );
}


QString SysUtil::runCommand( const QString     & command,
                             const QStringList & args,
                             int               * exitCode_ret,
                             int                 timeout_sec,
                             bool                logCommand,
                             bool                logOutput,
                             bool                logError )
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
	logDebug() << command << " " << args.join( u' ' ) << Qt::endl;

    process.start();
    const bool success = process.waitForFinished( timeout_sec * 1000 );
    QByteArray output = process.readAll();

    if ( success )
    {
	if ( process.exitStatus() == QProcess::NormalExit )
	{
	    if ( exitCode_ret )
		*exitCode_ret = process.exitCode();

	    if ( logError && process.exitCode() )
	    {
		logError() << "Command exited with exit code " << process.exitCode() << ": "
			   << command << "\" args: " << args << Qt::endl;
	    }
	}
	else
	{
	    logError() << "Command crashed: \"" << command << "\" args: " << args << Qt::endl;
	    output.prepend( "ERROR: Command crashed\n\n" );
	}
    }
    else
    {
	logError() << "Timeout or crash: \"" << command << "\" args: " << args << Qt::endl;
	output.prepend( "ERROR: Timeout or crash\n\n" );
    }

    if ( logOutput || ( process.exitCode() != 0 && logError ) )
    {
        if ( output.contains( u'\n' ) )
            logDebug() << "Output: \n" << output << Qt::endl;
        else
            logDebug() << "Output: \"" << output.trimmed() << '"' << Qt::endl;
    }

    return output;
}


QByteArray SysUtil::readLink( const QByteArray & path )
{
#ifdef PATH_MAX
    const int maxLen = PATH_MAX;
#else
    // no max path, just pick a big number that isn't too out of control
    const int maxLen = 1024 * 1024;
#endif

    // 99+% of symlinks will fit in a small buffer
    QByteArray buf{ 256, Qt::Uninitialized };

    auto len = ::readlink( path, buf.data(), buf.size() );
    while ( len == buf.size() ) {
        // readlink(2) will fill our buffer and not necessarily terminate with NUL;
        if ( buf.size() >= maxLen )
            return QByteArray{};

        // double the size and try again
        buf.resize( buf.size() * 2 );
        len = ::readlink( path, buf.data(), buf.size() );
    }

    if (len == -1)
        return QByteArray{};

    buf.resize( len );
    return buf;
}


QString SysUtil::baseName( const QString & fileName )
{
    //logDebug() << fileName << Qt::endl;
    const int delimiterIndex = fileName.lastIndexOf( u'/' );
    if ( delimiterIndex < 0 )
	return fileName;

    return fileName.mid( delimiterIndex + 1 );
}


void SysUtil::splitPath( const QString & fileNameWithPath,
                         QString       & path_ret, // return parameter
                         QString       & name_ret )    // return parameter
{
    const int delimeterIndex = fileNameWithPath.lastIndexOf ( u'/' );
    if ( delimeterIndex < 0 || delimeterIndex == fileNameWithPath.size() - 1 )
    {
	// Paths ending in "/" (notably root) or paths without any "/"
	path_ret = QString();
	name_ret = fileNameWithPath;

	return;
    }

    path_ret = fileNameWithPath.left( qMax( 1, delimeterIndex ) ); // at least "/"
    name_ret = fileNameWithPath.mid( delimeterIndex + 1 ); // everything after the last "/"
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

    return QString{};
}
