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

#include <QStringBuilder>

#include "DirInfo.h"


namespace QDirStat
{
    class DirTree;
    class PkgManager;

    /**
     * Information about one (installed) package.
     **/
    class PkgInfo final : public DirInfo
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
            DirInfo{ parent, tree, name },
            _pkgManager{ pkgManager },
            _baseName{ name },
            _version{ version },
            _arch{ arch },
            _multiVersion{ false },
            _multiArch{ false }
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
            PkgInfo{ nullptr, nullptr, name, version, arch, pkgManager }
        {}

        /**
         * Public constructor: used by the package reader for the top-level
         * package summary container.
         **/
        PkgInfo( DirTree          * tree,
                 DirInfo          * parent ):
            PkgInfo{ tree, parent, pkgSummaryUrl(), QString{}, QString{}, nullptr }
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
         * Return 'true' if this package is installed for more than one
         * architecture.
         **/
        bool isMultiArch() const { return _multiArch; }

        /**
         * Set the multiArch flag.
         **/
        void setMultiArch( bool val = true ) { _multiArch = val; }

        /**
         * Return 'true' if this package is installed in multiple versions
         * (but possibly for only one architecture).
         **/
        bool isMultiVersion() const { return _multiVersion; }

        /**
         * Set the multiVersion flag.
         **/
        void setMultiVersion( bool val = true ) { _multiVersion = val; }

        /**
         * Returns true if this is a PkgInfo object.
         *
         * Reimplemented - inherited from FileInfo.
         **/
        bool isPkgInfo() const override { return true; }

        /**
         * Returns the full URL of this object with full path.
         *
         * Reimplemented - inherited from FileInfo.
         **/
        QString url() const override
            { return pkgScheme() % ( isPkgUrl( name() ) ? QString{} : name() ); }

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
            { return isPkgUrl( path ) ? path : url() % path; }

        /**
         * Locate a path that within this subtree.
         *
         * Reimplemented from FileInfo.  The default locate() function does not
         * understand schemes, so handle that here.
         **/
        FileInfo * locate( const QString & locateUrl ) override
            { return locateUrl == url() ? this : FileInfo::locate( locateUrl ); }

        /**
         * Returns the name of the "root" package summary item url (ie. "Pkg:/").
         **/
        static QLatin1String pkgSummaryUrl() { return pkgScheme(); }


    protected:

        /**
         * Returns the package scheme prefix.
         **/
        static QLatin1String pkgScheme() { return "Pkg:/"_L1; }


    private:

        const PkgManager * _pkgManager;

        QString _baseName;
        QString _version;
        QString _arch;

        bool    _multiVersion :1;
        bool    _multiArch    :1;

    };  // class PkgInfo


    typedef QList<PkgInfo *> PkgInfoList; // QList from QMultiMap::values


    /**
     * Print the debugUrl() of a PkgInfo in a debug stream.
     **/
    inline QTextStream & operator<<( QTextStream & stream, const PkgInfo * info )
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
