/*
 *   File name: PkgManager.cpp
 *   Summary:	Package manager support for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include "PkgManager.h"
#include "SysUtil.h"
#include "Logger.h"
#include "Exception.h"


using namespace QDirStat;


QStringList PkgManager::fileList( const PkgInfo * pkg ) const
{
    const QString command = fileListCommand( pkg );

    if ( ! command.isEmpty() )
    {
        int exitCode = -1;
        const QString output = SysUtil::runCommand( command,
                                                    &exitCode,
                                                    true,          // logCommand
                                                    false );       // logOutput
        if ( exitCode == 0 )
            return parseFileList( output );
    }

    return QStringList();
}

