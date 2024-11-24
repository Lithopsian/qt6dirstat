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

    const PkgCommand pkgCommand = fileListCommand( pkg );
    if ( !pkgCommand.isEmpty() )
    {
        int exitCode;
        const QString output = SysUtil::runCommand( pkgCommand.command, pkgCommand.args, &exitCode );
        if ( exitCode == 0 )
            return parseFileList( output );
    }

    return QStringList{};
}

