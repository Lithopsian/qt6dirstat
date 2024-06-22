/*
 *   File name: FileSearchFilter.h
 *   Summary:   Package manager Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef FileSearchFilter_h
#define FileSearchFilter_h

#include <QTextStream>

#include "SearchFilter.h"


namespace QDirStat
{
    class DirInfo;

    /**
     * Filter class for searching for files and/or directories.
     **/
    class FileSearchFilter: public SearchFilter
    {
    public:

        /** Default constructor, constructs a filter with no tree and an
         * empty search pattern.
         **/
        FileSearchFilter():
	    FileSearchFilter { nullptr, "", Auto, true, true, true, true, true }
	{}

        /**
         * Constructor: Create a search filter with the specified pattern and
         * filter mode.
         *
         * 'dir' is the directory node to start searching from.
         *
         * Filter mode "Auto" tries to guess a useful mode from the pattern:
         *
         * - If it's a fixed string without any wildcards, it uses
         *   "StartsWith".
         * - If it contains "*" wildcard characters, it uses "Wildcard".
         * - If it contains ".*" or "^" or "$", it uses "RegExp".
         * - If it starts with "=", it uses "ExactMatch".
         * - If it's empty, it uses "SelectAll".
         **/
        FileSearchFilter( DirInfo       * dir,
                          const QString & pattern,
                          FilterMode      filterMode,
                          bool            caseSensitive,
			  bool            findFiles,
			  bool            findDirs,
			  bool            findSymLinks,
			  bool            findPkgs ):
	    SearchFilter( pattern,
			  filterMode,
			  Contains,         // defaultFilterMode
			  caseSensitive ),  // case-insensitive
	    _dir { dir },
	    _findFiles { findFiles },
	    _findDirs { findDirs },
	    _findSymLinks { findSymLinks },
	    _findPkgs { findPkgs }
	{}

        /**
         * Flags which node types to find
         **/
        bool findFiles()    const { return _findFiles;    }
        bool findDirs()     const { return _findDirs;     }
        bool findSymLinks() const { return _findSymLinks; }
        bool findPkgs()     const { return _findPkgs;     }
/*
        void setFindFiles   ( bool value ) { _findFiles    = value; }
        void setFindDirs    ( bool value ) { _findDirs     = value; }
        void setFindSymLinks( bool value ) { _findSymLinks = value; }
        void setFindPkgs    ( bool value ) { _findPkgs     = value; }
*/
        /**
         * Directory to start the search from
         **/
        DirInfo * dir() const { return _dir; }
        void setDir( DirInfo * dir ) { _dir = dir; }


    private:

        DirInfo * _dir;
        bool      _findFiles	{ true };
        bool      _findDirs	{ true };
        bool      _findSymLinks	{ true };
        bool      _findPkgs	{ true };

    };  // class FileSearchFilter


    inline QTextStream & operator<< ( QTextStream            & stream,
                                      const FileSearchFilter & filter )
    {
        QStringList findTypes;

        if ( filter.findFiles() )
            findTypes << "files";

        if ( filter.findDirs() )
            findTypes << "dirs";

        if ( filter.findPkgs() )
            findTypes << "pkgs";

        if ( filter.findSymLinks() )
            findTypes << "symlinks";

        stream << "<FileSearchFilter \""
               << filter.pattern()
               << "\" mode \""
               << SearchFilter::toString( filter.filterMode() )
               << "\" for "
               << findTypes.join( QLatin1String( " + " ) )
               <<( filter.isCaseSensitive()? " case sensitive" : "" )
               << ">";

        return stream;
    }
}

#endif  // FileSearchFilter_h
