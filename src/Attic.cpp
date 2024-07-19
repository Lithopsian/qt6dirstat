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


using namespace QDirStat;


FileInfo * Attic::locate( const QString & url )
{
    // Match exactly on this attic as long as it isn't nested in a dot entry
    if ( url == atticName() )
	return !parent()->isDotEntry() ? this : nullptr;

    // Try for an exact match on a dot entry nested in this attic
    if ( url == atticName() % '/' % dotEntryName() )
	return dotEntry();

   // Recursively search all children including dot entries
    for ( DotEntryIterator it { this }; *it; ++it )
    {
	FileInfo * foundChild = it->locate( url );
	if ( foundChild )
	    return foundChild;
    }

    return nullptr;
}
