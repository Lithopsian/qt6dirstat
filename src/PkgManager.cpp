/*
 *   File name: PkgManager.cpp
 *   Summary:   Package manager support for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QRegularExpression>

#include "PkgManager.h"
#include "PkgQuery.h"
#include "Exception.h"
#include "MainWindow.h"
#include "QDirStatApp.h"
#include "SysUtil.h"


using namespace QDirStat;


bool PkgManager::check()
{
    const auto finished = [ this ]( int exitCode, QProcess::ExitStatus exitStatus )
    {
        const auto isPrimaryPkgManager = [ this ]( int exitCode )
        {
            if ( exitCode != 0 || !supportsFileListCache() )
                return false;

            return QRegularExpression{ isPrimaryRegExp() }.match( _process->readAll() ).hasMatch();
        };

        if ( exitStatus != QProcess::NormalExit )
        {
            logWarning() << "Check primary package manager command crashed" << Qt::endl;
            return;
        }

        if ( isPrimaryPkgManager( exitCode ) )
        {
            logInfo() << name() << " is the primary package manager" << Qt::endl;
            PkgQuery::setPrimaryPkgManager( this );
        }

        delete _process;
        _process = nullptr;
    };

    PkgCommand command = isPrimaryCommand();
    if ( !SysUtil::haveCommand( command.program ) )
        return false;

    logInfo() << "... found package manager " << name() << Qt::endl;

    if ( supportsGetInstalledPkg() && supportsFileList() )
        app()->mainWindow()->enableOpenPkg();

    if ( supportsFileListCache() )
        app()->mainWindow()->enableOpenUnpkg();

    _process = SysUtil::commandProcess( command.program, command.args );
    QObject::connect( _process, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ), finished );
    _process->start();

    return true;
}
