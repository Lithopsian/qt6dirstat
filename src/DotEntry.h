/*
 *   File name: DotEntry.h
 *   Summary:	Support classes for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#ifndef DotEntry_h
#define DotEntry_h


#include "DirInfo.h"


namespace QDirStat
{
    class DirTree;

    /**
     * This is a very special kind if DirInfo: the <Files> pseudo container
     * that groups the non-directory children of a directory together. The
     * basic idea is keep the direct file children of a directory in one
     * container so their total size can easily be compared to any of the
     * subdirectories.
     **/
    class DotEntry: public DirInfo
    {
    public:
	/**
	 * Constructor.
	 **/
	DotEntry( DirTree * tree,
		  DirInfo * parent ) :
	    DirInfo ( parent, tree, dotEntryName() )
	{/*
	    if ( parent )
	    {
		_device = parent->device();
		_mode	= parent->mode();
		_uid	= parent->uid();
		_gid	= parent->gid();
	    }*/
	}

	/**
	 * Get the "Dot Entry" for this node if there is one (or 0 otherwise).
	 * Since this is a dot entry, this always returns 0: A dot entry does
	 * not have a dot entry itself.
	 **/
	DotEntry * dotEntry() const override { return nullptr; }

	/**
	 * Check if this is a dot entry.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	bool isDotEntry() const override { return true; }

	/**
	 * Sets a flag that this is the root directory of a cache file read.
	 **/
	bool isFromCache() const override { return _parent && _parent->isFromCache(); }

	/**
	 * Insert a child into the children list.
	 *
	 * The order of children in this list is absolutely undefined;
	 * don't rely on any implementation-specific order.
	 *
	 * Reimplemented - inherited from DirInfo.
	 **/
	void insertChild( FileInfo *newChild ) override;

	/**
	 * Recursively finalize all directories from here on -
	 * call finalizeLocal() recursively.
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
	DirReadState readState() const override
		{ return _parent ? _parent->readState() : readState(); }

	/**
	 * Reset to the same status like just after construction in preparation
	 * of refreshing the tree from this point on.
	 *
	 * Reimplemented - inherited from DirInfo.
	 **/
	void reset() override {}


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

