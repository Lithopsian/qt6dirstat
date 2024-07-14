/*
 *   File name: Attic.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef Attic_h
#define Attic_h

#include "DirInfo.h"


namespace QDirStat
{
    class DirTree;

    /**
     * Special DirInfo to store ignored files in. This behaves very much like a
     * normal DirInfo; it can have a DotEntry, and it can have subdirectories.
     *
     * But it is not a normal child of its parent DirInfo, i.e. you normally
     * don't iterate over it; use DirInfo::attic() to access it.  Its sizes,
     * children counts etc. are not added to the parent dir's sums.
     *
     * The treemap will not display anything in the attic; that's the whole
     * point of it. Yet, the user can open the attic (the <Ignored> pseudo
     * entry) in the tree view.
     **/
    class Attic: public DirInfo
    {
    public:
	/**
	 * Constructor.
	 **/
	Attic( DirTree * tree, DirInfo * parent ):
	    DirInfo { parent, tree, atticName() }
	{ setIgnored( true ); }

	/**
	 * Check if this is an attic entry where ignored files and directories
	 * are stored.
	 *
	 * Reimplemented - inherited from FileInfo.
	 **/
	bool isAttic() const override { return true; }

	/**
	 * Return the attic of this tree node. Since this already is an attic,
	 * this always returns 0.
	 *
	 * Reimplemented from DirInfo.
	 **/
	Attic * attic() const override { return nullptr; }

	/**
	 * Get the current state of the directory reading process.
	 * This reimplementation returns the parent directory's value.
	 *
	 * Reimplemented - inherited from DirInfo.
	 **/
	DirReadState readState() const override
	    { return parent() ? parent()->readState() : readState(); }

	/**
	 * Check the 'ignored' state of this item and set the '_isIgnored' flag
	 * accordingly.  An Attic is always ignored, so nothing to do.
	 *
	 * Reimplemented - inherited from DirInfo.
	 **/
	void checkIgnored() override {}

	/**
	 * Locate a child somewhere in this subtree whose URL (i.e. complete
	 * path) matches the URL passed. Returns 0 if there is no such child.
	 *
	 * Reimplemented - inherited from FileInfo.  This implementation does
	 * not search for the "<Ignored>" or "<Ignored><Files>" portion of a
	 * url unless that is an exact match. The urls of children inside an
	 * attic do not include "<Ignored>".
	 **/
	FileInfo * locate( const QString & url ) override;

    };	// class Attic

}	// namespace QDirStat


#endif // ifndef Attic_h

