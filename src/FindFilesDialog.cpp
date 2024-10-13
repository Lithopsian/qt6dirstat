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
#include "FileInfo.h"
#include "DiscoverActions.h"
#include "FileSearchFilter.h"
#include "FormatUtil.h"
#include "Logger.h"
#include "QDirStatApp.h"
#include "Settings.h"


using namespace QDirStat;


namespace
{
    /**
     * Return the currently selected subtree if a directory is selected,
     * otherwise the top level directory.
     **/
    FileInfo * currentSubtree()
    {
        FileInfo * fileInfo = app()->currentDirInfo();
        if ( fileInfo )
            return fileInfo;

        return app()->firstToplevel();
    }

}


FindFilesDialog::FindFilesDialog( QWidget * parent, const QString & pattern ):
    QDialog{ parent },
    _ui{ new Ui::FindFilesDialog }
{
    _ui->setupUi( this );

    readSettings();

    _ui->patternField->setText( pattern );

    QString subtreeText = currentSubtree()->url();
    _ui->currentSubtreePathLabel->setStatusTip( subtreeText );
    _ui->currentSubtreeRadioButton->setEnabled( !subtreeText.isEmpty() );

    // The subtree label will be filled in by the resizeEvent handler
}


FindFilesDialog::~FindFilesDialog()
{
    //logDebug() << " destructor called" << Qt::endl;

    // Always save the window geometry
    Settings::writeWindowSettings( this, "FindFilesDialog" );
}


FileSearchFilter FindFilesDialog::fileSearchFilter()
{
    FileInfo * fileInfo  = _ui->wholeTreeRadioButton->isChecked() ? app()->firstToplevel() : currentSubtree();
    const bool findBoth  = _ui->findBothRadioButton->isChecked();
    const bool findDirs  = _ui->findDirectoriesRadioButton->isChecked() || findBoth;
    const bool findFiles = _ui->findFilesRadioButton->isChecked() || findBoth;

    return FileSearchFilter{ fileInfo,
                            _ui->patternField->text(),
                            static_cast<FilterMode>( _ui->filterModeComboBox->currentIndex() ),
                            _ui->caseSensitiveCheckBox->isChecked(),
                            findFiles,
                            findDirs,
                            _ui->findSymLinksCheckBox->isChecked(),
                            findDirs };
}


void FindFilesDialog::askFindFiles( QWidget * parent )
{
    // Remember the pattern string, but don't put it in Settings
    static QString pattern;

    // Execute as a modal dialog - will wait here for it to complete
    FindFilesDialog dialog{ parent, pattern };
    int result = dialog.exec();

    // Only save the dialog values and execute the search if the dialog is accepted
    if ( result == QDialog::Accepted )
    {
        const FileSearchFilter filter = dialog.fileSearchFilter();
        pattern = filter.pattern();
        dialog.writeSettings();
        DiscoverActions::findFiles( filter );
    }
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
    // The first resize event is before the layouts are done, so ignore it
    if ( event && !event->oldSize().isValid() )
        return;

    // Calculate the right-hand edge of the available space from the parent frame less various margins
    QLabel * label = _ui->currentSubtreePathLabel;
    const int lastPixel = _ui->treeFrame->width() -
                          _ui->treeFrame->layout()->contentsMargins().right() -
                          _ui->pathHBox->contentsMargins().right() -
                          label->contentsMargins().right() -
                          label->frameWidth() * 2;

    // Because of the frame, there is also an indent
    const int indent = horizontalAdvance( font(), u'x' );

    // Elide the label to the available space less a pixel to allow shrinking
    elideLabel( label, label->statusTip(), lastPixel - indent - 16 );
}


void FindFilesDialog::showEvent( QShowEvent * )
{
    // (Re-)display the path label
    resizeEvent( nullptr );
}
