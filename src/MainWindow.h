/*
 *   File name: MainWindow.h
 *   Summary:   QDirStat main window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef MainWindow_h
#define MainWindow_h

#include <memory>

#include <QActionGroup>
#include <QContextMenuEvent>
#include <QElapsedTimer>
#include <QMainWindow>
#include <QTimer>

#include "ui_main-window.h"
#include "Subtree.h"


class SysCallFailedException;


namespace QDirStat
{
    class FileDetailsView;
    class HistoryButtons;
    class PkgFilter;
    class TreemapView;
    class UnpkgSettings;

    /**
     * This is the MainWindow object for the QDirStat application. It
     * creates and manages the main application window.
     **/
    class MainWindow final : public QMainWindow
    {
        Q_OBJECT

        enum LayoutSettings
        {
            LayoutShowBreadcrumbs,
            LayoutShowDetails,
            LayoutShowDirTree,
            LayoutShowTreemap,
//            LayoutTreemapOnSide,
        };


    public:

        /**
         * Constructor.  This is created on the stack by main.
         *
         * Note that this isn't a singleton class, but only one
         * instance should be created.
         **/
        MainWindow( bool slowUpdate );
        ~MainWindow() override;

        /**
         * Clear the current tree and replace it with the content of the specified
         * cache file.
         **/
        void readCache( const QString & cacheFileName );

        /**
         * Return the message panel widget to enable a PanelMessage to be displayed.
         **/
        QVBoxLayout * messagePanelContainer() { return _ui->vBox; }

        /**
         * Show an error popup that a directory could not be opened and wait until
         * the user confirmed it.
         *
         * The relevant information is all in the exception.
         **/
        void showOpenDirErrorPopup( const SysCallFailedException & ex );

        // for the config dialog

        /**
         * Return the DirTreeView for this window
         **/
//        DirTreeView * dirTreeView() const { return _ui->dirTreeView; }

        /**
         * Return the TreemapView for this window
         **/
        TreemapView * treemapView() const { return _ui->treemapView; }

        /**
         * The setting for whether the url of the current tree root is
         * included in the window title.
         **/
        bool urlInWindowTitle() const { return _urlInWindowTitle; }
        void setUrlInWindowTitle( bool newValue );

        /**
         * The setting for whether the read permissions warning panel message
         * is displayed when not all directories can be read due to
         * insufficient permissions.
         **/
        bool showDirPermissionsMsg() const { return _showDirPermissionsMsg; }
        void setShowDirPermissionsMsg( bool newValue ) { _showDirPermissionsMsg = newValue; }

        /**
         * Return the setting for StatusBarTimeoutMillisec
         **/
        int statusBarTimeout() const { return _statusBarTimeout; }
        void setStatusBarTimeout( int newValue ) { _statusBarTimeout = newValue; }

        /**
         * Return the setting for Long StatusBarTimeout
         **/
        int longStatusBarTimeout() const { return _longStatusBarTimeout; }
        void setLongStatusBarTimeout( int newValue ) { _longStatusBarTimeout = newValue; }

        /**
         * Open an URL (directory or package URL).
         **/
        void openUrl( const QString & url );

        /**
         * Read a filesystem, as requested from the filesystems window.
         **/
        void readFilesystem( const QString & path );

        /**
         * Return a pointer to the FileDetailsView panel widget.
         **/
        FileDetailsView * fileDetailsView() const { return _ui->fileDetailsView; }

        /**
         * Enable the action for a packaged read.
         **/
        void enableOpenPkg()
            { _ui->actionOpenPkg->setEnabled( true ); }

        /**
         * Enable the action for an unpackage dread.
         **/
        void enableOpenUnpkg()
            { _ui->actionOpenUnpkg->setEnabled( true ); }


    public slots:

        /**
         * Open a directory selection dialog and open the selected URL.
         **/
        void askOpenDir();

        /**
         * Show the directories that could not be read in a separate non-modal
         * window.
         **/
        void showUnreadableDirs();

        /**
         * Set focus to the tree view.
         *
         * Note that this may be called from a destructor and _ui may no longer
         * exist.
         **/
        void focusDirTree()
            { if ( _ui ) _ui->dirTreeView->setFocus(); }


    protected slots:

        /**
         * Open a file selection dialog to ask for a cache file, clear the
         * current tree and replace it with the content of the cache file.
         **/
        void askReadCache();

        /**
         * Open a file selection dialog and save the current tree to the selected
         * file.
         **/
        void askWriteCache();

        /**
         * Re-read the complete directory tree.
         **/
        void refreshAll();

        /**
         * Re-read the selected branch of the tree.
         **/
        void refreshSelected();

        /**
         * Stop reading if reading is in process.
         **/
        void stopReading();

        /**
         * Navigate one directory level up.
         **/
        void navigateUp();

        /**
         * Navigate to the toplevel directory of this tree.
         **/
        void navigateToToplevel();

        /**
         * Show the "about" dialog.
         **/
        void showAboutDialog();

        /**
         * Show the "about Qt" dialog.
         **/
        void showAboutQtDialog();

        /**
         * Show the "Donate" dialog.
         **/
        void showDonateDialog();

        /**
         * Open a package selection dialog and open the selected URL.
         **/
        void askOpenPkg();

        /**
         * Open a "show unpackaged files" dialog and start reading the selected
         * starting dir with the selected exclude dirs.
         **/
        void askOpenUnpkg();

        /**
         * Switch display to "busy display" after reading was started and restart
         * the stopwatch.
         **/
        void startingReading();

        /**
         * Finalize display after reading is finished.
         **/
        void readingFinished();

        /**
         * Finalize display after reading has been aborted.
         **/
        void readingAborted();

        /**
         * Refresh after the tree has been sorted.
         **/
        void layoutChanged( const QList<QPersistentModelIndex> &,
                            QAbstractItemModel::LayoutChangeHint changeHint );

        /**
         * Change display mode to "busy" (while reading a directory tree):
         * Sort tree view by read jobs, hide treemap view.
         **/
        void busyDisplay();

        /**
         * Enable or disable actions depending on current status.
         **/
        void updateActions();

        /**
         * Enable or disable the treemap view, depending on the value of
         * the corresponding action.
         **/
        void showTreemapView( bool show );

        /**
         * Switch between showing the treemap view beside the file directory
         * or below it, depending on the corresponding action.
         **/
        void treemapAsSidePanel( bool asSidePanel );

        /**
         * Switch between showing the file details panel next to the
         * directory tree or next to the treemap.
         **/
        void detailsWithTreemap( bool withPanel );

        /**
         * Notification that a cleanup action was started.
         **/
        void startingCleanup( const QString & cleanupName );

        /**
         * Notification that the last process of a cleanup action is finished.
         *
         * 'errorCount' is the total number of errors reported by all processes
         * that were started.
         **/
        void cleanupFinished( int errorCount );

        /**
         * Notification that a Cleanup has completed and removed the affected
         * items from the tree under refresh policy AssumeDeleted.
         **/
        void assumedDeleted();

        /**
         * Navigate to the specified URL, i.e. make that directory the current and
         * selected one; scroll there and open the tree branches so that URL is
         * visible.
         **/
        void navigateToUrl( const QString & url );

        /**
         * Open the config dialog.
         **/
        void openConfigDialog();

        /**
         * Show file size statistics for the currently selected directory.
         **/
        void showFileSizeStats();

        /**
         * Show file type statistics for the currently selected directory.
         **/
        void showFileTypeStats();

        /**
         * Show file age statistics for the currently selected directory.
         **/
        void showFileAgeStats();

        /**
         * Show detailed information about mounted filesystems in a separate window.
         **/
        void showFilesystems();

        /**
         * Change the main window layout when triggered by an action.  The layout name
         * is found from the QAction data.
         **/
        void changeLayoutSlot();

        /**
         * Show the elapsed time while reading.
         **/
        void showElapsedTime();

        /**
         * Switch verbose logging for selection changes on or off.
         *
         * This is normally done by the invisible checkable action
         * _ui->actionVerboseSelection in the main window UI file.
         *
         * The hotkey for this is Shift-F7.
         **/
        void toggleVerboseSelection( bool verboseSelection );

        /**
         * Copy the path of the current item (if there is one) to the system
         * clipboard for use in other applications.
         **/
        void copyCurrentPathToClipboard();

        /**
         * Move the selected items to the trash bin.
         **/
        void moveToTrash();

        /**
         * Open the "Find Files" dialog and display the results.
         **/
        void askFindFiles();

        /**
         * Open the URL stored in an action's statusTip property with an external
         * browser.
         *
         * For the "Help" menu, those URLs are defined in the Qt Designer UI file
         * for the main window (main-window.ui). See actionHelp for an example.
         **/
        void openActionUrl();

        /**
         * Show the URL of 'item' and its total size in the status line.
         **/
        void showCurrent( FileInfo * item );

        /**
         * Show a summary of the current selection in the status line.
         **/
        void showSummary();

        /**
         * Update the status bar and file details panel when the selection has
         * been changed.
         **/
        void selectionChanged();

        /**
         * Update the status bar and file details panel when the current item has
         * been changed.
         **/
        void currentItemChanged( FileInfo * newCurrent, const FileInfo * oldCurrent );

        /**
         * For debugging: dump the currently selected items and the current
         * item to the log.
         **/
        void dumpSelectedItems();


    protected:

        /**
         * Change display mode to "idle" (after reading the directory tree is
         * finished): If the tree view is still sorted by read jobs, now sort it by
         * subtree percent, show the treemap view if enabled.
         **/
        void idleDisplay();

        /**
         * Close the children of this window before starting a new read.
         **/
        void closeChildren();

        /**
         * Open a directory URL (start reading that directory).
         **/
        void openDir( const QString & url );

        /**
         * Replace the current tree with the list of installed
         * packages from the system's package manager that match 'pkgUrl'.
         **/
        void readPkg( const PkgFilter & pkgFilter );

        /**
         * Show unpackaged files with the specified 'unpkgSettings' parameters
         * (startingDir, excludeDirs, ignorePatterns).
         *
         * The URL may start with "unpkg:/".
         **/
        void showUnpkgFiles( const UnpkgSettings & unpkgSettings );

        /**
         * Show unpackaged files with the UnpkgSettings parameters from the config
         * file or default values if no config was written yet.
         **/
        void showUnpkgFiles( const QString & url );

        /**
         * Return 'true' if the URL starts with "unpkg:/".
         **/
        static bool isUnpkgUrl( const QString & url )
            { return url.startsWith( unpkgScheme(), Qt::CaseInsensitive ); }

        /**
         * Return the url prefix for the top- level unpackaged view (ie. "Unpkg:/").
         **/
        static QLatin1String unpkgScheme() { return QLatin1String{ "Unpkg:/" }; }

        /**
         * Disable the treemap, reset the permissions warning, breadcrumbs,
         * and trees, then display a BusyPopup to prepare for a packaged or
         * unpackaged files read.
         **/
        void pkgQuerySetup();

        /**
         * Update the window title: Show "[root]" if running as root and add the
         * URL if that is configured.
         **/
        void updateWindowTitle( const QString & url );

        /**
         * Show progress text in the status bar for a few seconds.
         **/
        void showProgress( const QString & text );

        /**
         * Return whether verbose selection is enabled.
         **/
        bool verboseSelection() const { return _ui->actionVerboseSelection->isChecked(); }

        /**
         * Set a subtree to either one of the selected items, or to the current item if
         * there are no selected items.
         **/
        void setFutureSelection();

        /**
         * Show a warning (as a panel message) about insufficient permissions when
         * reading directories.
         **/
        void showDirPermissionsWarning();

        /**
         * Enable or disable the directory permissions panel message.  This is
         * only shown once when a directory is read, then not again after limited
         * refreshes such as Refresh Selected or Cleanups.  If the Refresh All or
         * different directory (or Package) read is done, the message will be
         * displayed again.  The ShowDirPermissionsMsg setting can be used to
         * prevent the message being shown at all.
         **/
        void enableDirPermissionsMsg() { _enableDirPermissionsMsg = _showDirPermissionsMsg; }
        void disableDirPermissionsMsg() { _enableDirPermissionsMsg = false; }

        /**
         * Expand the directory tree's branches to depth 'level'.
         **/
        void expandTreeToLevel( int level ) const;

        /**
         * Read parameters from the settings file.
         **/
        void readSettings();

        /**
         * Write parameters to the settings file.
         **/
        void writeSettings();

        /**
         * Set up QObject connections (except from QActions)
         **/
        void connectSignals( DirTree * dirTree, DirTreeModel * dirTreeModel, SelectionModel * selectionModel );

        /**
         * Connect menu QActions from the .ui file to actions of this class
         **/
        void connectMenuActions();
        void connectAction( QAction * action, void( MainWindow::*actee )( void ) );
        void mapTreeExpandAction( QAction * action, int level );
        void connectToggleAction( QAction * action, void( MainWindow::*actee )( bool ) );
        void connectTreemapAction( QAction * action, void( TreemapView::*actee )( void ) );
        void connectFunctorAction( QAction * action, void( *actee )( void ) );

        /**
         * Map actions to action names (eg. "L3").
         **/
        QString layoutName( const QAction * action ) const;
        QAction * layoutAction( const QString & layoutName ) const;

        /**
         * Return the action or name string (eg. "L2") of the current layout.
         **/
        QString currentLayoutName() const
            { return layoutName( _layoutActionGroup->checkedAction() ); }

        /**
         * Change the main window layout.
         **/
        void changeLayout( const QString & layoutName );

        /**
         * Show or hide the menu bar and status bar.
         **/
        void showBars();

        /**
         * Create the different top layouts.
         **/
        void initLayouts( const QString & currentLayoutName );

        /**
         * Get the layout details show values from an action.
         **/
         bool layoutShowBreadcrumbs( const QAction * action ) const
            { return action->data().toList().at( LayoutShowBreadcrumbs ).toBool(); }
         bool layoutShowDetailsPanel( const QAction * action ) const
            { return action->data().toList().at( LayoutShowDetails ).toBool(); }
         bool layoutShowDirTree( const QAction * action ) const
            { return action->data().toList().at( LayoutShowDirTree ).toBool(); }
         bool layoutShowTreemap( const QAction * action ) const
            { return action->data().toList().at( LayoutShowTreemap ).toBool(); }
//         bool layoutTreemapOnSide( const QAction * action ) const
//            { return action->data().toList().at( LayoutTreemapOnSide ).toBool(); }
        void setData( LayoutSettings setting, bool value );

        /**
         * Save whether the breadcrumbs are visible in the current layout.
         **/
        void updateLayoutBreadcrumbs( bool breadcrumbsVisible );

        /**
         * Save whether the details panel is visible in the current layout.
         **/
        void updateLayoutDetailsPanel( bool detailsPanelVisible );

        /**
         * Save whether the directory tree is visible in the current layout.
         **/
        void updateLayoutDirTree( bool dirTreeVisible );

        /**
         * Save whether the treemap is visible in the current layout.
         **/
        void updateLayoutTreemap( bool treemapVisible );

        /**
         * Save whether the treemap is on the side in the current layout.
         **/
//        void updateLayoutTreemapOnSide( bool treemapOnSide );

        /**
         * Apply a layout to the current settings.
         **/
        void applyLayout( const QAction * action );

        /**
         * Read settings for one layout.
         **/
        void readLayoutSetting( const QString & layoutName );

        /**
         * Write layout settings.
         **/
        void writeLayoutSettings();

        /**
         * Apply the future selection: Select the URL that was stored in
         * _futureSelection, open that branch and clear _futureSelection.
         **/
        void applyFutureSelection();

        /**
         * Apply the exclude rules from 'unpkgSettings' to the DirTree.
         **/
        void setUnpkgExcludeRules( const UnpkgSettings & unpkgSettings );

        /**
         * Apply the filters to the DirTree:
         * - Ignore all files that belong to an installed package
         * - Ignore all file patterns ("*.pyc" etc.) the user wishes to ignore
         **/
        void setUnpkgFilters( const UnpkgSettings & unpkgSettings );

        /**
         * Apply the cross-filesystem settings to the tree.
         **/
        void setUnpkgCrossFilesystems( const UnpkgSettings & unpkgSettings );

        /**
         * Parse the starting directory in the 'unpkgSettings' and remove the
         * starting "unpkg:" part to it is suitable for actually opening a
         * directory. Return the parsed directory path.
         **/
        QString parseUnpkgStartingDir( const UnpkgSettings & unpkgSettings );

        /**
         * Detect theme changes, resizes, and attempts to close the
         * window.
         *
         * The breadcrumbs are redrawn when the font changes or the
         * window is resized, in case the elided text needs to change.
         *
         * Active reads are cleanly aborted before allowing the window
         * to close.  Once the window closes, the main loop will exit
         * and the application will shutdown.
         *
         * Reimplemented from QMainWindow/QWidget.
         **/
        bool event( QEvent * event ) override;

        /**
         * Handle mouse buttons: activate history actions actionGoBack and
         * actionGoForward with the "back" and "forward" mouse buttons.
         *
         * Reimplemented from QWidget.
         **/
        void mousePressEvent( QMouseEvent * event ) override;

        /**
         * Context menu event.
         *
         * Reimplemented from QMainWindow/QWidget.
         **/
        void contextMenuEvent( QContextMenuEvent * event ) override;


    private:

        std::unique_ptr<Ui::MainWindow> _ui;

        HistoryButtons * _historyButtons;
        QActionGroup   * _layoutActionGroup;
        Subtree          _futureSelection;

        bool             _showDirPermissionsMsg;
        bool             _enableDirPermissionsMsg{ false };
        bool             _urlInWindowTitle;

        QTimer           _updateTimer;
        int              _statusBarTimeout;
        int              _longStatusBarTimeout;
        QElapsedTimer    _stopWatch;

        int              _sortCol;
        Qt::SortOrder    _sortOrder;

    };  // class MainWindow

}       // namespace QDirStat

#endif  // MainWindow_H
