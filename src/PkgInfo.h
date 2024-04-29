/*
 *   File name: PkgInfo.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef PkgInfo_h
#define PkgInfo_h

#include <QList>

#include "DirInfo.h"


namespace QDirStat
{
    class DirTree;
    class PkgManager;

    /**
     * Information about one (installed) package.
     **/
    class PkgInfo: public DirInfo
    {
        /**
         * Private constructor, the public ones delegate to this.  Note that the
         * relevant DirInfo constructor is critical, it does not create a DotEntry
         * because PkgInfo objects do not have DotEntry direct children.
         **/
        PkgInfo( DirTree          * tree,
                 DirInfo          * parent,
                 const QString    & name,
                 const QString    & version,
                 const QString    & arch,
                 const PkgManager * pkgManager ):
            DirInfo( parent,
                     tree,
                     name ),
            _pkgManager ( pkgManager ),
            _baseName { name },
            _version { version },
            _arch { arch },
            _multiVersion { false },
            _multiArch { false }
        {}

    public:

        /**
         * Public constructor: used by the package managers for creating a
         * package list. without a tree or parent until it is inserted by
         * PkgReader.
         **/
        PkgInfo( const QString    & name,
                 const QString    & version,
                 const QString    & arch,
                 const PkgManager * pkgManager ):
            PkgInfo ( nullptr, nullptr, name, version, arch, pkgManager )
        {}

        /**
         * Public constructor: used by the package reader for the top-level
         * package summary container.
         **/
        PkgInfo( DirTree          * tree,
                 DirInfo          * parent ):
            PkgInfo ( tree, parent, pkgScheme(), QString(), QString(), nullptr )
        {}

        /**
         * Return the package's base name, i.e. the short name without any
         * version number or architecture information. This may be different
         * from name() if this package is installed in multiple versions or for
         * different architectures. Initially, this starts with a copy of
         * name().
         **/
        const QString & baseName() const { return _baseName; }

        /**
         * Set the (display) name for this package.
         *
         * This is useful if this package is installed in multiple versions or
         * for multiple architectures; in that case, it is advisable to use the
         * base name plus either the version or the architecture or both.
         **/
        void setName( const QString & newName ) { _name = newName; }

        /**
         * Return the version of this package.
         **/
        const QString & version() const { return _version; }

        /**
         * Return the architecture of this package ("x86_64", "i386").
         **/
        const QString & arch() const { return _arch; }

        /**
         * Return the package manager that this package is managed by.
         **/
        const PkgManager * pkgManager() const { return _pkgManager; }

        /**
         * Set the parent DirTree for this pkg.
         **/
        void setTree( DirTree * tree ) { _tree = tree; }

        /**
         * Return 'true' if this package is installed for more than one
         * architecture.
         **/
        bool isMultiArch() const { return _multiArch; }

        /**
         * Set the multiArch flag.
         **/
        void setMultiArch( bool val ) { _multiArch = val; }

        /**
         * Return 'true' if this package is installed in multiple versions
         * (but possibly for only one architecture).
         **/
        bool isMultiVersion() const { return _multiVersion; }

        /**
         * Set the multiVersion flag.
         **/
        void setMultiVersion( bool val ) { _multiVersion = val; }

        /**
         * Returns true if this is a PkgInfo object.
         *
         * Reimplemented - inherited from FileInfo.
         **/
        virtual bool isPkgInfo() const override { return true; }

        /**
         * Returns the full URL of this object with full path.
         *
         * Reimplemented - inherited from FileInfo.
         **/
        virtual QString url() const override
            { return pkgSummaryUrl() + ( isPkgUrl( _name ) ? "" : _name ); }

        /**
         * Return 'true' if this is a package URL, i.e. it starts with "Pkg:".
         **/
        static bool isPkgUrl( const QString & url )
            { return url.startsWith( pkgScheme(), Qt::CaseInsensitive ); }

        /**
         * Create a package URL from 'path'. If it already is a package URL,
         * just return 'path'.
         **/
        QString pkgUrl( const QString & path ) const
            { return isPkgUrl( path ) ? path : url() + path; }

        /**
         * Locate a path that is already split up into its components within a
         * subtree: Return the corresponding FileInfo or 0 if not found.
         **/
//        FileInfo * locate( DirInfo           * subtree,
//                           const QStringList & pathComponents );
//        FileInfo * locate( const QString & pathComponent );

        /**
         * Returns the name of the "root" package summary item url (ie. "Pkg:/").
         **/
        static QString pkgSummaryUrl() { return pkgScheme() + '/'; }


    protected:

        /**
         * Locate a path in this PkgInfo subtree:
         * Return the corresponding FileInfo or 0 if not found.
         **/
//        FileInfo * locate( const QString & path );

        /**
         * Locate a path that is already split up into its components in this
         * PkgInfo subtree: Return the corresponding FileInfo or 0 if not
         * found.
         **/
//        FileInfo * locate( const QStringList & pathComponents )
//            { return locate( this, pathComponents ); }

        /**
         * Returns the package scheme prefix.
         **/
        static QString pkgScheme() { return "Pkg:"; }


    private:

        // Data members

        const PkgManager * _pkgManager;

        QString _baseName;
        QString _version;
        QString _arch;

        bool    _multiVersion :1;
        bool    _multiArch    :1;

    };  // class PkgInfo


    typedef QList<PkgInfo *> PkgInfoList;


    /**
     * Print the debugUrl() of a PkgInfo in a debug stream.
     **/
    inline QTextStream & operator<< ( QTextStream & stream, const PkgInfo * info )
    {
        if ( info )
        {
            if ( info->checkMagicNumber() )
                stream << "<Pkg " << info->name() << ">";
            else
                stream << "<INVALID PkgInfo *>";
        }
        else
            stream << "<NULL PkgInfo *>";

        return stream;
    }

}       // namespace QDirStat


#endif // ifndef PkgInfo_h
