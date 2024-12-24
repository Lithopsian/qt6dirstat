/********************************************************************************
** Form generated from reading UI file 'file-details-view.ui'
**
** Created by: Qt User Interface Compiler version 5.15.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FILE_2D_DETAILS_2D_VIEW_H
#define UI_FILE_2D_DETAILS_2D_VIEW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_FileDetailsView
{
public:
    QWidget *fileDetailsPage;
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *fileHeadingHBox;
    QLabel *fileIcon;
    QLabel *specialIcon;
    QLabel *blockIcon;
    QLabel *charIcon;
    QLabel *symlinkIcon;
    QLabel *fileNameLabel;
    QGridLayout *gridLayout;
    QLabel *fileTypeCaption;
    QLabel *fileTypeLabel;
    QLabel *fileMimeCaption;
    QLabel *fileMimeLabel;
    QLabel *fileLinkCaption;
    QLabel *fileLinkLabel;
    QLabel *fileSizeCaption;
    QLabel *fileSizeLabel;
    QLabel *fileAllocatedCaption;
    QLabel *fileAllocatedLabel;
    QLabel *fileSpacer;
    QLabel *fileUserCaption;
    QLabel *fileUserLabel;
    QLabel *fileGroupCaption;
    QLabel *fileGroupLabel;
    QLabel *filePermissionsCaption;
    QLabel *filePermissionsLabel;
    QLabel *fileMTimeCaption;
    QLabel *fileMTimeLabel;
    QVBoxLayout *verticalLayout_2;
    QLabel *fileSystemFileWarning;
    QLabel *filePackageCaption;
    QLabel *filePackageLabel;
    QSpacerItem *fileBottomVSpacer;
    QWidget *dirDetailsPage;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *dirHeadingHBox;
    QLabel *dirIcon;
    QLabel *mountPointIcon;
    QLabel *dotEntryIcon;
    QLabel *dirUnreadableIcon;
    QLabel *dirNameLabel;
    QGridLayout *dirDetailsGrid;
    QLabel *dirTypeCaption;
    QLabel *dirTypeLabel;
    QLabel *dirSpacer;
    QHBoxLayout *dirIconHBox;
    QLabel *dirFromCacheIcon;
    QLabel *dirDuplicateIcon;
    QSpacerItem *dirIconHSpacer;
    QLabel *dirSubTreeHeading;
    QLabel *dirTotalSizeCaption;
    QLabel *dirTotalSizeLabel;
    QLabel *dirAllocatedCaption;
    QLabel *dirAllocatedLabel;
    QLabel *dirItemCountCaption;
    QLabel *dirItemCountLabel;
    QLabel *dirFileCountCaption;
    QLabel *dirFileCountLabel;
    QLabel *dirSubDirCountCaption;
    QLabel *dirSubDirCountLabel;
    QLabel *dirLatestMTimeCaption;
    QLabel *dirLatestMTimeLabel;
    QWidget *dirDirectoryGrid;
    QGridLayout *dirDirectoryGridLayout;
    QLabel *dirUserCaption;
    QLabel *dirMTimeLabel;
    QLabel *dirGroupCaption;
    QLabel *dirPermissionsLabel;
    QLabel *dirMTimeCaption;
    QLabel *dirUserLabel;
    QLabel *dirPermissionsCaption;
    QLabel *dirGroupLabel;
    QLabel *dirOwnSizeCaption;
    QLabel *dirDirectoryHeading;
    QLabel *dirOwnSizeLabel;
    QSpacerItem *dirBottomVSpacer;
    QWidget *pkgDetailsPage;
    QHBoxLayout *pkgDetailsHBox;
    QVBoxLayout *pkgDetailsVBox;
    QHBoxLayout *pkgHeadingHBox;
    QLabel *pkgIcon;
    QLabel *pkgNameLabel;
    QGridLayout *pkgGridLayout;
    QLabel *pkgLatestMTimeLabel;
    QLabel *pkgItemCountCaption;
    QLabel *pkgInstalledFilesHeading;
    QLabel *pkgFileCountLabel;
    QLabel *pkgTypeCaption;
    QLabel *pkgAllocatedCaption;
    QLabel *pkgVersionCaption;
    QLabel *pkgTypeLabel;
    QLabel *pkgTotalSizeCaption;
    QLabel *pkgTotalSizeLabel;
    QLabel *pkgLatestMTimeCaption;
    QLabel *pkgAllocatedLabel;
    QLabel *pkgSubDirCountLabel;
    QLabel *pkgSubDirCountCaption;
    QLabel *pkgVersionLabel;
    QLabel *pkgFileCountCaption;
    QLabel *pkgArchLabel;
    QLabel *pkgItemCountLabel;
    QLabel *pkgArchCaption;
    QLabel *pkgSpacer;
    QSpacerItem *pkgBottomVSpacer;
    QWidget *pkgSummaryPage;
    QHBoxLayout *horizontalLayout_11;
    QVBoxLayout *pkgSummaryVBox;
    QLabel *pkgSummaryHeading;
    QGridLayout *pkgSummaryGridLayout;
    QLabel *pkgSummaryLatestMTimeCaption;
    QLabel *pkgSummaryPkgCountLabel;
    QLabel *pkgSummarySubDirCountCaption;
    QLabel *pkgSummaryItemCountLabel;
    QLabel *pkgSummarySubDirCountLabel;
    QLabel *pkgSummaryItemCountCaption;
    QLabel *pkgSummaryFileCountCaption;
    QLabel *pkgSummaryFileCountLabel;
    QLabel *pkgSummaryInstalledFilesHeading;
    QLabel *pkgSummaryAllocatedLabel;
    QLabel *pkgSummaryAllocatedCaption;
    QLabel *pkgSummaryLatestMTimeLabel;
    QLabel *pkgSummaryTotalSizeLabel;
    QLabel *pkgSummaryTotalSizeCaption;
    QLabel *pkgSummaryPkgCountCaption;
    QLabel *pkgSummarySpacer;
    QSpacerItem *pkgSummaryBottomVSpacer;
    QWidget *selectionSummaryPage;
    QHBoxLayout *selPageHBox;
    QVBoxLayout *selectionSummaryVBox;
    QLabel *selHeading;
    QGridLayout *selGridLayout;
    QLabel *selTotalSizeCaption;
    QLabel *selAllocatedLabel;
    QLabel *selSubtreeFileCountLabel;
    QLabel *selFileCountLabel;
    QLabel *selDirCountCaption;
    QLabel *selFileCountCaption;
    QLabel *selDirCountLabel;
    QLabel *selTotalSizeLabel;
    QLabel *selSubtreeFileCountCaption;
    QLabel *selAllocatedCaption;
    QSpacerItem *selBottomVSpacer;
    QWidget *emptyPage;
    QVBoxLayout *verticalLayout_4;

    void setupUi(QStackedWidget *FileDetailsView)
    {
        if (FileDetailsView->objectName().isEmpty())
            FileDetailsView->setObjectName(QString::fromUtf8("FileDetailsView"));
        FileDetailsView->resize(250, 389);
        fileDetailsPage = new QWidget();
        fileDetailsPage->setObjectName(QString::fromUtf8("fileDetailsPage"));
        verticalLayout_3 = new QVBoxLayout(fileDetailsPage);
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        fileHeadingHBox = new QHBoxLayout();
        fileHeadingHBox->setSpacing(5);
        fileHeadingHBox->setObjectName(QString::fromUtf8("fileHeadingHBox"));
        fileIcon = new QLabel(fileDetailsPage);
        fileIcon->setObjectName(QString::fromUtf8("fileIcon"));
        fileIcon->setPixmap(QPixmap(QString::fromUtf8(":/icons/tree-medium/file.png")));

        fileHeadingHBox->addWidget(fileIcon);

        specialIcon = new QLabel(fileDetailsPage);
        specialIcon->setObjectName(QString::fromUtf8("specialIcon"));
        specialIcon->setPixmap(QPixmap(QString::fromUtf8(":/icons/tree-medium/special.png")));

        fileHeadingHBox->addWidget(specialIcon);

        blockIcon = new QLabel(fileDetailsPage);
        blockIcon->setObjectName(QString::fromUtf8("blockIcon"));
        blockIcon->setPixmap(QPixmap(QString::fromUtf8(":/icons/tree-medium/block-device.png")));

        fileHeadingHBox->addWidget(blockIcon);

        charIcon = new QLabel(fileDetailsPage);
        charIcon->setObjectName(QString::fromUtf8("charIcon"));
        charIcon->setPixmap(QPixmap(QString::fromUtf8(":/icons/tree-medium/char-device.png")));

        fileHeadingHBox->addWidget(charIcon);

        symlinkIcon = new QLabel(fileDetailsPage);
        symlinkIcon->setObjectName(QString::fromUtf8("symlinkIcon"));
        symlinkIcon->setPixmap(QPixmap(QString::fromUtf8(":/icons/tree-medium/symlink.png")));

        fileHeadingHBox->addWidget(symlinkIcon);

        fileNameLabel = new QLabel(fileDetailsPage);
        fileNameLabel->setObjectName(QString::fromUtf8("fileNameLabel"));
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        fileNameLabel->setFont(font);
        fileNameLabel->setText(QString::fromUtf8("myfile.txt"));
        fileNameLabel->setTextFormat(Qt::PlainText);
        fileNameLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);

        fileHeadingHBox->addWidget(fileNameLabel);

        fileHeadingHBox->setStretch(5, 1);

        verticalLayout_3->addLayout(fileHeadingHBox);

        gridLayout = new QGridLayout();
        gridLayout->setSpacing(5);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(-1, 15, -1, -1);
        fileTypeCaption = new QLabel(fileDetailsPage);
        fileTypeCaption->setObjectName(QString::fromUtf8("fileTypeCaption"));
        QFont font1;
        font1.setItalic(true);
        fileTypeCaption->setFont(font1);

        gridLayout->addWidget(fileTypeCaption, 0, 0, 1, 1);

        fileTypeLabel = new QLabel(fileDetailsPage);
        fileTypeLabel->setObjectName(QString::fromUtf8("fileTypeLabel"));
        fileTypeLabel->setText(QString::fromUtf8("file"));

        gridLayout->addWidget(fileTypeLabel, 0, 1, 1, 1);

        fileMimeCaption = new QLabel(fileDetailsPage);
        fileMimeCaption->setObjectName(QString::fromUtf8("fileMimeCaption"));
        fileMimeCaption->setFont(font1);
        fileMimeCaption->setText(QString::fromUtf8("Mime category:"));

        gridLayout->addWidget(fileMimeCaption, 1, 0, 1, 1);

        fileMimeLabel = new QLabel(fileDetailsPage);
        fileMimeLabel->setObjectName(QString::fromUtf8("fileMimeLabel"));
        fileMimeLabel->setText(QString::fromUtf8("category"));
        fileMimeLabel->setTextFormat(Qt::PlainText);

        gridLayout->addWidget(fileMimeLabel, 1, 1, 1, 1);

        fileLinkCaption = new QLabel(fileDetailsPage);
        fileLinkCaption->setObjectName(QString::fromUtf8("fileLinkCaption"));
        fileLinkCaption->setFont(font1);

        gridLayout->addWidget(fileLinkCaption, 2, 0, 1, 1);

        fileLinkLabel = new QLabel(fileDetailsPage);
        fileLinkLabel->setObjectName(QString::fromUtf8("fileLinkLabel"));
        fileLinkLabel->setTextFormat(Qt::PlainText);

        gridLayout->addWidget(fileLinkLabel, 2, 1, 1, 1);

        fileSizeCaption = new QLabel(fileDetailsPage);
        fileSizeCaption->setObjectName(QString::fromUtf8("fileSizeCaption"));
        fileSizeCaption->setFont(font1);

        gridLayout->addWidget(fileSizeCaption, 3, 0, 1, 1);

        fileSizeLabel = new QLabel(fileDetailsPage);
        fileSizeLabel->setObjectName(QString::fromUtf8("fileSizeLabel"));
        fileSizeLabel->setText(QString::fromUtf8("42.0 kB"));

        gridLayout->addWidget(fileSizeLabel, 3, 1, 1, 1);

        fileAllocatedCaption = new QLabel(fileDetailsPage);
        fileAllocatedCaption->setObjectName(QString::fromUtf8("fileAllocatedCaption"));
        fileAllocatedCaption->setFont(font1);

        gridLayout->addWidget(fileAllocatedCaption, 4, 0, 1, 1);

        fileAllocatedLabel = new QLabel(fileDetailsPage);
        fileAllocatedLabel->setObjectName(QString::fromUtf8("fileAllocatedLabel"));
        fileAllocatedLabel->setText(QString::fromUtf8("42.0 kB"));

        gridLayout->addWidget(fileAllocatedLabel, 4, 1, 1, 1);

        fileSpacer = new QLabel(fileDetailsPage);
        fileSpacer->setObjectName(QString::fromUtf8("fileSpacer"));
        fileSpacer->setMaximumSize(QSize(16777215, 10));

        gridLayout->addWidget(fileSpacer, 5, 0, 1, 1);

        fileUserCaption = new QLabel(fileDetailsPage);
        fileUserCaption->setObjectName(QString::fromUtf8("fileUserCaption"));
        fileUserCaption->setFont(font1);

        gridLayout->addWidget(fileUserCaption, 6, 0, 1, 1);

        fileUserLabel = new QLabel(fileDetailsPage);
        fileUserLabel->setObjectName(QString::fromUtf8("fileUserLabel"));
        fileUserLabel->setText(QString::fromUtf8("user"));

        gridLayout->addWidget(fileUserLabel, 6, 1, 1, 1);

        fileGroupCaption = new QLabel(fileDetailsPage);
        fileGroupCaption->setObjectName(QString::fromUtf8("fileGroupCaption"));
        fileGroupCaption->setFont(font1);

        gridLayout->addWidget(fileGroupCaption, 7, 0, 1, 1);

        fileGroupLabel = new QLabel(fileDetailsPage);
        fileGroupLabel->setObjectName(QString::fromUtf8("fileGroupLabel"));
        fileGroupLabel->setText(QString::fromUtf8("group"));

        gridLayout->addWidget(fileGroupLabel, 7, 1, 1, 1);

        filePermissionsCaption = new QLabel(fileDetailsPage);
        filePermissionsCaption->setObjectName(QString::fromUtf8("filePermissionsCaption"));
        filePermissionsCaption->setFont(font1);

        gridLayout->addWidget(filePermissionsCaption, 8, 0, 1, 1);

        filePermissionsLabel = new QLabel(fileDetailsPage);
        filePermissionsLabel->setObjectName(QString::fromUtf8("filePermissionsLabel"));
        filePermissionsLabel->setText(QString::fromUtf8("rw-r--r--  0644"));

        gridLayout->addWidget(filePermissionsLabel, 8, 1, 1, 1);

        fileMTimeCaption = new QLabel(fileDetailsPage);
        fileMTimeCaption->setObjectName(QString::fromUtf8("fileMTimeCaption"));
        fileMTimeCaption->setFont(font1);

        gridLayout->addWidget(fileMTimeCaption, 9, 0, 1, 1);

        fileMTimeLabel = new QLabel(fileDetailsPage);
        fileMTimeLabel->setObjectName(QString::fromUtf8("fileMTimeLabel"));
        fileMTimeLabel->setText(QString::fromUtf8("31.06.2018 09:18"));

        gridLayout->addWidget(fileMTimeLabel, 9, 1, 1, 1);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(-1, 15, -1, -1);
        fileSystemFileWarning = new QLabel(fileDetailsPage);
        fileSystemFileWarning->setObjectName(QString::fromUtf8("fileSystemFileWarning"));
        QFont font2;
        font2.setBold(true);
        font2.setItalic(true);
        font2.setWeight(75);
        fileSystemFileWarning->setFont(font2);

        verticalLayout_2->addWidget(fileSystemFileWarning);


        gridLayout->addLayout(verticalLayout_2, 10, 0, 1, 1);

        filePackageCaption = new QLabel(fileDetailsPage);
        filePackageCaption->setObjectName(QString::fromUtf8("filePackageCaption"));
        filePackageCaption->setFont(font1);

        gridLayout->addWidget(filePackageCaption, 11, 0, 1, 1);

        filePackageLabel = new QLabel(fileDetailsPage);
        filePackageLabel->setObjectName(QString::fromUtf8("filePackageLabel"));
        filePackageLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);

        gridLayout->addWidget(filePackageLabel, 11, 1, 1, 1);

        gridLayout->setColumnStretch(1, 1);

        verticalLayout_3->addLayout(gridLayout);

        fileBottomVSpacer = new QSpacerItem(13, 84, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(fileBottomVSpacer);

        FileDetailsView->addWidget(fileDetailsPage);
        dirDetailsPage = new QWidget();
        dirDetailsPage->setObjectName(QString::fromUtf8("dirDetailsPage"));
        verticalLayout = new QVBoxLayout(dirDetailsPage);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        dirHeadingHBox = new QHBoxLayout();
        dirHeadingHBox->setSpacing(5);
        dirHeadingHBox->setObjectName(QString::fromUtf8("dirHeadingHBox"));
        dirIcon = new QLabel(dirDetailsPage);
        dirIcon->setObjectName(QString::fromUtf8("dirIcon"));
        dirIcon->setPixmap(QPixmap(QString::fromUtf8(":/icons/tree-medium/dir.png")));

        dirHeadingHBox->addWidget(dirIcon);

        mountPointIcon = new QLabel(dirDetailsPage);
        mountPointIcon->setObjectName(QString::fromUtf8("mountPointIcon"));
        mountPointIcon->setPixmap(QPixmap(QString::fromUtf8(":/icons/tree-medium/mount-point.png")));

        dirHeadingHBox->addWidget(mountPointIcon);

        dotEntryIcon = new QLabel(dirDetailsPage);
        dotEntryIcon->setObjectName(QString::fromUtf8("dotEntryIcon"));
        dotEntryIcon->setPixmap(QPixmap(QString::fromUtf8(":/icons/tree-medium/dot-entry.png")));

        dirHeadingHBox->addWidget(dotEntryIcon);

        dirUnreadableIcon = new QLabel(dirDetailsPage);
        dirUnreadableIcon->setObjectName(QString::fromUtf8("dirUnreadableIcon"));
        dirUnreadableIcon->setPixmap(QPixmap(QString::fromUtf8(":/icons/tree-medium/unreadable-dir.png")));

        dirHeadingHBox->addWidget(dirUnreadableIcon);

        dirNameLabel = new QLabel(dirDetailsPage);
        dirNameLabel->setObjectName(QString::fromUtf8("dirNameLabel"));
        dirNameLabel->setFont(font);
        dirNameLabel->setText(QString::fromUtf8("doc/"));
        dirNameLabel->setTextFormat(Qt::PlainText);
        dirNameLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);

        dirHeadingHBox->addWidget(dirNameLabel);

        dirHeadingHBox->setStretch(4, 1);

        verticalLayout->addLayout(dirHeadingHBox);

        dirDetailsGrid = new QGridLayout();
        dirDetailsGrid->setSpacing(5);
        dirDetailsGrid->setObjectName(QString::fromUtf8("dirDetailsGrid"));
        dirDetailsGrid->setContentsMargins(-1, 15, -1, -1);
        dirTypeCaption = new QLabel(dirDetailsPage);
        dirTypeCaption->setObjectName(QString::fromUtf8("dirTypeCaption"));
        dirTypeCaption->setFont(font1);

        dirDetailsGrid->addWidget(dirTypeCaption, 0, 0, 1, 1);

        dirTypeLabel = new QLabel(dirDetailsPage);
        dirTypeLabel->setObjectName(QString::fromUtf8("dirTypeLabel"));
        dirTypeLabel->setText(QString::fromUtf8("directory"));

        dirDetailsGrid->addWidget(dirTypeLabel, 0, 1, 1, 1);

        dirSpacer = new QLabel(dirDetailsPage);
        dirSpacer->setObjectName(QString::fromUtf8("dirSpacer"));
        dirSpacer->setMaximumSize(QSize(16777215, 10));

        dirDetailsGrid->addWidget(dirSpacer, 1, 0, 1, 1);

        dirIconHBox = new QHBoxLayout();
        dirIconHBox->setSpacing(8);
        dirIconHBox->setObjectName(QString::fromUtf8("dirIconHBox"));
        dirFromCacheIcon = new QLabel(dirDetailsPage);
        dirFromCacheIcon->setObjectName(QString::fromUtf8("dirFromCacheIcon"));
        dirFromCacheIcon->setPixmap(QPixmap(QString::fromUtf8(":/icons/import.png")));
        dirFromCacheIcon->setAlignment(Qt::AlignBottom|Qt::AlignLeading|Qt::AlignLeft);

        dirIconHBox->addWidget(dirFromCacheIcon);

        dirDuplicateIcon = new QLabel(dirDetailsPage);
        dirDuplicateIcon->setObjectName(QString::fromUtf8("dirDuplicateIcon"));
        dirDuplicateIcon->setPixmap(QPixmap(QString::fromUtf8(":/icons/bind-mount.png")));
        dirDuplicateIcon->setAlignment(Qt::AlignBottom|Qt::AlignLeading|Qt::AlignLeft);

        dirIconHBox->addWidget(dirDuplicateIcon);

        dirIconHSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        dirIconHBox->addItem(dirIconHSpacer);


        dirDetailsGrid->addLayout(dirIconHBox, 1, 1, 2, 1);

        dirSubTreeHeading = new QLabel(dirDetailsPage);
        dirSubTreeHeading->setObjectName(QString::fromUtf8("dirSubTreeHeading"));
        dirSubTreeHeading->setFont(font2);

        dirDetailsGrid->addWidget(dirSubTreeHeading, 2, 0, 1, 1);

        dirTotalSizeCaption = new QLabel(dirDetailsPage);
        dirTotalSizeCaption->setObjectName(QString::fromUtf8("dirTotalSizeCaption"));
        dirTotalSizeCaption->setFont(font1);

        dirDetailsGrid->addWidget(dirTotalSizeCaption, 3, 0, 1, 1);

        dirTotalSizeLabel = new QLabel(dirDetailsPage);
        dirTotalSizeLabel->setObjectName(QString::fromUtf8("dirTotalSizeLabel"));
        dirTotalSizeLabel->setText(QString::fromUtf8("512.0 MiB"));

        dirDetailsGrid->addWidget(dirTotalSizeLabel, 3, 1, 1, 1);

        dirAllocatedCaption = new QLabel(dirDetailsPage);
        dirAllocatedCaption->setObjectName(QString::fromUtf8("dirAllocatedCaption"));
        dirAllocatedCaption->setFont(font1);

        dirDetailsGrid->addWidget(dirAllocatedCaption, 4, 0, 1, 1);

        dirAllocatedLabel = new QLabel(dirDetailsPage);
        dirAllocatedLabel->setObjectName(QString::fromUtf8("dirAllocatedLabel"));
        dirAllocatedLabel->setText(QString::fromUtf8("512.0 MiB"));

        dirDetailsGrid->addWidget(dirAllocatedLabel, 4, 1, 1, 1);

        dirItemCountCaption = new QLabel(dirDetailsPage);
        dirItemCountCaption->setObjectName(QString::fromUtf8("dirItemCountCaption"));
        dirItemCountCaption->setFont(font1);

        dirDetailsGrid->addWidget(dirItemCountCaption, 5, 0, 1, 1);

        dirItemCountLabel = new QLabel(dirDetailsPage);
        dirItemCountLabel->setObjectName(QString::fromUtf8("dirItemCountLabel"));
        dirItemCountLabel->setText(QString::fromUtf8("17"));

        dirDetailsGrid->addWidget(dirItemCountLabel, 5, 1, 1, 1);

        dirFileCountCaption = new QLabel(dirDetailsPage);
        dirFileCountCaption->setObjectName(QString::fromUtf8("dirFileCountCaption"));
        dirFileCountCaption->setFont(font1);

        dirDetailsGrid->addWidget(dirFileCountCaption, 6, 0, 1, 1);

        dirFileCountLabel = new QLabel(dirDetailsPage);
        dirFileCountLabel->setObjectName(QString::fromUtf8("dirFileCountLabel"));
        dirFileCountLabel->setText(QString::fromUtf8("15"));

        dirDetailsGrid->addWidget(dirFileCountLabel, 6, 1, 1, 1);

        dirSubDirCountCaption = new QLabel(dirDetailsPage);
        dirSubDirCountCaption->setObjectName(QString::fromUtf8("dirSubDirCountCaption"));
        dirSubDirCountCaption->setFont(font1);

        dirDetailsGrid->addWidget(dirSubDirCountCaption, 7, 0, 1, 1);

        dirSubDirCountLabel = new QLabel(dirDetailsPage);
        dirSubDirCountLabel->setObjectName(QString::fromUtf8("dirSubDirCountLabel"));
        dirSubDirCountLabel->setText(QString::fromUtf8("2"));

        dirDetailsGrid->addWidget(dirSubDirCountLabel, 7, 1, 1, 1);

        dirLatestMTimeCaption = new QLabel(dirDetailsPage);
        dirLatestMTimeCaption->setObjectName(QString::fromUtf8("dirLatestMTimeCaption"));
        dirLatestMTimeCaption->setFont(font1);

        dirDetailsGrid->addWidget(dirLatestMTimeCaption, 8, 0, 1, 1);

        dirLatestMTimeLabel = new QLabel(dirDetailsPage);
        dirLatestMTimeLabel->setObjectName(QString::fromUtf8("dirLatestMTimeLabel"));
        dirLatestMTimeLabel->setText(QString::fromUtf8("31.06.2018 09:18"));

        dirDetailsGrid->addWidget(dirLatestMTimeLabel, 8, 1, 1, 1);

        dirDetailsGrid->setColumnStretch(1, 1);

        verticalLayout->addLayout(dirDetailsGrid);

        dirDirectoryGrid = new QWidget(dirDetailsPage);
        dirDirectoryGrid->setObjectName(QString::fromUtf8("dirDirectoryGrid"));
        dirDirectoryGridLayout = new QGridLayout(dirDirectoryGrid);
        dirDirectoryGridLayout->setObjectName(QString::fromUtf8("dirDirectoryGridLayout"));
        dirDirectoryGridLayout->setContentsMargins(0, 15, 0, 0);
        dirUserCaption = new QLabel(dirDirectoryGrid);
        dirUserCaption->setObjectName(QString::fromUtf8("dirUserCaption"));
        dirUserCaption->setFont(font1);

        dirDirectoryGridLayout->addWidget(dirUserCaption, 2, 0, 1, 1);

        dirMTimeLabel = new QLabel(dirDirectoryGrid);
        dirMTimeLabel->setObjectName(QString::fromUtf8("dirMTimeLabel"));
        dirMTimeLabel->setText(QString::fromUtf8("31.06.2018 09:18"));

        dirDirectoryGridLayout->addWidget(dirMTimeLabel, 5, 1, 1, 1);

        dirGroupCaption = new QLabel(dirDirectoryGrid);
        dirGroupCaption->setObjectName(QString::fromUtf8("dirGroupCaption"));
        dirGroupCaption->setFont(font1);

        dirDirectoryGridLayout->addWidget(dirGroupCaption, 3, 0, 1, 1);

        dirPermissionsLabel = new QLabel(dirDirectoryGrid);
        dirPermissionsLabel->setObjectName(QString::fromUtf8("dirPermissionsLabel"));
        dirPermissionsLabel->setText(QString::fromUtf8("rwxr-xr-x  0755"));

        dirDirectoryGridLayout->addWidget(dirPermissionsLabel, 4, 1, 1, 1);

        dirMTimeCaption = new QLabel(dirDirectoryGrid);
        dirMTimeCaption->setObjectName(QString::fromUtf8("dirMTimeCaption"));
        dirMTimeCaption->setFont(font1);

        dirDirectoryGridLayout->addWidget(dirMTimeCaption, 5, 0, 1, 1);

        dirUserLabel = new QLabel(dirDirectoryGrid);
        dirUserLabel->setObjectName(QString::fromUtf8("dirUserLabel"));
        dirUserLabel->setText(QString::fromUtf8("kilroy"));

        dirDirectoryGridLayout->addWidget(dirUserLabel, 2, 1, 1, 1);

        dirPermissionsCaption = new QLabel(dirDirectoryGrid);
        dirPermissionsCaption->setObjectName(QString::fromUtf8("dirPermissionsCaption"));
        dirPermissionsCaption->setFont(font1);

        dirDirectoryGridLayout->addWidget(dirPermissionsCaption, 4, 0, 1, 1);

        dirGroupLabel = new QLabel(dirDirectoryGrid);
        dirGroupLabel->setObjectName(QString::fromUtf8("dirGroupLabel"));
        dirGroupLabel->setText(QString::fromUtf8("users"));

        dirDirectoryGridLayout->addWidget(dirGroupLabel, 3, 1, 1, 1);

        dirOwnSizeCaption = new QLabel(dirDirectoryGrid);
        dirOwnSizeCaption->setObjectName(QString::fromUtf8("dirOwnSizeCaption"));
        dirOwnSizeCaption->setFont(font1);

        dirDirectoryGridLayout->addWidget(dirOwnSizeCaption, 1, 0, 1, 1);

        dirDirectoryHeading = new QLabel(dirDirectoryGrid);
        dirDirectoryHeading->setObjectName(QString::fromUtf8("dirDirectoryHeading"));
        dirDirectoryHeading->setFont(font2);

        dirDirectoryGridLayout->addWidget(dirDirectoryHeading, 0, 0, 1, 1);

        dirOwnSizeLabel = new QLabel(dirDirectoryGrid);
        dirOwnSizeLabel->setObjectName(QString::fromUtf8("dirOwnSizeLabel"));
        dirOwnSizeLabel->setText(QString::fromUtf8("4.0 kiB"));

        dirDirectoryGridLayout->addWidget(dirOwnSizeLabel, 1, 1, 1, 1);

        dirDirectoryGridLayout->setColumnStretch(1, 1);

        verticalLayout->addWidget(dirDirectoryGrid);

        dirBottomVSpacer = new QSpacerItem(0, 30, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(dirBottomVSpacer);

        FileDetailsView->addWidget(dirDetailsPage);
        pkgDetailsPage = new QWidget();
        pkgDetailsPage->setObjectName(QString::fromUtf8("pkgDetailsPage"));
        pkgDetailsHBox = new QHBoxLayout(pkgDetailsPage);
        pkgDetailsHBox->setObjectName(QString::fromUtf8("pkgDetailsHBox"));
        pkgDetailsVBox = new QVBoxLayout();
        pkgDetailsVBox->setSpacing(0);
        pkgDetailsVBox->setObjectName(QString::fromUtf8("pkgDetailsVBox"));
        pkgHeadingHBox = new QHBoxLayout();
        pkgHeadingHBox->setSpacing(5);
        pkgHeadingHBox->setObjectName(QString::fromUtf8("pkgHeadingHBox"));
        pkgIcon = new QLabel(pkgDetailsPage);
        pkgIcon->setObjectName(QString::fromUtf8("pkgIcon"));
        pkgIcon->setPixmap(QPixmap(QString::fromUtf8(":/icons/tree-medium/folder-pkg.png")));

        pkgHeadingHBox->addWidget(pkgIcon);

        pkgNameLabel = new QLabel(pkgDetailsPage);
        pkgNameLabel->setObjectName(QString::fromUtf8("pkgNameLabel"));
        pkgNameLabel->setFont(font);
        pkgNameLabel->setText(QString::fromUtf8("superfoomatic"));
        pkgNameLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);

        pkgHeadingHBox->addWidget(pkgNameLabel);

        pkgHeadingHBox->setStretch(1, 1);

        pkgDetailsVBox->addLayout(pkgHeadingHBox);

        pkgGridLayout = new QGridLayout();
        pkgGridLayout->setSpacing(5);
        pkgGridLayout->setObjectName(QString::fromUtf8("pkgGridLayout"));
        pkgGridLayout->setContentsMargins(-1, 15, -1, -1);
        pkgLatestMTimeLabel = new QLabel(pkgDetailsPage);
        pkgLatestMTimeLabel->setObjectName(QString::fromUtf8("pkgLatestMTimeLabel"));
        pkgLatestMTimeLabel->setText(QString::fromUtf8("2019-04-28 16:02"));

        pkgGridLayout->addWidget(pkgLatestMTimeLabel, 10, 1, 1, 1);

        pkgItemCountCaption = new QLabel(pkgDetailsPage);
        pkgItemCountCaption->setObjectName(QString::fromUtf8("pkgItemCountCaption"));
        pkgItemCountCaption->setFont(font1);

        pkgGridLayout->addWidget(pkgItemCountCaption, 7, 0, 1, 1);

        pkgInstalledFilesHeading = new QLabel(pkgDetailsPage);
        pkgInstalledFilesHeading->setObjectName(QString::fromUtf8("pkgInstalledFilesHeading"));
        pkgInstalledFilesHeading->setFont(font2);

        pkgGridLayout->addWidget(pkgInstalledFilesHeading, 4, 0, 1, 2);

        pkgFileCountLabel = new QLabel(pkgDetailsPage);
        pkgFileCountLabel->setObjectName(QString::fromUtf8("pkgFileCountLabel"));
        pkgFileCountLabel->setText(QString::fromUtf8("280"));

        pkgGridLayout->addWidget(pkgFileCountLabel, 8, 1, 1, 1);

        pkgTypeCaption = new QLabel(pkgDetailsPage);
        pkgTypeCaption->setObjectName(QString::fromUtf8("pkgTypeCaption"));
        pkgTypeCaption->setFont(font1);

        pkgGridLayout->addWidget(pkgTypeCaption, 0, 0, 1, 1);

        pkgAllocatedCaption = new QLabel(pkgDetailsPage);
        pkgAllocatedCaption->setObjectName(QString::fromUtf8("pkgAllocatedCaption"));
        pkgAllocatedCaption->setFont(font1);

        pkgGridLayout->addWidget(pkgAllocatedCaption, 6, 0, 1, 1);

        pkgVersionCaption = new QLabel(pkgDetailsPage);
        pkgVersionCaption->setObjectName(QString::fromUtf8("pkgVersionCaption"));
        pkgVersionCaption->setFont(font1);

        pkgGridLayout->addWidget(pkgVersionCaption, 1, 0, 1, 1);

        pkgTypeLabel = new QLabel(pkgDetailsPage);
        pkgTypeLabel->setObjectName(QString::fromUtf8("pkgTypeLabel"));
        pkgTypeLabel->setText(QString::fromUtf8("package"));

        pkgGridLayout->addWidget(pkgTypeLabel, 0, 1, 1, 1);

        pkgTotalSizeCaption = new QLabel(pkgDetailsPage);
        pkgTotalSizeCaption->setObjectName(QString::fromUtf8("pkgTotalSizeCaption"));
        pkgTotalSizeCaption->setFont(font1);

        pkgGridLayout->addWidget(pkgTotalSizeCaption, 5, 0, 1, 1);

        pkgTotalSizeLabel = new QLabel(pkgDetailsPage);
        pkgTotalSizeLabel->setObjectName(QString::fromUtf8("pkgTotalSizeLabel"));
        pkgTotalSizeLabel->setText(QString::fromUtf8("240.0 MB"));

        pkgGridLayout->addWidget(pkgTotalSizeLabel, 5, 1, 1, 1);

        pkgLatestMTimeCaption = new QLabel(pkgDetailsPage);
        pkgLatestMTimeCaption->setObjectName(QString::fromUtf8("pkgLatestMTimeCaption"));
        pkgLatestMTimeCaption->setFont(font1);

        pkgGridLayout->addWidget(pkgLatestMTimeCaption, 10, 0, 1, 1);

        pkgAllocatedLabel = new QLabel(pkgDetailsPage);
        pkgAllocatedLabel->setObjectName(QString::fromUtf8("pkgAllocatedLabel"));
        pkgAllocatedLabel->setText(QString::fromUtf8("240.0 MB"));

        pkgGridLayout->addWidget(pkgAllocatedLabel, 6, 1, 1, 1);

        pkgSubDirCountLabel = new QLabel(pkgDetailsPage);
        pkgSubDirCountLabel->setObjectName(QString::fromUtf8("pkgSubDirCountLabel"));
        pkgSubDirCountLabel->setText(QString::fromUtf8("37"));

        pkgGridLayout->addWidget(pkgSubDirCountLabel, 9, 1, 1, 1);

        pkgSubDirCountCaption = new QLabel(pkgDetailsPage);
        pkgSubDirCountCaption->setObjectName(QString::fromUtf8("pkgSubDirCountCaption"));
        pkgSubDirCountCaption->setFont(font1);

        pkgGridLayout->addWidget(pkgSubDirCountCaption, 9, 0, 1, 1);

        pkgVersionLabel = new QLabel(pkgDetailsPage);
        pkgVersionLabel->setObjectName(QString::fromUtf8("pkgVersionLabel"));
        pkgVersionLabel->setText(QString::fromUtf8("42.0"));

        pkgGridLayout->addWidget(pkgVersionLabel, 1, 1, 1, 1);

        pkgFileCountCaption = new QLabel(pkgDetailsPage);
        pkgFileCountCaption->setObjectName(QString::fromUtf8("pkgFileCountCaption"));
        pkgFileCountCaption->setFont(font1);

        pkgGridLayout->addWidget(pkgFileCountCaption, 8, 0, 1, 1);

        pkgArchLabel = new QLabel(pkgDetailsPage);
        pkgArchLabel->setObjectName(QString::fromUtf8("pkgArchLabel"));
        pkgArchLabel->setText(QString::fromUtf8("amd64"));

        pkgGridLayout->addWidget(pkgArchLabel, 2, 1, 1, 1);

        pkgItemCountLabel = new QLabel(pkgDetailsPage);
        pkgItemCountLabel->setObjectName(QString::fromUtf8("pkgItemCountLabel"));
        pkgItemCountLabel->setText(QString::fromUtf8("317"));

        pkgGridLayout->addWidget(pkgItemCountLabel, 7, 1, 1, 1);

        pkgArchCaption = new QLabel(pkgDetailsPage);
        pkgArchCaption->setObjectName(QString::fromUtf8("pkgArchCaption"));
        pkgArchCaption->setFont(font1);

        pkgGridLayout->addWidget(pkgArchCaption, 2, 0, 1, 1);

        pkgSpacer = new QLabel(pkgDetailsPage);
        pkgSpacer->setObjectName(QString::fromUtf8("pkgSpacer"));
        pkgSpacer->setMaximumSize(QSize(16777215, 10));

        pkgGridLayout->addWidget(pkgSpacer, 3, 0, 1, 1);

        pkgGridLayout->setColumnStretch(1, 1);

        pkgDetailsVBox->addLayout(pkgGridLayout);

        pkgBottomVSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        pkgDetailsVBox->addItem(pkgBottomVSpacer);


        pkgDetailsHBox->addLayout(pkgDetailsVBox);

        FileDetailsView->addWidget(pkgDetailsPage);
        pkgSummaryPage = new QWidget();
        pkgSummaryPage->setObjectName(QString::fromUtf8("pkgSummaryPage"));
        horizontalLayout_11 = new QHBoxLayout(pkgSummaryPage);
        horizontalLayout_11->setObjectName(QString::fromUtf8("horizontalLayout_11"));
        pkgSummaryVBox = new QVBoxLayout();
        pkgSummaryVBox->setSpacing(0);
        pkgSummaryVBox->setObjectName(QString::fromUtf8("pkgSummaryVBox"));
        pkgSummaryHeading = new QLabel(pkgSummaryPage);
        pkgSummaryHeading->setObjectName(QString::fromUtf8("pkgSummaryHeading"));
        pkgSummaryHeading->setMinimumSize(QSize(0, 22));
        pkgSummaryHeading->setFont(font2);

        pkgSummaryVBox->addWidget(pkgSummaryHeading);

        pkgSummaryGridLayout = new QGridLayout();
        pkgSummaryGridLayout->setSpacing(5);
        pkgSummaryGridLayout->setObjectName(QString::fromUtf8("pkgSummaryGridLayout"));
        pkgSummaryGridLayout->setContentsMargins(-1, 15, -1, -1);
        pkgSummaryLatestMTimeCaption = new QLabel(pkgSummaryPage);
        pkgSummaryLatestMTimeCaption->setObjectName(QString::fromUtf8("pkgSummaryLatestMTimeCaption"));
        pkgSummaryLatestMTimeCaption->setFont(font1);

        pkgSummaryGridLayout->addWidget(pkgSummaryLatestMTimeCaption, 8, 0, 1, 1);

        pkgSummaryPkgCountLabel = new QLabel(pkgSummaryPage);
        pkgSummaryPkgCountLabel->setObjectName(QString::fromUtf8("pkgSummaryPkgCountLabel"));
        pkgSummaryPkgCountLabel->setText(QString::fromUtf8("42"));

        pkgSummaryGridLayout->addWidget(pkgSummaryPkgCountLabel, 0, 1, 1, 1);

        pkgSummarySubDirCountCaption = new QLabel(pkgSummaryPage);
        pkgSummarySubDirCountCaption->setObjectName(QString::fromUtf8("pkgSummarySubDirCountCaption"));
        pkgSummarySubDirCountCaption->setFont(font1);

        pkgSummaryGridLayout->addWidget(pkgSummarySubDirCountCaption, 7, 0, 1, 1);

        pkgSummaryItemCountLabel = new QLabel(pkgSummaryPage);
        pkgSummaryItemCountLabel->setObjectName(QString::fromUtf8("pkgSummaryItemCountLabel"));
        pkgSummaryItemCountLabel->setText(QString::fromUtf8("317"));

        pkgSummaryGridLayout->addWidget(pkgSummaryItemCountLabel, 5, 1, 1, 1);

        pkgSummarySubDirCountLabel = new QLabel(pkgSummaryPage);
        pkgSummarySubDirCountLabel->setObjectName(QString::fromUtf8("pkgSummarySubDirCountLabel"));
        pkgSummarySubDirCountLabel->setText(QString::fromUtf8("37"));

        pkgSummaryGridLayout->addWidget(pkgSummarySubDirCountLabel, 7, 1, 1, 1);

        pkgSummaryItemCountCaption = new QLabel(pkgSummaryPage);
        pkgSummaryItemCountCaption->setObjectName(QString::fromUtf8("pkgSummaryItemCountCaption"));
        pkgSummaryItemCountCaption->setFont(font1);

        pkgSummaryGridLayout->addWidget(pkgSummaryItemCountCaption, 5, 0, 1, 1);

        pkgSummaryFileCountCaption = new QLabel(pkgSummaryPage);
        pkgSummaryFileCountCaption->setObjectName(QString::fromUtf8("pkgSummaryFileCountCaption"));
        pkgSummaryFileCountCaption->setFont(font1);

        pkgSummaryGridLayout->addWidget(pkgSummaryFileCountCaption, 6, 0, 1, 1);

        pkgSummaryFileCountLabel = new QLabel(pkgSummaryPage);
        pkgSummaryFileCountLabel->setObjectName(QString::fromUtf8("pkgSummaryFileCountLabel"));
        pkgSummaryFileCountLabel->setText(QString::fromUtf8("280"));

        pkgSummaryGridLayout->addWidget(pkgSummaryFileCountLabel, 6, 1, 1, 1);

        pkgSummaryInstalledFilesHeading = new QLabel(pkgSummaryPage);
        pkgSummaryInstalledFilesHeading->setObjectName(QString::fromUtf8("pkgSummaryInstalledFilesHeading"));
        pkgSummaryInstalledFilesHeading->setFont(font2);

        pkgSummaryGridLayout->addWidget(pkgSummaryInstalledFilesHeading, 2, 0, 1, 2);

        pkgSummaryAllocatedLabel = new QLabel(pkgSummaryPage);
        pkgSummaryAllocatedLabel->setObjectName(QString::fromUtf8("pkgSummaryAllocatedLabel"));
        pkgSummaryAllocatedLabel->setText(QString::fromUtf8("240.0 MB"));

        pkgSummaryGridLayout->addWidget(pkgSummaryAllocatedLabel, 4, 1, 1, 1);

        pkgSummaryAllocatedCaption = new QLabel(pkgSummaryPage);
        pkgSummaryAllocatedCaption->setObjectName(QString::fromUtf8("pkgSummaryAllocatedCaption"));
        pkgSummaryAllocatedCaption->setFont(font1);

        pkgSummaryGridLayout->addWidget(pkgSummaryAllocatedCaption, 4, 0, 1, 1);

        pkgSummaryLatestMTimeLabel = new QLabel(pkgSummaryPage);
        pkgSummaryLatestMTimeLabel->setObjectName(QString::fromUtf8("pkgSummaryLatestMTimeLabel"));
        pkgSummaryLatestMTimeLabel->setText(QString::fromUtf8("2019-04-28 16:02"));

        pkgSummaryGridLayout->addWidget(pkgSummaryLatestMTimeLabel, 8, 1, 1, 1);

        pkgSummaryTotalSizeLabel = new QLabel(pkgSummaryPage);
        pkgSummaryTotalSizeLabel->setObjectName(QString::fromUtf8("pkgSummaryTotalSizeLabel"));
        pkgSummaryTotalSizeLabel->setText(QString::fromUtf8("240.0 MB"));

        pkgSummaryGridLayout->addWidget(pkgSummaryTotalSizeLabel, 3, 1, 1, 1);

        pkgSummaryTotalSizeCaption = new QLabel(pkgSummaryPage);
        pkgSummaryTotalSizeCaption->setObjectName(QString::fromUtf8("pkgSummaryTotalSizeCaption"));
        pkgSummaryTotalSizeCaption->setFont(font1);

        pkgSummaryGridLayout->addWidget(pkgSummaryTotalSizeCaption, 3, 0, 1, 1);

        pkgSummaryPkgCountCaption = new QLabel(pkgSummaryPage);
        pkgSummaryPkgCountCaption->setObjectName(QString::fromUtf8("pkgSummaryPkgCountCaption"));
        pkgSummaryPkgCountCaption->setFont(font1);

        pkgSummaryGridLayout->addWidget(pkgSummaryPkgCountCaption, 0, 0, 1, 1);

        pkgSummarySpacer = new QLabel(pkgSummaryPage);
        pkgSummarySpacer->setObjectName(QString::fromUtf8("pkgSummarySpacer"));
        pkgSummarySpacer->setMaximumSize(QSize(16777215, 10));

        pkgSummaryGridLayout->addWidget(pkgSummarySpacer, 1, 0, 1, 1);

        pkgSummaryGridLayout->setColumnStretch(1, 1);

        pkgSummaryVBox->addLayout(pkgSummaryGridLayout);

        pkgSummaryBottomVSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        pkgSummaryVBox->addItem(pkgSummaryBottomVSpacer);


        horizontalLayout_11->addLayout(pkgSummaryVBox);

        FileDetailsView->addWidget(pkgSummaryPage);
        selectionSummaryPage = new QWidget();
        selectionSummaryPage->setObjectName(QString::fromUtf8("selectionSummaryPage"));
        selPageHBox = new QHBoxLayout(selectionSummaryPage);
        selPageHBox->setObjectName(QString::fromUtf8("selPageHBox"));
        selectionSummaryVBox = new QVBoxLayout();
        selectionSummaryVBox->setSpacing(0);
        selectionSummaryVBox->setObjectName(QString::fromUtf8("selectionSummaryVBox"));
        selHeading = new QLabel(selectionSummaryPage);
        selHeading->setObjectName(QString::fromUtf8("selHeading"));
        selHeading->setMinimumSize(QSize(0, 22));
        selHeading->setFont(font2);

        selectionSummaryVBox->addWidget(selHeading);

        selGridLayout = new QGridLayout();
        selGridLayout->setSpacing(5);
        selGridLayout->setObjectName(QString::fromUtf8("selGridLayout"));
        selGridLayout->setContentsMargins(-1, 15, -1, -1);
        selTotalSizeCaption = new QLabel(selectionSummaryPage);
        selTotalSizeCaption->setObjectName(QString::fromUtf8("selTotalSizeCaption"));
        selTotalSizeCaption->setFont(font1);

        selGridLayout->addWidget(selTotalSizeCaption, 0, 0, 1, 1);

        selAllocatedLabel = new QLabel(selectionSummaryPage);
        selAllocatedLabel->setObjectName(QString::fromUtf8("selAllocatedLabel"));
        selAllocatedLabel->setText(QString::fromUtf8("512.0 MB"));

        selGridLayout->addWidget(selAllocatedLabel, 1, 1, 1, 1);

        selSubtreeFileCountLabel = new QLabel(selectionSummaryPage);
        selSubtreeFileCountLabel->setObjectName(QString::fromUtf8("selSubtreeFileCountLabel"));
        selSubtreeFileCountLabel->setText(QString::fromUtf8("84"));

        selGridLayout->addWidget(selSubtreeFileCountLabel, 4, 1, 1, 1);

        selFileCountLabel = new QLabel(selectionSummaryPage);
        selFileCountLabel->setObjectName(QString::fromUtf8("selFileCountLabel"));
        selFileCountLabel->setText(QString::fromUtf8("7"));

        selGridLayout->addWidget(selFileCountLabel, 2, 1, 1, 1);

        selDirCountCaption = new QLabel(selectionSummaryPage);
        selDirCountCaption->setObjectName(QString::fromUtf8("selDirCountCaption"));
        selDirCountCaption->setFont(font1);

        selGridLayout->addWidget(selDirCountCaption, 3, 0, 1, 1);

        selFileCountCaption = new QLabel(selectionSummaryPage);
        selFileCountCaption->setObjectName(QString::fromUtf8("selFileCountCaption"));
        selFileCountCaption->setFont(font1);

        selGridLayout->addWidget(selFileCountCaption, 2, 0, 1, 1);

        selDirCountLabel = new QLabel(selectionSummaryPage);
        selDirCountLabel->setObjectName(QString::fromUtf8("selDirCountLabel"));
        selDirCountLabel->setText(QString::fromUtf8("2"));

        selGridLayout->addWidget(selDirCountLabel, 3, 1, 1, 1);

        selTotalSizeLabel = new QLabel(selectionSummaryPage);
        selTotalSizeLabel->setObjectName(QString::fromUtf8("selTotalSizeLabel"));
        selTotalSizeLabel->setText(QString::fromUtf8("512.0 MB"));

        selGridLayout->addWidget(selTotalSizeLabel, 0, 1, 1, 1);

        selSubtreeFileCountCaption = new QLabel(selectionSummaryPage);
        selSubtreeFileCountCaption->setObjectName(QString::fromUtf8("selSubtreeFileCountCaption"));
        selSubtreeFileCountCaption->setFont(font1);

        selGridLayout->addWidget(selSubtreeFileCountCaption, 4, 0, 1, 1);

        selAllocatedCaption = new QLabel(selectionSummaryPage);
        selAllocatedCaption->setObjectName(QString::fromUtf8("selAllocatedCaption"));
        selAllocatedCaption->setFont(font1);

        selGridLayout->addWidget(selAllocatedCaption, 1, 0, 1, 1);

        selGridLayout->setColumnStretch(1, 1);

        selectionSummaryVBox->addLayout(selGridLayout);

        selBottomVSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        selectionSummaryVBox->addItem(selBottomVSpacer);


        selPageHBox->addLayout(selectionSummaryVBox);

        FileDetailsView->addWidget(selectionSummaryPage);
        emptyPage = new QWidget();
        emptyPage->setObjectName(QString::fromUtf8("emptyPage"));
        verticalLayout_4 = new QVBoxLayout(emptyPage);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        FileDetailsView->addWidget(emptyPage);

        retranslateUi(FileDetailsView);
        QObject::connect(dirPermissionsLabel, SIGNAL(linkActivated(QString)), dirSpacer, SLOT(clear()));
    } // setupUi

    void retranslateUi(QStackedWidget *FileDetailsView)
    {
        fileTypeCaption->setText(QCoreApplication::translate("FileDetailsView", "Type:", nullptr));
        fileLinkCaption->setText(QCoreApplication::translate("FileDetailsView", "Link target:", nullptr));
        fileLinkLabel->setText(QCoreApplication::translate("FileDetailsView", "target", nullptr));
        fileSizeCaption->setText(QCoreApplication::translate("FileDetailsView", "Size:", nullptr));
        fileAllocatedCaption->setText(QCoreApplication::translate("FileDetailsView", "Allocated:", nullptr));
        fileUserCaption->setText(QCoreApplication::translate("FileDetailsView", "User:", nullptr));
        fileGroupCaption->setText(QCoreApplication::translate("FileDetailsView", "Group:", nullptr));
        filePermissionsCaption->setText(QCoreApplication::translate("FileDetailsView", "Permissions:", nullptr));
        fileMTimeCaption->setText(QCoreApplication::translate("FileDetailsView", "Last modified:   ", nullptr));
        fileSystemFileWarning->setText(QCoreApplication::translate("FileDetailsView", "System File", nullptr));
        filePackageCaption->setText(QCoreApplication::translate("FileDetailsView", "Package:", nullptr));
        dirTypeCaption->setText(QCoreApplication::translate("FileDetailsView", "Type:", nullptr));
#if QT_CONFIG(tooltip)
        dirFromCacheIcon->setToolTip(QCoreApplication::translate("FileDetailsView", "This directory was automatically read from a cache file.", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        dirDuplicateIcon->setToolTip(QCoreApplication::translate("FileDetailsView", "<nobr>This is a duplicate or bind mount and it may</nobr> also appear elsewhere in the tree.", nullptr));
#endif // QT_CONFIG(tooltip)
        dirSubTreeHeading->setText(QCoreApplication::translate("FileDetailsView", "Subtree", nullptr));
        dirTotalSizeCaption->setText(QCoreApplication::translate("FileDetailsView", "Total size:", nullptr));
        dirAllocatedCaption->setText(QCoreApplication::translate("FileDetailsView", "Allocated:", nullptr));
        dirItemCountCaption->setText(QCoreApplication::translate("FileDetailsView", "Items:", nullptr));
        dirFileCountCaption->setText(QCoreApplication::translate("FileDetailsView", "Files:", nullptr));
        dirSubDirCountCaption->setText(QCoreApplication::translate("FileDetailsView", "Subdirs:", nullptr));
        dirLatestMTimeCaption->setText(QCoreApplication::translate("FileDetailsView", "Last modified:   ", nullptr));
        dirUserCaption->setText(QCoreApplication::translate("FileDetailsView", "User:", nullptr));
        dirGroupCaption->setText(QCoreApplication::translate("FileDetailsView", "Group:", nullptr));
        dirMTimeCaption->setText(QCoreApplication::translate("FileDetailsView", "Last modified:   ", nullptr));
        dirPermissionsCaption->setText(QCoreApplication::translate("FileDetailsView", "Permissions:", nullptr));
        dirOwnSizeCaption->setText(QCoreApplication::translate("FileDetailsView", "Own size:", nullptr));
        dirDirectoryHeading->setText(QCoreApplication::translate("FileDetailsView", "Directory", nullptr));
        pkgIcon->setText(QString());
        pkgItemCountCaption->setText(QCoreApplication::translate("FileDetailsView", "Items:", nullptr));
        pkgInstalledFilesHeading->setText(QCoreApplication::translate("FileDetailsView", "Installed Files", nullptr));
        pkgTypeCaption->setText(QCoreApplication::translate("FileDetailsView", "Type:", nullptr));
        pkgAllocatedCaption->setText(QCoreApplication::translate("FileDetailsView", "Allocated:", nullptr));
        pkgVersionCaption->setText(QCoreApplication::translate("FileDetailsView", "Version:", nullptr));
        pkgTotalSizeCaption->setText(QCoreApplication::translate("FileDetailsView", "Total size:", nullptr));
        pkgLatestMTimeCaption->setText(QCoreApplication::translate("FileDetailsView", "Last modified:   ", nullptr));
        pkgSubDirCountCaption->setText(QCoreApplication::translate("FileDetailsView", "Subdirs:", nullptr));
        pkgFileCountCaption->setText(QCoreApplication::translate("FileDetailsView", "Files:", nullptr));
        pkgArchCaption->setText(QCoreApplication::translate("FileDetailsView", "Architecture:", nullptr));
        pkgSummaryHeading->setText(QCoreApplication::translate("FileDetailsView", "Packages Summary", nullptr));
        pkgSummaryLatestMTimeCaption->setText(QCoreApplication::translate("FileDetailsView", "Last modified:   ", nullptr));
        pkgSummarySubDirCountCaption->setText(QCoreApplication::translate("FileDetailsView", "Subdirs:", nullptr));
        pkgSummaryItemCountCaption->setText(QCoreApplication::translate("FileDetailsView", "Items:", nullptr));
        pkgSummaryFileCountCaption->setText(QCoreApplication::translate("FileDetailsView", "Files:", nullptr));
        pkgSummaryInstalledFilesHeading->setText(QCoreApplication::translate("FileDetailsView", "Installed Files", nullptr));
        pkgSummaryAllocatedCaption->setText(QCoreApplication::translate("FileDetailsView", "Allocated:", nullptr));
        pkgSummaryTotalSizeCaption->setText(QCoreApplication::translate("FileDetailsView", "Total size:", nullptr));
        pkgSummaryPkgCountCaption->setText(QCoreApplication::translate("FileDetailsView", "Packages:", nullptr));
        selHeading->setText(QCoreApplication::translate("FileDetailsView", "%n Selected Items", nullptr));
        selTotalSizeCaption->setText(QCoreApplication::translate("FileDetailsView", "Total size:", nullptr));
        selDirCountCaption->setText(QCoreApplication::translate("FileDetailsView", "Directories:", nullptr));
        selFileCountCaption->setText(QCoreApplication::translate("FileDetailsView", "Files:", nullptr));
        selSubtreeFileCountCaption->setText(QCoreApplication::translate("FileDetailsView", "Files in subtrees:", nullptr));
        selAllocatedCaption->setText(QCoreApplication::translate("FileDetailsView", "Allocated:", nullptr));
        (void)FileDetailsView;
    } // retranslateUi

};

namespace Ui {
    class FileDetailsView: public Ui_FileDetailsView {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FILE_2D_DETAILS_2D_VIEW_H
