/*
 *   File name: SearchFilter.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "SearchFilter.h"
#include "Wildcard.h"
#include "Logger.h"


using namespace QDirStat;


SearchFilter::SearchFilter( const QString & pattern,
                            FilterMode      filterMode,
                            FilterMode      defaultFilterMode,
                            bool            caseSensitive ):
    _pattern { pattern },
    _filterMode { filterMode },
    _defaultFilterMode { defaultFilterMode },
    _caseSensitive { caseSensitive }
{
    if ( _filterMode == Auto )
        guessFilterMode();

    QRegularExpression::PatternOptions patternOptions;
    if ( !caseSensitive )
        patternOptions = QRegularExpression::CaseInsensitiveOption;

    if ( _filterMode == Wildcard )
        _regexp = Wildcard::wildcardRegularExpression( pattern, patternOptions );
    else if ( _filterMode == RegExp )
        _regexp = QRegularExpression( pattern, patternOptions );

    // Make an attempt to recover from guessing an invalid regexp
    if ( filterMode == Auto && _filterMode == RegExp && !_regexp.isValid() )
        _filterMode = StartsWith;
}


void SearchFilter::guessFilterMode()
{
    if ( _pattern.isEmpty() )
    {
        _filterMode = SelectAll;
    }
    else if ( _pattern.startsWith( '=' ) )
    {
        _filterMode = ExactMatch;
        _pattern.remove( 0, 1 );
    }
    else if ( _pattern.startsWith( '*' ) ||
              _pattern.contains( QLatin1String( "*.*" ) ) )
    {
        _filterMode = Wildcard;
    }
    else if ( _pattern.contains( QLatin1String( ".*" ) ) ||
              _pattern.contains( '^' ) ||
              _pattern.contains( '$' ) ||
              _pattern.contains( '(' ) ||
              _pattern.contains( '|' ) ||
              _pattern.contains( '[' ) )
    {
        _filterMode = RegExp;
    }
    else if ( _pattern.contains( '*' ) ||
              _pattern.contains( '?' ) )
    {
        _filterMode = Wildcard;
    }
    else if ( _defaultFilterMode == Auto )
    {
        _filterMode = StartsWith;
    }
    else
    {
        _filterMode = _defaultFilterMode;
    }

#if 0
    logDebug() << "using filter mode " << toString( _filterMode ) << " from \"" << _pattern << "\"" << Qt::endl;
#endif
}


bool SearchFilter::matches( const QString & str ) const
{
    const Qt::CaseSensitivity caseSensitivity = isCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive;

    switch ( _filterMode )
    {
        case Contains:   return str.contains  ( _pattern, caseSensitivity );
        case StartsWith: return str.startsWith( _pattern, caseSensitivity );
        case ExactMatch: return QString::compare( str, _pattern, caseSensitivity ) == 0;
        case Wildcard:   return _regexp.match( str ).hasMatch();
        case RegExp:     return _regexp.isValid() && _regexp.match( str ).hasMatch();
        case SelectAll:  return true;
        case Auto:
            logWarning() << "Unexpected filter mode 'Auto' - assuming 'Contains'" << Qt::endl;
            return str.contains( _pattern, caseSensitivity );
    }

    logError() << "Undefined filter mode " << (int)_filterMode << Qt::endl;

    return false;
}

/*
void SearchFilter::setCaseSensitive( bool sensitive )
{
    _caseSensitive = sensitive;

    if ( _regexp.isValid() )
    {
        QRegularExpression::PatternOptions options = _regexp.patternOptions();
        _regexp.setPatternOptions( options.setFlag( QRegularExpression::CaseInsensitiveOption, !sensitive ) );
    }
}
*/
