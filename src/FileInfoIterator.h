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


namespace QDirStat
{
    typedef FileInfoList::const_iterator FileInfoListPos;

    /**
     * Iterator class for children of a FileInfo object. For optimum
     * performance, this iterator class does NOT return children in any
     * specific sort order.  When there are no more children, operator*
     * will return 0.  Unlike many C++ iterators it is safe to call
     * operator++ or operator* even on an iterator past the end of the
     * children.
     *
     * Sample usage:
     *
     *	  for ( FileInfoIterator it { node }; *it; ++it )
     *	     logDebug() << *it << ":\t" << (*it)->totalSize() << Qt::endl;
     *
     * This will output the URL (path+name) and the total size of each (direct)
     * subdirectory child and each (direct) file child of 'node'.
     *
     * This does not recurse into subdirectories, and the dot entry is treated
     * just like a subdirectory.  Note that the iterator may return 0 when it
     * is first created if there are no children.
     **/
    class FileInfoIterator
    {
    public:
	/**
	 * Constructor: initialize an object to iterate over the children
	 * of 'parent', including any dot entry.  The children will be
	 * returned in no particular order although the dot entry will
	 * always be last.
	 **/
	FileInfoIterator( const FileInfo * parent );

	/**
	 * Return the current child pointer or 0 if there are none.
	 **/
	FileInfo * operator*() const { return _current; }

	/**
	 * Advance to the next child.
	 **/
	void operator++() { _current = next(); }


    protected:

	/**
	 * Return the next child of this parent, or 0 if there are none.
	 **/
	FileInfo * next();


    private:

	const FileInfo * _parent;
	FileInfo       * _current;

    };	// class FileInfoIterator


    class FileInfoBySizeIterator
    {
    public:

	/**
	 * Constructor.
	 **/
	FileInfoBySizeIterator( const FileInfo * parent );

	/**
	 * Return the current child object or 0 if there is no more.
	 * Same as operator*() .
	 **/
	FileInfo * current() const
	    { return _currentIt == _sortedChildren.cend() ? nullptr : *_currentIt; }

	/**
	 * Return the current child object or 0 if there is no more.
	 * Same as current().
	 **/
	FileInfo * operator*() const { return current(); }

	/**
	 * Advance to the next child.
	 **/
	void next()
	    { if ( _currentIt != _sortedChildren.cend() ) ++_currentIt; }

	/**
	 * Advance to the next child. Same as next().
	 **/
	void operator++() { next(); }

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
	 * position to start layout out the row.
	 **/
	FileInfoListPos currentPos() const { return _currentIt; }
	void setPos( FileInfoListPos pos ) { _currentIt = pos; }


    private:

	FileInfoList     _sortedChildren;
	FileInfoListPos  _currentIt;
	FileSize         _totalSize { 0LL };
    };

} // namespace QDirStat


#endif // ifndef FileInfoIterator_h

