/*
 *   File name: FileInfoIterator.h
 *   Summary:   Support classes for QDirStat - DirTree iterators
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef FileInfoIterator_h
#define FileInfoIterator_h

#include "Attic.h"
#include "DotEntry.h"


/**
 * Iterator classes for children of a FileInfo object:
 *
 * - FileInfoIterator: iterates only the direct children;
 * - DirInfoIterator: only iterates DirInfo children (not dot entries);
 * - DotEntryIterator: iterates the direct children plus a dot entry;
 * - AtticIterator: iterates the direct children plus dot entry plus attic;
 * - BySizeIterator: iterates in order by size descending, including
 *   the dot entry.
 *
 * These classes are heavily inlined to improved performance and
 * reduce code size.
 *
 * Sample usage:
 *
 *	  for ( FileInfoIterator it { parent }; *it; ++it )
 *	     logDebug() << *it << ":\t" << it->totalSize() << Qt::endl;
 *
 * This will output the debug URL (path+name) and the total size of each
 * (direct) subdirectory child and each (direct) file child of 'node'.
 *
 * Range for loops on a FileInfo * container are set up to use the
 * FileInfoIterator class, so to iterate only over the direct children,
 * this can be simplified (or at least shortened) to:
 *
 *	  for ( auto child : parent )
 *	     logDebug() << child << ":\t" << child->totalSize() << Qt::endl;
 *
 **/
namespace QDirStat
{
    /**
     * Iterator class for children of a FileInfo object. For optimum
     * performance, this iterator class does NOT return children in any
     * specific sort order.  When there are no more children, operator*
     * will return 0.  The iterator does not recurse into sub-directories.
     *
     * Note that the iterator may return 0 when it is first created if
     * there are no children.
     **/
    class FileInfoIterator
    {
    public:
	/**
	 * STL iterator tags.  These are necessary to use std:: algorithm
	 * functions with this iterator.
	 **/
	using iterator_category = std::forward_iterator_tag;
	using value_type = FileInfo *;
	using difference_type = ptrdiff_t;
	using pointer = FileInfo **;
	using reference = FileInfo *&;

	/**
	 * Default constructor: returns an invalid iterator which
	 * corresponds to a position past the last child.
	 **/
	FileInfoIterator():
	    _current { nullptr }
	{}

	/**
	 * Constructor: initialize an object to iterate over the children
	 * of 'parent', not including any dot entry.  The children will be
	 * returned in no particular order.
	 **/
	FileInfoIterator( const FileInfo * parent ):
	    _current { parent->firstChild() }
	{}

	/**
	 * Return the current child pointer or 0 if there are no more.
	 **/
	FileInfo * operator*() const { return _current; }

	/**
	 * Dereference the iterator so that syntax such as (*it)->
	 * can be simplified to it->.  Like operator*, this will
	 * return 0 if the iterator is no longer valid, so using
	 * it will cause undefined behaviour (usually a crash).
	 **/
	FileInfo * operator->() const { return _current; }

	/**
	 * Advance to the next child.  Do NOT call this if the iterator
	 * is not valid (ie. *it = 0).
	 **/
	void operator++() { _current = _current->next(); }

	/**
	 * Comparison operator overloads for STL operations.
	 **/
	bool operator==( const FileInfoIterator & other ) const { return _current == other._current;}
	bool operator!=( const FileInfoIterator & other ) const { return _current != other._current;}

    private:
	FileInfo * _current;

    }; // class FileInfoIterator


    /**
     * Begin and end iterators for a FileInfo pointer.
     *
     * Note that these cannot be FileInfo members because they accept
     * pointers, not the class itself.
     **/
    inline FileInfoIterator begin( const FileInfo * item ) { return item; }
    inline FileInfoIterator end( const FileInfo * ) { return {}; }




    /**
     * Iterator for DirInfo children.  The dot entry is not returned.
     **/
    class DirInfoIterator
    {
    public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = FileInfo *;
	using difference_type = ptrdiff_t;
	using pointer = FileInfo **;
	using reference = FileInfo *&;

	DirInfoIterator(): _current { nullptr } {}
	DirInfoIterator( const FileInfo * parent ): _current { findNextDirInfo( parent->firstChild() ) } {}

	DirInfo * operator*() const { return _current; }
	DirInfo * operator->() const { return _current; }

	void operator++() { _current = findNextDirInfo( _current->next() ); }

	bool operator==( const DirInfoIterator & other ) const { return _current == other._current;}
	bool operator!=( const DirInfoIterator & other ) const { return _current != other._current;}

    protected:
	/**
	 * Find the next child that is a DirInfo object, starting
	 * from 'item'.
	 **/
	DirInfo * findNextDirInfo( FileInfo * item )
	{
	    while ( item && !item->isDirInfo() )
		item = item->next();
	    return item ? item->toDirInfo() : nullptr;
	}

    private:
	DirInfo * _current;

    }; // class DirInfoIterator


    inline DirInfoIterator dirInfoBegin( const FileInfo * item ) { return item; }
    inline DirInfoIterator dirInfoEnd( const FileInfo * ) { return {}; }




    /**
     * Iterator class for children of a FileInfo object, including any
     * dot entry.  Although the children are not returned in any
     * particular order, the dot entry will always be returned last.
     **/
    class DotEntryIterator
    {
    public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = FileInfo *;
	using difference_type = ptrdiff_t;
	using pointer = FileInfo **;
	using reference = FileInfo *&;

	DotEntryIterator():
	    _dotEntry { nullptr },
	    _current { nullptr }
	{}
	DotEntryIterator( const FileInfo * parent ):
	    _dotEntry { parent->dotEntry() },
	    _current { parent->firstChild() ? parent->firstChild() : _dotEntry }
	{}

	FileInfo * operator*() const { return _current; }
	FileInfo * operator->() const { return _current; }

	void operator++() { _current = next(); }

	bool operator==( const DotEntryIterator & other ) const { return _current == other._current;}
	bool operator!=( const DotEntryIterator & other ) const { return _current != other._current;}

    protected:

	/**
	 * Return the next child of this parent, or 0 if there are none.
	 **/
	FileInfo * next() const
	    { return _current->next() ? _current->next() : dotEntry(); }

	/**
	 * Return the dot entry, or 0 if the iterator already points to
	 * the dot entry.
	 **/
	FileInfo * dotEntry() const
	    { return _current == _dotEntry ? nullptr : _dotEntry; }


    private:

	FileInfo * _dotEntry;
	FileInfo * _current;

    };	// class DotEntryIterator


    inline DotEntryIterator dotEntryBegin( const FileInfo * item ) { return item; }
    inline DotEntryIterator dotEntryEnd( const FileInfo * ) { return {}; }




    /**
     * Iterator class for children of a FileInfo object, including any
     * dot entry and attic.  Although the children are not returned in
     * any particular order, the dot entry and attic will always be last.
     **/
    class AtticIterator
    {
    public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = FileInfo *;
	using difference_type = ptrdiff_t;
	using pointer = FileInfo **;
	using reference = FileInfo *&;

	AtticIterator():
	    _dotEntry { nullptr },
	    _attic { nullptr },
	    _current { nullptr }
	{}
	AtticIterator( const FileInfo * parent ):
	    _dotEntry { parent->dotEntry() },
	    _attic { parent->attic() },
	    _current { parent->firstChild() ? parent->firstChild() : _dotEntry ? _dotEntry : _attic }
	{}

	FileInfo * operator*() const { return _current; }
	FileInfo * operator->() const { return _current; }

	void operator++() { _current = next(); }

	bool operator==( const AtticIterator & other ) const { return _current == other._current;}
	bool operator!=( const AtticIterator & other ) const { return _current != other._current;}

    protected:

	/**
	 * Return the next child of this parent, or 0 if there are none.
	 **/
	FileInfo * next() const
	    { return _current->next() ? _current->next() : dotEntry(); }

	/**
	 * Return the dot entry if the iterator doesn't already point
	 * to the dot entry.  Otherwise it will return the attic or 0.
	 **/
	FileInfo * dotEntry() const
	    { return !_dotEntry || _current == _dotEntry ? attic() : _dotEntry; }

	/**
	 * Return the attic, or 0 if the iterator already points to
	 * the attic.
	 **/
	FileInfo * attic() const
	    { return _current == _attic ? nullptr : _attic; }


    private:

	FileInfo * _dotEntry;
	FileInfo * _attic;
	FileInfo * _current;

    };	// class AtticIterator


    inline AtticIterator atticBegin( const FileInfo * item ) { return item; }
    inline AtticIterator atticEnd( const FileInfo * ) { return {}; }




    /**
     * Iterator class for children of a FileInfo object.  The children,
     * including the dot entry but not tany attic, are returned in
     * order of descending size.
     *
     * This iterator provides additional functions for returning the
     * total size of all children, and for "bookmarking" a position in
     * the list of children.  It is specialised for use by TreemapTile.
     **/
    class BySizeIterator
    {
	using BySizeIteratorList = QVector<FileInfo *>;
	using BySizeIteratorPos  = BySizeIteratorList::const_iterator;

    public:

	using iterator_category = std::forward_iterator_tag;
	using value_type = FileInfo *;
	using difference_type = ptrdiff_t;
	using pointer = FileInfo **;
	using reference = FileInfo *&;

	/**
	 * Constructor.  Finds the children of 'parent', including a
	 * dot entry, and sorts them by decreasing size.  It calculates
	 * the total size of all the children.
	 **/
	BySizeIterator( const FileInfo * parent );

	/**
	 * Return the current child object or 0 if there are no more.
	 **/
	FileInfo * operator*() const
	    { return _currentIt == _sortedChildren.cend() ? nullptr : *_currentIt; }

	/**
	 * Dereference the iterator so that syntax such as (*it)->
	 * can be simplified to it->
	 **/
	FileInfo * operator->() const { return *_currentIt; }

	/**
	 * Advance to the next child.
	 **/
	void operator++()
	    { if ( _currentIt != _sortedChildren.cend() ) ++_currentIt; }

	/**
	 * Return the total size of the children to be iterated, calculated
	 * using the optional function passed to the constructor.  This is mainly
	 * to avoid TreemapTile having to iterate all the children again.
	 **/
	FileSize totalSize() const { return _totalSize; }

	/**
	 * Functions for "bookmarking" a position in the children that can be returned
	 * to at a later point.  This allows TreemapTile to iterate ahead to identify
	 * tiles to form a row in the squarified layout, but then go back to the original
	 * position to start laying out the row.
	 **/
	BySizeIteratorPos currentPos() const { return _currentIt; }
	void setPos( const BySizeIteratorPos pos ) { _currentIt = pos; }


    private:

	BySizeIteratorList _sortedChildren;
	BySizeIteratorPos  _currentIt;
	FileSize           _totalSize { 0LL };

    }; // class BySizeIterator

} // namespace QDirStat


#endif // ifndef FileInfoIterator_h

