/*
 *   File name: SearchFilter.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "SearchFilter.h"
#include "Logger.h"
#include "Wildcard.h"


using namespace QDirStat;


namespace
{
    SearchFilter::FilterMode guessFilterMode( QString & pattern, SearchFilter::FilterMode defaultFilterMode )
    {
        if ( pattern.isEmpty() )
            return SearchFilter::SelectAll;

        if ( pattern.startsWith( u'=' ) )
        {
            pattern.remove( 0, 1 );
            return SearchFilter::ExactMatch;
        }

        if ( pattern.startsWith( u'*' ) || pattern.contains( "*.*"_L1 ) )
            return SearchFilter::WildcardMode;

        if ( pattern.contains( ".*"_L1 ) ||
             pattern.contains( u'^'    ) ||
             pattern.contains( u'$'    ) ||
             pattern.contains( u'('    ) ||
             pattern.contains( u'|'    ) ||
             pattern.contains( u'['    ) )
        {
            return SearchFilter::RegExp;
        }

        if ( pattern.contains( u'*' ) || pattern.contains( u'?' ) )
            return SearchFilter::WildcardMode;

        if ( defaultFilterMode == SearchFilter::Auto )
            return SearchFilter::StartsWith;

        return defaultFilterMode;
    }

} // namespace


SearchFilter::SearchFilter( const QString & pattern,
                            FilterMode      filterMode,
                            FilterMode      defaultFilterMode,
                            bool            caseSensitive ):
    _pattern{ pattern },
    _filterMode{ filterMode },
    _defaultFilterMode{ defaultFilterMode },
    _caseSensitive{ caseSensitive }
{
    if ( _filterMode == Auto )
    {
        _filterMode = guessFilterMode( _pattern, _defaultFilterMode );
#if 0
            logDebug() << "using filter mode " << toString( _filterMode ) << " from \"" << _pattern << "\"" << Qt::endl;
#endif
    }

    QRegularExpression::PatternOptions patternOptions;
    if ( !caseSensitive )
        patternOptions = QRegularExpression::CaseInsensitiveOption;

    if ( _filterMode == WildcardMode )
        _regexp = Wildcard::wildcardRegularExpression( pattern, patternOptions );
    else if ( _filterMode == RegExp )
        _regexp = QRegularExpression{ pattern, patternOptions };

    // Make an attempt to recover from guessing an invalid regexp
    if ( filterMode == Auto && _filterMode == RegExp && !_regexp.isValid() )
        _filterMode = StartsWith;
}


bool SearchFilter::matches( const QString & str ) const
{
    const Qt::CaseSensitivity caseSensitivity = isCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive;

    switch ( _filterMode )
    {
        case Contains:     return str.contains  ( _pattern, caseSensitivity );
        case StartsWith:   return str.startsWith( _pattern, caseSensitivity );
        case ExactMatch:   return QString::compare( str, _pattern, caseSensitivity ) == 0;
        case WildcardMode: return _regexp.match( str ).hasMatch();
        case RegExp:       return _regexp.isValid() && _regexp.match( str ).hasMatch();
        case SelectAll:    return true;
        case Auto:
            logWarning() << "Unexpected filter mode 'Auto' - assuming 'Contains'" << Qt::endl;
            return str.contains( _pattern, caseSensitivity );
    }

    logError() << "Undefined filter mode " << (int)_filterMode << Qt::endl;

    return false;
}
