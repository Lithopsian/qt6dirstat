/*
 *   File name: FormatUtil.h
 *   Summary:   String formatting utilities for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef FormatUtil_h
#define FormatUtil_h

#include <cmath>       // llround()
#include <sys/types.h> // mode_t
#include <sys/stat.h>  // ALLPERMS, S_IRUSR, S_ISUID, etc

#include <QFontMetrics>
#include <QLocale>
#include <QStringBuilder>

#include "Typedefs.h" // FileSize


#ifndef ALLPERMS
#  define ALLPERMS 07777

// Uncomment for debugging:
// #  warning "Using ALLPERMS replacement"

// Not available in musl-libc used on Gentoo:
//
//   https://github.com/shundhammer/qdirstat/issues/187
//
// Original from Linux / glibc /usr/include/x86_64-linux-gnu/sys/stat.h :
//
//   #define ALLPERMS (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO)/* 07777 */
//
// But that might induce more complaints because any of S_IRWXU, S_IRWXG
// etc. may also not be defined on such a system. So let's keep it simple.
// If they also use a different bit pattern for those permissions, that's their
// problem.
#endif


class QLabel;


namespace QDirStat
{
    /**
     * Format simple numbers according to the locale conventions.
     * Overloaded for integer and floating point numeric types.
     **/
    inline QString formatCount( qint64 size )
        { return QString{ "%L1" }.arg( size ); }
    inline QString formatCount( double size, int precision )
        { return QString{ "%L1" }.arg( size, 0, 'f', precision ); }

    /**
     * Standard strings for formatting file sizes.  More complex
     * prefixed byte units are handled inside formatSize().
     **/
    inline QString oneB()    { return QObject::tr( "B"      ); }
    inline QString oneByte() { return QObject::tr( "1 byte" ); }
    inline QString bytes()   { return QObject::tr( "bytes"  ); }

    /**
     * Format a file / subtree size human readable, i.e. in "GB" / "MB"
     * etc. rather than huge numbers of digits. 'precision' is the number of
     * digits after the decimal point.
     **/
    QString formatSize( FileSize size, int precision );

    /**
     * Can't use a default argument when using formatSize() as a function
     * pointer, so provide an overloaded version (although it isn't used
     * as a function pointer any more).
     **/
    inline QString formatSize( FileSize size )    { return formatSize( size, 1 ); }
    inline QString formatSize( long double size ) { return formatSize( std::llround( size ), 1 ); }

    /**
     * Format a file / subtree size as bytes, but still human readable with a
     * thousands separator.
     **/
    inline QString formatByteSize( FileSize size )
	{ return size == 1 ? oneByte() : QString{ "%L1 " }.arg( size ) % bytes(); }
    inline QString formatByteSize( long double size, int precision )
	{ return QString{ "%L1 " }.arg( static_cast<double>( size ), 0, 'f', precision ) % bytes(); }

    /**
     * Format a file size string with no thousands separators and "B" for the units.
     * This is only intended for small values, typically less than 1,000.
     **/
    inline QString formatShortByteSize( FileSize size )
	{ return QString::number( size ) % oneB(); }

    /**
     * Format a string of the form "/ 3 links" for describing hard links.  If the
     * number of links is less than 2, an empty string is returned.
     **/
    inline QString formatLinksInline( nlink_t numLinks )
	{ return numLinks > 1 ? QObject::tr( " / %1 links" ).arg( numLinks) : QString{}; }

    /**
     * Format a string of the form "<br/>3 links" for describing hard links on a
     * separate line, typically in a tooltip.  If the number of links is less than 2,
     * an empty string is returned.
     **/
    inline QString formatLinksRichText( nlink_t numLinks )
	{ return numLinks > 1 ? QObject::tr( "<br/>%1 hard links" ).arg( numLinks ) : QString{}; }

    /**
     * Wraps the text in html formatting to prevent line breaks except at explicit
     * newlines and break tags.
     **/
    inline QString whitespacePre( const QString & text )
	{ return "<p style='white-space:pre'>"_L1 % text % "</p>"_L1; }

    /**
     * Format a timestamp (like the latestMTime()) human-readable.
     **/
    QString formatTime( time_t rawTime );

    /**
     * Format a millisecond-based time
     **/
    QString formatMillisec( qint64 millisec );

    /**
     * Format a percentage.
     **/
    inline QString formatPercent( float percent )
	{ return percent < 0.0f ? QString{} : QString::number( percent, 'f', 1 ) % '%'; }

    /**
     * Return the mode (the permission bits) returned from stat() like the
     * "ls -l" shell command does, e.g.
     *
     *	   drwxr-xr-x
     **/
    QString symbolicMode( mode_t perm );

    /**
     * Format a number in octal with a leading zero.
     **/
    inline QString formatOctal( int number )
	{ return '0' % QString::number( number, 8 ); }

    /**
     * Format a file stat mode as octal.
     **/
    inline QString octalMode( mode_t mode )
	{ return formatOctal( ALLPERMS & mode ); }

    /**
     * Format the mode (the permissions bits) returned from the stat() system
     * call in the commonly used formats, both symbolic and octal, e.g.
     *	   drwxr-xr-x  0755
     **/
    inline QString formatPermissions( mode_t mode )
	{ return symbolicMode( mode ) % "  "_L1 % formatOctal( ALLPERMS & mode ); }

    /**
     * Returns the string resized to the given width and padded with
     * non-breaking spaces.
     **/
    inline QString pad( const QString & string, int width )
	{ return string.leftJustified( width, QChar( 0x00A0 ) ); }

    /**
     * Returns a three-letter abbreviation for the requested month.
     **/
    QString monthAbbreviation( short month );

    /**
     * Returns the height in pixels of the given font.
     **/
    inline int fontHeight( const QFont & font )
	{ return QFontMetrics{ font }.height(); }

    /**
     * Returns the width in pixels of the given text rendered using
     * the given font.
     **/
    inline int textWidth( const QFont & font, const QString & text )
	{ return QFontMetrics{ font }.boundingRect( text ).width(); }

    /**
     * Elide a string, removing characters from the middle to fit
     * with maxLen characters.
     **/
    QString elideMiddle( const QString & text, int maxLen );

    /**
     * Elide the text to fit within 'maxSize' using the label widget font
     * and place it in the label.
     **/
    void elideLabel( QLabel * label, const QString & text, int maxSize );

    /**
     * Human-readable output of a file size in a debug stream.
     *
     * Removed because the overload of is ambiguous between FileSize
     * and qsizetype (and potentially other long long ints).  Use
     * formatSize() explicitly if you need this.
     **/
//    inline QTextStream & operator<<( QTextStream & stream, FileSize lSize )
//	{ return stream << formatSize( lSize ); }

}       // namespace QDirStat

#endif  // FormatUtil_h
