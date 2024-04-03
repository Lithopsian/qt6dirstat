/*
 *   File name: Attic.cpp
 *   Summary:	Support classes for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include "Attic.h"
#include "Exception.h"
#include "Logger.h"


using namespace QDirStat;


Attic::Attic( DirTree * tree,
	      DirInfo * parent ) :
    DirInfo ( parent, tree, atticName() )
{
    _isIgnored = true;
/*
    if ( parent )
    {
	_device = parent->device();
	_mode	= parent->mode();
	_uid	= parent->uid();
	_gid	= parent->gid();
    }*/
}


FileInfo * Attic::locate( QString url, bool findPseudoDirs )
{
    if ( !_tree || !_parent )
	return nullptr;

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

