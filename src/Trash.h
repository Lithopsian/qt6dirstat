/*
 *   File name: Trash.h
 *   Summary:   Implementation of the XDG Trash spec for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef Trash_h
#define Trash_h

#include <unistd.h> // getuid()

#include "Typedefs.h" // _L1


namespace QDirStat
{
    class TrashDir;

    typedef QMap<dev_t, TrashDir *> TrashDirMap;

    /**
     * This class implements the XDG Trash specification:
     *
     *     http://standards.freedesktop.org/trash-spec/trashspec-1.0.html
     *
     * Basically, this is a desktop trashcan that works just like the trashcan in
     * KDE, Gnome, Xfce and other major Linux desktops. It should integrate well
     * with any of them, i.e., files or directories moved to this trash can should
     * appear in the desktop's native trashcan implementation (the window you get
     * when you click on the trashcan icon on the desktop or in the file manager).
     *
     * Note that, starting in 5.15, Qt has a function QFile::moveToTrash() which
     * would completely repace this class, but it doesn't appear to work 100%
     * correctly.  For example, trashing broken symlinks always fails.
     **/
    class Trash final
    {
    public:

        /**
         * Constructor.
         **/
        Trash();

        /**
         * Destructor.
         **/
        ~Trash() { qDeleteAll( _trashDirs ); }

        /**
         * Suppress copy constructors, would need a deep copy.
         **/
        Trash( const Trash & ) = delete;
        Trash & operator=( const Trash & ) = delete;

        /**
         * Throw a file or directory into the trash.
         * Return 'true' on success, 'false' on error.
         **/
        bool trash( const QString & path );

        /**
         * Return the path of the files directory for the trash directory
         * 'trashDir'.
         **/
        static QString homeTrash( const QString & homePath );

        /**
         * Return the path of the main trash directory for a filesystem with
         * top directory 'topDir'.
         **/
        static QString trashRoot( const QString & topDir )
            { return topDir % "/.Trash"_L1; }

        /**
         * Return the path of the main trash directory for a filesystem with
         * top directory 'topDir'.
         **/
        static QString mainTrashPath( const QString & trashRoot )
            { return trashRoot % '/' % QString::number( getuid() ); }

        /**
         * Return the path of the main trash directory for a filesystem with
         * top directory 'topDir'.
         **/
        static QString userTrashPath( const QString & trashRoot )
            { return trashRoot % '-' % QString::number( getuid() ); }

        /**
         * Return the path of the files directory for the trash directory
         * 'trashDir'.
         **/
        static QString filesDirPath( const QString & trashDir )
            { return trashDir % "/files"_L1; }

        /**
         * Return the path of the info directory for the trash directory
         * 'trashDir'.
         **/
        static QString infoDirPath( const QString & trashDir )
            { return trashDir % "/info"_L1; }

        /**
         * Return the path of the trashinfo file corresponding to an entry
         * 'filesEntry'.
         **/
        static QString trashEntryPath( const QString & trashDir, const QString & filesEntry )
            { return filesDirPath( trashDir ) % '/' % filesEntry; }

        /**
         * Return the path of the trashinfo file corresponding to an entry
         * 'filesEntry'.
         **/
        static QLatin1String trashInfoSuffix()
            { return ".trashinfo"_L1; }

        /**
         * Return the path of the trashinfo file corresponding to an entry
         * 'filesEntry'.
         **/
        static QString trashInfoPath( const QString & trashDir, const QString & filesEntry )
            { return infoDirPath( trashDir ) % '/' % filesEntry % trashInfoSuffix(); }

        /**
         * Return whether 'trashRoot' is a directory (not a symlink) and has
         * the sticky bit (and execute permission) set.
         *
         * Note that if the lstat() call fails, including because the directory
         * does not exist, this function returns false.  errno must be checked
         * to distinguish the reason for the failure.
         **/
        static bool isValidMainTrash( const QString & trashRoot );


    protected:

        /**
         * Return the trash dir for 'path', creating it if necsssary and falling
         * back to the home trash dir if necessary.
         **/
        TrashDir * trashDir( const QString & path );


    private:

        TrashDir    * _homeTrashDir;
        TrashDirMap   _trashDirs;

    };  // class Trash



    /**
     * One trash directory. There might be several on a system:
     *
     * - one in the user's home directory in $XDG_DATA_HOME/Trash
     *   or ~/.local/share/Trash if $XDG_DATA_HOME is not set or empty
     *
     * - one in the toplevel directory (the mount point) of each filesystem:
     *   $TOPLEVEL/.Trash/$UID
     *
     * - if $TOPLEVEL/.Trash does not exist or does not pass some checks, one in
     *   $TOPLEVEL/.Trash-$UID
     **/
    class TrashDir final
    {
    public:

        /**
         * Constructor. This will create the trash directory and its required
         * subdirectories if it doesn't exist yet.
         *
         * This might throw a FileException if the corresponding disk directories
         * could not be created.
         **/
        TrashDir( const QString & _path );

        /**
         * Return the full path for this trash directory.
         **/
        const QString & path() const { return _path; }

        /**
         * Create a .trashinfo file for a file or directory 'path' that will be
         * named 'targetName' (the unique name) in the trash dir.  If possible,
         * this is done in exclusive mode so the trashinfo name is gauranteed
         * to be unique, but no attempt is made to retry if this fails.
         *
         * This might throw a FileException.
         **/
        void createTrashInfo( const QString & path, const QString & targetName );

        /**
         * Move a file or directory 'path' to to targetName in the trash dir's
         * /files subdirectory. If both are on different devices, copy the file and
         * then delete the original.
         *
         * This might throw a FileException.
         **/
        void move( const QString & path, const QString & targetName );

        /**
         * Return the path of the "files" subdirectory of this trash dir.
         **/
        QString filesDirPath() const { return Trash::filesDirPath( _path ); }

        /**
         * Return the path of the "info" subdirectory of this trash directory.
         **/
        QString infoDirPath() const { return Trash::infoDirPath( _path ); }

        /**
         * Return the tag (first line) of a trashinfo file.
         **/
        static QLatin1String trashInfoTag()
            { return "[Trash Info]"_L1; }

        /**
         * Return the path (second line) field name of a trashinfo file.
         **/
        static QLatin1String trashInfoPathTag()
            { return "Path="_L1; }

        /**
         * Return the deletion date (third line) field name of a trashinfo
         * file.
         **/
        static QLatin1String trashInfoDateTag()
            { return "DeletionDate="_L1; }


    protected:

        /**
         * Return the path of a trashinfo file for this trash directory.
         **/
        QString infoPath( const QString & target) const { return Trash::trashInfoPath( _path, target ); }


    private:

        QString _path;

    };  // class TrashDir


    /**
     * Print a TrashDir path to 'stream'.
     **/
    inline QTextStream & operator<<( QTextStream & stream, const TrashDir * trashDir )
    {
        stream << "TrashDir: " << trashDir->path();

        return stream;
    }

}       // namespace QDirStat

#endif  // Trash_h
