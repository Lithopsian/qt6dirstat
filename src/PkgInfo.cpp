/*
 *   File name: PkgInfo.cpp
 *   Summary:	Support classes for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include "PkgInfo.h"
#include "DirTree.h"
#include "FileInfo.h"
#include "FileInfoIterator.h"
#include "Logger.h"


using namespace QDirStat;

/*
FileInfo * PkgInfo::locate( const QString & path )
{
    QStringList components = path.split( "/", Qt::SkipEmptyParts );

    if ( isPkgUrl( path ) )
    {
        components.removeFirst();       // Remove the leading "Pkg:"

        if ( components.isEmpty() )
            return ( this == _tree->root() ) ? this : 0;

        const QString pkgName = components.takeFirst();

        if ( pkgName != _name )
        {
            logError() << "Path " << path << " does not belong to " << this << Qt::endl;
            return nullptr;
        }

        if ( components.isEmpty() )
            return this;
    }

    return locate( this, components );
}
*/
/*
FileInfo * PkgInfo::locate( DirInfo       * subtree,
                            const QString & pathComponent )
{
    if ( !subtree || pathComponent.isEmpty() )
        return nullptr;

    for ( FileInfoIterator it( subtree ); *it; ++it )
    {
        //logDebug() << "Checking " << (*it)->name() << " in " << subtree << " for " << pathComponent << Qt::endl;

        if ( (*it)->name() == pathComponent )
            return *it;
    }

    return nullptr;
}*/
/*
FileInfo * PkgInfo::locate( DirInfo *           subtree,
                            const QStringList & pathComponents )
{
    // logDebug() << "Locating /" << pathComponents.join( "/" ) << " in " << subtree << Qt::endl;

    if ( !subtree || pathComponents.isEmpty() )
        return nullptr;

    QStringList   components = pathComponents;
    const QString wanted     = components.takeFirst();

    for ( FileInfoIterator it( subtree ); *it; ++it )
    {
        // logDebug() << "Checking " << (*it)->name() << " in " << subtree << " for " << wanted << Qt::endl;

        if ( (*it)->name() == wanted )
        {
            if ( components.isEmpty() )
            {
                // logDebug() << "  Found " << *it << Qt::endl;
                return *it;
            }
            else
            {
                if ( !(*it)->isDirInfo() )
                    return nullptr;
                else
                    return locate( (*it)->toDirInfo(), components );
            }
        }
    }

    return nullptr;
}
*/
