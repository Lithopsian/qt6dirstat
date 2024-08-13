/*
 *   File name: Exception.cpp
 *   Summary:   Exception classes for QDirstat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "Exception.h"
#include "FormatUtil.h" // formatSize


QString SysCallFailedException::errMsg( const QString & sysCall,
                                        const QString & resourceName ) const
{
    const QString msg = QString{ "%1( \"%2\" ) failed" }.arg( sysCall, resourceName );

    return errno == 0 ? msg : ( msg + ": "_L1 + formatErrno() );
}


QString FilesystemTooBigException::errMsg() const
{
    return "filesystem larger than " + QDirStat::formatSize( FileSizeMax );
}
