/*
 *   File name: Logger.h
 *   Summary:   Logger class for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef Logger_h
#define Logger_h

#include <cerrno>

#include <QFile>
#include <QRectF>
#include <QSizeF>
#include <QStringList>
#include <QTextStream>

#include "Typedefs.h" // Qt::endl, _L1


// Intentionally not using LogDebug, LogMilestone etc. to avoid confusion
// because of simple typos: logDebug() vs. LogDebug()
//
// ...well knowing that C++ compilers will throw all kinds of crazy and
// impossible to understand error messages if somebody confuses very different
// types like a function (logDebug()) vs. an enum value.

enum LogSeverity
{
    LogSeverityVerbose,
    LogSeverityDebug,
    LogSeverityInfo,
    LogSeverityWarning,
    LogSeverityError
};


// Log macros for stream (QTextStream) output.
//
// Unlike qDebug() etc., they also record the location in the source code that
// wrote the log entry.
//
// These macros all use the default logger. Create similar macros to use your
// own class-specific logger.

#define logVerbose() Logger::log( 0, __FILE__, __LINE__, __func__, LogSeverityVerbose )
#define logDebug()   Logger::log( 0, __FILE__, __LINE__, __func__, LogSeverityDebug   )
#define logInfo()    Logger::log( 0, __FILE__, __LINE__, __func__, LogSeverityInfo    )
#define logWarning() Logger::log( 0, __FILE__, __LINE__, __func__, LogSeverityWarning )
#define logError()   Logger::log( 0, __FILE__, __LINE__, __func__, LogSeverityError   )
#define logNewline() Logger::newline( 0 )


/**
 * Logging class. Use one of the macros above for stream output:
 *
 *     logDebug() << "Debug logging demo " << myString << ": " << 42 << Qt::endl;
 *     logError() << "Can't open file " << filename << ": " << errno << Qt::endl;
 *
 * Remember to terminate each log line with 'endl'.
 * Unlike qDebug() etc., this class does NOT add spaces or quotes.
 * There are overloads for most common types (QString, const char *,
 * QByteArray, int).
 *
 * This class also redirects Qt logging (qDebug() etc.) to the same log file.
 */
class Logger final
{
public:

    /**
     * Constructor: Create a logger that logs to the specified file.
     * The first logger created is also implicitly used as the default
     * logger. This can be changed later with setDefaultLogger().
     *
     * Not used.
     */
//    Logger( const QString & filename );

    /**
     * Constructor: create a logger that logs to 'filename' in directory
     * 'logDir'. $USER and $UID are expanded in both to the login user name or
     * the numeric user ID, respectively.
     *
     * If 'doRotate' is 'true, rotate any old logs in that directory
     * before opening the log and keep a maximum of 'logRotateCount' old logs
     * in that directory.
     *
     * The first logger created is also implicitly used as the default
     * logger. This can be changed later with setDefaultLogger().
     **/
    Logger( const QString & logDir,
            const QString & filename,
            bool            doRotate = true,
            int             logRotateCount = 3 );

    /**
     * Destructor.
     */
    ~Logger();

    /**
     * Suppress copy and assignment constructors (wouldn't work)
     **/
    Logger( const Logger & ) = delete;
    Logger & operator=( const Logger & ) = delete;

    /**
     * Static version of the internal logging function. Use the
     * logDebug(), logWarning() etc. macros instead of calling
     * this directly.
     *
     * If 'logger' is 0, the default logger is used.
     */
    static QTextStream & log( Logger        * logger,
                              const QString & srcFile,
                              int             srcLine,
                              const QString & srcFunction,
                              LogSeverity     severity );

    /**
     * Log a plain newline without any prefix (timestamp, source file name,
     * line number).
     */
    static void newline( Logger * logger );


protected:

    /**
     * Actually open the log file.
     **/
    void openLogFile( const QString & filename );

    /**
     * Return the default logger.  This is the first logger to be created,
     * or the first to be created after the previous default logger was
     * closed.
     */
    static Logger * defaultLogger() { return _defaultLogger; }

    /**
     * Return the QTextStream associated with this logger.
     */
    QTextStream & logStream() { return _logStream; }

    /**
     * Return the current log level, i.e. the severity that will actually be
     * logged. Any lower severity will be suppressed.
     *
     * Due to the way C++ evaluates expressions, the runtime cost will not
     * change significantly, only the log file size:
     *
     *	   logDebug() << "Result: " << myObj->result() << Qt::endl;
     *
     * Even if the log level is higher than logDebug(), this will still call
     * myObj->result() and its operator<<(). If you want to avoid that, use
     * your own 'if' around the log output:
     *
     * if ( logLevel() >= LogSeverityDebug )
     *	   logDebug() ...
     */
    LogSeverity logLevel() const { return _logLevel; }

    /**
     * Set the log level.
     */
//    void setLogLevel( LogSeverity newLevel ) { _logLevel = newLevel; }

    /**
     * Return the log level of the specified logger.
     *
     * If 'logger' is 0, the default logger is used.
     */
//    static LogSeverity logLevel( Logger * logger );

    /**
     * Set the log level of the specified logger.
     *
     * If 'logger' is 0, the default logger is used.
     */
//    static void setLogLevel( Logger * logger, LogSeverity newLevel );

    /**
     * Internal logging function.  Use the static function or the
     * logDebug(), etc. macros.
     */
    QTextStream & log( const QString & srcFile,
                       int             srcLine,
                       const QString & srcFunction,
                       LogSeverity     severity );

    /**
     * Log a plain newline without any prefix (timestamp, source filename,
     * line number).  This is the internal version; use the static
     * version instead.
     */
    void newline() { _logStream << Qt::endl; }


private:

    static Logger * _defaultLogger;

    QFile       _logFile;
    QTextStream _logStream{ stderr, QIODevice::WriteOnly };
    QFile       _nullDevice;
    QTextStream _nullStream{ stderr, QIODevice::WriteOnly };
    LogSeverity _logLevel{ LogSeverityVerbose };

}; // class Logger



inline QTextStream & operator<<( QTextStream & str, bool val )
    { return str << ( val ? "true" : "false" ); }

inline QTextStream & operator<<( QTextStream & str, const QStringList & stringList )
    { return str << stringList.join( ", "_L1 ); }

inline QTextStream & operator<<( QTextStream & str, const QRectF & rect )
    { return str << "QRectF( x: " << rect.x() << " y: " << rect.y()
                 << " width: " << rect.width() << " height: " << rect.height() << " )"; }

inline QTextStream & operator<<( QTextStream & str, QPointF point )
    { return str << "QPointF( x: " << point.x() << " y: " << point.y() << " )"; }

inline QTextStream & operator<<( QTextStream & str, QSizeF size )
    { return str << "QSizeF( width: " << size.width() << " height: " << size.height() << " )"; }

inline QTextStream & operator<<( QTextStream & str, QSize size )
    { return str << "QSize( width: " << size.width() << " height: " << size.height() << " )"; }


/**
 * Format errno as text.  In Qt5, const char * is treated as Latin1 in,
 * for example, QTextstream::operator<<, so convert it to QString.  In
 * Qt6, QTextStream treats const char * as UTF-8, so this can just
 * return the plain const char * text.
 **/
#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
    inline QString formatErrno() { return QString::fromUtf8( strerror( errno ) ); }
#else
    inline const char * formatErrno() { return strerror( errno ); }
#endif

#ifndef DONT_DEPRECATE_STRERROR
    // Use formatErrno() instead which deals with UTF-8 issues
    char * strerror( int ) __attribute__ ((deprecated));
#endif

#endif // Logger_h
