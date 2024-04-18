/*
 *   File name: DotEntry.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "DotEntry.h"
#include "DirTree.h"
#include "Exception.h"
#include "Logger.h"


using namespace QDirStat;


void DotEntry::insertChild( FileInfo * newChild )
{
    CHECK_PTR( newChild );

    // Whatever is added here is added directly to this node; a dot entry
    // cannot have a dot entry itself.
    newChild->setNext( firstChild() );
    setFirstChild( newChild );
    newChild->setParent( this );	// make sure the parent pointer is correct

    childAdded( newChild );		// update summaries
}

