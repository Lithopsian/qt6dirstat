/*
 *   File name: Logger.cpp
 *   Summary:	Logger class for QDirstat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#define DONT_DEPRECATE_STRERROR
#include "Logger.h"

#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QString>
#include <QRectF>
#include <QPointF>
#include <QSizeF>
#include <QSize>
#include <QStringList>
#include <QRegularExpression>

#include <stdio.h>	// stderr, fprintf()
#include <stdlib.h>	// abort(), mkdtemp()
#include <unistd.h>	// getpid()
#include <errno.h>
#include <pwd.h>	// getpwuid()
#include <sys/types.h>	// pid_t, getpwuid()

#include "SysUtil.h"


#define VERBOSE_ROTATE 0


static LogSeverity toLogSeverity( QtMsgType msgType );

static void qt_logger( QtMsgType msgType,
		       const QMessageLogContext & context,
		       const QString & msg );


Logger * Logger::_defaultLogger = nullptr;


Logger::Logger( const QString &filename ):
    _logStream { stderr, QIODevice::WriteOnly },
    _nullStream { stderr, QIODevice::WriteOnly }
{
    init();
    createNullStream();
    openLogFile( filename );
}


Logger::Logger( const QString & rawLogDir,
		const QString & rawFilename,
		bool		doRotate,
		int		logRotateCount ):
    _logStream { stderr, QIODevice::WriteOnly },
    _nullStream { stderr, QIODevice::WriteOnly }
{
    init();
    createNullStream();

    const QString filename = expandVariables( rawFilename );
    QString logDir         = expandVariables( rawLogDir   );
    logDir = createLogDir( logDir );

    if ( doRotate )
	logRotate( logDir, filename, logRotateCount );

    openLogFile( logDir + "/" + filename );
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
	qInstallMessageHandler(0); // Restore default message handler
    }
}


void Logger::init()
{
    _logLevel = LogSeverityVerbose;
    _nullDevice.setFileName( "/dev/null" );
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
	    log( __FILE__, __LINE__, __FUNCTION__, LogSeverityInfo )
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


QTextStream & Logger::log( Logger *	  logger,
			   const QString &srcFile,
			   int		  srcLine,
			   const QString &srcFunction,
			   LogSeverity	  severity )
{
    static QTextStream stderrStream( stderr, QIODevice::WriteOnly );

    if ( !logger )
	logger = Logger::defaultLogger();

    if ( logger )
	return logger->log( srcFile, srcLine, srcFunction, severity );
    else
	return stderrStream;
}


QTextStream & Logger::log( const QString &srcFile,
			   int		  srcLine,
			   const QString &srcFunction,
			   LogSeverity	  severity )
{
    if ( severity < _logLevel )
	return _nullStream;

    QString sev;

    switch ( severity )
    {
	case LogSeverityVerbose:   sev = "<Verbose>"; break;
	case LogSeverityDebug:	   sev = "<Debug>  "; break;
	case LogSeverityInfo:	   sev = "<Info>   "; break;
	case LogSeverityWarning:   sev = "<WARNING>"; break;
	case LogSeverityError:	   sev = "<ERROR>  "; break;
	    // Intentionally omitting 'default' branch so the compiler can
	    // complain about unhandled enum values
    }

    _logStream << Logger::timeStamp() << " " << "[" << (int) getpid() << "] " << sev << " ";

    if ( !srcFile.isEmpty() )
    {
	_logStream << srcFile;

	if ( srcLine > 0 )
	    _logStream << ":" << srcLine;

	_logStream << " ";

	if ( !srcFunction.isEmpty() )
	_logStream << srcFunction << "():  ";
    }

    return _logStream;
}


void Logger::newline( Logger *logger )
{
    if ( !logger )
	logger = Logger::defaultLogger();

    if ( logger )
	logger->newline();
}


LogSeverity Logger::logLevel( Logger *logger )
{
    if ( !logger )
	logger = Logger::defaultLogger();

    if ( logger )
	return logger->logLevel();
    else
	return LogSeverityVerbose;
}


void Logger::setLogLevel( Logger *logger, LogSeverity newLevel )
{
    if ( !logger )
	logger = Logger::defaultLogger();

    if ( logger )
	logger->setLogLevel( newLevel );
}


QString Logger::timeStamp()
{
    return QDateTime::currentDateTime().toString( "yyyy-MM-dd hh:mm:ss.zzz" );
}


QString Logger::prefixLines( const QString &prefix,
			     const QString &multiLineText )
{
    const QStringList lines = multiLineText.split( "\n" );
    QString result = lines.isEmpty() ? QString() : prefix;
    result += lines.join( QString( "\n" ) + prefix );

    return result;
}


QString Logger::indentLines( int indentWidth,
			     const QString &multiLineText )
{
    const QString prefix( indentWidth, ' ' );

    return prefixLines( prefix, multiLineText );
}



static LogSeverity toLogSeverity( QtMsgType msgType )
{
    switch ( msgType )
    {
	case QtDebugMsg:    return LogSeverityVerbose;
	case QtWarningMsg:  return LogSeverityWarning;
	case QtCriticalMsg: return LogSeverityError;
	case QtFatalMsg:    return LogSeverityError;
#if QT_VERSION >= 0x050500
	case QtInfoMsg:	    return LogSeverityInfo;
#endif
    }

    return LogSeverityVerbose;
}


static void qt_logger( QtMsgType msgType,
		       const QMessageLogContext & context,
		       const QString & msg )
{
    const QStringList lines = msg.split("\n");
    for ( QString line : lines )
    {
        // Remove utterly misleading message that will just dump a ton of bug
        // reports on the application maintainers just because some clueless
        // moron put this message into the Qt libs

        line.remove( "Reinstalling the application may fix this problem." );

        if ( !line.trimmed().isEmpty() )
        {
            Logger::log( 0, // use default logger
                         context.file, context.line, context.function,
                         toLogSeverity( msgType ) )
                << "[Qt] " << line << Qt::endl;
        }
    }

    if ( msgType == QtFatalMsg )
    {
	if ( msg.contains( "Could not connect to display" ) ||
             msg.contains( "failed to start because no Qt platform plugin" ) )
        {
            if ( msg.contains( "Reinstalling the application may fix this problem" ) )
            {
                // Suppress this new message which is complete and utter garbage:
                //
                // "This application failed to start because no Qt platform
                // plugin could be initialized. Reinstalling the application
                // may fix this problem.
                //
                // Available platform plugins are: wayland-org.kde.kwin.qpa,
                // eglfs, linuxfb, minimal, minimalegl, offscreen, vnc,
                // wayland-egl, wayland, wayland-xcomposite-egl,
                // wayland-xcomposite-glx, xcb."
                //
                // Seriously, who comes up with bullshit like this?
                //
                // Anybody who thinks that this might help a user does not have
                // the first clue about user interaction. This only confuses
                // and frightens most users. Telling users to reinstall the
                // application (!) is no more than computer voodoo and cargo
                // cult. There is not even the slightest shred of evidence that
                // this might help, yet it is the most destructive approach.
                //
                // Even a simple "I don't know what the problem is" is more
                // helpful than this.
                //
                // Worse, the plethora of plug-ins and methods how to start a
                // Qt application and thus how to connect to a display only
                // multiplies the confusion. WTF is a normal user expected to
                // do? What user even knows which of those technologies his
                // system uses? And WTF should a user do with that
                // non-information about the various plug-ins? This is a
                // perfect example of completely overengineered bullshit.
                //
                // I with my 25+ years as a Unix/Linux/X11 software developer
                // have no clue what to do when they dump this message on
                // me. WTF do they expect from a normal user?
                //
                // Get it right or get out of this business. Seriously. The IT
                // world already has enough morons with enough useless messages
                // designed by a committee of clueless product managers and
                // marketing people.

                const QString text = "FATAL: Could not connect to the display.";
                fprintf( stderr, "\n%s\n", qPrintable( text ) );
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


QString Logger::userName()
{
    // Not using getuid() because that function relies on the user owning the
    // controlling terminal which is wrong in many aspects:
    //
    // - There might not be a controlling terminal at all.
    // - The user owning the controlling terminal may or may not be the one
    //	 starting this program.

    return QDirStat::SysUtil::userName( getuid() );
}


QString Logger::createLogDir( const QString & rawLogDir )
{
    QString logDir( rawLogDir );
    const QDir rootDir( "/" );
    bool created = false;

    if ( !rootDir.exists( logDir ) )
    {
	rootDir.mkpath( logDir );
	created = true;
    }

    const QFileInfo dirInfo( logDir );
    if ( (uid_t) dirInfo.ownerId() != getuid() )
    {
	logError() << "ERROR: Directory " << logDir << " is not owned by " << userName() << Qt::endl;

	QByteArray nameTemplate( QString( logDir + "-XXXXXX" ).toUtf8() );
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
	QFile dir( logDir );
	dir.setPermissions( QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner );
    }

    return logDir;
}


QString Logger::oldName( const QString & filename, int no )
{
    QString oldName = filename;
    oldName.remove( QRegularExpression( "\\.log$" ) );
    oldName += QString( "-%1.old" ).arg( no, 2, 10, QChar( '0' ) );

    return oldName;
}


QString Logger::oldNamePattern( const QString & filename )
{
    QString pattern = filename;
    pattern.remove( QRegularExpression( "\\.log$" ) );
    pattern += "-??.old";

    return pattern;
}


void Logger::logRotate( const QString & logDir,
			const QString & filename,
			int		logRotateCount )
{
    QDir dir( logDir );
    QStringList keepers;
    keepers << filename;

    for ( int i = logRotateCount - 1; i >= 0; --i )
    {
	const QString currentName = i > 0 ? oldName( filename, i-1 ) : filename;
	QString newName	    = oldName( filename, i );

	if ( dir.exists( newName ) )
	{
	    const bool success = dir.remove( newName );
	    Q_UNUSED( success );
#if VERBOSE_ROTATE
	    logDebug() << "Removing " << newName << ( success ? "" : " FAILED" ) << Qt::endl;
#endif
	}

	if ( dir.exists( currentName ) )
	{
	    const bool success = dir.rename( currentName, newName );
	    Q_UNUSED( success );
#if VERBOSE_ROTATE
	    logDebug() << "Renaming " << currentName << " to " << newName
		       << ( success ? "" : " FAILED" )
		       << Qt::endl;
#endif

	    keepers << newName;
	}
    }

    const QStringList matches = dir.entryList( QStringList() << oldNamePattern( filename ), QDir::Files );
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


QString Logger::expandVariables( const QString & unexpanded )
{
    QString expanded = unexpanded;
    expanded.replace( "$USER", userName() );
    expanded.replace( "$UID" , QString::number( getuid() ) );

    return expanded;
}




QTextStream & operator<<( QTextStream & str, const QRectF & rect )
{
    str << "QRectF( x: " << rect.x() << " y: " << rect.y()
        << " width: " << rect.width() << " height: " << rect.height() << " )";

    return str;
}


QTextStream & operator<<( QTextStream & str, const QPointF & point )
{
    str << "QPointF( x: " << point.x() << " y: " << point.y() << " )";

    return str;
}


QTextStream & operator<<( QTextStream & str, const QSizeF & size )
{
    str << "QSizeF( width: " << size.width() << " height: " << size.height() << " )";

    return str;
}


QTextStream & operator<<( QTextStream & str, const QSize & size )
{
    str << "QSize( width: " << size.width() << " height: " << size.height() << " )";

    return str;
}


// Un-deprecate strerror() just for this one call.
#ifdef strerror
#    undef strerror
#endif

QString formatErrno()
{
    return QString::fromUtf8( strerror( errno ) );
}