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
	 * Constructor with just a subtree.
	 **/
	FileSizeStats( FileInfo * subtree );

	/**
	 * Constructor with a subtree and suffix.
	 *
	 * 'suffix' should start with ".", e.g. ".jpg".
	 **/
	FileSizeStats( const FileInfo * subtree, const QString & suffix );


    protected:

	/**
	 * Recurse through all file elements in the subtree and append the own
	 * size for each file to the data collection. Note that the data are
	 * unsorted after this.
	 **/
	void collect( const FileInfo * subtree );

	/**
	 * Recurse through all file elements in the subtree and append the own
	 * size for each file with the specified suffix to the data
	 * collection. Note that the data are unsorted after this.
	 *
	 **/
	void collect( const FileInfo * subtree, const QString & suffix );

    };	// class FileSizeStats

}	// namespace QDirStat

#endif // ifndef FileSizeStats_h

