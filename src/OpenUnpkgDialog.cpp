/*
 *   File name: OpenUnpkgDialog.cpp
 *   Summary:   QDirStat "show unpackaged files" dialog
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
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
    _ui->setupUi( this );

    _ui->startingDirComboBox->setCompleter( new ExistingDirCompleter( this ) );

    ExistingDirValidator * validator = new ExistingDirValidator( this );
    _ui->startingDirComboBox->setValidator( validator );

    QLineEdit * lineEdit = _ui->startingDirComboBox->lineEdit();
    if ( lineEdit )
        lineEdit->setClearButtonEnabled( true );

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
    delete _ui;
    writeWindowSettings( this, "OpenUnpkgDialog" );
}


QString OpenUnpkgDialog::startingDir() const
{
    return result() == QDialog::Accepted ? _ui->startingDirComboBox->currentText() : QString();
}


QStringList OpenUnpkgDialog::cleanedLines( const QPlainTextEdit * widget )
{
    QStringList lines = widget->toPlainText().split( '\n', Qt::SkipEmptyParts );

    QMutableListIterator<QString> it( lines );
    while ( it.hasNext() )
    {
	const QString & line = it.next();
	it.setValue( line.trimmed() );
	if ( it.value().isEmpty() )
	    it.remove();
    }

    return lines;
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
    _ui->excludeDirsTextEdit->setPlainText( settings.excludeDirs().join( '\n' ) );
    _ui->ignorePatternsTextEdit->setPlainText( settings.ignorePatterns().join( '\n' ) );
    _ui->crossFilesystemsCheckBox->setChecked( settings.crossFilesystems() );
}


void OpenUnpkgDialog::readSettings()
{
    setValues( UnpkgSettings() );

    readWindowSettings( this, "OpenUnpkgDialog" );
}


void OpenUnpkgDialog::writeSettings()
{
    UnpkgSettings settings = values();
    settings.write();
}
