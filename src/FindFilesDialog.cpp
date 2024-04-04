/*
 *   File name: FindFilesDialog.cpp
 *   Summary:	QDirStat "Find Files" dialog
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


//#include "Qt4Compat.h"       // qEnableClearButton()

#include "FindFilesDialog.h"
#include "QDirStatApp.h"
#include "DirInfo.h"
#include "DirTree.h"
#include "FileSearchFilter.h"
#include "Settings.h"
#include "SettingsHelpers.h"
#include "Subtree.h"
#include "Logger.h"
#include "Exception.h"


using namespace QDirStat;

// Values that should be persistent for one program run,
// but not written to the settings / config file
static QString lastPattern;
static QString lastPath;


FindFilesDialog::FindFilesDialog( QWidget * parent ):
    QDialog ( parent ),
    _ui { new Ui::FindFilesDialog }
{
    // logDebug() << "init" << Qt::endl;

    CHECK_NEW( _ui );
    _ui->setupUi( this );

    if ( lastPath.isEmpty() && app()->root() )
        lastPath = app()->root()->url();

    _ui->patternField->setClearButtonEnabled( true );
    _ui->patternField->setFocus();

    connect( this, &FindFilesDialog::accepted,
	     this, &FindFilesDialog::saveValues);

    loadValues();
}


FindFilesDialog::~FindFilesDialog()
{
    //logDebug() << " destructor called" << Qt::endl;
    delete _ui;
}


FileSearchFilter FindFilesDialog::fileSearchFilter()
{
    FileInfo * subtree = nullptr;

    if ( _ui->wholeTreeRadioButton->isChecked() )
        subtree = app()->root();
    else if ( _ui->currentSubtreeRadioButton->isChecked() )
        subtree = currentSubtree();

    FileSearchFilter filter( subtree ? subtree->toDirInfo() : nullptr,
                             _ui->patternField->text(),
                             ( SearchFilter::FilterMode )_ui->filterModeComboBox->currentIndex(),
                             _ui->caseSensitiveCheckBox->isChecked() );

    filter.setFindFiles( _ui->findFilesRadioButton->isChecked() ||
                         _ui->findBothRadioButton->isChecked()    );

    filter.setFindDirs( _ui->findDirectoriesRadioButton->isChecked() ||
                        _ui->findBothRadioButton->isChecked()          );

    filter.setFindSymLinks( _ui->findSymLinksCheckBox->isChecked() );

    filter.setFindPkg( filter.findDirs() );

    //logDebug() << filter << Qt::endl;

    return filter;
}


DirInfo * FindFilesDialog::currentSubtree()
{
    FileInfo * subtree = app()->selectedDirInfo();
    if ( subtree )
    {
        lastPath = subtree->url();
    }
    else
    {
        subtree = app()->dirTree()->locate( lastPath,
                                            true     ); // findPseudoDirs
        if ( ! subtree ) // lastPath outside of this tree?
        {
            if ( app()->root() )
            {
                subtree  = app()->root();
                lastPath = subtree->url();
            }
            else
            {
                lastPath.clear();
            }
        }
    }

    return subtree ? subtree->toDirInfo() : nullptr;
}


FileSearchFilter FindFilesDialog::askFindFiles( bool    * canceled_ret,
                                                QWidget * parent )
{
    FindFilesDialog dialog( parent );
    const int result = dialog.exec();

    const bool canceled = ( result == QDialog::Rejected );

    const FileSearchFilter filter = canceled ? FileSearchFilter() : dialog.fileSearchFilter();

    if ( canceled_ret )
	*canceled_ret = filter.pattern().isEmpty() ? true : canceled;

    return filter;
}


void FindFilesDialog::loadValues()
{
    readSettings();

    // Restore those values from static variables
    _ui->patternField->setText( lastPattern );

    const FileInfo * sel  = currentSubtree();
    const QString  & path = sel ? sel->url() : lastPath;
    if ( sel )
        lastPath = path;

    _ui->currentSubtreePathLabel->setText( path );
    _ui->currentSubtreeRadioButton->setEnabled( ! path.isEmpty() );
}


void FindFilesDialog::saveValues()
{
    writeSettings();

    //
    // Values that should not be written to the settings / the config file:
    // Save to static variables just for the duration of this program run as
    // the dialog is created, destroyed and created every time the user starts
    // the "Find Files" action (Ctrl-F).
    //

    lastPattern = _ui->patternField->text();
    lastPath    = _ui->currentSubtreePathLabel->text();
}


void FindFilesDialog::readSettings()
{
    // logDebug() << Qt::endl;

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

    // Intentionally NOT reading from the settings / the config file:
    //
    // _ui->patternField->setText(...);
    // _ui->currentSubtreePathLabel->setText(...);

    readWindowSettings( this, "FindFilesDialog" );
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

    // Intentionally NOT writing to the settings / the config file:
    //
    // _ui->patternField->text();
    // _ui->currentSubtreePathLabel->text();

    writeWindowSettings( this, "FindFilesDialog" );
}
