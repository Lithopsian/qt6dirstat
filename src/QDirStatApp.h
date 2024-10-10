/*
 *   File name: QDirStatApp.h
 *   Summary:   QDirStat application class for key objects
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef QDirStatApp_h
#define QDirStatApp_h

#include <QApplication>
#include <QScreen>


namespace QDirStat
{
    class DirTree;
    class DirTreeModel;
    class FileInfo;
    class MainWindow;
    class SelectionModel;

    /**
     * This is the QApplication object for the QDirStat application. It
     * does not create any windows or widgets and is intended to be
     * instantiated before any windows or widgets.
     *
     * This class holds key objects and gives access to those key objects
     * from other classes without having to pass every single one of them
     * to each of the other classes that needs them.
     *
     * Although not strictly a singleton class, there will only ever be one
     * instance.  QApplication prevents additional objects being created.
     * Access will normally be through the global app() function which
     * provides completely inline access to the instance of this class and
     * the objects that it holds.
     *
     * Note that the instance guarantees to return object pointers for
     * MainWindow, DirTreeModel, SelectionModel, and DirTree (ownded by
     * DirTreeModel), but it will only do so after they have been set using
     * setModels().  They will become invalid once MainWindow and its
     * children are destroyed and should not be accessed.
     **/
    class QDirStatApp: public QApplication
    {
    public:

        /**
         * Constructor.  This is created on the stack by main.
         **/
        QDirStatApp( int &argc, char **argv ):
            QApplication{ argc, argv }
        {}

        /**
         * Store pointers to the main window and models.  Access to
         * the getters through app() before this is called will return 0.
         * Ownership of these objects is not transferred to QDirStatApp.
         **/
        static void setModels( MainWindow     * mainWindow,
                               DirTreeModel   * dirTreeModel,
                               SelectionModel * selectionModel );

        /**
         * Reset the internal pointers to null.  This happens when the main
         * window is destroyed and it is generally unsafe to use app()
         * after this.
         **/
        static void resetModels();

        /**
         * Access the only instance of the QDirStatApp class, through the Qt
         * global define aApp.
         *
         * Typically, you will want to use the global app() function instead.
         * This function provides non-const access.
         **/
        static QDirStatApp * instance() { return static_cast<QDirStatApp *>( qApp ); }

        //
        // Access to key objects
        //

        /**
         * Return the MainWindow instance given to it, or 0 if it has not yet
         * been given one.
         **/
        MainWindow * mainWindow() const { return _mainWindow; }

        /**
         * Return the directory tree model. This is the model part of Qt
         * model/view widgets such as the DirTreeView (QAbstractItemView) or
         * the TreemapView.
         *
         * It has a DirTree that actually holds the in-memory tree of FileInfo
         * / DirInfo nodes.
         **/
        DirTreeModel * dirTreeModel() const { return _dirTreeModel; }

        /**
         * Return the DirTree that is owned by the DirTreeModel.
         *
         * A DirTree is the in-memory representation of a directory tree
         * consisting of FileInfo nodes or more specialized classes derived
         * from FileInfo such as DirInfo, DotEntry, Attic, or even PkgInfo.
         *
         * A DirTree may start with PkgInfo nodes that each represent one
         * installed software package. A PkgInfo node typically has DirInfo /
         * FileInfo child nodes each representing a directory with files that
         * belong to that software package.
         **/
        DirTree * dirTree() const;

        /**
         * Return the SelectionModel that keeps track of what items are marked
         * as selected across the different connected views, i.e. the DirTreeView
         * and the TreemapView.
         **/
        SelectionModel * selectionModel() const { return _selectionModel; }

        /**
         * Return the DirTree's top level directory (the first child of the
         * tree root) or 0 if the tree is completely empty.
         **/
        FileInfo * firstToplevel() const;

        /**
         * Return the current selected directory, or the parent of the current
         * selected file, or 0 if there is no current item.
         **/
        FileInfo * currentDirInfo() const;

        /**
         * Return the primary screen size.  This is the size less any window
         * frames, etc.
         **/
        QSize screenSize() const { return primaryScreen()->availableSize(); }

        /**
         * Return the hard maximum width of a message box dialog.  This is
         * hard-coded into the message box and is not exposed in the API,
         * but this is the calculation used since at least Qt4.
         **/
        int maxDialogWidth() const
        {
            const int screenWidth = screenSize().width();
            return screenWidth < 1024 ? screenWidth : qMin( screenWidth - 480, 1000 );
        }


    private:

        MainWindow     * _mainWindow{ nullptr };
        DirTreeModel   * _dirTreeModel{ nullptr };
        SelectionModel * _selectionModel{ nullptr };

    };  // class QDirStatApp



    /**
     * Access the QDirStatApp instance (just a cast of qApp).
     **/
    inline const QDirStatApp * app() { return QDirStatApp::instance(); }

}       // namespace QDirStat

#endif  // class QDirStatApp_h
