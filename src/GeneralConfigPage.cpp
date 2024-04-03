/*
 *   File name: GeneralConfigPage.cpp
 *   Summary:	QDirStat configuration dialog classes
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include "GeneralConfigPage.h"
#include "ConfigDialog.h"
#include "DirTreeModel.h"
#include "MainWindow.h"
#include "QDirStatApp.h"
#include "Settings.h"
#include "Logger.h"
#include "Exception.h"


using namespace QDirStat;


GeneralConfigPage::GeneralConfigPage( ConfigDialog * parent ):
    QWidget( parent ),
    _ui( new Ui::GeneralConfigPage )
{
    CHECK_NEW( _ui );
    _ui->setupUi( this );

    setup();

    connect( parent, &ConfigDialog::applyChanges,
	     this,   &GeneralConfigPage::applyChanges );
}


GeneralConfigPage::~GeneralConfigPage()
{
    delete _ui;
}


void GeneralConfigPage::setup()
{
    // All the values on this page are held in variables in MainWindow and
    // DirTreeModel (or DirTree).
    const MainWindow *mainWindow = (MainWindow *)app()->findMainWindow();
    if ( !mainWindow )
        return;

    const DirTreeModel *dirTreeModel = app()->dirTreeModel();
    if ( dirTreeModel )
    {
        _ui->crossFilesystemsCheckBox->setChecked  ( dirTreeModel->crossFilesystems() );
        _ui->useBoldForDominantCheckBox->setChecked( dirTreeModel->useBoldForDominantItems() );
        _ui->treeUpdateIntervalSpinBox->setValue   ( dirTreeModel->updateTimerMillisec() );
        _ui->treeIconThemeComboBox->setCurrentIndex( dirTreeModel->dirTreeItemSize() );
    }

    _ui->urlInWindowTitleCheckBox->setChecked( mainWindow->urlInWindowTitle() );
    _ui->useTreemapHoverCheckBox->setChecked ( mainWindow->treemapView()->useTreemapHover() );
    _ui->statusBarShortTimeoutSpinBox->setValue( mainWindow->statusBarTimeout() / 1000.0 );
    _ui->statusBarLongTimeoutSpinBox->setValue( mainWindow->longStatusBarTimeout() / 1000.0 );
}


void GeneralConfigPage::applyChanges()
{
    //logDebug() << Qt::endl;

    MainWindow *mainWindow = (MainWindow *)app()->findMainWindow();
    if ( !mainWindow )
        return;

    DirTreeModel *dirTreeModel = app()->dirTreeModel();
    if ( dirTreeModel )
    {
        dirTreeModel->updateSettings( _ui->crossFilesystemsCheckBox->isChecked(),
                                      _ui->useBoldForDominantCheckBox->isChecked(),
                                      ( DirTreeItemSize )_ui->treeIconThemeComboBox->currentIndex(),
                                      _ui->treeUpdateIntervalSpinBox->value() );

//        mainWindow->dirTreeView()->setStyles( _ui->treeIconThemeComboBox->currentIndex() );
    }

    mainWindow->updateSettings( _ui->urlInWindowTitleCheckBox->isChecked(),
                                _ui->useTreemapHoverCheckBox->isChecked(),
                                1000 * _ui->statusBarShortTimeoutSpinBox->value(),
                                1000 * _ui->statusBarLongTimeoutSpinBox->value() );

}
