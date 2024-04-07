/*
 *   File name: OpenUnpkgDialog.cpp
 *   Summary:	QDirStat "show unpackaged files" dialog
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <QPushButton>
#include <QLineEdit>

#include "OpenUnpkgDialog.h"
#include "ExistingDir.h"
#include "Settings.h"
#include "SettingsHelpers.h"
#include "Logger.h"
#include "Exception.h"


using namespace QDirStat;


OpenUnpkgDialog::OpenUnpkgDialog( QWidget * parent ):
    QDialog ( parent ),
    _ui { new Ui::OpenUnpkgDialog }
{
    CHECK_NEW( _ui );
    _ui->setupUi( this );

    _okButton = _ui->buttonBox->button( QDialogButtonBox::Ok );
    CHECK_PTR( _okButton );

    QCompleter * completer = new ExistingDirCompleter( this );
    CHECK_NEW( completer );
    _ui->startingDirComboBox->setCompleter( completer );

    ExistingDirValidator * validator = new ExistingDirValidator( this );
    CHECK_NEW( validator );
    _ui->startingDirComboBox->setValidator( validator );

    QLineEdit * lineEdit = _ui->startingDirComboBox->lineEdit();
    if ( lineEdit )
        lineEdit->setClearButtonEnabled( true );

    QPushButton * resetButton = _ui->buttonBox->button( QDialogButtonBox::RestoreDefaults );
    CHECK_PTR( resetButton );

    connect( resetButton, &QPushButton::clicked,
	     this,        &OpenUnpkgDialog::restoreDefaults );

    connect( validator, &ExistingDirValidator::isOk,
	     _okButton, &QPushButton::setEnabled );

    connect( this, &OpenUnpkgDialog::accepted,
	     this, &OpenUnpkgDialog::writeSettings );

    readSettings();
}


OpenUnpkgDialog::~OpenUnpkgDialog()
{
    delete _ui;
}


QString OpenUnpkgDialog::startingDir() const
{
    return result() == QDialog::Accepted ? _ui->startingDirComboBox->currentText() : "";
}


QStringList OpenUnpkgDialog::cleanedLines( const QPlainTextEdit *widget ) const
{
    const QString text = widget->toPlainText();
    const QStringList lines  = text.split( '\n', Qt::SkipEmptyParts );
    QStringList result;

    for ( QString line : lines )
    {
	line = line.trimmed();
	if ( !line.isEmpty() )
	    result << line;
    }

    return result;
}


void OpenUnpkgDialog::restoreDefaults()
{
    setValues( UnpkgSettings::defaultSettings() );
}


UnpkgSettings OpenUnpkgDialog::values() const
{
    UnpkgSettings settings( startingDir(), excludeDirs(), ignorePatterns(), crossFilesystems() );
    settings.dump();

    return settings;
}


void OpenUnpkgDialog::setValues( const UnpkgSettings & settings )
{
    settings.dump();
    _ui->startingDirComboBox->setCurrentText( settings.startingDir() );
    _ui->excludeDirsTextEdit->setPlainText( settings.excludeDirs().join( "\n" ) );
    _ui->ignorePatternsTextEdit->setPlainText( settings.ignorePatterns().join( "\n" ) );
    _ui->crossFilesystemsCheckBox->setChecked( settings.crossFilesystems() );
}


void OpenUnpkgDialog::readSettings()
{
    setValues( UnpkgSettings() );

    readWindowSettings( this, "OpenUnkpgDialog" );
}


void OpenUnpkgDialog::writeSettings()
{
    UnpkgSettings settings = values();
    settings.write();

    writeWindowSettings( this, "OpenUnkpgDialog" );
}
