/*
 *   File name: Typedefs.h
 *   Summary:   Basic typedefs for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Author:    Ian Nartowicz
 */

#ifndef Typedefs_h
#define Typedefs_h

#include <limits>

#include <QColor>
#include <QTextStream> // endl


// The size of a standard disk block. This may be different from st_blksize
// in the stat struct, even though st_blocks is normally calculated based
// a 512-byte block.
#define STD_BLOCK_SIZE 512L

#define FileSizeMax  std::numeric_limits<qint64>::max() // ~1 billion TB!
#define DirSizeMax   std::numeric_limits<qint32>::max() // QAbstractItemModel limit
#define FileCountMax std::numeric_limits<qint32>::max() // ~2 billion

#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
#define asConst(OBJECT) const_cast<std::add_const<decltype( OBJECT )>::type &>( OBJECT )
#else
#define asConst std::as_const
#endif

namespace QDirStat
{
    using FileSize  = qint64;
    using DirSize   = qint32;
    using FileCount = qint32;

    using ColorList = QVector<QColor>;

    /**
     * Provide a qreal literal suffix.  qreal is not always
     * a double (although it almost always is).
     **/
    constexpr inline qreal operator""_qr( long double x ) noexcept
        { return static_cast<qreal>( x ); }
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
                { return QLatin1String{ str, int( size ) }; }
        }
    }
#endif
}

// So everyone gets _L1 automatically
using namespace Qt::Literals::StringLiterals;

#endif  // Typedefs_h
