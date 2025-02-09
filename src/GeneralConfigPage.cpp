/*
 *   File name: GeneralConfigPage.cpp
 *   Summary:   QDirStat configuration dialog classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "GeneralConfigPage.h"
#include "ConfigDialog.h"
#include "DirTreeModel.h"
#include "Logger.h"
#include "MainWindow.h"
#include "QDirStatApp.h"
#include "Settings.h"


using namespace QDirStat;


namespace
{
    /**
     * Populate the widgets from the values held in MainWindow and DirTreeModel.
     **/
    void setup( const Ui::GeneralConfigPage * ui )
    {
        // All these settings are held in variables in MainWindow, DirTreeModel, and FileDetailsView
        const DirTreeModel * dirTreeModel = app()->dirTreeModel();
        ui->crossFilesystemsCheckBox->setChecked  ( dirTreeModel->crossFilesystems() );
        ui->useBoldForDominantCheckBox->setChecked( dirTreeModel->useBoldForDominantItems() );
        ui->treeUpdateIntervalSpinBox->setValue   ( dirTreeModel->updateTimerMillisec() );
        ui->treeIconThemeComboBox->setCurrentIndex( dirTreeModel->dirTreeItemSize() );

        const MainWindow * mainWindow = app()->mainWindow();
        ui->urlInWindowTitleCheckBox->setChecked  ( mainWindow->urlInWindowTitle() );
        ui->elidePathsCheckBox->setChecked        ( mainWindow->fileDetailsView()->elideToFit() );
        ui->dirReadWarningCheckBox->setChecked    ( mainWindow->showDirPermissionsMsg() );
        ui->useTreemapHoverCheckBox->setChecked   ( mainWindow->treemapView()->useTreemapHover() );
        ui->statusBarShortTimeoutSpinBox->setValue( mainWindow->statusBarTimeout() / 1000.0 );
        ui->statusBarLongTimeoutSpinBox->setValue ( mainWindow->longStatusBarTimeout() / 1000.0 );
        ui->homeTrashCheckBox->setChecked         ( mainWindow->onlyUseHomeTrashDir() );
        ui->copyAndDeleteCheckBox->setChecked     ( mainWindow->copyAndDeleteTrash() );

        // Use word-joiner character to stop unwanted line breaks
        const QString joinedFileName = Settings::primaryFileName().replace( u'/', "/⁠" );
        ui->explainerLabel->setText( QObject::tr( "There are many more settings in the file " ) + joinedFileName );
    }

}


GeneralConfigPage::GeneralConfigPage( ConfigDialog * parent ):
    QWidget{ parent },
    _ui{ new Ui::GeneralConfigPage }
{
    _ui->setupUi( this );

    setup( _ui.get() );

    connect( parent, &ConfigDialog::applyChanges,
             this,   &GeneralConfigPage::applyChanges );
}


void GeneralConfigPage::applyChanges()
{
    //logDebug() << Qt::endl;

    DirTreeModel * dirTreeModel = app()->dirTreeModel();
    dirTreeModel->updateSettings( _ui->crossFilesystemsCheckBox->isChecked(),
                                  _ui->useBoldForDominantCheckBox->isChecked(),
                                  static_cast<DirTreeItemSize>( _ui->treeIconThemeComboBox->currentIndex() ),
                                  _ui->treeUpdateIntervalSpinBox->value() );

    MainWindow * mainWindow = app()->mainWindow();
    mainWindow->setUrlInWindowTitle              ( _ui->urlInWindowTitleCheckBox->isChecked() );
    mainWindow->setShowDirPermissionsMsg         ( _ui->dirReadWarningCheckBox->isChecked() );
    mainWindow->treemapView()->setUseTreemapHover( _ui->useTreemapHoverCheckBox->isChecked() );
    mainWindow->setStatusBarTimeout              ( 1000 * _ui->statusBarShortTimeoutSpinBox->value() );
    mainWindow->setLongStatusBarTimeout          ( 1000 * _ui->statusBarLongTimeoutSpinBox->value() );
    mainWindow->setOnlyUseHomeTrashDir           ( _ui->homeTrashCheckBox->isChecked() );
    mainWindow->setCopyAndDeleteTrash            ( _ui->copyAndDeleteCheckBox->isChecked() );

    // Only do this relativcely expensive operation if the value has changed
    const bool elideToFit = _ui->elidePathsCheckBox->isChecked();
    if ( elideToFit != mainWindow->fileDetailsView()->elideToFit() )
        mainWindow->fileDetailsView()->setElideToFit( elideToFit );
}
