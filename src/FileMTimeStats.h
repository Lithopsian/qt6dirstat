/*
 *   File name: FileMTimeStats.h
 *   Summary:   Statistics classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef FileMTimeStats_h
#define FileMTimeStats_h

#include "PercentileStats.h"


namespace QDirStat
{
    class FileInfo;

    /**
     * Helper class for extended file mtime (modification time) statistics.
     * This is very similar to FileSizeStats.
     *
     * This collects file mtime data for trees or subtrees for later use for
     * calculating a median or quantiles or histograms.
     **/
    class FileMTimeStats: public PercentileStats
    {
    public:

	/**
	 * Constructor. If 'subtree' is non-null, immediately collect data from
	 * that subtree.
	 **/
	FileMTimeStats( FileInfo * subtree );

    protected:

	/**
	 * Recurse through all file elements in the subtree and append the
	 * mtime for each file to the data collection. Note that the data is
	 * unsorted after this.
	 **/
	void collect( FileInfo * subtree );

    };	// class FileMTimeStats

}	// namespace QDirStat

#endif	// ifndef FileMTimeStats_h

