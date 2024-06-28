/*
 *   File name: PkgManager.cpp
 *   Summary:   Package manager support for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "PkgManager.h"
#include "Exception.h"
#include "SysUtil.h"


using namespace QDirStat;


QStringList PkgManager::fileList( const PkgInfo * pkg ) const
{
    CHECK_PTR( pkg );

    const QString command = fileListCommand( pkg );
    if ( !command.isEmpty() )
    {
        int exitCode = -1;
        const QString output = SysUtil::runCommand( command, &exitCode );
        if ( exitCode == 0 )
            return parseFileList( output );
    }

    return QStringList();
}

