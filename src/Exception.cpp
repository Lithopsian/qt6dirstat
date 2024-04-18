/*
 *   File name: Exception.cpp
 *   Summary:   Exception classes for QDirstat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <errno.h>

#include <QObject>

#include "Exception.h"


void Exception::setSrcLocation( const QString & srcFile,
				int             srcLine,
				const QString & srcFunction ) const
{
    // This is why those member variables are 'mutable':
    // We need to be able to set the source location from RETHROW even after
    // the exception was caught as const reference.
    //
    // This is not 100% elegant, but it keeps in line with usual conventions -
    // conventions like "catch exception objects as const reference".

    _srcFile	 = srcFile;
    _srcLine	 = srcLine;
    _srcFunction = srcFunction;
}


QString SysCallFailedException::errMsg( const QString & sysCall,
					const QString & resourceName ) const
{
    QString msg = QObject::tr( "%1( \"%2\" ) failed" ).arg( sysCall ).arg( resourceName );

    if ( errno != 0 )
	msg += ": " + formatErrno();

    return msg;
}


QString IndexOutOfRangeException::errMsg( int             invalidIndex,
                                          int             validMin,
                                          int             validMax,
                                          const QString & prefix ) const
{
    const QString msg = prefix.isEmpty() ? "Index out of range" : prefix;

    return msg + QString( ": %1 valid: %2...%3" ).arg( invalidIndex ).arg( validMin ).arg( validMax );
}
