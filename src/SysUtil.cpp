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


namespace
{
    /**
     * Return a QProcessEnvironment object with the C locale added.  This
     * is necessary when running some commands to avoid problems trying
     * to parse localised output.
     **/
    QProcessEnvironment cProcessEnvironment()
    {
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	env.insert( "LANG", "C" ); // Prevent output in translated languages
	return env;
    }

}


QProcess * SysUtil::commandProcess( const QString & program, const QStringList & args )
{

    QProcess * process = new QProcess;

    process->setProgram( program );
    process->setArguments( args );
    process->setProcessEnvironment( cProcessEnvironment() );
    process->setProcessChannelMode( QProcess::MergedChannels ); // combine stdout and stderr

    return process;
}


QString SysUtil::runCommand( const QString     & program,
                             const QStringList & args,
                             int               * exitCode_ret,
                             int                 timeoutSec,
                             bool                logCommand,
                             bool                logOutput,
                             bool                logError )
{
    int exitCode = -1;

    if ( logCommand )
	logDebug() << program << " " << args.join( u' ' ) << Qt::endl;

    const QString output = [ &program, &args, timeoutSec, logError, &exitCode ]() -> QString
    {
	if ( !haveCommand( program ) )
	    return "ERROR: Command not found\n\n";

	const auto logErrorMsg = [ &program, &args ]( const QString msg )
	    { logError() << msg << ": \"" << program << "\" args: " << args << Qt::endl; };

	QProcess * process = commandProcess( program, args );
	process->start();

	const bool finished = process->waitForFinished( timeoutSec * 1000 );
	const QString output{ process->readAll() };
	if ( finished )
	{
	    if ( process->exitStatus() == QProcess::NormalExit )
	    {
		exitCode = process->exitCode();
		if ( logError && exitCode != 0 )
		    logErrorMsg( "Command exited with exit code " % QString::number( exitCode ) );
	    }
	    else
	    {
		logErrorMsg( "Command crashed" );
	    }
	}
	else
	{
	    logErrorMsg( "Timeout or error" );
	}

	delete process;

	return output;
    }();

    if ( logOutput )
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
