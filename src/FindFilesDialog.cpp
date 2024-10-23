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
     * Read settings from the config file.  All the dialog fields
     * except for 'pattern' are saved.  The 'pattern' string is
     * remembered in local static storage only for as long as the
     * program is open.  The window size and position are also
     * loaded from Settings.
     **/
    void readSettings( FindFilesDialog * window, const Ui::FindFilesDialog * ui )
    {
        QDirStat::Settings settings;

        settings.beginGroup( "FindFilesDialog" );

        ui->filterModeComboBox->setCurrentText     ( settings.value( "filterMode",     "Auto" ).toString() );
        ui->caseSensitiveCheckBox->setChecked      ( settings.value( "caseSensitive",  false  ).toBool()   );

        ui->findFilesRadioButton->setChecked       ( settings.value( "findFiles",      false  ).toBool() );
        ui->findDirectoriesRadioButton->setChecked ( settings.value( "findDirs",       false  ).toBool() );
        ui->findBothRadioButton->setChecked        ( settings.value( "findBoth",       true   ).toBool() );
        ui->findSymLinksCheckBox->setChecked       ( settings.value( "findSymLinks",   true   ).toBool() );

        ui->wholeTreeRadioButton->setChecked       ( settings.value( "wholeTree",      true   ).toBool() );
        ui->currentSubtreeRadioButton->setChecked  ( settings.value( "currentSubtree", false  ).toBool() );

        settings.endGroup();

        Settings::readWindowSettings( window, "FindFilesDialog" );
    }


    /**
     * Write settings to the config file.  The dialog fields are
     * written to Settings only if the dialog is accepted.  The
     * window geometry is always saved when the dialog is destroyed.
     **/
    void writeSettings( const Ui::FindFilesDialog * ui )
    {
        QDirStat::Settings settings;

        settings.beginGroup( "FindFilesDialog" );

        settings.setValue( "filterMode",     ui->filterModeComboBox->currentText()       );
        settings.setValue( "caseSensitive",  ui->caseSensitiveCheckBox->isChecked()      );

        settings.setValue( "findFiles",      ui->findFilesRadioButton->isChecked()       );
        settings.setValue( "findDirs",       ui->findDirectoriesRadioButton->isChecked() );
        settings.setValue( "findBoth",       ui->findBothRadioButton->isChecked()        );
        settings.setValue( "findSymLinks",   ui->findSymLinksCheckBox->isChecked()       );

        settings.setValue( "wholeTree",      ui->wholeTreeRadioButton->isChecked()       );
        settings.setValue( "currentSubtree", ui->currentSubtreeRadioButton->isChecked()  );

        settings.endGroup();
    }


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


    /**
     * Return a file search filter coresponding to the values entered
     * in the dialog.
     **/
    FileSearchFilter fileSearchFilter( const Ui::FindFilesDialog * ui )
    {
        FileInfo * fileInfo  = ui->wholeTreeRadioButton->isChecked() ? app()->firstToplevel() : currentSubtree();
        const bool findBoth  = ui->findBothRadioButton->isChecked();
        const bool findDirs  = ui->findDirectoriesRadioButton->isChecked() || findBoth;
        const bool findFiles = ui->findFilesRadioButton->isChecked() || findBoth;

        return FileSearchFilter{ fileInfo,
                                ui->patternField->text(),
                                static_cast<FilterMode>( ui->filterModeComboBox->currentIndex() ),
                                ui->caseSensitiveCheckBox->isChecked(),
                                findFiles,
                                findDirs,
                                ui->findSymLinksCheckBox->isChecked(),
                                findDirs };
    }


    /**
     * Elide the path label to fit inside the current dialog width, so
     * that it fills the available width but very long subtree paths
     * don't stretch the dialog.  A little extra room is left for the
     * user to shrink the dialog, which would then force the label to
     * be elided further.
    **/
    void showPathLabel( QLabel * label, const QLayout * hbox )
    {
        // Elide the label to the available space
        elideLabel( label, label->statusTip(), hbox->contentsRect().right() );
    }

}


FindFilesDialog::FindFilesDialog( QWidget * parent, const QString & pattern ):
    QDialog{ parent },
    _ui{ new Ui::FindFilesDialog }
{
    _ui->setupUi( this );

    _ui->patternField->setValidator( new QRegularExpressionValidator{ hasNoControlCharacters(), this } );

    readSettings( this, ui() );

    _ui->patternField->setText( pattern );

    QString subtreeText = currentSubtree()->url();
    _ui->currentSubtreePathLabel->setStatusTip( replaceCrLf( subtreeText ) );
    _ui->currentSubtreeRadioButton->setEnabled( !subtreeText.isEmpty() );

    // The subtree label will be filled in by the resizeEvent handler
}


FindFilesDialog::~FindFilesDialog()
{
    //logDebug() << " destructor called" << Qt::endl;

    // Always save the window geometry
    Settings::writeWindowSettings( this, "FindFilesDialog" );
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
        const FileSearchFilter filter = fileSearchFilter( dialog.ui() );
        pattern = filter.pattern();
        writeSettings( dialog.ui() );
        DiscoverActions::findFiles( filter );
    }
}


void FindFilesDialog::resizeEvent( QResizeEvent * event )
{
    // The first resize event is before the layouts are done, so ignore it
    if ( event && event->oldSize().isValid() )
        showEvent( nullptr );
}


void FindFilesDialog::showEvent( QShowEvent * )
{
    // (Re-)display the path label
    showPathLabel( _ui->currentSubtreePathLabel, _ui->pathHBox );
}
