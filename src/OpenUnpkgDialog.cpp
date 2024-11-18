/*
 *   File name: OpenUnpkgDialog.cpp
 *   Summary:   QDirStat "show unpackaged files" dialog
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QCompleter>
#include <QFileSystemModel>
#include <QLineEdit>
#include <QPushButton>

#include "OpenUnpkgDialog.h"
#include "ExistingDirValidator.h"
#include "Logger.h"
#include "Settings.h"


using namespace QDirStat;


namespace
{
    /**
     * Get the content of a QPlainTextEdit widget as QStringList with
     * leading and trailing whitespace removed from each line and without
     * empty lines.
     **/
    QStringList cleanedLines( const QPlainTextEdit * widget )
    {
	QStringList lines = widget->toPlainText().split( u'\n', Qt::SkipEmptyParts );

	QMutableListIterator<QString> it{ lines };
	while ( it.hasNext() )
	{
	    const QString & line = it.next();
	    it.setValue( line.trimmed() );
	    if ( it.value().isEmpty() )
		it.remove();
	}

	return lines;
    }

} // namespace


OpenUnpkgDialog::OpenUnpkgDialog( QWidget * parent ):
    QDialog{ parent },
    _ui{ new Ui::OpenUnpkgDialog }
{
    _ui->setupUi( this );

    QLineEdit * lineEdit = _ui->startingDirComboBox->lineEdit();
    if ( lineEdit )
        lineEdit->setClearButtonEnabled( true );

    QFileSystemModel * model = new QFileSystemModel{ this };
    model->setRootPath( "/" );
    model->setFilter( QDir::Dirs );
    model->setReadOnly( true );
    _ui->startingDirComboBox->setCompleter( new QCompleter{ model, this } );

    ExistingDirValidator * validator = new ExistingDirValidator{ this };
    _ui->startingDirComboBox->setValidator( validator );

    const QPushButton * resetButton = _ui->buttonBox->button( QDialogButtonBox::RestoreDefaults );
    const QPushButton * okButton    = _ui->buttonBox->button( QDialogButtonBox::Ok );

    connect( resetButton, &QPushButton::clicked,
             this,        &OpenUnpkgDialog::restoreDefaults );

    connect( validator, &ExistingDirValidator::isOk,
             okButton, &QPushButton::setEnabled );

    connect( this, &OpenUnpkgDialog::accepted,
             this, &OpenUnpkgDialog::writeSettings );

    readSettings();
}


OpenUnpkgDialog::~OpenUnpkgDialog()
{
    Settings::writeWindowSettings( this, "OpenUnpkgDialog" );
}


void OpenUnpkgDialog::restoreDefaults()
{
    setValues( UnpkgSettings::defaultSettings() );
}


UnpkgSettings OpenUnpkgDialog::values() const
{
    const QString startingDir =
	result() == QDialog::Accepted ? _ui->startingDirComboBox->currentText() : QString{};
    const QStringList excludeDirs = cleanedLines( _ui->excludeDirsTextEdit );
    const QStringList ignoredPatterns = cleanedLines( _ui->ignorePatternsTextEdit );
    const bool crossFilesystems = _ui->crossFilesystemsCheckBox->isChecked();
    UnpkgSettings settings{ startingDir, excludeDirs, ignoredPatterns, crossFilesystems };
    settings.dump();

    return settings;
}


void OpenUnpkgDialog::setValues( const UnpkgSettings & settings )
{
    settings.dump();
    _ui->startingDirComboBox->setCurrentText( settings.startingDir() );
    _ui->excludeDirsTextEdit->setPlainText( settings.excludeDirs().join( u'\n' ) );
    _ui->ignorePatternsTextEdit->setPlainText( settings.ignorePatterns().join( u'\n' ) );
    _ui->crossFilesystemsCheckBox->setChecked( settings.crossFilesystems() );
}


void OpenUnpkgDialog::readSettings()
{
    setValues( UnpkgSettings() );

    Settings::readWindowSettings( this, "OpenUnpkgDialog" );
}


void OpenUnpkgDialog::writeSettings()
{
    UnpkgSettings settings = values();
    settings.write();
}
