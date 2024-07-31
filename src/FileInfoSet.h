/*
 *   File name: FileInfoSet.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef FileInfoSet_h
#define FileInfoSet_h

#include <QSet>

#include "Typedefs.h" // FileSize


namespace QDirStat
{
    class FileInfo;

    /**
     * Container for FileInfo pointers. This is a wrapper around QSet with a
     * few add-on functions.
     **/
    class FileInfoSet: public QSet<FileInfo *>
    {
    public:

	/**
	 * Constructor, creates an empty QSet.  This is the default constructor,
	 * only specified here for clarity.
	 **/
	FileInfoSet():
	    QSet<FileInfo *> {}
	{}

	/**
	 * Constructor, initialises the set from a list.
	 **/
	FileInfoSet( std::initializer_list<FileInfo *> list ):
	    QSet<FileInfo *> { list }
	{}

	/**
	 * Return the first item in this set or 0 if the set is empty.
	 *
	 * This makes most sense if there is only one item at all;
	 * otherwise it would be completely random which item would be
	 * returned as the first.
	 **/
	FileInfo * first() const { return isEmpty() ? nullptr : *begin(); }

	/**
	 * Return 'true' if the set contains any directory item.
	 **/
	bool containsDir() const;

	/**
	 * Return 'true' if the set contains any file item.  "File" here
	 * is in the broadest sense, ie. anything that isn't a DirInfo, not
	 * just regular files.
	 **/
	bool containsFile() const;

	/**
	 * Return 'true' if the set contains any PkgInfo item.
	 **/
	bool containsPkg() const;

	/**
	 * Return 'true' if the set contains any pseudo directory, i.e. any dot
	 * entry ("<Files>") or attic ("<Ignored>).
	 **/
	bool containsPseudoDir() const;

	/**
	 * Return 'true' if the set contains any dot entry ("<Files">).
	 **/
	bool containsDotEntry() const;

	/**
	 * Return 'true' if the set contains any attic ("<Ignored>").
	 **/
	bool containsAttic() const;

	/**
	 * Return the sum of all total sizes in the set.
	 *
	 * It is desirable to call this on a normalized() set to avoid
	 * duplicate accounting of sums.
	 **/
	FileSize totalSize() const;

	/**
	 * Return the sum of all total allocated sizes in the set.
	 *
	 * It is desirable to call this on a normalized() set to avoid
	 * duplicate accounting of sums.
	 **/
	FileSize totalAllocatedSize() const;

	/**
	 * Return 'true' if this set contains any ancestor (parent, parent's
	 * parent etc.) of 'item'. This does not check if 'item' itself is in
	 * the set.
	 **/
	bool containsAncestorOf( FileInfo * item ) const;

	/**
	 * Return 'true' if any item in this set is busy.
	 **/
	bool containsBusyItem() const;

	/**
	 * Return 'true' if this set is non-empty and the dir tree is busy.
	 **/
	bool treeIsBusy() const;

	/**
	 * Return a 'normalized' set, i.e. with all items removed that have
	 * ancestors in the set.
	 **/
	FileInfoSet normalized() const;

	/**
	 * Return a FileInfoSet of all parents of all items in the set.
	 * If a parent is a dot entry, use the true parent, i.e. the dot
	 * entry's parent.
	 **/
	FileInfoSet parents() const;

    };	// class FileInfoSet

}	// namespace QDirStat

#endif	// FileInfoSet_h
