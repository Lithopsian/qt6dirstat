/*
 *   File name: Wildcard.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Author:    Ian Nartowicz (backported from Qt)
 */

#include "Wildcard.h"


using namespace QDirStat;


#if QT_VERSION < QT_VERSION_CHECK( 6, 6, 0 )
QString Wildcard::wildcardToRegularExpression( const QString & pattern,
                                               QDirStat::WildcardConversionOptions options)
{
    const qsizetype wclen = pattern.size();
    QString rx;
    rx.reserve( wclen + wclen / 16 );
    qsizetype i = 0;
    const QChar * wc = pattern.data();

    struct GlobSettings
    {
        char16_t nativePathSeparator;
        QString starEscape;
        QString questionMarkEscape;
    };

    const GlobSettings settings = [ options ]() {
        if ( options.testFlag( NonPathWildcardConversion ) )
        {
            // using [\d\D] to mean "match everything";
            // dot doesn't match newlines, unless in /s mode
            return GlobSettings{ u'\0', QStringLiteral( u"[\\d\\D]*" ), QStringLiteral( u"[\\d\\D]" ) };
        }
        else
        {
#ifdef Q_OS_WIN
            return GlobSettings{ u'\\', QStringLiteral( u"[^/\\\\]*" ), QStringLiteral( u"[^/\\\\]" ) };
#else
            return GlobSettings{ u'/', QStringLiteral( u"[^/]*" ), QStringLiteral( u"[^/]" ) };
#endif
        }
    }();

    while ( i < wclen )
    {
        const QChar c = wc[ i++ ];
        switch ( c.unicode() )
        {
            case '*':
                rx += settings.starEscape;
                break;

            case '?':
                rx += settings.questionMarkEscape;
                break;

            // When not using filepath globbing: \ is escaped, / is itself
            // When using filepath globbing:
            // * Unix: \ gets escaped. / is itself
            // * Windows: \ and / can match each other -- they become [/\\] in regexp
            case '\\':
#ifdef Q_OS_WIN
                if ( options.testFlag( NonPathWildcardConversion ) )
                    rx += u"\\\\";
                else
                    rx += u"[/\\\\]";
                break;
            case '/':
                if ( options.testFlag( NonPathWildcardConversion ) )
                    rx += u'/';
                else
                    rx += u"[/\\\\]";
                break;
#endif

            case '$':
            case '(':
            case ')':
            case '+':
            case '.':
            case '^':
            case '{':
            case '|':
            case '}':
                rx += u'\\';
                rx += c;
                break;

            case '[':
                rx += c;
                // Support for the [!abc] or [!a-c] syntax
                if ( i < wclen )
                {
                    if ( wc[ i ] == u'!' )
                    {
                        rx += u'^';
                        ++i;
                    }

                    if ( i < wclen && wc[ i ] == u']' )
                        rx += wc[ i++ ];

                    while ( i < wclen && wc[ i ] != u']' )
                    {
                        if ( !options.testFlag( NonPathWildcardConversion ) )
                        {
                            // The '/' appearing in a character class invalidates the
                            // regular expression parsing. It also concerns '\\' on
                            // Windows OS types.
                            if ( wc[ i ] == u'/' || wc[ i ] == settings.nativePathSeparator )
                                return rx;
                        }
                        if ( wc[ i ] == u'\\' )
                            rx += u'\\';
                        rx += wc[ i++ ];
                    }
                }
                break;

            default:
                rx += c;
                break;
        }
    }

    if (!(options & UnanchoredWildcardConversion))
        rx = anchoredPattern(rx);

    return rx;
}
#endif
