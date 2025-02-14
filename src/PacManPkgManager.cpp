/*
 *   File name: PacManPkgManager.cpp
 *   Summary:   Simple package manager support for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QRegularExpression>

#include "PacManPkgManager.h"
#include "Logger.h"
#include "PkgQuery.h"
#include "SysUtil.h"


using namespace QDirStat;


namespace
{
    /**
     * Parse a package list as output by "/usr/bin/pacman -Qn".
     **/
    PkgInfoList parsePkgList( const PkgManager * pkgManager, const QString & output )
    {
        PkgInfoList pkgList;

        const QStringList splitOutput = output.split( u'\n' );
        for ( const QString & line : splitOutput )
        {
            if ( !line.isEmpty() )
            {
                const QStringList fields = line.split( u' ' );

                if ( fields.size() == 2 )
                {
                    const QString & name    = fields.at( 0 );
                    const QString & version = fields.at( 1 );
                    pkgList << new PkgInfo{ name, version, "", pkgManager };
                }
                else
                {
                    logError() << "Invalid pacman -Qn output: \"" << line << '\n' << Qt::endl;
                }
            }
        }

        return pkgList;
    }

}


QString PacManPkgManager::owningPkg( const QString & path ) const
{
    int exitCode;
    QString output = SysUtil::runCommand( pacmanCommand(),
                                          { "-Qo", path },
                                          &exitCode,
                                          PkgQuery::owningPkgTimeoutSecs() );
    if ( exitCode != 0 || output.contains( "No package owns"_L1 ) )
        return QString{};

    // Sample output:
    //
    //   /usr/bin/pacman is owned by pacman 5.1.1-3
    //
    // The path might contain blanks, so it might not be safe to just use
    // blank-separated section #4; let's remove the part before the package
    // name.

    output.remove( QRegularExpression{ "^.*is owned by " } );
    const auto firstSpaceIndex = output.indexOf( u' ' );
    const QString pkg = firstSpaceIndex < 0 ? output : output.left( firstSpaceIndex );

    return pkg;
}


PkgInfoList PacManPkgManager::installedPkg() const
{
    int exitCode;
    const QString output = SysUtil::runCommand( pacmanCommand(), { "-Qn" }, &exitCode );
    return exitCode == 0 ? parsePkgList( this, output ) : PkgInfoList{};
}
