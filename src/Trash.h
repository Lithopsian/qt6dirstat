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

#include "Typedefs.h" // _L1


namespace QDirStat
{
    class TrashDir;

    typedef QHash<dev_t, TrashDir *> TrashDirMap;

    /**
     * This class implements the XDG Trash specification:
     *
     *     https://specifications.freedesktop.org/trash-spec/1.0/
     *
     * Basically, this is a desktop trashcan that works just like the trashcan in
     * KDE, Gnome, Xfce and other major Linux desktops. It should integrate well
     * with any of them, i.e., files or directories moved to this trash can should
     * appear in the desktop's native trashcan implementation (the window you get
     * when you click on the trashcan icon on the desktop or in the file manager).
     *
     * Note that, starting in 5.15, Qt has a function QFile::moveToTrash() which
     * could completely repace this class, but it doesn't appear to work 100%
     * correctly.  For example, trashing broken symlinks always fails.
     **/
    class Trash final
    {
    public:

        /**
         * Constructor.  Attempts to identify, and create if necessary, the
         * home trash directory that should exist.
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
         * Throw a file or directory into the trash.  Returns 'true' on
         * success, 'false' on error.  If the call fails, 'msg' will contain
         * details.
         **/
        bool trash( const QString & path, QString & msg );

        /**
         * Return the path of the main trash directory for a filesystem with
         * top directory 'topDir'.
         **/
        static QString trashRoot( const QString & topDir )
            { return topDir % "/.Trash"_L1; }

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
        static QString trashInfoName( const QString & entryName )
            { return entryName % ".trashinfo"_L1; }

        /**
         * Return the path of the trashinfo file corresponding to an entry
         * 'filesEntry'.
         **/
        static QString trashInfoPath( const QString & trashDir, const QString & filesEntry )
            { return infoDirPath( trashDir ) % '/' % filesEntry % trashInfoSuffix(); }

        /**
         * Return a list of all the trash directories found.  This may include
         * the one in the users's home directory and any at the top level of
         * mounted filesystems.
         *
         * If 'allRoots' is false, only trash directories in which both the
         * files and info directories exist and are accessible will be
         * returned.
         **/
        static QStringList trashRoots( bool allRoots = false );

        /**
         * Return whether 'path' is in any trash directory or any trash
         * directory in in 'path'.
         *
         * Note that this function will return false for valid trash dirs for
         * other users, and these may then be trashed (usually only by root).
         **/
        static bool isTrashDir( const QString & path );

        /**
         * Try to move 'path' to 'targetPath', first using rename().  If that
         * fails with errno=EXDEV (invalid cross-device link) and
         * 'copyAndDelete' is specified, by copying and deleting.  If false is
         * returned then 'msg' will contain some hint as to why.
         *
         * There is no C copy function, and none until C++17, so use an external
         * process.  This is considered successful if the process completes with an
         * exit code of 0.  Similarly, there is no low-level function to remove a
         * directory and all of its contents.  Qt can do this, but "rm -rf" is more
         * reliable.
         *
         * If the copy or delete, which may have performed a partial copy or
         * delete, fails, then make sure that all the contents at 'path' are still
         * in place and that 'targetPath' is completely removed.  Short of some
         * bizarre circumstance such as the permissions being changed or the device
         * becoming full while the operation is happening, this should leave
         * everything unchanged.  In an extremely rare worst case, a partial
         * subtree may remain at 'path' and the full subtree at 'targetPath'.
         **/
        static bool move( const QString & path, const QString targetPath, QString & msg, bool copyAndDelete );


    protected:

        /**
         * Attempt to locate or create a home trash directory for
         * '_homeTrashPath'.
         **/
        void createHomeTrashDir();

        /**
         * Return the trash dir for 'path', creating it if necessary and falling
         * back to the home trash dir if necessary.
         **/
        TrashDir * trashDir( const QString & path );


    private:

        bool        _copyAndDelete;
        QString     _homeTrashPath;
        dev_t       _homeTrashDev;
        TrashDirMap _trashDirs;

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
         * Create a .trashinfo file for a file or directory 'path' and move
         * 'path' to a uniquely-named entry  in the "files" directory for this
         * TrashDir.
         *
         * The .trashinfo file is opened in exclusive mode, to prevent races
         * with other trash operations.  An exception will be thrown if the
         * attempted trash fails unexpectedly.  The caller is expected to
         * ensure as far as possible that the operation can succeed, and to
         * catch any exceptions.
         **/
        void trash( const QString & path );

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
         * Return the path of the "files" subdirectory of this trash dir.
         **/
        QString filesDirPath() const { return Trash::filesDirPath( _path ); }

        /**
         * Return the path of the "info" subdirectory of this trash directory.
         **/
        QString infoDirPath() const { return Trash::infoDirPath( _path ); }


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
