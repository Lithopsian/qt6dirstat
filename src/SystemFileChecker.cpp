/*
 *   File name: SystemFileChecker.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <sys/types.h> // uid_t

#include "SystemFileChecker.h"
#include "DirInfo.h"


#define MIN_NON_SYSTEM_UID      500


using namespace QDirStat;


namespace
{
    bool isSystemUid( uid_t uid )
    {
        return uid < (uid_t) MIN_NON_SYSTEM_UID;
    }


    bool isSystemPath( const QString & path )
    {
        if ( path.startsWith( QLatin1String( "/boot/"  ) ) ||
             path.startsWith( QLatin1String( "/bin/"   ) ) ||
             path.startsWith( QLatin1String( "/dev/"   ) ) ||
             path.startsWith( QLatin1String( "/etc/"   ) ) ||
             path.startsWith( QLatin1String( "/lib/"   ) ) ||
             path.startsWith( QLatin1String( "/lib32/" ) ) ||
             path.startsWith( QLatin1String( "/lib64/" ) ) ||
             path.startsWith( QLatin1String( "/opt/"   ) ) ||
             path.startsWith( QLatin1String( "/proc/"  ) ) ||
             path.startsWith( QLatin1String( "/sbin/"  ) ) ||
             path.startsWith( QLatin1String( "/sys/"   ) ) )
        {
            return true;
        }

        if ( path.startsWith( QLatin1String( "/usr/" ) ) && !path.startsWith( QLatin1String( "/usr/local/" ) ) )
            return true;

        /**
         * Intentionally NOT considered true system paths:
         *
         *   /cdrom
         *   /home
         *   /lost+found
         *   /media
         *   /mnt
         *   /root
         *   /run
         *   /srv
         *   /tmp
         *   /var
         *
         * Some of those might be debatable: While it is true that no mere user
         * should mess with anything outside his home directory, some might work on
         * web projects below /srv, some might write or use software that does
         * things below /run, some might be in the process of cleaning up a mess
         * left behind by fsck below /lost+found, some may wish to clean up
         * accumulated logs and spool files and whatnot below /var.
         *
         * Of course many users might legitimately use classic removable media
         * mount points like /cdrom, /media, /mnt, and all users are free to use
         * /tmp and /var/tmp.
         **/

        return false;
    }


    bool mightBeSystemPath( const QString & path )
    {
        if ( path.contains  ( QLatin1String( "/lost+found/" ) ) ||   // Also on other mounted filesystems!
             path.startsWith( QLatin1String( "/run/"        ) ) ||
             path.startsWith( QLatin1String( "/srv/"        ) ) ||
             path.startsWith( QLatin1String( "/var/"        ) ) )
        {
            return true;
        }

        return false;
    }

} // namespace



bool SystemFileChecker::isSystemFile( const FileInfo * file )
{
    if ( !file )
        return false;

    if ( file->isPseudoDir() && file->parent() )
        file = file->parent();

    if ( file->parent() && file->parent()->url() == QLatin1String( "/" ) )
        return true;

    QString path = file->url();
    if ( file->isDir() )
        path += '/';
    if ( isSystemPath( path ) )
        return true;

    if ( file->hasUid() && isSystemUid( file->uid() ) && mightBeSystemPath( path ) )
        return true;

    return false;
}
