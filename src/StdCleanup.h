/*
 *   File name: StdCleanup.h
 *   Summary:   QDirStat classes to reclaim disk space
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef StdCleanup_h
#define StdCleanup_h


namespace QDirStat
{
    /**
     * Create any of the predefined standard Cleanup actions to be
     * performed on DirTree items. Ownership of the objects is passed to the
     * caller.
     **/

    namespace StdCleanup
    {
	CleanupList stdCleanups( QObject * parent );
    };

}	// namespace QDirStat

#endif	// ifndef StdCleanup_h

