/*
 *   File name: PacManPkgManager.cpp
 *   Summary:   Simple package manager support for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "PacManPkgManager.h"
#include "Exception.h"
#include "Logger.h"
#include "SysUtil.h"


using namespace QDirStat;

using SysUtil::runCommand;
using SysUtil::tryRunCommand;
using SysUtil::haveCommand;


bool PacManPkgManager::isPrimaryPkgManager() const
{
    return tryRunCommand( "/usr/bin/pacman -Qo /usr/bin/pacman", ".*is owned by pacman.*" );
}


bool PacManPkgManager::isAvailable() const
{
    return haveCommand( "/usr/bin/pacman" );
}


QString PacManPkgManager::owningPkg( const QString & path ) const
{
    int exitCode = -1;
    QString output = runCommand( "/usr/bin/pacman",
                                 { "-Qo", path },
                                 &exitCode );

    if ( exitCode != 0 || output.contains( QLatin1String( "No package owns" ) ) )
	return "";

    // Sample output:
    //
    //   /usr/bin/pacman is owned by pacman 5.1.1-3
    //
    // The path might contain blanks, so it might not be safe to just use
    // blank-separated section #4; let's remove the part before the package
    // name.

    output.remove( QRegularExpression( "^.*is owned by " ) );
    const QString pkg = output.section( ' ', 0, 0 );

    return pkg;
}


PkgInfoList PacManPkgManager::installedPkg() const
{
    int exitCode = -1;
    const QString output = runCommand( "/usr/bin/pacman",
                                       { "-Qn" },
                                       &exitCode );

    PkgInfoList pkgList;

    if ( exitCode == 0 )
        pkgList = parsePkgList( output );

    return pkgList;
}


PkgInfoList PacManPkgManager::parsePkgList( const QString & output ) const
{
    PkgInfoList pkgList;

    const QStringList splitOutput = output.split( '\n' );
    for ( const QString & line : splitOutput )
    {
        if ( ! line.isEmpty() )
        {
            QStringList fields = line.split( ' ' );

            if ( fields.size() != 2 )
                logError() << "Invalid pacman -Qn output: \"" << line << "\n" << Qt::endl;
            else
            {
                const QString name    = fields.takeFirst();
                const QString version = fields.takeFirst();
                const QString arch    = "";

                PkgInfo * pkg = new PkgInfo( name, version, arch, this );
                CHECK_NEW( pkg );

                pkgList << pkg;
            }
        }
    }

    return pkgList;
}
