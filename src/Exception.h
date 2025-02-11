/*
 *   File name: Exception.h
 *   Summary:   Exception classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef Exception_h
#define Exception_h

#include "Logger.h"


/**
 * Generic exception class
 *
 * Unlike std::exception, this class uses QString rather than const char *
 */
class Exception
{
public:

    /**
     * Constructor.
     */
    Exception( const QString & msg ):
	_what{ msg }
    {}

    /**
     * Destructor.
     */
    virtual ~Exception() noexcept = default;

    /**
     * Return a text description of what was wrong.
     *
     * This text is intended for developers or admins, not for end users.
     */
    const QString & what() const { return _what; }

    /**
     * Return the class name of this exception as string.  This is used
     * in the log message.
     */
    QLatin1String className() const { return "Exception"_L1; }


private:

    QString _what;

}; // class Exception


/**
 * Exception class for null pointers.
 * Use with CHECK_PTR().
 */
class NullPointerException final : public Exception
{
public:

    NullPointerException():
	Exception{ "Null pointer" }
    {}

    ~NullPointerException() noexcept override = default;

}; // class NullPointerException


/**
 * Exception class for file handling exception
 **/
class FileException final : public Exception
{
public:

    FileException( const QString & filename, const QString & msg ):
	Exception{ msg },
	_filename{ filename }
    {}

    ~FileException() noexcept override = default;


private:

    QString _filename;

}; // class FileException


/**
 * Exception class for system call failed
 **/
class SysCallFailedException final : public Exception
{
public:

    SysCallFailedException( const QString & sysCall,
                            const QString & resourceName ):
	Exception{ errMsg( sysCall, resourceName ) },
	_resourceName{ resourceName }
    {}

    ~SysCallFailedException() noexcept override = default;

    /**
     * Return the resource for which this syscall failed. This is typically a
     * file name.
     **/
    const QString & resourceName() const { return _resourceName; }


protected:

    QString errMsg( const QString & sysCall, const QString & resourceName ) const;


private:

    QString _resourceName;

}; // class SysCallFailedException


/**
 * Exception class for "index out of range"
 * Use with CHECK_DYNAMIC_CAST().
 **/
class DynamicCastException final : public Exception
{
public:

    DynamicCastException( const QString & expectedType ):
	Exception{ "dynamic_cast failed; expected: " + expectedType }
    {}

    ~DynamicCastException() noexcept override = default;

}; // class DynamicCastException


/**
 * Exception class for magic number check failed
 * Use with CHECK_MAGIC().
 **/
class BadMagicNumberException final : public Exception
{
public:

    BadMagicNumberException( const void * badPointer ):
	Exception{ QString{ "Magic number check failed for address 0x%1" }
	           .arg( reinterpret_cast<quintptr>( badPointer ), 0, 16 ) }
    {}

    ~BadMagicNumberException() noexcept override = default;

}; // class BadMagicNumberException


/**
 * Exception class for "index out of range"
 * Use with CHECK_INDEX()
 **/
class IndexOutOfRangeException final : public Exception
{
public:

    /**
     * Constructor.
     *
     * 'invalidIndex' is the offending index value. It should be between
     *'validMin' and 'validMax':
     *
     *	   validMin <= index <= validMax
     **/
    IndexOutOfRangeException( int             invalidIndex,
                              int             validMin,
                              int             validMax,
                              const QString & msg ):
	Exception{ QString{ "%1: %2 valid: %3...%4" }
	           .arg( msg )
	           .arg( invalidIndex )
	           .arg( validMin )
	           .arg( validMax ) }
    {}

    ~IndexOutOfRangeException() noexcept override = default;

}; // class IndexOutOfRangeException



/**
 * Exception class for "too many files"
 **/
class TooManyFilesException final : public Exception
{
public:

    /**
     * Constructor.
     *
     * 'invalidIndex' is the offending index value. It should be between
     *'validMin' and 'validMax':
     *
     *	   validMin <= index <= validMax
     **/
    TooManyFilesException():
	Exception{ QString{ "more than %1 files" }.arg( FileCountMax ) }
    {}

    ~TooManyFilesException() noexcept override = default;

}; // class TooManyFilesException


/**
 * Exception class for "filesystem too big"
 **/
class FilesystemTooBigException final : public Exception
{
public:

    /**
     * Constructor.
     *
     * 'invalidIndex' is the offending index value. It should be between
     *'validMin' and 'validMax':
     *
     *	   validMin <= index <= validMax
     **/
    FilesystemTooBigException():
	Exception{ errMsg() }
    {}

    ~FilesystemTooBigException() noexcept override = default;


protected:

    QString errMsg() const;

}; // class FilesystemTooBigException




//
// Helper macros
//

/**
 * Throw an exception and write it to the log, together with the source code
 * location where it was thrown. This makes it MUCH easier to find out where
 * a problem occurred.
 *
 * Use this as a substitute for normal 'throw( exception )'.
 */
#define THROW( EXCEPTION ) \
    _throw_helper( ( EXCEPTION ), 0, __FILE__, __LINE__, __func__ )

/**
 * Write a log notification that an exception has been caught.
 * This is not a substitute for a 'catch' statement. Rather, use it inside
 * a 'catch' block.
 *
 * Example:
 *
 *     try
 *     {
 *	   ...do something...
 *	   THROW( Exception( "Catastrophic failure" ) );
 *	   ...
 *     }
 *     catch( const Exception & exception )
 *     {
 *	   CAUGHT( exception );
 *	   ...clean up to prevent memory leaks etc. ...
 *	   RETHROW( exception ); // equivalent of THROW without args
 *     }
 *
 * This will leave 3 lines for that exception in the log file: one for
 * THROW, one for CAUGHT, one for RETHROW. Each log line contains the
 * source file, the line number, and the function of the THROW or CAUGHT
 * or RETHROW calls.
 */
#define CAUGHT( EXCEPTION ) \
    _caught_helper( ( EXCEPTION ), 0, __FILE__, __LINE__, __func__ )

/**
 * Write a log notification that an exception that has been caught is
 * being thrown again inside a 'catch' block. Use this as a substitute for
 * plain 'throw' without arguments. Unlike a plain 'throw', this macro does
 * have an argument.
 */
#define RETHROW( EXCEPTION ) \
    _rethrow_helper( ( EXCEPTION ), 0, __FILE__, __LINE__, __func__ )



/**
 * Check a pointer and throw an exception if it returned 0.
 */
#define CHECK_PTR( PTR )			\
    do						\
    {						\
	if ( !(PTR) )				\
	{					\
	    THROW( NullPointerException{} );	\
	}					\
    } while( 0 )


/**
 * Check the result of a dynamic_cast and throw an exception if it returned 0.
 */
#define CHECK_DYNAMIC_CAST( PTR, EXPECTED_TYPE )		\
    do								\
    {								\
	if ( !(PTR) )						\
	{							\
	    THROW( DynamicCastException{ EXPECTED_TYPE } );	\
	}							\
    } while( 0 )


/**
 * Check the magic number of an object and throw an exception if it returned false.
 */
#define CHECK_MAGIC( PTR )					\
    do								\
    {								\
	if ( !(PTR) )						\
	{							\
	    THROW( NullPointerException{} );			\
	}							\
								\
	if ( !PTR->checkMagicNumber() )				\
	{							\
	    THROW( BadMagicNumberException{ PTR } );		\
	}							\
    } while( 0 )


/**
 * Check if an index is in range:
 * VALID_MIN <= INDEX <= VALID_MAX
 *
 * Throws IndexOutOfRangeException if out of range.
 **/
#define CHECK_INDEX( INDEX, VALID_MIN, VALID_MAX, MSG )						\
    do												\
    {												\
	if ( (INDEX) < (VALID_MIN) || (INDEX) > (VALID_MAX) )					\
	{											\
	    THROW( ( IndexOutOfRangeException{ (INDEX), (VALID_MIN), (VALID_MAX), (MSG) } ) ); 	\
	}											\
    } while( 0 )


//
// Helper functions. Do not use directly; use the corresponding macros instead.
//

template<class EX_t>
void _throw_helper( const EX_t    & exception,
                    Logger        * logger,
                    const QString & srcFile,
                    int             srcLine,
                    const QString & srcFunction )
{
    Logger::log( logger, srcFile, srcLine, srcFunction, LogSeverityWarning )
	<< "THROW " << exception.className() << ": " << exception.what() << Qt::endl;

    throw( exception );
}


template<class EX_t>
void _caught_helper( const EX_t    & exception,
                     Logger        * logger,
                     const QString & srcFile,
                     int             srcLine,
                     const QString & srcFunction )
{
    Logger::log( logger, srcFile, srcLine, srcFunction, LogSeverityWarning )
	<< "CAUGHT " << exception.className() << ": " << exception.what() << Qt::endl;
}


template<class EX_t>
void _rethrow_helper( const EX_t    & exception,
                      Logger        * logger,
                      const QString & srcFile,
                      int             srcLine,
                      const QString & srcFunction )
{
    Logger::log( logger, srcFile, srcLine, srcFunction, LogSeverityWarning )
	<< "RETHROW " << exception.className() << ": " << exception.what() << Qt::endl;

    throw;
}

#endif // Exception_h
