/*
 *   File name: FindFilesDialog.cpp
 *   Summary:   QDirStat "Find Files" dialog
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QResizeEvent>

#include "FindFilesDialog.h"
#include "DirInfo.h"
#include "DirTree.h"
#include "DiscoverActions.h"
#include "FileSearchFilter.h"
#include "FormatUtil.h"
#include "Logger.h"
#include "QDirStatApp.h"
#include "Settings.h"
#include "Subtree.h"


using namespace QDirStat;


QString FindFilesDialog::_lastPattern;
QString FindFilesDialog::_lastPath;


FindFilesDialog::FindFilesDialog( QWidget * parent ):
    QDialog { parent },
    _ui { new Ui::FindFilesDialog }
{
    // logDebug() << "init" << Qt::endl;

    _ui->setupUi( this );

    _lastPath = app()->dirTree()->url();

    connect( this, &FindFilesDialog::accepted,
             this, &FindFilesDialog::saveValues );

    loadValues();
}


FindFilesDialog::~FindFilesDialog()
{
    //logDebug() << " destructor called" << Qt::endl;

    // Always save the window geometry
    Settings::writeWindowSettings( this, "FindFilesDialog" );;
}


FileSearchFilter FindFilesDialog::fileSearchFilter()
{
    FileInfo * fileInfo = _ui->wholeTreeRadioButton->isChecked() ? app()->firstToplevel() : currentSubtree();
    const bool findDirs = _ui->findDirectoriesRadioButton->isChecked() || _ui->findBothRadioButton->isChecked();

    FileSearchFilter filter( fileInfo ? fileInfo->toDirInfo() : nullptr,
                             _ui->patternField->text(),
                             static_cast<FilterMode>( _ui->filterModeComboBox->currentIndex() ),
                             _ui->caseSensitiveCheckBox->isChecked(),
                             _ui->findFilesRadioButton->isChecked() || _ui->findBothRadioButton->isChecked(),
                             findDirs,
                             _ui->findSymLinksCheckBox->isChecked(),
                             findDirs );

    return filter;
}


DirInfo * FindFilesDialog::currentSubtree()
{
    FileInfo * fileInfo = app()->currentDirInfo();
    if ( fileInfo )
    {
        _lastPath = fileInfo->url();
    }
    else
    {
        fileInfo = app()->dirTree()->locate( _lastPath );
        if ( !fileInfo ) // _lastPath outside of this tree
        {
            auto firstToplevel = app()->firstToplevel();
            if ( firstToplevel )
            {
                fileInfo  = firstToplevel;
                _lastPath = fileInfo->url();
            }
            else
            {
                _lastPath.clear();
            }
        }
    }

    return fileInfo ? fileInfo->toDirInfo() : nullptr;
}


void FindFilesDialog::askFindFiles( QWidget * parent )
{
    FindFilesDialog dialog( parent );

    const int result = dialog.exec();
    const bool cancelled = ( result == QDialog::Rejected );
    const FileSearchFilter filter = cancelled ? FileSearchFilter() : dialog.fileSearchFilter();

    if ( !cancelled )
        DiscoverActions::findFiles( filter );
}


void FindFilesDialog::loadValues()
{
    readSettings();

    _ui->patternField->setText( _lastPattern );

    const FileInfo * sel = currentSubtree();
    if ( sel )
        _lastPath = sel->url();

    _ui->currentSubtreeRadioButton->setEnabled( !_lastPath.isEmpty() );

    // The subtree label will be filled in by the resizeEvent handler
}


void FindFilesDialog::saveValues()
{
    writeSettings();

    // Values that should not be written to the settings / the config file:
    // Save to static variables just for the duration of this program run as
    // the dialog is created, destroyed and created every time the user starts
    // the "Find Files" action (Ctrl-F).
    _lastPattern = _ui->patternField->text();
    _lastPath    = _ui->currentSubtreePathLabel->text();
}


void FindFilesDialog::readSettings()
{
    QDirStat::Settings settings;

    settings.beginGroup( "FindFilesDialog" );

    _ui->filterModeComboBox->setCurrentText     ( settings.value( "filterMode",     "Auto" ).toString() );
    _ui->caseSensitiveCheckBox->setChecked      ( settings.value( "caseSensitive",  false  ).toBool()   );

    _ui->findFilesRadioButton->setChecked       ( settings.value( "findFiles",      false  ).toBool() );
    _ui->findDirectoriesRadioButton->setChecked ( settings.value( "findDirs",       false  ).toBool() );
    _ui->findBothRadioButton->setChecked        ( settings.value( "findBoth",       true   ).toBool() );
    _ui->findSymLinksCheckBox->setChecked       ( settings.value( "findSymLinks",   true   ).toBool() );

    _ui->wholeTreeRadioButton->setChecked       ( settings.value( "wholeTree",      true   ).toBool() );
    _ui->currentSubtreeRadioButton->setChecked  ( settings.value( "currentSubtree", false  ).toBool() );

    settings.endGroup();

    Settings::readWindowSettings( this, "FindFilesDialog" );
}


void FindFilesDialog::writeSettings()
{
    // logDebug() << Qt::endl;

    QDirStat::Settings settings;

    settings.beginGroup( "FindFilesDialog" );

    settings.setValue( "filterMode",     _ui->filterModeComboBox->currentText()       );
    settings.setValue( "caseSensitive",  _ui->caseSensitiveCheckBox->isChecked()      );

    settings.setValue( "findFiles",      _ui->findFilesRadioButton->isChecked()       );
    settings.setValue( "findDirs",       _ui->findDirectoriesRadioButton->isChecked() );
    settings.setValue( "findBoth",       _ui->findBothRadioButton->isChecked()        );
    settings.setValue( "findSymLinks",   _ui->findSymLinksCheckBox->isChecked()       );

    settings.setValue( "wholeTree",      _ui->wholeTreeRadioButton->isChecked()       );
    settings.setValue( "currentSubtree", _ui->currentSubtreeRadioButton->isChecked()  );

    settings.endGroup();
}


void FindFilesDialog::resizeEvent( QResizeEvent * event )
{
    // Calculate a width from the dialog less margins, less a bit more
    elideLabel( _ui->currentSubtreePathLabel, _lastPath, event->size().width() - 100 );
}
