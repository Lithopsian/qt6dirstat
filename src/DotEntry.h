/*
 *   File name: DotEntry.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef DotEntry_h
#define DotEntry_h

#include "DirInfo.h"


namespace QDirStat
{
    class DirTree;

    /**
     * This is a special kind of DirInfo: the <Files> pseudo container that
     * groups the non-directory children of a directory together. The basic
     * idea is keep the direct file children of a directory in one container
     * so their total size can easily be compared to any of the subdirectories.
     **/
    class DotEntry: public DirInfo
    {
    public:
	/**
	 * Constructor.
	 **/
	DotEntry( DirTree * tree, DirInfo * parent ):
	    DirInfo{ parent, tree, dotEntryName() }
	{}

	/**
	 * Returns the "Dot Entry" for this node if there is one (or 0 otherwise).
	 *
	 * Since this is a dot entry, this always returns 0: a dot entry does
	 * not have a dot entry itself.
	 **/
	DotEntry * dotEntry() const override { return nullptr; }

	/**
	 * Returns whether this is a dot entry.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	bool isDotEntry() const override { return true; }

	/**
	 * Returns whether this was populated automatically from a cache
	 * file read.
	 **/
	bool isFromCache() const override { return parent() && parent()->isFromCache(); }

	/**
	 * Insert a child into the children list.
	 *
	 * The order of children in this list is absolutely undefined;
	 * don't rely on any implementation-specific order.
	 *
	 * Reimplemented - inherited from DirInfo.
	 **/
//	void insertChild( FileInfo * newChild ) override;

	/**
	 * Recursively finalize all directories from here on - call
	 * finalizeLocal() recursively.  A DotEntry has no directory
	 * children so nothing to do.
	 *
	 * Reimplemented - inherited from DirInfo.
	 **/
	void finalizeAll() override {}

	/**
	 * Get the current state of the directory reading process.
	 * This reimplementation returns the parent directory's value.
	 *
	 * Reimplemented - inherited from DirInfo.
	 **/
	DirReadState readState() const override { return parent() ? parent()->readState() : readState(); }

	/**
	 * Locate a child somewhere in this subtree whose URL (i.e. complete
	 * path) matches the URL passed. Returns 0 if there is no such child.
	 *
	 * Reimplemented - inherited from FileInfo.  This implementation does
	 * not search for the "<Files>" or "<Files><Ignored>" portion of a
	 * url unless that is an exact match. The urls of children inside a
	 * dot entry do not include "<Files>".
	 **/
	FileInfo * locate( const QString & url ) override;


    protected:

	/**
	 * Clean up unneeded / undesired dot entries.
	 *
	 * Since a dot entry does not have a dot entry itself, this
	 * reimplementation does nothing.
	 *
	 * Reimplemented - inherited from DirInfo.
	 **/
	void cleanupDotEntries() override {}

    };	// class DotEntry

}	// namespace QDirStat

#endif // ifndef DotEntry_h

