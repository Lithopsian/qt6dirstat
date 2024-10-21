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
#include "FileInfoIterator.h"


using namespace QDirStat;


FileInfo * DotEntry::locate( const QString & url )
{
    // Match exactly on this dot entry as long as it isn't nested in an attic
    if ( url == dotEntryName() )
	return !parent()->isAttic() ? this : nullptr;

    // Try an exact match for an attic nested in this dot entry
    if ( url == dotEntryName() % '/' % atticName() )
	return attic();

    // If the local url is a leaf item, search the dot entry direct children for it
    if ( !url.contains( u'/' ) )  // no (more) "/" in this URL
    {
	// logDebug() << "Searching DotEntry for " << url << " in " << this << Qt::endl;

	const auto compare = [ &url ]( FileInfo * item ) { return item->name() == url; };
	auto it = std::find_if( begin( this ), end( this ), compare );
	if ( *it )
	    return *it;
    }

    // Search the attic and its children
    if ( attic() )
	return attic()->locate( url );

    return nullptr;
}

