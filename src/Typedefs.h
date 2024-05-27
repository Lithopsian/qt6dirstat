/*
 *   File name: FileSize.h
 *   Summary:   Basic typedefs for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Author:    Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */

#ifndef Typedefs_h
#define Typedefs_h

#include "limits.h" // LLONG_MAX

#include <QColor>


// The size of a standard disk block.
//
// Notice that this is different from st_blksize in the struct that the stat()
// syscall returns, yet it is the reference unit for st_blocks in that same
// struct.
#define STD_BLOCK_SIZE 512L

#define FileSizeMax   LLONG_MAX
// 0x7FFFFFFFFFFFFFFFLL == 9223372036854775807LL


namespace QDirStat
{
    using FileSize = long long;

    using ColorList = QList<QColor>;

    class MountPoint;
    using MountPointList = QList<MountPoint *>;
}


#endif  // Typedefs_h
