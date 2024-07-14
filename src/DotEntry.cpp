/*
 *   File name: DotEntry.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QStringBuilder>

#include "DotEntry.h"
#include "Attic.h"
#include "FileInfoIterator.h"
#include "Logger.h"


using namespace QDirStat;


FileInfo * DotEntry::locate( const QString & url )
{
    // Don't let a directly-nested attic return a spurious match on an un-nested url
    if ( url == atticName() )
	return nullptr;

    // Match exactly on this un-nested dot entry
    if ( url == dotEntryName() )
	return this;

    // Try an exact match for an attic nested in this dot entry
    if ( url == dotEntryName() % '/' % atticName() )
	return attic();

    // If the local url is a leaf item, search the dot entry direct children for it
    if ( !url.contains( u'/' ) )  // no (more) "/" in this URL
    {
	// logDebug() << "Searching DotEntry for " << url << " in " << this << Qt::endl;

	auto it = std::find_if( FileInfoIterator { this },
	                        FileInfoIterator {},
	                        [ &url ]( FileInfo * item ) { return item->name() == url; } );
	if ( *it )
	    return *it;
    }

    // Search the attic and its children
    if ( attic() )
	return attic()->locate( url );

    return nullptr;
}

