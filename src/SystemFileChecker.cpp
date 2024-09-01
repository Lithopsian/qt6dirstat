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
        return uid < (uid_t)MIN_NON_SYSTEM_UID;
    }


    bool isSystemPath( const QString & path )
    {
        if ( path.startsWith( "/boot/"_L1  ) ||
             path.startsWith( "/bin/"_L1   ) ||
             path.startsWith( "/dev/"_L1   ) ||
             path.startsWith( "/etc/"_L1   ) ||
             path.startsWith( "/lib/"_L1   ) ||
             path.startsWith( "/lib32/"_L1 ) ||
             path.startsWith( "/lib64/"_L1 ) ||
             path.startsWith( "/opt/"_L1   ) ||
             path.startsWith( "/proc/"_L1  ) ||
             path.startsWith( "/sbin/"_L1  ) ||
             path.startsWith( "/sys/"_L1   ) )
        {
            return true;
        }

        if ( path.startsWith( "/usr/"_L1 ) && !path.startsWith( "/usr/local/"_L1 ) )
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
         * Some of those might be debatable: while it is true that no mere user
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
        if ( path.contains  ( "/lost+found/"_L1 ) ||   // Also on other mounted filesystems!
             path.startsWith( "/run/"_L1        ) ||
             path.startsWith( "/srv/"_L1        ) ||
             path.startsWith( "/var/"_L1        ) )
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

    if ( file->parent() && file->parent()->url() == "/"_L1 )
        return true;

    QString path = file->url();
    if ( file->isDir() )
        path += u'/';
    if ( isSystemPath( path ) )
        return true;

    if ( file->hasUid() && isSystemUid( file->uid() ) && mightBeSystemPath( path ) )
        return true;

    return false;
}
