# qmake .pro file for qdirstat/src
#
# Go to the project toplevel dir and build all Makefiles:
#
#     qmake
#
# Then build with
#
#     make
#

TEMPLATE	 = app

QT		+= widgets
# Comment out to get a non-optimized debug build
# CONFIG	+= debug
DEPENDPATH	+= .
MOC_DIR		 = .moc
OBJECTS_DIR	 = .obj
LIBS		+= -lz

isEmpty(INSTALL_PREFIX):INSTALL_PREFIX = /usr

TARGET		 = qdirstat
TARGET.files	 = qdirstat
TARGET.path	 = $$INSTALL_PREFIX/bin
INSTALLS	+= TARGET desktop icons


# QMAKE_CXXFLAGS	+=  -Wno-deprecated -Wno-deprecated-declarations
# QMAKE_CXXFLAGS	+=  -std=c++11
# QMAKE_CXXFLAGS	+=  -Wconversion
QMAKE_CXXFLAGS	+=  -Wsuggest-override
# QMAKE_CXXFLAGS	+=  -flto=auto
# QMAKE_LDDFLAGS	+=  -flto=auto


SOURCES =   main.cpp			\
            QDirStatApp.cpp		\
	    ActionManager.cpp		\
	    AdaptiveTimer.cpp		\
	    Attic.cpp			\
	    BreadcrumbNavigator.cpp	\
	    BusyPopup.cpp		\
	    Cleanup.cpp			\
	    CleanupCollection.cpp	\
	    CleanupConfigPage.cpp	\
	    ConfigDialog.cpp		\
	    DataColumns.cpp		\
	    DirInfo.cpp			\
	    DirReadJob.cpp		\
	    DirTree.cpp			\
	    DirTreeCache.cpp		\
	    DirTreeModel.cpp		\
	    DirTreeFilter.cpp		\
	    DirTreeView.cpp		\
	    DiscoverActions.cpp		\
	    DotEntry.cpp		\
	    DpkgPkgManager.cpp		\
	    Exception.cpp		\
	    ExcludeRules.cpp		\
	    ExcludeRulesConfigPage.cpp	\
	    ExistingDirValidator.cpp	\
	    FileAgeStats.cpp		\
	    FileAgeStatsWindow.cpp	\
	    FileDetailsView.cpp		\
	    FileInfo.cpp		\
	    FileInfoIterator.cpp	\
	    FileInfoSet.cpp		\
	    FileInfoSorter.cpp		\
	    FileMTimeStats.cpp		\
	    FileSizeStats.cpp		\
	    FileSizeStatsModels.cpp	\
	    FileSizeStatsWindow.cpp	\
	    FileSystemsWindow.cpp	\
	    FileTypeStats.cpp		\
	    FileTypeStatsWindow.cpp	\
	    FindFilesDialog.cpp		\
	    FormatUtil.cpp		\
	    GeneralConfigPage.cpp	\
	    HeaderTweaker.cpp		\
	    HistogramItems.cpp		\
	    HistogramView.cpp		\
	    History.cpp			\
	    HistoryButtons.cpp		\
	    ListEditor.cpp		\
	    LocateFileTypeWindow.cpp	\
	    LocateFilesWindow.cpp	\
	    Logger.cpp			\
	    MainWindow.cpp		\
	    MainWindowActions.cpp	\
	    MainWindowHelp.cpp		\
	    MainWindowLayout.cpp	\
	    MainWindowUnpkg.cpp		\
	    MimeCategorizer.cpp		\
	    MimeCategory.cpp		\
	    MimeCategoryConfigPage.cpp	\
	    MountPoints.cpp		\
	    OpenDirDialog.cpp		\
	    OpenPkgDialog.cpp		\
	    OpenUnpkgDialog.cpp		\
	    OutputWindow.cpp		\
	    PacManPkgManager.cpp	\
	    PanelMessage.cpp		\
	    PathSelector.cpp		\
	    PercentBar.cpp		\
	    PercentileStats.cpp		\
	    PkgFilter.cpp		\
	    PkgManager.cpp		\
	    PkgQuery.cpp		\
	    PkgReader.cpp		\
	    ProcessStarter.cpp		\
	    Refresher.cpp		\
	    RpmPkgManager.cpp		\
	    SearchFilter.cpp		\
	    SelectionModel.cpp		\
	    Settings.cpp		\
	    SizeColDelegate.cpp		\
	    StdCleanup.cpp		\
	    Subtree.cpp			\
	    SysUtil.cpp			\
	    SystemFileChecker.cpp	\
	    Trash.cpp			\
	    TrashWindow.cpp		\
	    TreeWalker.cpp		\
	    TreemapTile.cpp		\
	    TreemapView.cpp		\
	    UnpkgSettings.cpp		\
	    UnreadableDirsWindow.cpp	\
	    Wildcard.cpp


HEADERS =   QDirStatApp.h		\
	    ActionManager.h		\
	    AdaptiveTimer.h		\
	    Attic.h			\
	    BreadcrumbNavigator.h	\
	    BusyPopup.h			\
	    Cleanup.h			\
	    CleanupCollection.h		\
	    CleanupConfigPage.h		\
	    ConfigDialog.h		\
	    DataColumns.h		\
	    DirInfo.h			\
	    DirReadJob.h		\
	    DirTree.h			\
	    DirTreeCache.h		\
	    DirTreeFilter.h		\
	    DirTreeModel.h		\
	    DirTreeView.h		\
	    DiscoverActions.h		\
	    DotEntry.h			\
	    DpkgPkgManager.h		\
	    Exception.h			\
	    ExcludeRules.h		\
	    ExcludeRulesConfigPage.h	\
	    ExistingDirValidator.h	\
	    FileAgeStats.h		\
	    FileAgeStatsWindow.h	\
	    FileDetailsView.h		\
	    FileInfo.h			\
	    FileInfoIterator.h		\
	    FileInfoSet.h		\
	    FileInfoSorter.h		\
	    FileMTimeStats.h		\
	    FileSearchFilter.h		\
	    FileSizeStats.h		\
	    FileSizeStatsModels.h	\
	    FileSizeStatsWindow.h	\
	    FileSystemsWindow.h		\
	    FileTypeStats.h		\
	    FileTypeStatsWindow.h	\
	    FindFilesDialog.h		\
	    FormatUtil.h		\
	    GeneralConfigPage.h		\
	    HeaderTweaker.h		\
	    HistogramItems.h		\
	    HistogramView.h		\
	    History.h			\
	    HistoryButtons.h		\
	    ListEditor.h		\
	    LocateFileTypeWindow.h	\
	    LocateFilesWindow.h		\
	    Logger.h			\
	    MainWindow.h		\
	    MimeCategorizer.h		\
	    MimeCategory.h		\
	    MimeCategoryConfigPage.h	\
	    MountPoints.h		\
	    OpenDirDialog.h		\
	    OpenPkgDialog.h		\
	    OpenUnpkgDialog.h		\
	    OutputWindow.h		\
	    PacManPkgManager.h		\
	    PanelMessage.h		\
	    PathSelector.h		\
	    PercentBar.h		\
	    PercentileStats.h		\
	    PkgFileListCache.h		\
	    PkgFilter.h			\
	    PkgInfo.h			\
	    PkgManager.h		\
	    PkgQuery.h			\
	    PkgReader.h			\
	    ProcessStarter.h		\
	    Refresher.h			\
	    RpmPkgManager.h		\
	    SearchFilter.h		\
	    SelectionModel.h		\
	    Settings.h			\
	    SignalBlocker.h		\
	    SizeColDelegate.h		\
	    StdCleanup.h		\
	    Subtree.h			\
	    SysUtil.h			\
	    SystemFileChecker.h		\
	    Trash.h			\
	    TrashWindow.h		\
	    TreemapTile.h		\
	    TreemapView.h		\
	    TreeWalker.h		\
	    Typedefs.h			\
	    UnpkgSettings.cpp		\
	    UnreadableDirsWindow.h	\
	    Version.h			\
	    Wildcard.h


FORMS =	    main-window.ui		   \
	    cleanup-config-page.ui	   \
	    config-dialog.ui		   \
	    exclude-rules-config-page.ui   \
	    find-files-dialog.ui	   \
	    file-age-stats-window.ui	   \
	    file-details-view.ui	   \
	    file-size-stats-window.ui	   \
	    file-type-stats-window.ui	   \
	    filesystems-window.ui	   \
	    general-config-page.ui	   \
	    locate-file-type-window.ui	   \
	    locate-files-window.ui	   \
	    mime-category-config-page.ui   \
	    open-dir-dialog.ui		   \
	    open-pkg-dialog.ui		   \
	    open-unpkg-dialog.ui	   \
	    output-window.ui		   \
	    panel-message.ui		   \
	    trash-window.ui		   \
	    unreadable-dirs-window.ui


RESOURCES = icons.qrc

desktop.files	= *.desktop
desktop.path	= $$INSTALL_PREFIX/share/applications

icons.files	= icons/qdirstat.svg
icons.path	= $$INSTALL_PREFIX/share/icons/hicolor/scalable/apps

mac:ICON	= icons/qdirstat.icns

# Regenerate this from the .png file with
#   sudo apt install icnsutils
#   png2icns qdirstat.icns qdirstat.png

