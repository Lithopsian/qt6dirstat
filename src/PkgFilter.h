/*
 *   File name: PkgFilter.h
 *   Summary:   Package manager Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef PkgFilter_h
#define PkgFilter_h

#include <QTextStream>

#include "SearchFilter.h"


namespace QDirStat
{
    /**
     * Class to select packages from the complete set of installed packages.
     **/
    class PkgFilter final : public SearchFilter
    {
    public:

        /**
         * Default constructor: create a package filter with an empty pattern
         * and SelectAll filter mode.
         **/
        PkgFilter():
            PkgFilter{ "", SelectAll }
        {}

        /**
         * Constructor: Create a package filter with the specified pattern and
         * filter mode. If the pattern is a package URL (i.e. if it starts with
         * "Pkg:" or "pkg:"), use only the relevant part of the URL:
         *
         * "Pkg:wantedPkg"        -> "wantedPkg"
         * "Pkg:/wantedPkg"       -> "wantedPkg"
         * "Pkg:/wantedPkg/foo"   -> "wantedPkg"
         * "Pkg:/wanted*Pkg/foo"  -> "wanted*Pkg"
         *
         * Filter mode "Auto" tries to guess a useful mode from the relevant
         * part of the URL:
         *
         * - If it's a fixed string without any wildcards, it uses
         *   "StartsWith".
         * - If it contains "*" wildcard characters, it uses "Wildcard".
         * - If it contains ".*" or "^" or "$", it uses "RegExp".
         * - If it starts with "=", it uses "ExactMatch".
         * - If it's empty (i.e. just "Pkg:/"), it uses "SelectAll".
         **/
        PkgFilter( const QString & pattern,
                   FilterMode      filterMode = Auto ):
            SearchFilter{ normalizedPattern( pattern ),
                          filterMode,
                          StartsWith, // defaultFilterMode
                          true }      // case-sensitive
        {}

        /**
         * Return the pattern without the leading "Pkg:/".
         **/
//        const QString & pattern() const { return SearchFilter::pattern(); }

        /**
         * Return the filter package URL including the leading "Pkg:/".
         **/
        QString url() const;


    protected:

        /**
         * Normalize a pattern, i.e. remove any leading "Pkg:" or "Pkg:/" and
         * any trailing part after any slashes.
         **/
        QString normalizedPattern( const QString & pattern );

    };  // class PkgFilter



    inline QTextStream & operator<<( QTextStream     & stream,
                                     const PkgFilter & filter )
    {
        stream << "<PkgFilter \""
               << filter.pattern()
               << "\" mode \""
               << SearchFilter::toString( filter.filterMode() ) << "\" "
               <<( filter.isCaseSensitive()? " case sensitive" : "" )
               << ">";

        return stream;
    }

}       // namespace QDirStat

#endif  // PkgFilter_h
