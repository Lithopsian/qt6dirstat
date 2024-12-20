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
         * Restore a file or directory from the trash to its original location.
         * Return 'true' on success, 'false' on error.
         **/
//        static bool restore( const QString & path );

        /**
         * Empty the trash can, i.e. delete all its contents permanently.
         *
         * This does not just affect files or directories moved to the trash with
         * this class, but everything in all known trash directories, i.e. all
         * trash directories that were used during the life time of the singleton
         * of this class.
         **/
//        static void empty();


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
     * - One in the user's home directory in $XDG_DATA_HOME/Trash
     *   or ~/.local/share/Trash if $XDG_DATA_HOME is not set or empty
     *
     * - One in the toplevel directory (the mount point) of each filesystem:
     *   $TOPLEVEL/.Trash/$UID
     *
     * - If $TOPLEVEL/.Trash does not exist or does not pass some checks, one in
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
         * named 'targetName' (the unique name) in the trash dir.
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
        QString filesPath() const { return _path % QLatin1String{ "/files" }; }


    protected:

        /**
         * Return the path of the "info" subdirectory of this trash dir.
         **/
        QString infoPath() const { return _path % QLatin1String{ "/info" }; }


    private:

        QString _path;

    };  // class TrashDir

}       // namespace QDirStat

#endif  // Trash_h
