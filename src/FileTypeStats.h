/*
 *   File name: FileTypeStats.h
 *   Summary:	Statistics classes for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#ifndef FileTypeStats_h
#define FileTypeStats_h

#include <QMap>

#include "ui_file-type-stats-window.h"
#include "FileSize.h"


// Using a suffix that can never occur: A slash is illegal in Linux/Unix
// filenames.
#define NO_SUFFIX        "//<No Suffix>"
#define NON_SUFFIX_RULE  "//<Other>"


namespace QDirStat
{
    class DirTree;
    class FileInfo;
    class MimeCategorizer;
    class MimeCategory;

    typedef QPair<QString, const MimeCategory *>	MapCategory;

    typedef QMap<MapCategory, FileSize>			StringFileSizeMap;
    typedef QMap<MapCategory, int>			StringIntMap;
    typedef QMap<const MimeCategory *, FileSize>	CategoryFileSizeMap;
    typedef QMap<const MimeCategory *, int>		CategoryIntMap;

    typedef StringFileSizeMap::const_iterator		StringFileSizeMapIterator;
    typedef CategoryFileSizeMap::const_iterator 	CategoryFileSizeMapIterator;


    /**
     * Class to calculate file type statistics for a subtree, such as how much
     * disk space is used for each kind of filename extension (*.jpg, *.mp4
     * etc.).
     **/
    class FileTypeStats
    {

    public:

	/**
	 * Constructor.
	 **/
	FileTypeStats( FileInfo * subtree );

	/**
	 * Destructor.
	 **/
	~FileTypeStats();

	/**
	 * Suppress copy and assignment constructors (would need a deep copy)
	 **/
	FileTypeStats( const FileTypeStats & ) = delete;
	FileTypeStats & operator=( const FileTypeStats & ) = delete;

	/**
	 * Return the number of files in the tree with the specified suffix.
	 **/
	int suffixCount( const QString & suffix, const MimeCategory * category ) const
	    { return _suffixCount.value( { suffix, category }, 0 ); }

	/**
	 * Return the total file size of files in the tree with the specified
	 * suffix.
	 **/
	FileSize suffixSum( const QString & suffix, const MimeCategory * category ) const
	    { return _suffixSum.value( { suffix, category }, 0LL ); }

	/**
	 * Return the number of files in the tree with the specified category.
	 **/
	int categoryCount( const MimeCategory * category ) const
	    { return _categoryCount.value( category, 0 ); }

	/**
	 * Return the total file size of files in the tree with the specified
	 * category.
	 **/
	FileSize categorySum( const MimeCategory * category ) const
	    { return _categorySum.value( category, 0LL ); }

	/**
	 * Return the number of files in the tree matched by a non-suffix rule
	 * with the specified category.
	 **/
	int categoryNonSuffixRuleCount( const MimeCategory * category ) const
	    { return _categoryNonSuffixRuleCount.value( category, 0 ); }

	/**
	 * Return the total file size of files in the tree matched by a
	 * non-suffix rule with the specified category.
	 **/
	FileSize categoryNonSuffixRuleSum( const MimeCategory * category ) const
	    { return _categoryNonSuffixRuleSum.value( category, 0LL ); }

	/**
	 * Return the special category for "other", i.e. unclassified files.
	 **/
	const MimeCategory * otherCategory() const { return _otherCategory; }

	/**
	 * Return the total size of the tree.
	 **/
	FileSize totalSize() const { return _totalSize; }

	/**
	 * Return the percentage of 'size' of the tree total size.
	 **/
	double percentage( FileSize size ) const
	    { return totalSize() == 0LL ? 0.0 : 100.0 * size / (double)totalSize(); }

	//
	// Iterators
	//

	StringFileSizeMapIterator suffixSumBegin() const
	    { return _suffixSum.constBegin(); }

	StringFileSizeMapIterator suffixSumEnd() const
	    { return _suffixSum.constEnd(); }

	CategoryFileSizeMapIterator categorySumBegin() const
	    { return _categorySum.constBegin(); }

	CategoryFileSizeMapIterator categorySumEnd() const
	    { return _categorySum.constEnd(); }


    protected:

	/**
	 * Collect information from the associated widget tree:
	 *
	 * Recursively go through the tree and collect sizes for each file type
	 * (filename extension).
	 **/
	void collect( const FileInfo * dir );

	/**
	 * Add the various sums.
	 **/
        void addCategorySum     ( const MimeCategory * category, const FileInfo * item );
        void addNonSuffixRuleSum( const MimeCategory * category, const FileInfo * item );
        void addSuffixSum       ( const QString & suffix, const MimeCategory * category, const FileInfo * item );

   	/**
	 * Remove useless content from the maps. On a Linux system, there tend
	 * to be a lot of files that have a '.' in the name, but it's not a
	 * meaningful suffix but a general-purpose separator for dates, SHAs,
	 * version numbers or whatever. All that stuff accumulates in the maps,
	 * and it's typically just a single file with that non-suffix. This
	 * function tries a best effort to get rid of that stuff.
	 **/
	void removeCruft();

	/**
	 * Remove empty suffix entries from the internal maps.
	 **/
	void removeEmpty();

	/**
	 * Check if a suffix is cruft, i.e. a nonstandard suffix that is not
	 * useful for display.
	 *
	 * Notice that this is a highly heuristical algorithm that might give
	 * false positives.
	 **/
	bool isCruft( const QString & suffix, const MimeCategory * category ) const;

	/**
	 * Check if the sums add up and how much is unaccounted for.
	 **/
	void sanityCheck();


    private:

	//
	// Data members
	//

	const MimeCategory    * _otherCategory;
	MimeCategorizer       * _mimeCategorizer;

	StringFileSizeMap	_suffixSum;
	StringIntMap		_suffixCount;
	CategoryFileSizeMap	_categorySum;
	CategoryIntMap		_categoryCount;
	CategoryFileSizeMap	_categoryNonSuffixRuleSum;
	CategoryIntMap		_categoryNonSuffixRuleCount;

        FileSize                _totalSize { 0LL };
    };
}


#endif // FileTypeStats_h
