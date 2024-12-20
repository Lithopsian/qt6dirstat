/*
 *   File name: SearchFilter.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef SearchFilter_h
#define SearchFilter_h

#include <QRegularExpression>
#include <QTextStream>


namespace QDirStat
{
    /**
     * Base class for search filters like PkgFilter or FileSearchFilter.
     **/
    class SearchFilter
    {
    public:

        enum FilterMode
        {
          Auto,       // Guess from pattern (see below)
          Contains,   // Fixed string
          StartsWith, // Fixed string
          ExactMatch, // Fixed string
          WildcardMode,
          RegExp,
          SelectAll   // Pattern is irrelevant
        };


        /**
         * Constructor: Create a search filter with the specified pattern and
         * filter mode.
         *
         * Filter mode "Auto" tries to guess a useful mode from the pattern:
         *
         * - If it's a fixed string without any wildcards, it uses
         *   'defaultFilterMode'.
         * - If it contains "*" wildcard characters, it uses "Wildcard".
         * - If it contains ".*" or "^" or "$", it uses "RegExp".
         * - If it starts with "=", it uses "ExactMatch".
         * - If it's empty, it uses "SelectAll".
         **/
        SearchFilter( const QString & pattern,
                      FilterMode      filterMode,
                      FilterMode      defaultFilterMode,
                      bool            caseSensitive );

        /**
         * Check if a string matches this filter.
         **/
        bool matches( const QString & str ) const;

        /**
         * Return the pattern.
         **/
        const QString & pattern() const { return _pattern; }

        /**
         * Return the regexp. This is only meaningful in filter modes RegExp
         * and Wildcard.
         **/
//        const QRegularExpression & regexp() const { return _regexp; }

        /**
         * Return the filter mode.
         **/
        FilterMode filterMode() const { return _filterMode; }

        /**
         * Return 'true' if the matching is case sensitive, 'false if not.
         **/
        bool isCaseSensitive() const { return _caseSensitive; }

        /**
         * Convert a filter mode to a string.  Only used for logging stream
         * operators in the various classes derived from this one.
         **/
        static QString toString( FilterMode filterMode )
        {
            switch ( filterMode )
            {
                case Contains:     return "Contains";
                case StartsWith:   return "StartsWith";
                case ExactMatch:   return "ExactMatch";
                case WildcardMode: return "Wildcard";
                case RegExp:       return "Regexp";
                case SelectAll:    return "SelectAll";
                case Auto:         return "Auto";
            }

            return QString{ "<Unknown FilterMode %1" }.arg( filterMode );
        }


    private:

        QString            _pattern;
        QRegularExpression _regexp;
        FilterMode         _filterMode;
        FilterMode         _defaultFilterMode;
        bool               _caseSensitive;

    };  // class SearchFilter



    inline QTextStream & operator<<( QTextStream        & stream,
                                     const SearchFilter & filter )
    {
        stream << "<SearchFilter \""
               << filter.pattern()
               << "\" mode \""
               << SearchFilter::toString( filter.filterMode() )
               << "\" "
               << ( filter.isCaseSensitive()? " case sensitive>" : ">" );

        return stream;
    }

}       // namespace QDirStat

#endif  // SearchFilter_h
