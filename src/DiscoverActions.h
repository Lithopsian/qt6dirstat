/*
 *   File name: DiscoverActions.h
 *   Summary:   Actions for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef DiscoverActions_h
#define DiscoverActions_h


namespace QDirStat
{
    class FileSearchFilter;

    /**
     * Namespace for "discover" actions.
     *
     * They all use a TreeWalker to filter out FileInfo tree nodes and present
     * them as a list in a non-modal LocateFilesWindow. When the user clicks on
     * one of those results, it becomes the current item in the SelectionModel
     * which means that the main window's DirTreeView will scroll to it and
     * open branches until it is visible, and at the same time it will become
     * the current item in the TreemapView.
     *
     * All actions share the same LocateFilesWindow, so any subsequent call will
     * replace any previous content of that window.
     **/
    namespace DiscoverActions
    {
        /**
         * Actions that can be connected directly to a QAction in the main window.
         **/
        void discoverLargestFiles();
        void discoverNewestFiles();
        void discoverOldestFiles();
        void discoverHardLinkedFiles();
        void discoverBrokenSymLinks();
        void discoverSparseFiles();

        /**
         * Actions that are meant to be connected to the FileAgeWindow's
         * 'locate...()' signals (but they can be used stand-alone as well).
         **/
        void discoverFilesFromYear ( const QString & path, short year );
        void discoverFilesFromMonth( const QString & path, short year, short month );

        /**
         * Action from the FindFiles dialog.
         **/
        void findFiles( const FileSearchFilter & filter );

    };  // namespace DiscoverActions

}       // namespace QDirStat

#endif  // DiscoverActions_h
