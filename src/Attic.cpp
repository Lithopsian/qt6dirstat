/*
 *   File name: Attic.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "Attic.h"


using namespace QDirStat;


FileInfo * Attic::locate( const QString & url, bool findPseudoDirs )
{
    // Search all children
    for ( FileInfo * child = firstChild(); child; child = child->next() )
    {
	FileInfo * foundChild = child->locate( url, findPseudoDirs );
	if ( foundChild )
	    return foundChild;
    }

    // An attic can have neither an attic nor a dot entry, so there is no need
    // to search in either of those.

    return nullptr;
}
