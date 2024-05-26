/********************************************************************************
** Form generated from reading UI file 'main-window.ui'
**
** Created by: Qt User Interface Compiler version 5.15.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAIN_2D_WINDOW_H
#define UI_MAIN_2D_WINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "BreadcrumbNavigator.h"
#include "DirTreeView.h"
#include "FileDetailsView.h"
#include "TreemapView.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionQuit;
    QAction *actionOpenDir;
    QAction *actionCloseAllTreeLevels;
    QAction *actionExpandTreeLevel1;
    QAction *actionExpandTreeLevel2;
    QAction *actionExpandTreeLevel3;
    QAction *actionExpandTreeLevel4;
    QAction *actionExpandTreeLevel5;
    QAction *actionExpandTreeLevel6;
    QAction *actionExpandTreeLevel7;
    QAction *actionExpandTreeLevel8;
    QAction *actionExpandTreeLevel9;
    QAction *actionStopReading;
    QAction *actionAskWriteCache;
    QAction *actionAskReadCache;
    QAction *actionRefreshAll;
    QAction *actionCopyPath;
    QAction *actionTreemapZoomTo;
    QAction *actionTreemapZoomIn;
    QAction *actionTreemapZoomOut;
    QAction *actionResetTreemapZoom;
    QAction *actionShowTreemap;
    QAction *actionGoUp;
    QAction *actionGoToToplevel;
    QAction *actionAbout;
    QAction *actionAboutQt;
    QAction *actionRefreshSelected;
    QAction *actionReadExcluded;
    QAction *actionContinueReading;
    QAction *actionConfigure;
    QAction *actionVerboseSelection;
    QAction *actionMoveToTrash;
    QAction *actionDumpSelection;
    QAction *actionFileTypeStats;
    QAction *actionHelp;
    QAction *actionFileSizeStats;
    QAction *actionShowDetailsPanel;
    QAction *actionLayout1;
    QAction *actionLayout2;
    QAction *actionLayout3;
    QAction *actionWhatsNew;
    QAction *actionOpenPkg;
    QAction *actionPkgViewHelp;
    QAction *actionOpenUnpkg;
    QAction *actionUnpkgViewHelp;
    QAction *actionShowFilesystems;
    QAction *actionDiscoverLargestFiles;
    QAction *actionDiscoverNewestFiles;
    QAction *actionDiscoverOldestFiles;
    QAction *actionDiscoverHardLinkedFiles;
    QAction *actionDiscoverBrokenSymLinks;
    QAction *actionDiscoverSparseFiles;
    QAction *actionBtrfsSizeReporting;
    QAction *actionShadowedByMount;
    QAction *actionHeadlessServers;
    QAction *actionCantMoveDirToTrash;
    QAction *actionTreemapOnSide;
    QAction *actionFileAgeStats;
    QAction *actionGoBack;
    QAction *actionGoForward;
    QAction *actionFileAgeStatsHelp;
    QAction *actionFindFiles;
    QAction *actionTreemapHelp;
    QAction *actionShowStatusBar;
    QAction *actionShowMenuBar;
    QAction *actionDonate;
    QAction *actionShowBreadcrumbs;
    QAction *actionDetailsWithTreemap;
    QAction *actionShowDirTree;
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout;
    QDirStat::BreadcrumbNavigator *breadcrumbNavigator;
    QSplitter *mainWinSplitter;
    QSplitter *topViewsSplitter;
    QWidget *treeViewContainer;
    QVBoxLayout *verticalLayout_3;
    QVBoxLayout *verticalLayout_2;
    QDirStat::DirTreeView *dirTreeView;
    QWidget *messagePanel;
    QVBoxLayout *vBox;
    QScrollArea *topFileDetailsPanel;
    QDirStat::FileDetailsView *fileDetailsView;
    QSplitter *bottomViewsSplitter;
    QDirStat::TreemapView *treemapView;
    QScrollArea *bottomFileDetailsPanel;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuView;
    QMenu *menuExpandTreeToLevel;
    QMenu *menuEdit;
    QMenu *menuGo;
    QMenu *menuHelp;
    QMenu *menuProblemsAndSolutions;
    QMenu *menuCleanup;
    QMenu *menuDiscover;
    QStatusBar *statusBar;
    QToolBar *toolBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(1115, 687);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/qdirstat.svg"), QSize(), QIcon::Normal, QIcon::Off);
        MainWindow->setWindowIcon(icon);
        actionQuit = new QAction(MainWindow);
        actionQuit->setObjectName(QString::fromUtf8("actionQuit"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/window-close.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionQuit->setIcon(icon1);
        actionOpenDir = new QAction(MainWindow);
        actionOpenDir->setObjectName(QString::fromUtf8("actionOpenDir"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/open-dir.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionOpenDir->setIcon(icon2);
        actionCloseAllTreeLevels = new QAction(MainWindow);
        actionCloseAllTreeLevels->setObjectName(QString::fromUtf8("actionCloseAllTreeLevels"));
        actionExpandTreeLevel1 = new QAction(MainWindow);
        actionExpandTreeLevel1->setObjectName(QString::fromUtf8("actionExpandTreeLevel1"));
        actionExpandTreeLevel2 = new QAction(MainWindow);
        actionExpandTreeLevel2->setObjectName(QString::fromUtf8("actionExpandTreeLevel2"));
        actionExpandTreeLevel3 = new QAction(MainWindow);
        actionExpandTreeLevel3->setObjectName(QString::fromUtf8("actionExpandTreeLevel3"));
        actionExpandTreeLevel4 = new QAction(MainWindow);
        actionExpandTreeLevel4->setObjectName(QString::fromUtf8("actionExpandTreeLevel4"));
        actionExpandTreeLevel5 = new QAction(MainWindow);
        actionExpandTreeLevel5->setObjectName(QString::fromUtf8("actionExpandTreeLevel5"));
        actionExpandTreeLevel6 = new QAction(MainWindow);
        actionExpandTreeLevel6->setObjectName(QString::fromUtf8("actionExpandTreeLevel6"));
        actionExpandTreeLevel7 = new QAction(MainWindow);
        actionExpandTreeLevel7->setObjectName(QString::fromUtf8("actionExpandTreeLevel7"));
        actionExpandTreeLevel8 = new QAction(MainWindow);
        actionExpandTreeLevel8->setObjectName(QString::fromUtf8("actionExpandTreeLevel8"));
        actionExpandTreeLevel9 = new QAction(MainWindow);
        actionExpandTreeLevel9->setObjectName(QString::fromUtf8("actionExpandTreeLevel9"));
        actionStopReading = new QAction(MainWindow);
        actionStopReading->setObjectName(QString::fromUtf8("actionStopReading"));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/icons/stop.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionStopReading->setIcon(icon3);
        actionAskWriteCache = new QAction(MainWindow);
        actionAskWriteCache->setObjectName(QString::fromUtf8("actionAskWriteCache"));
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/icons/export.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionAskWriteCache->setIcon(icon4);
        actionAskReadCache = new QAction(MainWindow);
        actionAskReadCache->setObjectName(QString::fromUtf8("actionAskReadCache"));
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/icons/import.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionAskReadCache->setIcon(icon5);
        actionRefreshAll = new QAction(MainWindow);
        actionRefreshAll->setObjectName(QString::fromUtf8("actionRefreshAll"));
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/icons/refresh.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionRefreshAll->setIcon(icon6);
        actionCopyPath = new QAction(MainWindow);
        actionCopyPath->setObjectName(QString::fromUtf8("actionCopyPath"));
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/icons/edit-copy.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionCopyPath->setIcon(icon7);
        actionTreemapZoomTo = new QAction(MainWindow);
        actionTreemapZoomTo->setObjectName(QString::fromUtf8("actionTreemapZoomTo"));
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/icons/magnifier-zoom-actual.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionTreemapZoomTo->setIcon(icon8);
        actionTreemapZoomIn = new QAction(MainWindow);
        actionTreemapZoomIn->setObjectName(QString::fromUtf8("actionTreemapZoomIn"));
        QIcon icon9;
        icon9.addFile(QString::fromUtf8(":/icons/magnifier-zoom-in.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionTreemapZoomIn->setIcon(icon9);
        actionTreemapZoomOut = new QAction(MainWindow);
        actionTreemapZoomOut->setObjectName(QString::fromUtf8("actionTreemapZoomOut"));
        QIcon icon10;
        icon10.addFile(QString::fromUtf8(":/icons/magnifier-zoom-out.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionTreemapZoomOut->setIcon(icon10);
        actionResetTreemapZoom = new QAction(MainWindow);
        actionResetTreemapZoom->setObjectName(QString::fromUtf8("actionResetTreemapZoom"));
        QIcon icon11;
        icon11.addFile(QString::fromUtf8(":/icons/magnifier-zoom-actual-equal.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionResetTreemapZoom->setIcon(icon11);
        actionShowTreemap = new QAction(MainWindow);
        actionShowTreemap->setObjectName(QString::fromUtf8("actionShowTreemap"));
        actionShowTreemap->setCheckable(true);
        actionShowTreemap->setChecked(true);
        actionGoUp = new QAction(MainWindow);
        actionGoUp->setObjectName(QString::fromUtf8("actionGoUp"));
        QIcon icon12;
        icon12.addFile(QString::fromUtf8(":/icons/go-up.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionGoUp->setIcon(icon12);
        actionGoToToplevel = new QAction(MainWindow);
        actionGoToToplevel->setObjectName(QString::fromUtf8("actionGoToToplevel"));
        QIcon icon13;
        icon13.addFile(QString::fromUtf8(":/icons/go-top.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionGoToToplevel->setIcon(icon13);
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName(QString::fromUtf8("actionAbout"));
        QIcon icon14;
        icon14.addFile(QString::fromUtf8(":/icons/qdirstat.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionAbout->setIcon(icon14);
        actionAboutQt = new QAction(MainWindow);
        actionAboutQt->setObjectName(QString::fromUtf8("actionAboutQt"));
        actionRefreshSelected = new QAction(MainWindow);
        actionRefreshSelected->setObjectName(QString::fromUtf8("actionRefreshSelected"));
        actionReadExcluded = new QAction(MainWindow);
        actionReadExcluded->setObjectName(QString::fromUtf8("actionReadExcluded"));
        actionContinueReading = new QAction(MainWindow);
        actionContinueReading->setObjectName(QString::fromUtf8("actionContinueReading"));
        actionConfigure = new QAction(MainWindow);
        actionConfigure->setObjectName(QString::fromUtf8("actionConfigure"));
        QIcon icon15;
        icon15.addFile(QString::fromUtf8(":/icons/preferences.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionConfigure->setIcon(icon15);
        actionVerboseSelection = new QAction(MainWindow);
        actionVerboseSelection->setObjectName(QString::fromUtf8("actionVerboseSelection"));
        actionVerboseSelection->setCheckable(true);
        actionMoveToTrash = new QAction(MainWindow);
        actionMoveToTrash->setObjectName(QString::fromUtf8("actionMoveToTrash"));
        QIcon icon16;
        icon16.addFile(QString::fromUtf8(":/icons/trashcan.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionMoveToTrash->setIcon(icon16);
        actionDumpSelection = new QAction(MainWindow);
        actionDumpSelection->setObjectName(QString::fromUtf8("actionDumpSelection"));
        actionFileTypeStats = new QAction(MainWindow);
        actionFileTypeStats->setObjectName(QString::fromUtf8("actionFileTypeStats"));
        actionHelp = new QAction(MainWindow);
        actionHelp->setObjectName(QString::fromUtf8("actionHelp"));
        actionFileSizeStats = new QAction(MainWindow);
        actionFileSizeStats->setObjectName(QString::fromUtf8("actionFileSizeStats"));
        actionShowDetailsPanel = new QAction(MainWindow);
        actionShowDetailsPanel->setObjectName(QString::fromUtf8("actionShowDetailsPanel"));
        actionShowDetailsPanel->setCheckable(true);
        actionShowDetailsPanel->setChecked(true);
        actionLayout1 = new QAction(MainWindow);
        actionLayout1->setObjectName(QString::fromUtf8("actionLayout1"));
        actionLayout1->setCheckable(true);
        actionLayout2 = new QAction(MainWindow);
        actionLayout2->setObjectName(QString::fromUtf8("actionLayout2"));
        actionLayout2->setCheckable(true);
        actionLayout3 = new QAction(MainWindow);
        actionLayout3->setObjectName(QString::fromUtf8("actionLayout3"));
        actionLayout3->setCheckable(true);
        actionWhatsNew = new QAction(MainWindow);
        actionWhatsNew->setObjectName(QString::fromUtf8("actionWhatsNew"));
        actionOpenPkg = new QAction(MainWindow);
        actionOpenPkg->setObjectName(QString::fromUtf8("actionOpenPkg"));
        QIcon icon17;
        icon17.addFile(QString::fromUtf8(":/icons/package.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionOpenPkg->setIcon(icon17);
        actionPkgViewHelp = new QAction(MainWindow);
        actionPkgViewHelp->setObjectName(QString::fromUtf8("actionPkgViewHelp"));
        actionOpenUnpkg = new QAction(MainWindow);
        actionOpenUnpkg->setObjectName(QString::fromUtf8("actionOpenUnpkg"));
        QIcon icon18;
        icon18.addFile(QString::fromUtf8(":/icons/unpkg.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionOpenUnpkg->setIcon(icon18);
        actionUnpkgViewHelp = new QAction(MainWindow);
        actionUnpkgViewHelp->setObjectName(QString::fromUtf8("actionUnpkgViewHelp"));
        actionShowFilesystems = new QAction(MainWindow);
        actionShowFilesystems->setObjectName(QString::fromUtf8("actionShowFilesystems"));
        actionDiscoverLargestFiles = new QAction(MainWindow);
        actionDiscoverLargestFiles->setObjectName(QString::fromUtf8("actionDiscoverLargestFiles"));
        actionDiscoverNewestFiles = new QAction(MainWindow);
        actionDiscoverNewestFiles->setObjectName(QString::fromUtf8("actionDiscoverNewestFiles"));
        actionDiscoverOldestFiles = new QAction(MainWindow);
        actionDiscoverOldestFiles->setObjectName(QString::fromUtf8("actionDiscoverOldestFiles"));
        actionDiscoverHardLinkedFiles = new QAction(MainWindow);
        actionDiscoverHardLinkedFiles->setObjectName(QString::fromUtf8("actionDiscoverHardLinkedFiles"));
        actionDiscoverBrokenSymLinks = new QAction(MainWindow);
        actionDiscoverBrokenSymLinks->setObjectName(QString::fromUtf8("actionDiscoverBrokenSymLinks"));
        actionDiscoverSparseFiles = new QAction(MainWindow);
        actionDiscoverSparseFiles->setObjectName(QString::fromUtf8("actionDiscoverSparseFiles"));
        actionBtrfsSizeReporting = new QAction(MainWindow);
        actionBtrfsSizeReporting->setObjectName(QString::fromUtf8("actionBtrfsSizeReporting"));
        actionShadowedByMount = new QAction(MainWindow);
        actionShadowedByMount->setObjectName(QString::fromUtf8("actionShadowedByMount"));
        actionHeadlessServers = new QAction(MainWindow);
        actionHeadlessServers->setObjectName(QString::fromUtf8("actionHeadlessServers"));
        actionCantMoveDirToTrash = new QAction(MainWindow);
        actionCantMoveDirToTrash->setObjectName(QString::fromUtf8("actionCantMoveDirToTrash"));
        actionTreemapOnSide = new QAction(MainWindow);
        actionTreemapOnSide->setObjectName(QString::fromUtf8("actionTreemapOnSide"));
        actionTreemapOnSide->setCheckable(true);
        actionTreemapOnSide->setChecked(false);
        actionFileAgeStats = new QAction(MainWindow);
        actionFileAgeStats->setObjectName(QString::fromUtf8("actionFileAgeStats"));
        actionGoBack = new QAction(MainWindow);
        actionGoBack->setObjectName(QString::fromUtf8("actionGoBack"));
        QIcon icon19;
        icon19.addFile(QString::fromUtf8(":/icons/go-left.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionGoBack->setIcon(icon19);
        actionGoForward = new QAction(MainWindow);
        actionGoForward->setObjectName(QString::fromUtf8("actionGoForward"));
        QIcon icon20;
        icon20.addFile(QString::fromUtf8(":/icons/go-right.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionGoForward->setIcon(icon20);
        actionFileAgeStatsHelp = new QAction(MainWindow);
        actionFileAgeStatsHelp->setObjectName(QString::fromUtf8("actionFileAgeStatsHelp"));
        actionFindFiles = new QAction(MainWindow);
        actionFindFiles->setObjectName(QString::fromUtf8("actionFindFiles"));
        QIcon icon21;
        icon21.addFile(QString::fromUtf8(":/icons/magnifier-left.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFindFiles->setIcon(icon21);
        actionTreemapHelp = new QAction(MainWindow);
        actionTreemapHelp->setObjectName(QString::fromUtf8("actionTreemapHelp"));
        actionShowStatusBar = new QAction(MainWindow);
        actionShowStatusBar->setObjectName(QString::fromUtf8("actionShowStatusBar"));
        actionShowStatusBar->setCheckable(true);
        actionShowMenuBar = new QAction(MainWindow);
        actionShowMenuBar->setObjectName(QString::fromUtf8("actionShowMenuBar"));
        actionShowMenuBar->setCheckable(true);
        actionDonate = new QAction(MainWindow);
        actionDonate->setObjectName(QString::fromUtf8("actionDonate"));
        actionShowBreadcrumbs = new QAction(MainWindow);
        actionShowBreadcrumbs->setObjectName(QString::fromUtf8("actionShowBreadcrumbs"));
        actionShowBreadcrumbs->setCheckable(true);
        actionShowBreadcrumbs->setChecked(true);
        actionDetailsWithTreemap = new QAction(MainWindow);
        actionDetailsWithTreemap->setObjectName(QString::fromUtf8("actionDetailsWithTreemap"));
        actionDetailsWithTreemap->setCheckable(true);
        actionShowDirTree = new QAction(MainWindow);
        actionShowDirTree->setObjectName(QString::fromUtf8("actionShowDirTree"));
        actionShowDirTree->setCheckable(true);
        actionShowDirTree->setChecked(true);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        verticalLayout = new QVBoxLayout(centralWidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        breadcrumbNavigator = new QDirStat::BreadcrumbNavigator(centralWidget);
        breadcrumbNavigator->setObjectName(QString::fromUtf8("breadcrumbNavigator"));
        QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(breadcrumbNavigator->sizePolicy().hasHeightForWidth());
        breadcrumbNavigator->setSizePolicy(sizePolicy);
        breadcrumbNavigator->setTextInteractionFlags(Qt::TextBrowserInteraction);

        verticalLayout->addWidget(breadcrumbNavigator, 0, Qt::AlignTop);

        mainWinSplitter = new QSplitter(centralWidget);
        mainWinSplitter->setObjectName(QString::fromUtf8("mainWinSplitter"));
        mainWinSplitter->setOrientation(Qt::Vertical);
        topViewsSplitter = new QSplitter(mainWinSplitter);
        topViewsSplitter->setObjectName(QString::fromUtf8("topViewsSplitter"));
        topViewsSplitter->setOrientation(Qt::Horizontal);
        treeViewContainer = new QWidget(topViewsSplitter);
        treeViewContainer->setObjectName(QString::fromUtf8("treeViewContainer"));
        verticalLayout_3 = new QVBoxLayout(treeViewContainer);
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setSpacing(4);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        dirTreeView = new QDirStat::DirTreeView(treeViewContainer);
        dirTreeView->setObjectName(QString::fromUtf8("dirTreeView"));
        dirTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
        dirTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        dirTreeView->setTextElideMode(Qt::ElideMiddle);
        dirTreeView->setUniformRowHeights(true);
        dirTreeView->setSortingEnabled(true);
        dirTreeView->header()->setStretchLastSection(false);

        verticalLayout_2->addWidget(dirTreeView);


        verticalLayout_3->addLayout(verticalLayout_2);

        messagePanel = new QWidget(treeViewContainer);
        messagePanel->setObjectName(QString::fromUtf8("messagePanel"));
        vBox = new QVBoxLayout(messagePanel);
        vBox->setObjectName(QString::fromUtf8("vBox"));
        vBox->setContentsMargins(0, 0, 0, 0);

        verticalLayout_3->addWidget(messagePanel);

        verticalLayout_3->setStretch(0, 1);
        topViewsSplitter->addWidget(treeViewContainer);
        topFileDetailsPanel = new QScrollArea(topViewsSplitter);
        topFileDetailsPanel->setObjectName(QString::fromUtf8("topFileDetailsPanel"));
        topFileDetailsPanel->setEnabled(true);
        topFileDetailsPanel->setFrameShape(QFrame::Box);
        topFileDetailsPanel->setWidgetResizable(true);
        fileDetailsView = new QDirStat::FileDetailsView();
        fileDetailsView->setObjectName(QString::fromUtf8("fileDetailsView"));
        fileDetailsView->setGeometry(QRect(0, 0, 79, 280));
        topFileDetailsPanel->setWidget(fileDetailsView);
        topViewsSplitter->addWidget(topFileDetailsPanel);
        mainWinSplitter->addWidget(topViewsSplitter);
        bottomViewsSplitter = new QSplitter(mainWinSplitter);
        bottomViewsSplitter->setObjectName(QString::fromUtf8("bottomViewsSplitter"));
        bottomViewsSplitter->setOrientation(Qt::Horizontal);
        treemapView = new QDirStat::TreemapView(bottomViewsSplitter);
        treemapView->setObjectName(QString::fromUtf8("treemapView"));
        treemapView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        treemapView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        treemapView->setRenderHints(QPainter::SmoothPixmapTransform|QPainter::TextAntialiasing);
        treemapView->setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing|QGraphicsView::DontSavePainterState);
        bottomViewsSplitter->addWidget(treemapView);
        bottomFileDetailsPanel = new QScrollArea(bottomViewsSplitter);
        bottomFileDetailsPanel->setObjectName(QString::fromUtf8("bottomFileDetailsPanel"));
        bottomFileDetailsPanel->setVisible(false);
        bottomFileDetailsPanel->setFrameShape(QFrame::Box);
        bottomFileDetailsPanel->setWidgetResizable(true);
        bottomViewsSplitter->addWidget(bottomFileDetailsPanel);
        mainWinSplitter->addWidget(bottomViewsSplitter);

        verticalLayout->addWidget(mainWinSplitter);

        verticalLayout->setStretch(1, 1);
        MainWindow->setCentralWidget(centralWidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 1115, 21));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        menuView = new QMenu(menubar);
        menuView->setObjectName(QString::fromUtf8("menuView"));
        menuExpandTreeToLevel = new QMenu(menuView);
        menuExpandTreeToLevel->setObjectName(QString::fromUtf8("menuExpandTreeToLevel"));
        menuEdit = new QMenu(menubar);
        menuEdit->setObjectName(QString::fromUtf8("menuEdit"));
        menuGo = new QMenu(menubar);
        menuGo->setObjectName(QString::fromUtf8("menuGo"));
        menuHelp = new QMenu(menubar);
        menuHelp->setObjectName(QString::fromUtf8("menuHelp"));
        menuProblemsAndSolutions = new QMenu(menuHelp);
        menuProblemsAndSolutions->setObjectName(QString::fromUtf8("menuProblemsAndSolutions"));
        menuCleanup = new QMenu(menubar);
        menuCleanup->setObjectName(QString::fromUtf8("menuCleanup"));
        menuCleanup->setTearOffEnabled(true);
        menuDiscover = new QMenu(menubar);
        menuDiscover->setObjectName(QString::fromUtf8("menuDiscover"));
        MainWindow->setMenuBar(menubar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        MainWindow->setStatusBar(statusBar);
        toolBar = new QToolBar(MainWindow);
        toolBar->setObjectName(QString::fromUtf8("toolBar"));
        MainWindow->addToolBar(Qt::TopToolBarArea, toolBar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuEdit->menuAction());
        menubar->addAction(menuView->menuAction());
        menubar->addAction(menuGo->menuAction());
        menubar->addAction(menuDiscover->menuAction());
        menubar->addAction(menuCleanup->menuAction());
        menubar->addAction(menuHelp->menuAction());
        menuFile->addAction(actionOpenDir);
        menuFile->addAction(actionOpenPkg);
        menuFile->addAction(actionOpenUnpkg);
        menuFile->addSeparator();
        menuFile->addAction(actionStopReading);
        menuFile->addAction(actionRefreshAll);
        menuFile->addSeparator();
        menuFile->addAction(actionRefreshSelected);
        menuFile->addAction(actionReadExcluded);
        menuFile->addAction(actionContinueReading);
        menuFile->addSeparator();
        menuFile->addAction(actionAskWriteCache);
        menuFile->addAction(actionAskReadCache);
        menuFile->addSeparator();
        menuFile->addAction(actionQuit);
        menuView->addAction(actionCloseAllTreeLevels);
        menuView->addAction(menuExpandTreeToLevel->menuAction());
        menuView->addSeparator();
        menuView->addAction(actionLayout1);
        menuView->addAction(actionLayout2);
        menuView->addAction(actionLayout3);
        menuView->addSeparator();
        menuView->addAction(actionShowDirTree);
        menuView->addAction(actionShowBreadcrumbs);
        menuView->addAction(actionShowDetailsPanel);
        menuView->addAction(actionShowTreemap);
        menuView->addSeparator();
        menuView->addAction(actionTreemapOnSide);
        menuView->addAction(actionDetailsWithTreemap);
        menuExpandTreeToLevel->addAction(actionExpandTreeLevel1);
        menuExpandTreeToLevel->addAction(actionExpandTreeLevel2);
        menuExpandTreeToLevel->addAction(actionExpandTreeLevel3);
        menuExpandTreeToLevel->addAction(actionExpandTreeLevel4);
        menuExpandTreeToLevel->addAction(actionExpandTreeLevel5);
        menuEdit->addAction(actionCopyPath);
        menuEdit->addAction(actionFindFiles);
        menuEdit->addSeparator();
        menuEdit->addAction(actionMoveToTrash);
        menuEdit->addSeparator();
        menuEdit->addAction(actionConfigure);
        menuGo->addAction(actionGoBack);
        menuGo->addAction(actionGoForward);
        menuGo->addAction(actionGoUp);
        menuGo->addAction(actionGoToToplevel);
        menuGo->addSeparator();
        menuGo->addAction(actionTreemapZoomTo);
        menuGo->addAction(actionTreemapZoomIn);
        menuGo->addAction(actionTreemapZoomOut);
        menuGo->addAction(actionResetTreemapZoom);
        menuHelp->addAction(actionHelp);
        menuHelp->addSeparator();
        menuHelp->addAction(actionTreemapHelp);
        menuHelp->addAction(actionPkgViewHelp);
        menuHelp->addAction(actionUnpkgViewHelp);
        menuHelp->addAction(actionFileAgeStatsHelp);
        menuHelp->addSeparator();
        menuHelp->addAction(actionWhatsNew);
        menuHelp->addAction(menuProblemsAndSolutions->menuAction());
        menuHelp->addSeparator();
        menuHelp->addAction(actionDonate);
        menuHelp->addAction(actionAboutQt);
        menuHelp->addAction(actionAbout);
        menuProblemsAndSolutions->addAction(actionCantMoveDirToTrash);
        menuProblemsAndSolutions->addAction(actionBtrfsSizeReporting);
        menuProblemsAndSolutions->addAction(actionShadowedByMount);
        menuProblemsAndSolutions->addAction(actionHeadlessServers);
        menuCleanup->addSeparator();
        menuDiscover->addAction(actionDiscoverLargestFiles);
        menuDiscover->addAction(actionDiscoverNewestFiles);
        menuDiscover->addAction(actionDiscoverOldestFiles);
        menuDiscover->addAction(actionDiscoverHardLinkedFiles);
        menuDiscover->addAction(actionDiscoverBrokenSymLinks);
        menuDiscover->addAction(actionDiscoverSparseFiles);
        menuDiscover->addSeparator();
        menuDiscover->addAction(actionFileSizeStats);
        menuDiscover->addAction(actionFileTypeStats);
        menuDiscover->addAction(actionFileAgeStats);
        menuDiscover->addAction(actionShowFilesystems);
        toolBar->addAction(actionOpenDir);
        toolBar->addAction(actionOpenPkg);
        toolBar->addAction(actionOpenUnpkg);
        toolBar->addAction(actionRefreshAll);
        toolBar->addSeparator();
        toolBar->addAction(actionFindFiles);
        toolBar->addSeparator();
        toolBar->addAction(actionGoBack);
        toolBar->addAction(actionGoForward);
        toolBar->addAction(actionGoUp);
        toolBar->addAction(actionGoToToplevel);
        toolBar->addSeparator();
        toolBar->addAction(actionTreemapZoomTo);
        toolBar->addAction(actionTreemapZoomIn);
        toolBar->addAction(actionTreemapZoomOut);
        toolBar->addAction(actionResetTreemapZoom);
        toolBar->addSeparator();
        toolBar->addAction(actionLayout1);
        toolBar->addAction(actionLayout2);
        toolBar->addAction(actionLayout3);
        toolBar->addSeparator();
        toolBar->addAction(actionMoveToTrash);
        toolBar->addSeparator();

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        actionQuit->setText(QCoreApplication::translate("MainWindow", "&Quit", nullptr));
#if QT_CONFIG(shortcut)
        actionQuit->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+Q", nullptr));
#endif // QT_CONFIG(shortcut)
        actionOpenDir->setText(QCoreApplication::translate("MainWindow", "&Open Directory...", nullptr));
#if QT_CONFIG(tooltip)
        actionOpenDir->setToolTip(QCoreApplication::translate("MainWindow", "Read a directory and its contents", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionOpenDir->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+O", nullptr));
#endif // QT_CONFIG(shortcut)
        actionCloseAllTreeLevels->setText(QCoreApplication::translate("MainWindow", "&Close All Tree Branches", nullptr));
        actionExpandTreeLevel1->setText(QCoreApplication::translate("MainWindow", "Level &1", nullptr));
#if QT_CONFIG(shortcut)
        actionExpandTreeLevel1->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+1", nullptr));
#endif // QT_CONFIG(shortcut)
        actionExpandTreeLevel2->setText(QCoreApplication::translate("MainWindow", "Level &2", nullptr));
#if QT_CONFIG(shortcut)
        actionExpandTreeLevel2->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+2", nullptr));
#endif // QT_CONFIG(shortcut)
        actionExpandTreeLevel3->setText(QCoreApplication::translate("MainWindow", "Level &3", nullptr));
#if QT_CONFIG(shortcut)
        actionExpandTreeLevel3->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+3", nullptr));
#endif // QT_CONFIG(shortcut)
        actionExpandTreeLevel4->setText(QCoreApplication::translate("MainWindow", "Level &4", nullptr));
#if QT_CONFIG(shortcut)
        actionExpandTreeLevel4->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+4", nullptr));
#endif // QT_CONFIG(shortcut)
        actionExpandTreeLevel5->setText(QCoreApplication::translate("MainWindow", "Level &5", nullptr));
#if QT_CONFIG(shortcut)
        actionExpandTreeLevel5->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+5", nullptr));
#endif // QT_CONFIG(shortcut)
        actionExpandTreeLevel6->setText(QCoreApplication::translate("MainWindow", "Level &6", nullptr));
#if QT_CONFIG(shortcut)
        actionExpandTreeLevel6->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+6", nullptr));
#endif // QT_CONFIG(shortcut)
        actionExpandTreeLevel7->setText(QCoreApplication::translate("MainWindow", "Level &7", nullptr));
#if QT_CONFIG(shortcut)
        actionExpandTreeLevel7->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+7", nullptr));
#endif // QT_CONFIG(shortcut)
        actionExpandTreeLevel8->setText(QCoreApplication::translate("MainWindow", "Level &8", nullptr));
#if QT_CONFIG(shortcut)
        actionExpandTreeLevel8->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+8", nullptr));
#endif // QT_CONFIG(shortcut)
        actionExpandTreeLevel9->setText(QCoreApplication::translate("MainWindow", "Level &9", nullptr));
#if QT_CONFIG(shortcut)
        actionExpandTreeLevel9->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+9", nullptr));
#endif // QT_CONFIG(shortcut)
        actionStopReading->setText(QCoreApplication::translate("MainWindow", "&Stop Reading", nullptr));
#if QT_CONFIG(tooltip)
        actionStopReading->setToolTip(QCoreApplication::translate("MainWindow", "Stop reading immediately - some directories may be left in an aborted state", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionStopReading->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+S", nullptr));
#endif // QT_CONFIG(shortcut)
        actionAskWriteCache->setText(QCoreApplication::translate("MainWindow", "&Write To Cache File...", nullptr));
        actionAskReadCache->setText(QCoreApplication::translate("MainWindow", "&Read Cache File...", nullptr));
        actionRefreshAll->setText(QCoreApplication::translate("MainWindow", "Refresh &All", nullptr));
#if QT_CONFIG(tooltip)
        actionRefreshAll->setToolTip(QCoreApplication::translate("MainWindow", "Re-read the entire directory tree from disk", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionRefreshAll->setShortcut(QCoreApplication::translate("MainWindow", "F5", nullptr));
#endif // QT_CONFIG(shortcut)
        actionCopyPath->setText(QCoreApplication::translate("MainWindow", "&Copy Path", nullptr));
#if QT_CONFIG(shortcut)
        actionCopyPath->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+C", nullptr));
#endif // QT_CONFIG(shortcut)
        actionTreemapZoomTo->setText(QCoreApplication::translate("MainWindow", "&Zoom Treemap To", nullptr));
#if QT_CONFIG(tooltip)
        actionTreemapZoomTo->setToolTip(QCoreApplication::translate("MainWindow", "Zoom the treemap in to the selected item", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionTreemapZoomTo->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+=", nullptr));
#endif // QT_CONFIG(shortcut)
        actionTreemapZoomIn->setText(QCoreApplication::translate("MainWindow", "Zoom &In Treemap", nullptr));
#if QT_CONFIG(tooltip)
        actionTreemapZoomIn->setToolTip(QCoreApplication::translate("MainWindow", "Zoom the treemap in (enlarge) one level", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionTreemapZoomIn->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl++", nullptr));
#endif // QT_CONFIG(shortcut)
        actionTreemapZoomOut->setText(QCoreApplication::translate("MainWindow", "Zoom &Out Treemap", nullptr));
#if QT_CONFIG(tooltip)
        actionTreemapZoomOut->setToolTip(QCoreApplication::translate("MainWindow", "Zoom the treemap out one level", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionTreemapZoomOut->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+-", nullptr));
#endif // QT_CONFIG(shortcut)
        actionResetTreemapZoom->setText(QCoreApplication::translate("MainWindow", "&Reset Treemap Zoom", nullptr));
#if QT_CONFIG(tooltip)
        actionResetTreemapZoom->setToolTip(QCoreApplication::translate("MainWindow", "Reset the treemap zoom factor to normal", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionResetTreemapZoom->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+0", nullptr));
#endif // QT_CONFIG(shortcut)
        actionShowTreemap->setText(QCoreApplication::translate("MainWindow", "Show &Treemap", nullptr));
#if QT_CONFIG(tooltip)
        actionShowTreemap->setToolTip(QCoreApplication::translate("MainWindow", "Enable or disable showing the treemap view", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionShowTreemap->setShortcut(QCoreApplication::translate("MainWindow", "F9", nullptr));
#endif // QT_CONFIG(shortcut)
        actionGoUp->setText(QCoreApplication::translate("MainWindow", "&Up One Level", nullptr));
#if QT_CONFIG(tooltip)
        actionGoUp->setToolTip(QCoreApplication::translate("MainWindow", "Go up one level in the directory tree", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionGoUp->setShortcut(QCoreApplication::translate("MainWindow", "Alt+Up", nullptr));
#endif // QT_CONFIG(shortcut)
        actionGoToToplevel->setText(QCoreApplication::translate("MainWindow", "To &Toplevel", nullptr));
#if QT_CONFIG(tooltip)
        actionGoToToplevel->setToolTip(QCoreApplication::translate("MainWindow", "Navigate to the top level directory of this tree", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionGoToToplevel->setShortcut(QCoreApplication::translate("MainWindow", "Alt+Home", nullptr));
#endif // QT_CONFIG(shortcut)
        actionAbout->setText(QCoreApplication::translate("MainWindow", "A&bout Qt6DirStat...", nullptr));
#if QT_CONFIG(tooltip)
        actionAbout->setToolTip(QCoreApplication::translate("MainWindow", "About Qt6DirStat", nullptr));
#endif // QT_CONFIG(tooltip)
        actionAboutQt->setText(QCoreApplication::translate("MainWindow", "About &Qt...", nullptr));
        actionRefreshSelected->setText(QCoreApplication::translate("MainWindow", "Re&fresh Selected", nullptr));
#if QT_CONFIG(shortcut)
        actionRefreshSelected->setShortcut(QCoreApplication::translate("MainWindow", "F6", nullptr));
#endif // QT_CONFIG(shortcut)
        actionReadExcluded->setText(QCoreApplication::translate("MainWindow", "Read &Excluded Directory", nullptr));
#if QT_CONFIG(shortcut)
        actionReadExcluded->setShortcut(QCoreApplication::translate("MainWindow", "F6", nullptr));
#endif // QT_CONFIG(shortcut)
        actionContinueReading->setText(QCoreApplication::translate("MainWindow", "Continue Reading at &Mount Point", nullptr));
#if QT_CONFIG(shortcut)
        actionContinueReading->setShortcut(QCoreApplication::translate("MainWindow", "F6", nullptr));
#endif // QT_CONFIG(shortcut)
        actionConfigure->setText(QCoreApplication::translate("MainWindow", "&Settings...", nullptr));
        actionVerboseSelection->setText(QCoreApplication::translate("MainWindow", "Verbose Selection", nullptr));
#if QT_CONFIG(tooltip)
        actionVerboseSelection->setToolTip(QCoreApplication::translate("MainWindow", "Switch verbose logging of selecting and unselecting items on or off", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionVerboseSelection->setShortcut(QCoreApplication::translate("MainWindow", "Shift+F7", nullptr));
#endif // QT_CONFIG(shortcut)
        actionMoveToTrash->setText(QCoreApplication::translate("MainWindow", "Move to &Trash", nullptr));
#if QT_CONFIG(tooltip)
        actionMoveToTrash->setToolTip(QCoreApplication::translate("MainWindow", "Move the selected items to the trash bin", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionMoveToTrash->setShortcut(QCoreApplication::translate("MainWindow", "Del", nullptr));
#endif // QT_CONFIG(shortcut)
        actionDumpSelection->setText(QCoreApplication::translate("MainWindow", "Dump Selection to Log", nullptr));
#if QT_CONFIG(tooltip)
        actionDumpSelection->setToolTip(QCoreApplication::translate("MainWindow", "Dump selected items to the log file", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionDumpSelection->setShortcut(QCoreApplication::translate("MainWindow", "F7", nullptr));
#endif // QT_CONFIG(shortcut)
        actionFileTypeStats->setText(QCoreApplication::translate("MainWindow", "File &Type Statistics", nullptr));
#if QT_CONFIG(tooltip)
        actionFileTypeStats->setToolTip(QCoreApplication::translate("MainWindow", "Open the file type statistics window", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionFileTypeStats->setShortcut(QCoreApplication::translate("MainWindow", "F3", nullptr));
#endif // QT_CONFIG(shortcut)
        actionHelp->setText(QCoreApplication::translate("MainWindow", "QDirStat &Help", nullptr));
#if QT_CONFIG(statustip)
        actionHelp->setStatusTip(QCoreApplication::translate("MainWindow", "https://github.com/shundhammer/qdirstat/blob/master/README.md", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionHelp->setShortcut(QCoreApplication::translate("MainWindow", "F1", nullptr));
#endif // QT_CONFIG(shortcut)
        actionFileSizeStats->setText(QCoreApplication::translate("MainWindow", "File Si&ze Statistics", nullptr));
#if QT_CONFIG(tooltip)
        actionFileSizeStats->setToolTip(QCoreApplication::translate("MainWindow", "Open the file size statistics window", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionFileSizeStats->setShortcut(QCoreApplication::translate("MainWindow", "F2", nullptr));
#endif // QT_CONFIG(shortcut)
        actionShowDetailsPanel->setText(QCoreApplication::translate("MainWindow", "Show Details &Panel", nullptr));
        actionLayout1->setText(QCoreApplication::translate("MainWindow", "Layout &1 (Short)", nullptr));
        actionLayout1->setIconText(QCoreApplication::translate("MainWindow", "L1", nullptr));
#if QT_CONFIG(tooltip)
        actionLayout1->setToolTip(QCoreApplication::translate("MainWindow", "Switch to layout 1 (short)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionLayout1->setShortcut(QCoreApplication::translate("MainWindow", "Alt+1", nullptr));
#endif // QT_CONFIG(shortcut)
        actionLayout2->setText(QCoreApplication::translate("MainWindow", "Layout &2 (Classic)", nullptr));
        actionLayout2->setIconText(QCoreApplication::translate("MainWindow", "L2", nullptr));
#if QT_CONFIG(tooltip)
        actionLayout2->setToolTip(QCoreApplication::translate("MainWindow", "Switch to layout 2 (classic)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionLayout2->setShortcut(QCoreApplication::translate("MainWindow", "Alt+2", nullptr));
#endif // QT_CONFIG(shortcut)
        actionLayout3->setText(QCoreApplication::translate("MainWindow", "Layout &3 (Full)", nullptr));
        actionLayout3->setIconText(QCoreApplication::translate("MainWindow", "L3", nullptr));
#if QT_CONFIG(tooltip)
        actionLayout3->setToolTip(QCoreApplication::translate("MainWindow", "Switch to layout 3 (full)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionLayout3->setShortcut(QCoreApplication::translate("MainWindow", "Alt+3", nullptr));
#endif // QT_CONFIG(shortcut)
        actionWhatsNew->setText(QCoreApplication::translate("MainWindow", "What's &New in this Release...", nullptr));
        actionOpenPkg->setText(QCoreApplication::translate("MainWindow", "Show Installed &Packages...", nullptr));
#if QT_CONFIG(tooltip)
        actionOpenPkg->setToolTip(QCoreApplication::translate("MainWindow", "Show files in installed packages", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionOpenPkg->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+P", nullptr));
#endif // QT_CONFIG(shortcut)
        actionPkgViewHelp->setText(QCoreApplication::translate("MainWindow", "The &Packages View...", nullptr));
#if QT_CONFIG(statustip)
        actionPkgViewHelp->setStatusTip(QCoreApplication::translate("MainWindow", "https://github.com/shundhammer/qdirstat/blob/master/doc/Pkg-View.md", nullptr));
#endif // QT_CONFIG(statustip)
        actionOpenUnpkg->setText(QCoreApplication::translate("MainWindow", "Show &Unpackaged Files...", nullptr));
#if QT_CONFIG(tooltip)
        actionOpenUnpkg->setToolTip(QCoreApplication::translate("MainWindow", "Show only files that do NOT belong to an installed software package", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionOpenUnpkg->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+U", nullptr));
#endif // QT_CONFIG(shortcut)
        actionUnpkgViewHelp->setText(QCoreApplication::translate("MainWindow", "The &Unpackaged Files View...", nullptr));
#if QT_CONFIG(statustip)
        actionUnpkgViewHelp->setStatusTip(QCoreApplication::translate("MainWindow", "https://github.com/shundhammer/qdirstat/blob/master/doc/Unpkg-View.md", nullptr));
#endif // QT_CONFIG(statustip)
        actionShowFilesystems->setText(QCoreApplication::translate("MainWindow", "Show Mounted &Filesystems", nullptr));
#if QT_CONFIG(tooltip)
        actionShowFilesystems->setToolTip(QCoreApplication::translate("MainWindow", "Show mounted filesystems", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionShowFilesystems->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+M", nullptr));
#endif // QT_CONFIG(shortcut)
        actionDiscoverLargestFiles->setText(QCoreApplication::translate("MainWindow", "&Largest Files", nullptr));
        actionDiscoverNewestFiles->setText(QCoreApplication::translate("MainWindow", "&Newest Files", nullptr));
        actionDiscoverOldestFiles->setText(QCoreApplication::translate("MainWindow", "&Oldest Files", nullptr));
        actionDiscoverHardLinkedFiles->setText(QCoreApplication::translate("MainWindow", "Files with Multiple &Hard Links", nullptr));
        actionDiscoverBrokenSymLinks->setText(QCoreApplication::translate("MainWindow", "&Broken Symbolic LInks", nullptr));
        actionDiscoverSparseFiles->setText(QCoreApplication::translate("MainWindow", "&Sparse Files", nullptr));
        actionBtrfsSizeReporting->setText(QCoreApplication::translate("MainWindow", "&Btrfs Size Reporting...", nullptr));
#if QT_CONFIG(statustip)
        actionBtrfsSizeReporting->setStatusTip(QCoreApplication::translate("MainWindow", "https://github.com/shundhammer/qdirstat/blob/master/doc/Btrfs-Free-Size.md", nullptr));
#endif // QT_CONFIG(statustip)
        actionShadowedByMount->setText(QCoreApplication::translate("MainWindow", "Files Shadowed by a &Mount...", nullptr));
#if QT_CONFIG(statustip)
        actionShadowedByMount->setStatusTip(QCoreApplication::translate("MainWindow", "https://github.com/shundhammer/qdirstat/blob/master/doc/Shadowed-by-Mount.md", nullptr));
#endif // QT_CONFIG(statustip)
        actionHeadlessServers->setText(QCoreApplication::translate("MainWindow", "QDirStat for &Headless Servers...", nullptr));
        actionHeadlessServers->setIconText(QCoreApplication::translate("MainWindow", "QDirStat for Headless Servers...", nullptr));
#if QT_CONFIG(statustip)
        actionHeadlessServers->setStatusTip(QCoreApplication::translate("MainWindow", "https://github.com/shundhammer/qdirstat/blob/master/doc/QDirStat-for-Servers.md", nullptr));
#endif // QT_CONFIG(statustip)
        actionCantMoveDirToTrash->setText(QCoreApplication::translate("MainWindow", "Can't Move a Directory to &Trash...", nullptr));
#if QT_CONFIG(statustip)
        actionCantMoveDirToTrash->setStatusTip(QCoreApplication::translate("MainWindow", "https://github.com/shundhammer/qdirstat/blob/master/doc/Troubleshooting.md#cant-move-a-directory-to-trash", nullptr));
#endif // QT_CONFIG(statustip)
        actionTreemapOnSide->setText(QCoreApplication::translate("MainWindow", "Treemap as &Side Panel", nullptr));
        actionFileAgeStats->setText(QCoreApplication::translate("MainWindow", "File &Age Statistics", nullptr));
#if QT_CONFIG(tooltip)
        actionFileAgeStats->setToolTip(QCoreApplication::translate("MainWindow", "Open the file age statistics window", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionFileAgeStats->setShortcut(QCoreApplication::translate("MainWindow", "F4", nullptr));
#endif // QT_CONFIG(shortcut)
        actionGoBack->setText(QCoreApplication::translate("MainWindow", "&Back", nullptr));
#if QT_CONFIG(tooltip)
        actionGoBack->setToolTip(QCoreApplication::translate("MainWindow", "Go back to the previous directory", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionGoBack->setShortcut(QCoreApplication::translate("MainWindow", "Alt+Left", nullptr));
#endif // QT_CONFIG(shortcut)
        actionGoForward->setText(QCoreApplication::translate("MainWindow", "&Forward", nullptr));
#if QT_CONFIG(tooltip)
        actionGoForward->setToolTip(QCoreApplication::translate("MainWindow", "Go forward again to the next directory (after going back)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionGoForward->setShortcut(QCoreApplication::translate("MainWindow", "Alt+Right", nullptr));
#endif // QT_CONFIG(shortcut)
        actionFileAgeStatsHelp->setText(QCoreApplication::translate("MainWindow", "The File &Age Statistics...", nullptr));
#if QT_CONFIG(statustip)
        actionFileAgeStatsHelp->setStatusTip(QCoreApplication::translate("MainWindow", "https://github.com/shundhammer/qdirstat/blob/master/doc/File-Age-Stats.md", nullptr));
#endif // QT_CONFIG(statustip)
        actionFindFiles->setText(QCoreApplication::translate("MainWindow", "&Find...", nullptr));
#if QT_CONFIG(tooltip)
        actionFindFiles->setToolTip(QCoreApplication::translate("MainWindow", "Find files or directories in the scanned Tree", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionFindFiles->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+F", nullptr));
#endif // QT_CONFIG(shortcut)
        actionTreemapHelp->setText(QCoreApplication::translate("MainWindow", "The &Treemap Graphics...", nullptr));
#if QT_CONFIG(tooltip)
        actionTreemapHelp->setToolTip(QCoreApplication::translate("MainWindow", "The colored graphics in the main window explained", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionTreemapHelp->setStatusTip(QCoreApplication::translate("MainWindow", "https://github.com/shundhammer/qdirstat/blob/master/doc/Treemap.md", nullptr));
#endif // QT_CONFIG(statustip)
        actionShowStatusBar->setText(QCoreApplication::translate("MainWindow", "Show &Status Bar", nullptr));
        actionShowMenuBar->setText(QCoreApplication::translate("MainWindow", "Show &Menu Bar", nullptr));
        actionDonate->setText(QCoreApplication::translate("MainWindow", "&Donate...", nullptr));
        actionShowBreadcrumbs->setText(QCoreApplication::translate("MainWindow", "Show &Breadcrumbs", nullptr));
        actionDetailsWithTreemap->setText(QCoreApplication::translate("MainWindow", "File Details &with Treemap", nullptr));
        actionShowDirTree->setText(QCoreApplication::translate("MainWindow", "Show &DirTree", nullptr));
#if QT_CONFIG(shortcut)
        actionShowDirTree->setShortcut(QCoreApplication::translate("MainWindow", "F8", nullptr));
#endif // QT_CONFIG(shortcut)
        menuFile->setTitle(QCoreApplication::translate("MainWindow", "&File", nullptr));
        menuView->setTitle(QCoreApplication::translate("MainWindow", "&View", nullptr));
        menuExpandTreeToLevel->setTitle(QCoreApplication::translate("MainWindow", "E&xpand Tree to Level", nullptr));
        menuEdit->setTitle(QCoreApplication::translate("MainWindow", "&Edit", nullptr));
        menuGo->setTitle(QCoreApplication::translate("MainWindow", "&Go", nullptr));
        menuHelp->setTitle(QCoreApplication::translate("MainWindow", "&Help", nullptr));
        menuProblemsAndSolutions->setTitle(QCoreApplication::translate("MainWindow", "Problems and &Solutions", nullptr));
        menuCleanup->setTitle(QCoreApplication::translate("MainWindow", "&Clean Up", nullptr));
        menuDiscover->setTitle(QCoreApplication::translate("MainWindow", "&Discover", nullptr));
        toolBar->setWindowTitle(QCoreApplication::translate("MainWindow", "Main Toolbar", nullptr));
        (void)MainWindow;
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAIN_2D_WINDOW_H
