/********************************************************************************
** Form generated from reading UI file 'mime-category-config-page.ui'
**
** Created by: Qt User Interface Compiler version 5.15.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MIME_2D_CATEGORY_2D_CONFIG_2D_PAGE_H
#define UI_MIME_2D_CATEGORY_2D_CONFIG_2D_PAGE_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "TreemapView.h"

QT_BEGIN_NAMESPACE

class Ui_MimeCategoryConfigPage
{
public:
    QAction *actionColourPreviews;
    QHBoxLayout *horizontalLayout_6;
    QSplitter *horizontalSplitter;
    QWidget *layoutWidget1;
    QVBoxLayout *verticalLayout_3;
    QLabel *mimeCategoryCaption;
    QListWidget *listWidget;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_5;
    QLineEdit *nameLineEdit;
    QHBoxLayout *horizontalLayout_5;
    QPushButton *categoryColorButton;
    QLineEdit *categoryColorEdit;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer;
    QToolButton *addButton;
    QToolButton *removeButton;
    QSpacerItem *horizontalSpacer_5;
    QTabWidget *tabWidget;
    QWidget *patternsTab;
    QVBoxLayout *verticalLayout_5;
    QSplitter *patternsSplitter;
    QWidget *patternsTopWidget;
    QVBoxLayout *patternsTopLayout;
    QLabel *caseInsensitivePatternsCaption;
    QPlainTextEdit *caseInsensitivePatterns;
    QWidget *patternsBottomWidget;
    QVBoxLayout *patternsBottomLayout;
    QLabel *caseSensitivePatternsCaption;
    QPlainTextEdit *caseSensitivePatterns;
    QLabel *duplicateLabel;
    QWidget *treemapTab;
    QVBoxLayout *verticalLayout_2;
    QSplitter *treemapSplitter;
    QDirStat::TreemapView *treemapView;
    QWidget *layoutWidget2;
    QVBoxLayout *verticalLayout;
    QSpacerItem *verticalSpacer;
    QCheckBox *squarifiedCheckBox;
    QCheckBox *cushionShadingCheckBox;
    QGridLayout *gridLayout;
    QSpacerItem *horizontalSpacer_2;
    QDoubleSpinBox *cushionHeightSpinBox;
    QSpacerItem *horizontalSpacer_3;
    QDoubleSpinBox *heightScaleFactorSpinBox;
    QLabel *cushionHeightLabel;
    QLabel *heightScaleFactorLabel;
    QHBoxLayout *buttonsLayout_2;
    QPushButton *tileColorButton;
    QLineEdit *tileColorEdit;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_4;
    QSpinBox *minTileSizeSpinBox;
    QSpacerItem *horizontalSpacer_4;

    void setupUi(QWidget *MimeCategoryConfigPage)
    {
        if (MimeCategoryConfigPage->objectName().isEmpty())
            MimeCategoryConfigPage->setObjectName(QString::fromUtf8("MimeCategoryConfigPage"));
        MimeCategoryConfigPage->resize(550, 488);
        actionColourPreviews = new QAction(MimeCategoryConfigPage);
        actionColourPreviews->setObjectName(QString::fromUtf8("actionColourPreviews"));
        actionColourPreviews->setCheckable(true);
        horizontalLayout_6 = new QHBoxLayout(MimeCategoryConfigPage);
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        horizontalSplitter = new QSplitter(MimeCategoryConfigPage);
        horizontalSplitter->setObjectName(QString::fromUtf8("horizontalSplitter"));
        horizontalSplitter->setOrientation(Qt::Horizontal);
        horizontalSplitter->setHandleWidth(10);
        layoutWidget1 = new QWidget(horizontalSplitter);
        layoutWidget1->setObjectName(QString::fromUtf8("layoutWidget1"));
        verticalLayout_3 = new QVBoxLayout(layoutWidget1);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        mimeCategoryCaption = new QLabel(layoutWidget1);
        mimeCategoryCaption->setObjectName(QString::fromUtf8("mimeCategoryCaption"));

        verticalLayout_3->addWidget(mimeCategoryCaption);

        listWidget = new QListWidget(layoutWidget1);
        listWidget->setObjectName(QString::fromUtf8("listWidget"));
        listWidget->setSelectionMode(QAbstractItemView::NoSelection);

        verticalLayout_3->addWidget(listWidget);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        label_5 = new QLabel(layoutWidget1);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        horizontalLayout_4->addWidget(label_5);

        nameLineEdit = new QLineEdit(layoutWidget1);
        nameLineEdit->setObjectName(QString::fromUtf8("nameLineEdit"));

        horizontalLayout_4->addWidget(nameLineEdit);

        horizontalLayout_4->setStretch(1, 1);

        verticalLayout_3->addLayout(horizontalLayout_4);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        categoryColorButton = new QPushButton(layoutWidget1);
        categoryColorButton->setObjectName(QString::fromUtf8("categoryColorButton"));

        horizontalLayout_5->addWidget(categoryColorButton);

        categoryColorEdit = new QLineEdit(layoutWidget1);
        categoryColorEdit->setObjectName(QString::fromUtf8("categoryColorEdit"));
        categoryColorEdit->setMinimumSize(QSize(100, 0));

        horizontalLayout_5->addWidget(categoryColorEdit);

        horizontalLayout_5->setStretch(1, 1);

        verticalLayout_3->addLayout(horizontalLayout_5);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(16);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer);

        addButton = new QToolButton(layoutWidget1);
        addButton->setObjectName(QString::fromUtf8("addButton"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/add.png"), QSize(), QIcon::Normal, QIcon::Off);
        addButton->setIcon(icon);

        horizontalLayout_3->addWidget(addButton);

        removeButton = new QToolButton(layoutWidget1);
        removeButton->setObjectName(QString::fromUtf8("removeButton"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/remove.png"), QSize(), QIcon::Normal, QIcon::Off);
        removeButton->setIcon(icon1);

        horizontalLayout_3->addWidget(removeButton);

        horizontalSpacer_5 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_5);


        verticalLayout_3->addLayout(horizontalLayout_3);

        horizontalSplitter->addWidget(layoutWidget1);
        tabWidget = new QTabWidget(horizontalSplitter);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        patternsTab = new QWidget();
        patternsTab->setObjectName(QString::fromUtf8("patternsTab"));
        verticalLayout_5 = new QVBoxLayout(patternsTab);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        verticalLayout_5->setContentsMargins(4, -1, 4, 4);
        patternsSplitter = new QSplitter(patternsTab);
        patternsSplitter->setObjectName(QString::fromUtf8("patternsSplitter"));
        patternsSplitter->setOrientation(Qt::Vertical);
        patternsTopWidget = new QWidget(patternsSplitter);
        patternsTopWidget->setObjectName(QString::fromUtf8("patternsTopWidget"));
        patternsTopLayout = new QVBoxLayout(patternsTopWidget);
        patternsTopLayout->setObjectName(QString::fromUtf8("patternsTopLayout"));
        patternsTopLayout->setContentsMargins(0, 0, 0, 0);
        caseInsensitivePatternsCaption = new QLabel(patternsTopWidget);
        caseInsensitivePatternsCaption->setObjectName(QString::fromUtf8("caseInsensitivePatternsCaption"));

        patternsTopLayout->addWidget(caseInsensitivePatternsCaption);

        caseInsensitivePatterns = new QPlainTextEdit(patternsTopWidget);
        caseInsensitivePatterns->setObjectName(QString::fromUtf8("caseInsensitivePatterns"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(caseInsensitivePatterns->sizePolicy().hasHeightForWidth());
        caseInsensitivePatterns->setSizePolicy(sizePolicy);
        caseInsensitivePatterns->setMinimumSize(QSize(150, 0));

        patternsTopLayout->addWidget(caseInsensitivePatterns);

        patternsSplitter->addWidget(patternsTopWidget);
        patternsBottomWidget = new QWidget(patternsSplitter);
        patternsBottomWidget->setObjectName(QString::fromUtf8("patternsBottomWidget"));
        patternsBottomLayout = new QVBoxLayout(patternsBottomWidget);
        patternsBottomLayout->setObjectName(QString::fromUtf8("patternsBottomLayout"));
        patternsBottomLayout->setContentsMargins(0, 0, 0, 0);
        caseSensitivePatternsCaption = new QLabel(patternsBottomWidget);
        caseSensitivePatternsCaption->setObjectName(QString::fromUtf8("caseSensitivePatternsCaption"));

        patternsBottomLayout->addWidget(caseSensitivePatternsCaption);

        caseSensitivePatterns = new QPlainTextEdit(patternsBottomWidget);
        caseSensitivePatterns->setObjectName(QString::fromUtf8("caseSensitivePatterns"));
        caseSensitivePatterns->setMinimumSize(QSize(150, 0));

        patternsBottomLayout->addWidget(caseSensitivePatterns);

        patternsSplitter->addWidget(patternsBottomWidget);

        verticalLayout_5->addWidget(patternsSplitter);

        duplicateLabel = new QLabel(patternsTab);
        duplicateLabel->setObjectName(QString::fromUtf8("duplicateLabel"));

        verticalLayout_5->addWidget(duplicateLabel);

        tabWidget->addTab(patternsTab, QString());
        treemapTab = new QWidget();
        treemapTab->setObjectName(QString::fromUtf8("treemapTab"));
        verticalLayout_2 = new QVBoxLayout(treemapTab);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        treemapSplitter = new QSplitter(treemapTab);
        treemapSplitter->setObjectName(QString::fromUtf8("treemapSplitter"));
        treemapSplitter->setOrientation(Qt::Vertical);
        treemapSplitter->setHandleWidth(8);
        treemapView = new QDirStat::TreemapView(treemapSplitter);
        treemapView->setObjectName(QString::fromUtf8("treemapView"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(1);
        sizePolicy1.setHeightForWidth(treemapView->sizePolicy().hasHeightForWidth());
        treemapView->setSizePolicy(sizePolicy1);
        treemapView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        treemapView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        treemapView->setRenderHints(QPainter::SmoothPixmapTransform|QPainter::TextAntialiasing);
        treemapView->setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing|QGraphicsView::DontSavePainterState);
        treemapSplitter->addWidget(treemapView);
        layoutWidget2 = new QWidget(treemapSplitter);
        layoutWidget2->setObjectName(QString::fromUtf8("layoutWidget2"));
        verticalLayout = new QVBoxLayout(layoutWidget2);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        squarifiedCheckBox = new QCheckBox(layoutWidget2);
        squarifiedCheckBox->setObjectName(QString::fromUtf8("squarifiedCheckBox"));

        verticalLayout->addWidget(squarifiedCheckBox);

        cushionShadingCheckBox = new QCheckBox(layoutWidget2);
        cushionShadingCheckBox->setObjectName(QString::fromUtf8("cushionShadingCheckBox"));

        verticalLayout->addWidget(cushionShadingCheckBox);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setVerticalSpacing(0);
        gridLayout->setContentsMargins(20, -1, -1, -1);
        horizontalSpacer_2 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_2, 1, 3, 1, 1);

        cushionHeightSpinBox = new QDoubleSpinBox(layoutWidget2);
        cushionHeightSpinBox->setObjectName(QString::fromUtf8("cushionHeightSpinBox"));
        cushionHeightSpinBox->setDecimals(1);
        cushionHeightSpinBox->setMinimum(0.100000000000000);
        cushionHeightSpinBox->setMaximum(2.000000000000000);
        cushionHeightSpinBox->setSingleStep(0.100000000000000);
        cushionHeightSpinBox->setValue(0.600000000000000);

        gridLayout->addWidget(cushionHeightSpinBox, 0, 2, 1, 1);

        horizontalSpacer_3 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_3, 0, 3, 1, 1);

        heightScaleFactorSpinBox = new QDoubleSpinBox(layoutWidget2);
        heightScaleFactorSpinBox->setObjectName(QString::fromUtf8("heightScaleFactorSpinBox"));
        heightScaleFactorSpinBox->setDecimals(1);
        heightScaleFactorSpinBox->setMinimum(0.100000000000000);
        heightScaleFactorSpinBox->setMaximum(1.000000000000000);
        heightScaleFactorSpinBox->setSingleStep(0.100000000000000);
        heightScaleFactorSpinBox->setValue(0.800000000000000);

        gridLayout->addWidget(heightScaleFactorSpinBox, 1, 2, 1, 1);

        cushionHeightLabel = new QLabel(layoutWidget2);
        cushionHeightLabel->setObjectName(QString::fromUtf8("cushionHeightLabel"));

        gridLayout->addWidget(cushionHeightLabel, 0, 0, 1, 2);

        heightScaleFactorLabel = new QLabel(layoutWidget2);
        heightScaleFactorLabel->setObjectName(QString::fromUtf8("heightScaleFactorLabel"));
        heightScaleFactorLabel->setFrameShadow(QFrame::Plain);

        gridLayout->addWidget(heightScaleFactorLabel, 1, 0, 1, 2);


        verticalLayout->addLayout(gridLayout);

        buttonsLayout_2 = new QHBoxLayout();
        buttonsLayout_2->setObjectName(QString::fromUtf8("buttonsLayout_2"));
        buttonsLayout_2->setContentsMargins(-1, 4, -1, -1);
        tileColorButton = new QPushButton(layoutWidget2);
        tileColorButton->setObjectName(QString::fromUtf8("tileColorButton"));

        buttonsLayout_2->addWidget(tileColorButton);

        tileColorEdit = new QLineEdit(layoutWidget2);
        tileColorEdit->setObjectName(QString::fromUtf8("tileColorEdit"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(tileColorEdit->sizePolicy().hasHeightForWidth());
        tileColorEdit->setSizePolicy(sizePolicy2);
        tileColorEdit->setClearButtonEnabled(true);

        buttonsLayout_2->addWidget(tileColorEdit);

        buttonsLayout_2->setStretch(1, 1);

        verticalLayout->addLayout(buttonsLayout_2);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(-1, 4, -1, 8);
        label_4 = new QLabel(layoutWidget2);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setFrameShape(QFrame::NoFrame);

        horizontalLayout_2->addWidget(label_4);

        minTileSizeSpinBox = new QSpinBox(layoutWidget2);
        minTileSizeSpinBox->setObjectName(QString::fromUtf8("minTileSizeSpinBox"));
        QSizePolicy sizePolicy3(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(minTileSizeSpinBox->sizePolicy().hasHeightForWidth());
        minTileSizeSpinBox->setSizePolicy(sizePolicy3);
        minTileSizeSpinBox->setMaximum(10);
        minTileSizeSpinBox->setValue(3);

        horizontalLayout_2->addWidget(minTileSizeSpinBox);

        horizontalSpacer_4 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_4);


        verticalLayout->addLayout(horizontalLayout_2);

        treemapSplitter->addWidget(layoutWidget2);

        verticalLayout_2->addWidget(treemapSplitter);

        tabWidget->addTab(treemapTab, QString());
        horizontalSplitter->addWidget(tabWidget);

        horizontalLayout_6->addWidget(horizontalSplitter);

#if QT_CONFIG(shortcut)
        mimeCategoryCaption->setBuddy(listWidget);
        label_5->setBuddy(nameLineEdit);
        caseInsensitivePatternsCaption->setBuddy(caseInsensitivePatterns);
        caseSensitivePatternsCaption->setBuddy(caseSensitivePatterns);
        cushionHeightLabel->setBuddy(cushionHeightSpinBox);
        heightScaleFactorLabel->setBuddy(heightScaleFactorSpinBox);
        label_4->setBuddy(minTileSizeSpinBox);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(tabWidget, listWidget);
        QWidget::setTabOrder(listWidget, nameLineEdit);
        QWidget::setTabOrder(nameLineEdit, categoryColorButton);
        QWidget::setTabOrder(categoryColorButton, categoryColorEdit);
        QWidget::setTabOrder(categoryColorEdit, caseInsensitivePatterns);
        QWidget::setTabOrder(caseInsensitivePatterns, caseSensitivePatterns);
        QWidget::setTabOrder(caseSensitivePatterns, addButton);
        QWidget::setTabOrder(addButton, removeButton);
        QWidget::setTabOrder(removeButton, treemapView);
        QWidget::setTabOrder(treemapView, squarifiedCheckBox);
        QWidget::setTabOrder(squarifiedCheckBox, cushionShadingCheckBox);
        QWidget::setTabOrder(cushionShadingCheckBox, cushionHeightSpinBox);
        QWidget::setTabOrder(cushionHeightSpinBox, heightScaleFactorSpinBox);
        QWidget::setTabOrder(heightScaleFactorSpinBox, tileColorButton);
        QWidget::setTabOrder(tileColorButton, tileColorEdit);
        QWidget::setTabOrder(tileColorEdit, minTileSizeSpinBox);

        retranslateUi(MimeCategoryConfigPage);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MimeCategoryConfigPage);
    } // setupUi

    void retranslateUi(QWidget *MimeCategoryConfigPage)
    {
        actionColourPreviews->setText(QCoreApplication::translate("MimeCategoryConfigPage", "Colour previews", nullptr));
        mimeCategoryCaption->setText(QCoreApplication::translate("MimeCategoryConfigPage", "&MIME Category", nullptr));
        label_5->setText(QCoreApplication::translate("MimeCategoryConfigPage", "&Name", nullptr));
#if QT_CONFIG(tooltip)
        nameLineEdit->setToolTip(QCoreApplication::translate("MimeCategoryConfigPage", "Edit the name of the current category", nullptr));
#endif // QT_CONFIG(tooltip)
        nameLineEdit->setPlaceholderText(QCoreApplication::translate("MimeCategoryConfigPage", "Enter category name", nullptr));
#if QT_CONFIG(tooltip)
        categoryColorButton->setToolTip(QCoreApplication::translate("MimeCategoryConfigPage", "Select a colour for the current category", nullptr));
#endif // QT_CONFIG(tooltip)
        categoryColorButton->setText(QCoreApplication::translate("MimeCategoryConfigPage", "&Colour", nullptr));
#if QT_CONFIG(tooltip)
        categoryColorEdit->setToolTip(QCoreApplication::translate("MimeCategoryConfigPage", "Enter an RGB colour code directly", nullptr));
#endif // QT_CONFIG(tooltip)
        categoryColorEdit->setPlaceholderText(QCoreApplication::translate("MimeCategoryConfigPage", "Category colour", nullptr));
#if QT_CONFIG(tooltip)
        addButton->setToolTip(QCoreApplication::translate("MimeCategoryConfigPage", "Create a new category", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        removeButton->setToolTip(QCoreApplication::translate("MimeCategoryConfigPage", "Remove the current category", nullptr));
#endif // QT_CONFIG(tooltip)
        caseInsensitivePatternsCaption->setText(QCoreApplication::translate("MimeCategoryConfigPage", "Patterns (case &insensitive)", nullptr));
#if QT_CONFIG(tooltip)
        caseInsensitivePatterns->setToolTip(QCoreApplication::translate("MimeCategoryConfigPage", "<p style='white-space:pre'>Add filename patterns here, each one on a separate line.<br/>Use * or ? as wildcards and [] for matching sets of characters.</p>", nullptr));
#endif // QT_CONFIG(tooltip)
        caseSensitivePatternsCaption->setText(QCoreApplication::translate("MimeCategoryConfigPage", "Patterns (case &sensitive)", nullptr));
#if QT_CONFIG(tooltip)
        caseSensitivePatterns->setToolTip(QCoreApplication::translate("MimeCategoryConfigPage", "<p style='white-space:pre'>Add filename patterns here, each one on a separate line.<br/>Use * or ? as wildcards and [] for matching sets of characters.</p>", nullptr));
#endif // QT_CONFIG(tooltip)
        tabWidget->setTabText(tabWidget->indexOf(patternsTab), QCoreApplication::translate("MimeCategoryConfigPage", "&Patterns", nullptr));
#if QT_CONFIG(tooltip)
        treemapView->setToolTip(QCoreApplication::translate("MimeCategoryConfigPage", "Preview treemap appearance", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        squarifiedCheckBox->setToolTip(QCoreApplication::translate("MimeCategoryConfigPage", "Whether to use the newer \"squarify\" tiling algorithm", nullptr));
#endif // QT_CONFIG(tooltip)
        squarifiedCheckBox->setText(QCoreApplication::translate("MimeCategoryConfigPage", "S&quarified tiles", nullptr));
#if QT_CONFIG(tooltip)
        cushionShadingCheckBox->setToolTip(QCoreApplication::translate("MimeCategoryConfigPage", "Whether to draw raised 3-dimensional tile surfaces", nullptr));
#endif // QT_CONFIG(tooltip)
        cushionShadingCheckBox->setText(QCoreApplication::translate("MimeCategoryConfigPage", "Cushion &shading", nullptr));
#if QT_CONFIG(tooltip)
        cushionHeightSpinBox->setToolTip(QCoreApplication::translate("MimeCategoryConfigPage", "Height of the base cushion - taller cushions produce deeper shadows", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        heightScaleFactorSpinBox->setToolTip(QCoreApplication::translate("MimeCategoryConfigPage", "Factor to reduce child cushion heights - lower values produce weaker shadows", nullptr));
#endif // QT_CONFIG(tooltip)
        cushionHeightLabel->setText(QCoreApplication::translate("MimeCategoryConfigPage", "Cushion &height", nullptr));
        heightScaleFactorLabel->setText(QCoreApplication::translate("MimeCategoryConfigPage", "Height scale &factor", nullptr));
#if QT_CONFIG(tooltip)
        tileColorButton->setToolTip(QCoreApplication::translate("MimeCategoryConfigPage", "Choose the same colour for all tiles", nullptr));
#endif // QT_CONFIG(tooltip)
        tileColorButton->setText(QCoreApplication::translate("MimeCategoryConfigPage", "Fi&xed colour", nullptr));
#if QT_CONFIG(tooltip)
        tileColorEdit->setToolTip(QCoreApplication::translate("MimeCategoryConfigPage", "Enter an RGB colour code directly to use for all tiles", nullptr));
#endif // QT_CONFIG(tooltip)
        tileColorEdit->setText(QString());
        tileColorEdit->setPlaceholderText(QCoreApplication::translate("MimeCategoryConfigPage", "Fixed tile colour", nullptr));
        label_4->setText(QCoreApplication::translate("MimeCategoryConfigPage", "Minimum tile si&ze", nullptr));
#if QT_CONFIG(tooltip)
        minTileSizeSpinBox->setToolTip(QCoreApplication::translate("MimeCategoryConfigPage", "The smallest tile size that will be placed into the treemap", nullptr));
#endif // QT_CONFIG(tooltip)
        tabWidget->setTabText(tabWidget->indexOf(treemapTab), QCoreApplication::translate("MimeCategoryConfigPage", "&Treemap", nullptr));
        (void)MimeCategoryConfigPage;
    } // retranslateUi

};

namespace Ui {
    class MimeCategoryConfigPage: public Ui_MimeCategoryConfigPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MIME_2D_CATEGORY_2D_CONFIG_2D_PAGE_H
