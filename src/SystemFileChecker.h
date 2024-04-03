/*
 *   File name: SystemFileChecker.h
 *   Summary:	Support classes for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#ifndef SystemFileChecker_h
#define SystemFileChecker_h


#include <QString>


namespace QDirStat
{
    class FileInfo;

    /**
     * Check functions to find out if a file is a system file.
     *
     * This might be a bit Linux-centric. It will work on other Unix-type
     * system, but it might not be too reliable on other systems like MacOS X.
     **/
    namespace SystemFileChecker
    {
        /**
         * Return 'true' if a file is clearly a system file.
         **/
        bool isSystemFile( const FileInfo * file );

    }; // SystemFileChecker
}

#endif // SystemFileChecker_h
