/*
 *   File name: FileSizeStats.h
 *   Summary:   Statistics classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef FileSizeStats_h
#define FileSizeStats_h

#include "PercentileStats.h"


namespace QDirStat
{
    class FileInfo;
    struct WildcardCategory;

    /**
     * Helper class for extended file size statistics.
     *
     * This collects file size data for trees or subtrees for later use for
     * calculating a median or quantiles or histograms.
     **/
    class FileSizeStats: public PercentileStats
    {
    public:

	/**
	 * Constructor with a subtree and optional flag whether to exclude
	 * symlinks.
	 **/
	FileSizeStats( FileInfo * subtree, bool excludeSymlinks = false );

	/**
	 * Constructor with a subtree and WildcardCategory.  Used with
	 * calls from FileTypeStatsWindow.
	 **/
	FileSizeStats( FileInfo * subtree, const WildcardCategory & wildcardCategory );


    protected:

	/**
	 * Recurse through all file elements in the subtree and append the own
	 * size for each file to the data collection. Note that the data are
	 * unsorted after this.
	 **/
	void collect( const FileInfo * subtree, bool excludeSymlinks );

	/**
	 * Recurse through all file elements in the subtree and append the own
	 * size for each file matching 'wildcardCategory' to the data
	 * collection. Note that the data are unsorted after this.
	 **/
	void collect( const FileInfo * subtree, const WildcardCategory & wildcardCategory );

    };	// class FileSizeStats

}	// namespace QDirStat

#endif	// ifndef FileSizeStats_h

/*
 *   File name: FileSizeStats.h
 *   Summary:   Statistics classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef FileSizeStats_h
#define FileSizeStats_h

#include "PercentileStats.h"
#include "MimeCategorizer.h" // WildcardCategory


namespace QDirStat
{
    class FileInfo;

    /**
     * Helper class for extended file size statistics.
     *
     * This collects file size data for trees or subtrees for later use for
     * calculating a median or quantiles or histograms.
     **/
    class FileSizeStats: public PercentileStats
    {
    public:

	/**
	 * Constructor with a subtree and optional flag whether to exclude
	 * symlinks.
	 **/
	FileSizeStats( FileInfo * subtree, bool excludeSymlinks = false );

	/**
	 * Constructor with a subtree and WildcardCategory.
	 **/
	FileSizeStats( FileInfo * subtree, const WildcardCategory & wildcardCategory );


    protected:

	/**
	 * Recurse through all file elements in the subtree and append the own
	 * size for each file to the data collection. Note that the data are
	 * unsorted after this.
	 **/
	void collect( const FileInfo * subtree, bool excludeSymlinks );

	/**
	 * Recurse through all file elements in the subtree and append the own
	 * size for each file matching 'wildcardCategory' to the data
	 * collection. Note that the data are unsorted after this.
	 **/
	void collect( const FileInfo * subtree, const WildcardCategory & wildcardCategory );

    };	// class FileSizeStats

}	// namespace QDirStat

#endif	// ifndef FileSizeStats_h

