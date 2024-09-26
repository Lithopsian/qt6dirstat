/*
 *   File name: FileTypeStats.h
 *   Summary:   Statistics classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef FileTypeStats_h
#define FileTypeStats_h

#include <memory>

#include <QHash>

#include "ui_file-type-stats-window.h"
#include "Typedefs.h" // FileSize


namespace QDirStat
{
    class FileInfo;
    class MimeCategory;

    /**
     * SuffixCategory and CountSum are defined instead of using
     * QPair for the hash keys and values.  This just means that
     * the pair members can be referred to by name rather than
     * simply 'first' and 'second'.
     **/
    struct SuffixCategory
    {
	QString suffix;
	const MimeCategory * category;
    };

    struct CountSum
    {
	int count;
	FileSize sum;
    };

    /**
     * Define operator== and qHash for SuffixCategory so that it
     * can be used as a QHash key.  The suffix hash is almost unique,
     * so simply XOR it with the category pointer.
     **/
#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
    inline uint qHash( const SuffixCategory & key, uint seed )
#else
    inline size_t qHash( const SuffixCategory & key, size_t seed )
#endif
	{ return ::qHash( key.suffix, seed ) ^ reinterpret_cast<quintptr>( key.category ); }
    inline bool operator==( const SuffixCategory & cat1, const SuffixCategory & cat2 )
	{ return cat1.suffix == cat2.suffix && cat1.category == cat2.category; }

    /**
     * Two types of hash maps: CategoryMap is used for satistics
     * aggregated at the category level; SuffixMap is used for
     * statistics aggregated by suffix per category.
     **/
    typedef QHash<const MimeCategory *, CountSum> CategoryMap;
    typedef QHash<SuffixCategory, CountSum>       SuffixMap;


    /**
     * Class to calculate file type statistics for a subtree, such as how much
     * disk space is used for each kind of filename extension (*.jpg, *.mp4
     * etc.) and how many files.  Only regular files are analyzed because other
     * file-like entities are tiny or don't have a meaningful size.  Specifically,
     * symlinks and directories are ignored.
     *
     * This class exists only to support FileTypeStatsWindow.  Access to the
     * completed statistics is through the iterators.
     **/
    class FileTypeStats
    {
    public:

	/**
	 * Constructor.  Constructing an instance will analyse the given subtree
	 * and populate the three statistics maps.
	 **/
	FileTypeStats( FileInfo * subtree );

	/**
	 * Iterators for the two maps.
	 **/
	SuffixMap::const_iterator suffixesBegin()     const { return _suffixes.cbegin();   }
	SuffixMap::const_iterator suffixesEnd()       const { return _suffixes.cend();     }
	CategoryMap::const_iterator categoriesBegin() const { return _categories.cbegin(); }
	CategoryMap::const_iterator categoriesEnd()   const { return _categories.cend();   }

	/**
	 * Return the special category for "other", i.e. unclassified files.
	 * This category is created and owned by the FileTypeStats object.
	 **/
	const MimeCategory * otherCategory() const { return _otherCategory.get(); }

	/**
	 * Return the percentage of 'size' relative to the tree total size.
	 **/
	float percentage( FileSize size ) const
	    { return _totalSize == 0LL ? 0.0f : 100.0f * size / _totalSize; }


    protected:

	/**
	 * Collect information from the associated widget tree:
	 *
	 * Recursively go through the tree and collect sizes for each file type
	 * (filename extension) into the two maps.
	 **/
	void collect( const FileInfo * dir );

	/**
	 * Aaggregate entries to each of the maps.
	 **/
	void addCategorySum( const MimeCategory * category, const FileInfo * item );
	void addSuffixSum  ( const QString & suffix, const MimeCategory * category, const FileInfo * item );

	/**
	 * Check if the sums add up and how much is unaccounted for.
	 *
	 * Note that this will never match exactly, because the map sizes don't
	 * include directories.
	 **/
	void sanityCheck();


    private:

	std::unique_ptr<const MimeCategory> _otherCategory;

	SuffixMap   _suffixes;
	CategoryMap _categories;

	FileSize    _totalSize{ 0LL };

    };	// class FileTypeStats

}	// namespace QDirStat

#endif	// FileTypeStats_h
