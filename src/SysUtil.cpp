/*
 *   File name: SysUtil.cpp
 *   Summary:   System utility functions for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <pwd.h> // getpwuid()
#include <grp.h> // getgrgid()

#include <QRegularExpression>

#include "SysUtil.h"
#include "Logger.h"


using namespace QDirStat;


bool SysUtil::tryRunCommand( const QString     & command,
                             const QStringList & args,
                             const QString     & expectedResult )
{
    int exitCode;
    QString output = runCommand( command,
                                 args,
                                 &exitCode,
                                 COMMAND_TIMEOUT_SEC,
                                 false, // don't log command
                                 false, // don't log output
                                 false ); // don't log errors, just return false

    if ( exitCode != 0 )
    {
	//logDebug() << "Exit code: " << exitCode
	//           << " command line: \"" << command << " " << args.join( u' ' ) << '"' << Qt::endl;
	return false;
    }

    return QRegularExpression{ expectedResult }.match( output ).hasMatch();
}


QString SysUtil::runCommand( const QString     & command,
                             const QStringList & args,
                             int               * exitCode_ret,
                             int                 timeoutSec,
                             bool                logCommand,
                             bool                logOutput,
                             bool                logError )
{
    int exitCode = -1;

    if ( logCommand )
	logDebug() << command << " " << args.join( u' ' ) << Qt::endl;

    const QString output = [ &command, &args, timeoutSec, logError, &exitCode ]() -> QString
    {
	if ( !haveCommand( command ) )
	    return "ERROR: Command not found\n\n";

	QProcess process;
	process.setProgram( command );
	process.setArguments( args );
	process.setProcessEnvironment( cProcessEnvironment() );
	process.setProcessChannelMode( QProcess::MergedChannels ); // combine stdout and stderr
	process.start();

	const bool finished = process.waitForFinished( timeoutSec * 1000 );
	const QString output{ process.readAll() };
	if ( finished )
	{
	    if ( process.exitStatus() == QProcess::NormalExit )
	    {
		exitCode = process.exitCode();

		if ( logError && exitCode != 0)
		{
		    logError() << "Command exited with exit code " << process.exitCode()
		               << ": " << command << "\" args: " << args << Qt::endl;
		}

		return output;
	    }
	    else
	    {
		logError() << "Command crashed: \"" << command << "\" args: " << args << Qt::endl;
		return "ERROR: Command crashed\n\n" % output;
	    }
	}
	else
	{
	    logError() << "Timeout or error: \"" << command << "\" args: " << args << Qt::endl;
	    return "ERROR: Timeout or error\n\n" % output;
	}
    }();

    if ( logOutput || ( exitCode != 0 && logError ) )
    {
	const auto formatOutput = [ &output ]() -> QString
	{
	    if ( output.contains( u'\n' ) )
		return '\n' % output;
	    else
		return '"' % output.trimmed() % '"';
	};
	logInfo() << "Output: " << formatOutput() << Qt::endl;
    }

    if ( exitCode_ret )
	*exitCode_ret = exitCode;

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
    const int delimiterIndex = fileNameWithPath.lastIndexOf ( u'/' );
    if ( delimiterIndex < 0 || delimiterIndex == fileNameWithPath.size() - 1 )
    {
	// Paths ending in "/" (notably root) or paths without any "/"
	path_ret = QString();
	name_ret = fileNameWithPath;

	return;
    }

    path_ret = fileNameWithPath.left( qMax( 1, delimiterIndex ) ); // at least "/"
    name_ret = fileNameWithPath.mid( delimiterIndex + 1 ); // everything after the last "/"
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
