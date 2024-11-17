/*
 *   File name: Logger.cpp
 *   Summary:   Logger class for QDirstat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

// We obviously need to use strerror
#define DONT_DEPRECATE_STRERROR

#include <cstdio>	// stderr, fprintf()
#include <cstdlib>	// abort(), mkdtemp()
#include <pwd.h>	// getpwuid()
#include <sys/types.h>	// pid_t, getpwuid()
#include <unistd.h>	// getpid()

#include <QDateTime>
#include <QDir>

#include "Logger.h"
#include "SysUtil.h"


#define VERBOSE_ROTATE 0


namespace
{
    LogSeverity toLogSeverity( QtMsgType msgType )
    {
	switch ( msgType )
	{
	    case QtDebugMsg:    return LogSeverityVerbose;
	    case QtWarningMsg:  return LogSeverityWarning;
	    case QtCriticalMsg: return LogSeverityError;
	    case QtFatalMsg:    return LogSeverityError;
#if QT_VERSION >= 0x050500
	    case QtInfoMsg:     return LogSeverityInfo;
#endif
	}

	return LogSeverityVerbose;
    }


    void qt_logger( QtMsgType msgType, const QMessageLogContext & context, const QString & msg )
    {
	const QStringList lines = msg.split( u'\n' );
	for ( QString line : lines )
	{
	    // Remove utterly misleading message
	    line.remove( "Reinstalling the application may fix this problem."_L1 );

	    if ( !line.trimmed().isEmpty() )
	    {
		Logger::log( 0, context.file, context.line, context.function, toLogSeverity( msgType ) )
		    << "[Qt] " << line << Qt::endl;
	    }
	}

	if ( msgType == QtFatalMsg )
	{
	    if ( msg.contains( "Could not connect to display"_L1 ) ||
		 msg.contains( "failed to start because no Qt platform plugin"_L1 ) )
	    {
		if ( msg.contains( "Reinstalling the application may fix this problem"_L1 ) )
		{
		    // Suppress this new message:
		    //
		    // "This application failed to start because no Qt platform
		    // plugin could be initialized. Reinstalling the application
		    // may fix this problem.
		    //
		    // Available platform plugins are: ... "
		    //
		    // Even a simple "I don't know what the problem is" is more
		    // helpful than this.
		    //
		    const char * text = "FATAL: Could not connect to the display.";
		    fprintf( stderr, "\n%s\n", text );
		    logError() << text << Qt::endl;
		}
		else
		{
		    fprintf( stderr, "FATAL: %s\n", qPrintable( msg ) );
		}

		logInfo() << "-- Exiting --\n" << Qt::endl;
		exit( 1 ); // Don't dump core, just exit
	    }
	    else
	    {
		fprintf( stderr, "FATAL: %s\n", qPrintable( msg ) );
		logInfo() << "-- Aborting with core dump --\n" << Qt::endl;
		abort(); // Exit with core dump (it might contain a useful backtrace)
	    }
	}
    }


    /**
     * Return a timestamp string in the format used in the log file:
     * "yyyy-MM-dd hh:mm:ss.zzz"
     */
    QString timeStamp()
    {
	return QDateTime::currentDateTime().toString( "yyyy-MM-dd hh:mm:ss.zzz" );
    }


    /**
     * Return the user name (the login name) of the user owning this process.
     * If that information cannot be obtained, this returns the UID as a string.
     **/
    QString userName()
    {
	return QDirStat::SysUtil::userName( getuid() );
    }


    /**
     * Create log directory 'logDir' and return the name of the directory
     * actually used. That might be different from the requested name if the
     * directory already exists and is not owned by the current user.
     **/
    QString createLogDir( const QString & rawLogDir )
    {
	QString logDir{ rawLogDir };
	const QDir rootDir{ "/" };
	bool created = false;

	if ( !rootDir.exists( logDir ) )
	{
	    rootDir.mkpath( logDir );
	    created = true;
	}

	const QFileInfo dirInfo{ logDir };
	if ( dirInfo.ownerId() != getuid() )
	{
	    logError() << "ERROR: Directory " << logDir << " is not owned by " << userName() << Qt::endl;

	    QByteArray nameTemplate{ QString{ logDir % "-XXXXXX"_L1 }.toUtf8() };
	    const char * result = mkdtemp( nameTemplate.data() );
	    if ( result )
	    {
		created = true;
		logDir = QString::fromUtf8( result );
	    }
	    else
	    {
		logError() << "Could not create log dir " << nameTemplate
		           << ": " << formatErrno() << Qt::endl;

		logDir = "/";
		// No permissions to write to /,
		// i.e. the log will go to stderr instead
	    }
	}

	if ( created )
	{
	    QFile dir{ logDir };
	    dir.setPermissions( QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner );
	}

	return logDir;
    }


    /**
     * Return the portion of 'filename' excluding the ".log" suffix.
     **/
    QString logNameStem( const QString & filename )
    {
	return filename.endsWith( ".log" ) ? filename.left( filename.size() - 4 ) : filename;
    }


    /**
     * Return the name for an old log file based on 'stem', the portion
     * of the log filename before the ".log", for old log number 'no'.
     **/
    QString oldName( const QString & stem, int no )
    {
	return QString{ "%1-%2.old" }.arg( stem ).arg( no, 2, 10, QChar{ u'0' } );
    }


    /**
     * Return the glob pattern for old log files based on 'stem', the
     * portion of the log filename before the ".log". This pattern can be
     * used for QDir matches.
     **/
    QString oldNamePattern( const QString & stem )
    {
	return stem % "-??.old"_L1;
    }


    /**
     * Rotate the logs in directory 'logDir' based on future log file
     * 'filename' (without path). Keep at most 'logRotateCount' old logs and
     * delete all other old logs.
     **/
    void logRotate( const QString & logDir, const QString & filename, int logRotateCount )
    {
	const QString logStem = logNameStem( filename );
	QDir dir{ logDir };
	QStringList keepers{ filename };

	for ( int i = logRotateCount - 1; i >= 0; --i )
	{
	    const QString newName = oldName( logStem, i );
	    if ( dir.exists( newName ) )
	    {
		const bool success = dir.remove( newName );
		Q_UNUSED( success );
#if VERBOSE_ROTATE
		logDebug() << "Removing " << newName << ( success ? "" : " FAILED" ) << Qt::endl;
#endif
	    }

	    const QString currentName = i > 0 ? oldName( logStem, i-1 ) : filename;
	    if ( dir.exists( currentName ) )
	    {
		keepers << newName;

		const bool success = dir.rename( currentName, newName );
		Q_UNUSED( success );
#if VERBOSE_ROTATE
		logDebug() << "Renaming " << currentName << " to " << newName
		           << ( success ? "" : " FAILED" ) << Qt::endl;
#endif
	    }
	}

	const auto matches = dir.entryList( QStringList{ oldNamePattern( logStem ) }, QDir::Files );
	for ( const QString & match : matches )
	{
	    if ( !keepers.contains( match ) )
	    {
		const bool success = dir.remove( match );
		Q_UNUSED( success );
#if VERBOSE_ROTATE
		logDebug() << "Removing leftover " << match << ( success ? "" : " FAILED" ) << Qt::endl;
#endif
	    }
	}
    }


    /**
     * Expand variables in 'unexpanded' and return the expanded string:
     *
     *   $USER  the login user name of the current user
     *   $UID   the numeric user ID of the current user
     **/
    QString expandVariables( QString unexpanded )
    {
	unexpanded.replace( "$USER"_L1, userName() );
	unexpanded.replace( "$UID"_L1 , QString::number( getuid() ) );

	return unexpanded;
    }

} // namespace


Logger * Logger::_defaultLogger = nullptr;


Logger::Logger( const QString & filename )
{
    init();
    openLogFile( filename );
}


Logger::Logger( const QString & rawLogDir,
                const QString & rawFilename,
                bool            doRotate,
                int             logRotateCount )
{
    init();

    const QString filename = expandVariables( rawFilename );
    QString logDir = expandVariables( rawLogDir );
    logDir = createLogDir( logDir );

    if ( doRotate )
	logRotate( logDir, filename, logRotateCount );

    openLogFile( logDir % '/' % filename );
}


Logger::~Logger()
{
    if ( _logFile.isOpen() )
    {
	logInfo() << "-- Log End --\n" << Qt::endl;
	_logFile.close();
    }

    if ( this == _defaultLogger )
    {
	_defaultLogger = nullptr;
	qInstallMessageHandler( nullptr ); // Restore default message handler
    }
}


void Logger::init()
{
    _nullDevice.setFileName( "/dev/null" );
    createNullStream();
}


void Logger::createNullStream()
{
    // Open the null device to suppress output below the log level: This is
    // necessary because each call to operator<<() for QTextStream returns the
    // QTextStream, so we really need to return _nullStream (connected with
    // /dev/null) to actually suppress anything; otherwise, it's just the
    // logger time stamp etc. that gets suppressed, not the real logging
    // output.

    if ( _nullDevice.open( QIODevice::WriteOnly | QIODevice::Text ) )
	_nullStream.setDevice( &_nullDevice );
    else
	fprintf( stderr, "ERROR: Can't open /dev/null to suppress log output\n" );
}


void Logger::openLogFile( const QString & filename )
{
    if ( !_logFile.isOpen() || _logFile.fileName() != filename )
    {
	_logFile.setFileName( filename );

	if ( _logFile.open( QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append ) )
	{
	    if ( !_defaultLogger )
		setDefaultLogger();

	    fprintf( stderr, "Logging to %s\n", qPrintable( filename ) );
	    _logStream.setDevice( &_logFile );
	    _logStream << "\n\n";
	    log( __FILE__, __LINE__, __func__, LogSeverityInfo )
		<< "-- Log Start --" << Qt::endl;
	}
	else
	{
	    fprintf( stderr, "ERROR: Can't open log file %s\n", qPrintable( filename ) );
	}
    }
}


void Logger::setDefaultLogger()
{
    _defaultLogger = this;
    qInstallMessageHandler( qt_logger );
}


QTextStream & Logger::log( Logger        * logger,
                           const QString & srcFile,
                           int             srcLine,
                           const QString & srcFunction,
                           LogSeverity     severity )
{
    static QTextStream stderrStream{ stderr, QIODevice::WriteOnly };

    if ( !logger )
	logger = Logger::defaultLogger();

    if ( logger )
	return logger->log( srcFile, srcLine, srcFunction, severity );
    else
	return stderrStream;
}


QTextStream & Logger::log( const QString & srcFile,
                           int             srcLine,
                           const QString & srcFunction,
                           LogSeverity     severity )
{
    if ( severity < _logLevel )
	return _nullStream;

    const auto sev = [ severity ]()
    {
	switch ( severity )
	{
	    case LogSeverityVerbose: return "<Verbose>";
	    case LogSeverityDebug:   return "<Debug>  ";
	    case LogSeverityInfo:    return "<Info>   ";
	    case LogSeverityWarning: return "<WARNING>";
	    case LogSeverityError:   return "<ERROR>  ";
	    // Intentionally omitting 'default' branch so the compiler can
	    // complain about unhandled enum values
	}
	return "";
    }();

    _logStream << timeStamp() << ' ' << '[' << getpid() << "] " << sev << ' ';

    if ( !srcFile.isEmpty() )
    {
	_logStream << srcFile;

	if ( srcLine > 0 )
	    _logStream << ':' << srcLine;

	_logStream << ' ';

	if ( !srcFunction.isEmpty() )
	    _logStream << srcFunction << "():  ";
    }

    return _logStream;
}


void Logger::newline( Logger * logger )
{
    if ( !logger )
	logger = Logger::defaultLogger();

    if ( logger )
	logger->newline();
}


LogSeverity Logger::logLevel( Logger * logger )
{
    if ( !logger )
	logger = Logger::defaultLogger();

    if ( logger )
	return logger->logLevel();
    else
	return LogSeverityVerbose;
}


void Logger::setLogLevel( Logger * logger, LogSeverity newLevel )
{
    if ( !logger )
	logger = Logger::defaultLogger();

    if ( logger )
	logger->setLogLevel( newLevel );
}
