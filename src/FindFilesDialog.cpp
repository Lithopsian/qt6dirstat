/*
 *   File name: FindFilesDialog.cpp
 *   Summary:	QDirStat "Find Files" dialog
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <QResizeEvent>

#include "FindFilesDialog.h"
#include "DirInfo.h"
#include "DirTree.h"
#include "FileSearchFilter.h"
#include "QDirStatApp.h"
#include "Settings.h"
#include "SettingsHelpers.h"
#include "Subtree.h"
#include "Logger.h"
#include "Exception.h"


using namespace QDirStat;


QString FindFilesDialog::_lastPattern;
QString FindFilesDialog::_lastPath;


FindFilesDialog::FindFilesDialog( QWidget * parent ):
    QDialog ( parent ),
    _ui { new Ui::FindFilesDialog }
{
    // logDebug() << "init" << Qt::endl;

    CHECK_NEW( _ui );
    _ui->setupUi( this );

    if ( _lastPath.isEmpty() && app()->root() )
        _lastPath = app()->root()->url();

    _ui->patternField->setClearButtonEnabled( true );
    _ui->patternField->setFocus();

    connect( this, &FindFilesDialog::accepted,
             this, &FindFilesDialog::saveValues );

    loadValues();
}


FindFilesDialog::~FindFilesDialog()
{
    //logDebug() << " destructor called" << Qt::endl;

    // Always save the window geometry
    writeWindowSettings( this, "FindFilesDialog" );;

    delete _ui;
}


FileSearchFilter FindFilesDialog::fileSearchFilter()
{
    FileInfo * subtree = _ui->wholeTreeRadioButton->isChecked() ? app()->root() : currentSubtree();
    FileSearchFilter filter( subtree ? subtree->toDirInfo() : nullptr,
                             _ui->patternField->text(),
                             static_cast<SearchFilter::FilterMode>( _ui->filterModeComboBox->currentIndex() ),
                             _ui->caseSensitiveCheckBox->isChecked() );
    //logDebug() << filter << Qt::endl;

    filter.setFindFiles( _ui->findFilesRadioButton->isChecked() || _ui->findBothRadioButton->isChecked() );
    filter.setFindDirs( _ui->findDirectoriesRadioButton->isChecked() || _ui->findBothRadioButton->isChecked() );
    filter.setFindSymLinks( _ui->findSymLinksCheckBox->isChecked() );
    filter.setFindPkg( filter.findDirs() );

    return filter;
}


DirInfo * FindFilesDialog::currentSubtree()
{
    FileInfo * subtree = app()->selectedDirInfo();
    if ( subtree )
    {
        _lastPath = subtree->url();
    }
    else
    {
        subtree = app()->dirTree()->locate( _lastPath,
                                            true     ); // findPseudoDirs
        if ( !subtree ) // _lastPath outside of this tree?
        {
            if ( app()->root() )
            {
                subtree  = app()->root();
                _lastPath = subtree->url();
            }
            else
            {
                _lastPath.clear();
            }
        }
    }

    return subtree ? subtree->toDirInfo() : nullptr;
}


FileSearchFilter FindFilesDialog::askFindFiles( bool    * cancelled_ret,
                                                QWidget * parent )
{
    FindFilesDialog dialog( parent );
    const int result = dialog.exec();

    const bool cancelled = ( result == QDialog::Rejected );

    const FileSearchFilter filter = cancelled ? FileSearchFilter() : dialog.fileSearchFilter();

    if ( cancelled_ret )
	*cancelled_ret = filter.pattern().isEmpty() ? true : cancelled;

    return filter;
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
}


void FindFilesDialog::resizeEvent( QResizeEvent * event )
{
    // Calculate a width from the dialog less margins, less a bit more
    const int maxSize = event->size().width() - 100;
    const QFontMetrics metrics( _ui->currentSubtreePathLabel->font() );
    _ui->currentSubtreePathLabel->setText( metrics.elidedText( _lastPath, Qt::ElideMiddle, maxSize ) );
}
