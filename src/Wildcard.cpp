/*
 *   File name: Wildcard.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Author:    Ian Nartowicz (backported from Qt)
 */

#include "Wildcard.h"
#include "FileInfo.h"
#include "MimeCategorizer.h"


using namespace QDirStat;


#if QT_VERSION < QT_VERSION_CHECK( 6, 6, 0 )
QString Wildcard::wildcardToRegularExpression( const QString & pattern,
                                               QDirStat::WildcardConversionOptions options)
{
    const auto wclen = pattern.size();
    QString rx;
    rx.reserve( wclen + wclen / 16 );
    const QChar * wc = pattern.data();

    struct GlobSettings
    {
        char16_t nativePathSeparator;
        QLatin1String starEscape;
        QLatin1String questionMarkEscape;
    };

    const GlobSettings settings = [ options ]() {
        if ( options.testFlag( NonPathWildcardConversion ) )
        {
            // using [\d\D] to mean "match everything";
            // dot doesn't match newlines, unless in /s mode
            return GlobSettings{ u'\0', "[\\d\\D]*"_L1, "[\\d\\D]"_L1 };
        }
        else
        {
#ifdef Q_OS_WIN
            return GlobSettings{ u'\\', "[^/\\\\]*"_L1, "[^/\\\\]"_L1 };
#else
            return GlobSettings{ u'/', "[^/]*"_L1, "[^/]"_L1 };
#endif
        }
    }();

    auto i = 0;
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
                    rx += "\\\\"_L1;
                else
                    rx += "[/\\\\]"_L1;
                break;
            case '/':
                if ( options.testFlag( NonPathWildcardConversion ) )
                    rx += u'/';
                else
                    rx += "[/\\\\]"_L1;
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

    if  ( !( options & UnanchoredWildcardConversion ) )
        rx = anchoredPattern( rx );

    return rx;
}
#endif



bool WildcardCategory::matches( const FileInfo * item ) const
{
    // We only deal with regular files and symlinks
    if ( !item->isFileOrSymlink() )
	return false;

    // If there is a Wildcard pattern, but it doesn't match ...
    const QString & pattern = wildcard.pattern();
    if ( !pattern.isEmpty() && !wildcard.isMatch( item->name() ) )
	return false;

    // Re-categorise any item matching the wildcard so we can compare the actual categoriser match
    QString matchPattern;
    bool matchCaseInsensitive;
    const MimeCategory * matchCategory =
	MimeCategorizer::instance()->category( item, matchPattern, matchCaseInsensitive );

    // Uncategorised files match if we are looking for uncategorised files
    if ( !matchCategory && !category )
	return true;

    // Check that the categoriser result matches the one we're looking for
    const bool caseInsensitive = wildcard.patternOptions() & Wildcard::CaseInsensitiveOption;
    return matchCategory == category && matchCaseInsensitive == caseInsensitive && matchPattern == pattern;
}
