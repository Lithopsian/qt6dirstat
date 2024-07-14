/*
 *   File name: Attic.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QStringBuilder>

#include "Attic.h"
#include "FileInfoIterator.h"
#include "Logger.h"


using namespace QDirStat;


FileInfo * Attic::locate( const QString & url )
{
    // Don't let a directly-nested dot entry return a spurious match on an un-nested url
    if ( url == dotEntryName() )
	return nullptr;

    // Match exactly on this un-nested attic
    if ( url == atticName() )
	return this;

    // Try for an exact match on a dot entry nested in this attic
    if ( url == atticName() % '/' % dotEntryName() )
	return dotEntry();

   // Recursively search all children including the dot entry
    for ( DotEntryIterator it { this }; *it; ++it )
    {
	FileInfo * foundChild = it->locate( url );
	if ( foundChild )
	    return foundChild;
    }

    return nullptr;
}
