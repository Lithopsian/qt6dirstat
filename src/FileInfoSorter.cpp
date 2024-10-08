/*
 *   File name: FileInfoSorter.cpp
 *   Summary:   Functor to handle sorting FileInfo objects
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "FileInfoSorter.h"
#include "FileInfo.h"


using namespace QDirStat;


bool FileInfoSorter::operator() ( FileInfo * a, FileInfo * b ) const
{
    if ( !a || !b ) return false;

    if ( _sortOrder == Qt::DescendingOrder )
	std::swap( a, b ); // Now we only need to handle the a < b case

    switch ( _sortCol )
    {
	case NameCol:
	    {
		// The dot entry should come "last" aphabetically
		if ( a->isDotEntry() ) return false;
		if ( b->isDotEntry() ) return true;

		return a->name() < b->name();
	    }

	case PercentBarCol:
	case PercentNumCol:
	case SizeCol:
	    return a->totalSize() == b->totalSize() ?
		a->totalAllocatedSize() < b->totalAllocatedSize() :
		a->totalSize() < b->totalSize();

	case TotalItemsCol:       return a->totalItems()      < b->totalItems();
	case TotalFilesCol:       return a->totalFiles()      < b->totalFiles();
	case TotalSubDirsCol:     return a->totalSubDirs()    < b->totalSubDirs();
	case LatestMTimeCol:      return a->latestMTime()     < b->latestMTime();
	case OldestFileMTimeCol:
	    {
		time_t a_time = a->oldestFileMTime();
		time_t b_time = b->oldestFileMTime();

		if ( a_time == 0 ) return false;
		if ( b_time == 0 ) return true;

		return a_time < b_time;
	    }

	case UserCol:             return a->uid()             < b->uid();
	case GroupCol:            return a->gid()             < b->gid();
	case PermissionsCol:      return a->mode()            < b->mode();
	case OctalPermissionsCol: return a->mode()            < b->mode();
	case ReadJobsCol:         return a->pendingReadJobs() < b->pendingReadJobs();
	case UndefinedCol:        return false;
	    // Intentionally omitting the 'default' branch
	    // so the compiler can warn about unhandled enum values
    }

    return false;
}
