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

#include <sys/types.h>
#include <sys/stat.h>       // ALLPERMS

#include <QFontMetrics>
#include <QLabel>
#include <QObject>
#include <QStringBuilder>
#include <QTextStream>

#include "FileSize.h"


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


namespace QDirStat
{
    /**
     * Can't use a default argument when using this as a function pointer,
     * so we really need the above overloaded version.
     **/
    QString formatSize( FileSize size, int precision );

    /**
     * Format a file / subtree size human readable, i.e. in "GB" / "MB"
     * etc. rather than huge numbers of digits. 'precision' is the number of
     * digits after the decimal point.
     *
     * Note: For logDebug() etc., operator<< is overwritten to do exactly that:
     *
     *	   logDebug() << "Size: " << x->totalSize() << Qt::endl;
     **/
    inline QString formatSize( FileSize size ) { return formatSize( size, 1 ); }

    /**
     * Format a file / subtree size as bytes, but still human readable with a
     * thousands separator.
     **/
    inline QString formatByteSize( FileSize size )
	{ return QObject::tr( "%L1 bytes" ).arg( size ); }

    /**
     * Format a file size string with no thousands separators and "B" for the units.
     * This is only intended for small values, typically less than 1,000.
     **/
    inline QString formatShortByteSize( FileSize size )
	{ return QString::number( size ) % " B"; }

    /**
     * Format a string of the form "/ 3 links" for describing hard links.  If the
     * number of links is less than 2, an empty string is returned.
     **/
    inline QString formatLinksInline( nlink_t numLinks )
	{ return numLinks > 1 ? QString( " / %1 links" ).arg( numLinks) : QString(); }

    /**
     * Format a string of the form "<br/>3 links" for describing hard links on a
     * separate line, typically in a tooltip.  If the number of links is less than 2,
     * an empty string is returned.
     **/
    inline QString formatLinksRichText( nlink_t numLinks )
	{ return numLinks > 1 ? QString( "<br/>%1 hard links" ).arg( numLinks ) : QString(); }

    /**
     * Wraps the text in html formatting to prevent line breaks except at explicit
     * newlines and break tags.
     **/
    inline QString whitespacePre( const QString & text )
	{ return "<p style='white-space:pre'>" % text % "</p>"; }

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
        { return percent < 0.0 ? QString() : QString( QString::number( percent, 'f', 1 ) % '%' ); }

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
	{ return symbolicMode( mode ) % "  " % formatOctal( ALLPERMS & mode ); }

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
	{ return QFontMetrics( font ).height(); }

    /**
     * Returns the width in pixels of the given text rendered using
     * the given font.
     **/
    inline int textWidth( const QFont & font, const QString & text )
#if QT_VERSION < QT_VERSION_CHECK( 5, 11, 0 )
	{ return QFontMetrics( font ).width( text ); }
#else
	{ return QFontMetrics( font ).horizontalAdvance( text ); }
#endif

    /**
     * Elide a long string, remnoving characters from the middle to fit
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
//    inline QTextStream & operator<< ( QTextStream & stream, FileSize lSize )
//	{ return stream << formatSize( lSize ); }

}       // namespace QDirStat

#endif  // FormatUtil_h
