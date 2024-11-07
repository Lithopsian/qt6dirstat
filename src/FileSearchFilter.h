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

#include "SearchFilter.h"
#include "Typedefs.h" // _L1


namespace QDirStat
{
    class FileInfo;

    /**
     * Filter class for searching for files and/or directories.
     **/
    class FileSearchFilter: public SearchFilter
    {
    public:

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
        FileSearchFilter( FileInfo      * dir,
                          const QString & pattern,
                          FilterMode      filterMode,
                          bool            caseSensitive,
                          bool            findFiles,
                          bool            findDirs,
                          bool            findSymlinks,
                          bool            findPkgs ):
            SearchFilter{ pattern, filterMode, Contains, caseSensitive },
            _dir{ dir },
            _findFiles{ findFiles },
            _findDirs{ findDirs },
            _findSymlinks{ findSymlinks },
            _findPkgs{ findPkgs }
        {}

        /** Default constructor, constructs a filter with no tree and an
         * empty search pattern.
         **/
        FileSearchFilter():
            FileSearchFilter{ nullptr, QString{}, Auto, true, true, true, true, true }
        {}

        /**
         * Flags for which node types to find
         **/
        bool findFiles()    const { return _findFiles;    }
        bool findDirs()     const { return _findDirs;     }
        bool findSymlinks() const { return _findSymlinks; }
        bool findPkgs()     const { return _findPkgs;     }

        /**
         * Directory to start the search from
         **/
        FileInfo * dir() const { return _dir; }


    private:

        FileInfo * _dir;
        bool       _findFiles{ true };
        bool       _findDirs{ true };
        bool       _findSymlinks{ true };
        bool       _findPkgs{ true };

    };  // class FileSearchFilter



    inline QTextStream & operator<<( QTextStream            & stream,
                                     const FileSearchFilter & filter )
    {
        QStringList findTypes;

        if ( filter.findFiles() )
            findTypes << "files";

        if ( filter.findDirs() )
            findTypes << "dirs";

        if ( filter.findPkgs() )
            findTypes << "pkgs";

        if ( filter.findSymlinks() )
            findTypes << "symlinks";

        stream << "<FileSearchFilter \""
               << filter.pattern()
               << "\" mode \""
               << SearchFilter::toString( filter.filterMode() )
               << "\" for "
               << findTypes.join( " + "_L1 )
               << ( filter.isCaseSensitive() ? " case sensitive>" : ">" );

        return stream;
    }

}       // namespace QDirStat

#endif  // FileSearchFilter_h
