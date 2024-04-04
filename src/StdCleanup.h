/*
 *   File name: StdCleanup.h
 *   Summary:	QDirStat classes to reclaim disk space
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#ifndef StdCleanup_h
#define StdCleanup_h

#include <QObject>
#include <QList>


namespace QDirStat
{
    class Cleanup;

    /**
     * Create any of the predefined standard Cleanup actions to be
     * performed on DirTree items. Ownership of the objects is passed to the
     * caller.
     *
     * This class is not meant to be ever instantiated - use the static methods
     * only.
     **/

    namespace StdCleanup
    {
	CleanupList stdCleanups( QObject * parent );
    };

}	// namespace QDirStat


#endif // ifndef StdCleanup_h

