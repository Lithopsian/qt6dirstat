/*
 *   File name: FileSize.h
 *   Summary:   Basic typedefs for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Author:    Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */

#ifndef Typedefs_h
#define Typedefs_h

#include <limits> // LLONG_MAX

#include <QColor>
#include <QList>
#include <QTextStream> // endl

// The size of a standard disk block.
//
// Notice that this is different from st_blksize in the struct that the stat()
// syscall returns, yet it is the reference unit for st_blocks in that same
// struct.
#define STD_BLOCK_SIZE 512L

#define FileSizeMax   LLONG_MAX
// 0x7FFFFFFFFFFFFFFFLL == 9223372036854775807LL

#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
#define asConst(OBJECT) const_cast<std::add_const<decltype( OBJECT )>::type &>( OBJECT )
#else
#define asConst std::as_const
#endif

namespace QDirStat
{
    using FileSize = long long;

    using ColorList = QList<QColor>;

    class MountPoint;
    using MountPointList = QList<MountPoint *>;

    /**
     * Provide a qreal literal suffix.  qreal is not always
     * a double (although it almost always is).
     **/
    constexpr qreal operator""_qr( long double x )
        { return qreal( x ); }
}

namespace Qt
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    const static auto endl = ::endl;
    const static auto SkipEmptyParts = QString::SkipEmptyParts;
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
    inline namespace Literals
    {
        inline namespace StringLiterals
        {
            constexpr inline QLatin1String operator""_L1( const char * str, size_t size ) noexcept
                { return QLatin1String( str, int( size ) ); }
        }
    }
#endif
}

// So everyone gets _L1 automatically
using namespace Qt::Literals::StringLiterals;

#endif  // Typedefs_h
