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

#include <QAbstractItemModel>
#include <QFontMetrics>
#include <QLabel>
#include <QRegularExpression>
#include <QStringBuilder>
#include <QTreeView>
#include <QTreeWidgetItem>

#include "Typedefs.h" // FileSize, _L1


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
     *           drwxr-xr-x
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
     *           drwxr-xr-x  0755
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
     * Returns whether 'test' is upper- or lower-case.  Upper case is
     * defined as any string which is the same as toUpper() of that
     * string.
     *
     * Before Qt 5.12, these functions do not exist in QString, so a
     * simplified check is implemented for compatibility.
     *
     * Qt versions before 5.14 used a different definition, where
     * isUpper() was only true if the string was not empty and
     * contained only upper-case letters, so don't use those.
     **/
#if QT_VERSION < QT_VERSION_CHECK( 5, 14, 0 )
    inline bool isLower( const QString & test )
    {
        for ( QChar testChar : test )
        {
            if ( testChar.isUpper() )
                return false;
        }
        return true;
    }
    inline bool isUpper( const QString & test )
    {
        for ( QChar testChar : test )
        {
            if ( testChar.isLower() )
                return false;
        }
        return true;
    }
#else
    inline bool isLower( const QString & test )
        { return test.isLower(); }
    inline bool isUpper( const QString & test )
        { return test.isUpper(); }
#endif

    /**
     * Returns the height in pixels of the given font.
     **/
    inline int fontHeight( const QFont & font )
        { return QFontMetrics{ font }.height(); }

    /**
     * Returns the width in pixels of the given text rendered using
     * the given font.  The two functions return subtly different
     * results: textWidth returns the size of the visible dark pixels
     * of 'text', while horizontalAdvance returns the distance from
     * the "start" of 'text' to the "start" of the character that
     * would follow it.  In particular, a space character returns
     * zero textWidth and the textWidth of italics may be wider than
     * the horizontal advance.
     **/
    inline int textWidth( const QFont & font, const QString & text )
        { return QFontMetrics{ font }.boundingRect( text ).width(); }
    inline int horizontalAdvance( const QFont & font, const QString & text )
#if QT_VERSION < QT_VERSION_CHECK( 5, 11, 0 )
        { return QFontMetrics{ font }.width( text ); }
#else
        { return QFontMetrics{ font }.horizontalAdvance( text ); }
#endif
    inline int horizontalAdvance( const QFont & font, QChar text )
#if QT_VERSION < QT_VERSION_CHECK( 5, 11, 0 )
        { return QFontMetrics{ font }.width( text ); }
#else
        { return QFontMetrics{ font }.horizontalAdvance( text ); }
#endif

    /**
     * Returns whether 'text' contains a carriage return or linefeed
     * character.
     **/
    inline bool hasLineBreak( const QString & text )
        { return text.contains( u'\n' ) || text.contains( u'\r' ); }

    /**
     * Return a copy of 'text' with carriage return and linefeed
     * characters replaced by spaces.  Despite the apparent
     * unconditional string copy, implicit sharing makes this
     * reasonable fast for the 99.99% of cases where there are no
     * CR/LF characters.
     *
     * There is also a function to replace ampersands and horizontal
     * tab characters, necessary for text to be displayed in a menu.
     **/
    QString replaceCrLf( const QString & text );

    /**
     * Replace ampersands and horizontal tab characters in 'text' and
     * return a reference to it.  This used for menus, where a single
     * ampersand will be interpreted as a mnemonic and underlined,
     * and a horizontal tab is used internally to delimit columns in
     * a menu item.
     **/
    inline QString & replaceAmpTab( QString & text )
        { return text.replace( u'\t', u' ' ).replace( u'&', "&&"_L1 ); }

    /**
     * Return whether 'text' contains any Unicode control characters.
     **/
    inline bool hasControlCharacter( const QString & text )
        { return QRegularExpression{ "\\p{C}" }.match( text ).hasMatch(); }

    /**
     * Return a regular expression matching any string that doesn't
     * include Unicode control characters.  It is intended for use
     * with QValidator so the regular expression itself is not
     * anchored.
     **/
    inline QRegularExpression hasNoControlCharacters()
        { return QRegularExpression{ "\\P{C}*" }; }

    /**
     * Return 'text', elided if necessary to fit 'maxSize' pixels
     * wide when rendered in 'font'.
     **/
    inline QString elidedText( const QFont       & font,
                               const QString     & text,
                               int                 maxSize )
        { return QFontMetrics{ font }.elidedText( text, Qt::ElideMiddle, maxSize ); }

    /**
     * Return the width of an ellipsis character in 'font'.
     **/
    inline int ellipsisWidth( const QFont & font )
        { return horizontalAdvance( font, u'…' ); }

    /**
     * Return the indent between a label frame and the text.
     **/
    inline int labelFrameIndent( const QFont & font )
        { return horizontalAdvance( font, u'x' ) / 2; }

    /**
     * Returns a reference to 'path', possibly modified.
     *
     * Zero-width spaces are inserted at regular intervals to allow
     * long paths to line-break naturally even if they don't contain
     * characters that would normally allow a line-break.
     *
     * If (very unusually) 'path' would be treated as rich text in
     * a label or tooltip, then all '<' characters are modified to
     * prevent them being identified as html tags.  Tooltips shown
     * as (html-escaped) rich text wrap at a very narrow width,
     * unsuitable for displaying paths and inconsistent with paths
     * that are shown as plain text.  This is usually unnecessary
     * for labels, which can be forced to display as plain text.
     **/
    QString & pathTooltip( QString & path );

    /**
     * Elide 'text' to fit between the start position of 'label' and
     * 'lastPixel', generally the end position of the parent minus any
     * margin.
     **/
    void elideLabel( QLabel * label, const QString & text, int lastPixel );

    /**
     * Resize the columns of 'tree'.  First, attempt to resize all
     * columns to fit their contents.  This will generally succeed,
     * but may leave the tree wider than the viewport.  Next, resize
     * the first column with a hard minimum size of its header width,
     * and then stretch the first column as much as possible from its
     * minimum size.  Use the smaller of those two widths and leave
     * the columns in interactive resize mode so the user can access
     * any text that is still ellipsized.
     **/
    void resizeTreeColumns( QTreeView * tree );

    /**
     * Compare the size of a tree item with the section width.  If
     * the item is wider than the section then a tooltip should be
     * shown.
     *
     * Two overloads are provided: the first calculates the
     * item and section widths for 'column' in 'item' and returns
     * either an empty string or a string to be displayed as the
     * tooltip; the second compares 'sizeHint' and 'visualRect' and
     * shows a tooltip itself from the display role for 'index' in
     * 'model'.
     **/
    QString tooltipForElided( const QTreeWidgetItem * item, int column, int treeLevel );
    void tooltipForElided( QRect                      visualRect,
                           QSize                      sizeHint,
                           const QAbstractItemModel * model,
                           const QModelIndex        & index,
                           QPoint                     pos );

    /**
     * Elide 'label' with the text stored in a standardised way on
     * the statusTip property.  The label is expected to be within
     * 'container'.
     **/
    void showElidedLabel( QLabel * label, const QWidget * container );

    /**
     * Human-readable output of a file size in a debug stream.
     *
     * Removed because this overload is ambiguous between FileSize and
     * qsizetype (and potentially other long long ints).  Use
     * formatSize() explicitly if you need this.
     **/
//    inline QTextStream & operator<<( QTextStream & stream, FileSize lSize )
//        { return stream << formatSize( lSize ); }

} // namespace QDirStat

#endif // FormatUtil_h
