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

#include <QHash>

#include "ui_file-type-stats-window.h"
#include "Typedefs.h" // FileCount, FileSize


namespace QDirStat
{
    class FileInfo;
    class MimeCategory;

    struct PatternCategory
    {
	QString              pattern;
	bool                 caseInsensitive;
	const MimeCategory * category;
    };
    struct CountSize
    {
	FileCount count;
	FileSize  size;
    };

    /**
     * Define operator== and qHash for PatternCategory so that it
     * can be used as a QHash key.  The pattern combined with the
     * caseInsensitive flag should be unique, but XOR it with the
     * category pointer to be certain.
     **/
#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
    inline uint qHash( const PatternCategory & key, uint seed )
#else
    inline size_t qHash( const PatternCategory & key, size_t seed )
#endif
    {
	return ::qHash( key.pattern, seed ) << key.caseInsensitive ^ reinterpret_cast<quintptr>( key.category );
    }
    inline bool operator==( const PatternCategory & lhs, const PatternCategory & rhs )
    {
	return lhs.pattern         == rhs.pattern &&
	       lhs.caseInsensitive == rhs.caseInsensitive &&
	       lhs.category        == rhs.category;
    }

    /**
     * Two types of hash maps: CategoryMap is used for satistics
     * aggregated at the category level; PatternMap is used for
     * statistics aggregated by pattern per category.
     **/
    typedef QHash<const MimeCategory *, CountSize> CategoryMap;
    typedef QHash<PatternCategory,      CountSize> PatternMap;


    /**
     * Class to calculate file type statistics for a subtree, such as how much
     * disk space is used for each kind of filename extension (*.jpg, *.mp4
     * etc.) and how many files.  Special files (pipes, sockets, block, and
     * character devices) and directories are ignored.
     *
     * This class exists only to support FileTypeStatsWindow.  Access to the
     * completed statistics is through the iterators.
     **/
    class FileTypeStats final
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
	PatternMap::const_iterator patternsBegin()    const { return _patterns.cbegin();   }
	PatternMap::const_iterator patternsEnd()      const { return _patterns.cend();     }
	CategoryMap::const_iterator categoriesBegin() const { return _categories.cbegin(); }
	CategoryMap::const_iterator categoriesEnd()   const { return _categories.cend();   }

	/**
	 * Return the total number of files collected or the total size of
	 * all the collected files.
	 **/
        FileCount totalCount() const { return _totalCount; }
        FileSize totalSize() const { return _totalSize; }


    protected:

	/**
	 * Collect information from the associated widget tree:
	 *
	 * Recursively go through the tree and collect sizes for each file type
	 * (filename extension) into the two maps.
	 **/
	void collect( const FileInfo           * dir,
                      const QRegularExpression & matchLetters,
                      const QRegularExpression & matchSpaces );

	/**
	 * Aggregate category entries to a map of CountSize structs with
	 * MimeCategory * keys.
	 **/
	void addCategoryItem( const MimeCategory * category, const FileInfo * item );

	/**
	 * Aggregate pattern entries to a map of CountSize structs with
	 * PatternCategory keys.
	 **/
	void addPatternItem( const QString      & pattern,
	                     bool                 caseInsensitive,
	                     const MimeCategory * category,
	                     const FileInfo     * item );

	/**
	 * Check if the sums add up and how much is unaccounted for.
	 *
	 * Note that this will never match exactly, because the map sizes don't
	 * include directories.
	 **/
	void sanityCheck( FileInfo * subtree ) const;


    private:

	PatternMap  _patterns;
	CategoryMap _categories;

	FileCount   _totalCount{ 0 };
	FileSize    _totalSize{ 0 };

    };	// class FileTypeStats

}	// namespace QDirStat

#endif	// FileTypeStats_h
