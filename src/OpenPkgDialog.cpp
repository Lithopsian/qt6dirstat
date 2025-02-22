/*
 *   File name: OpenPkgDialog.cpp
 *   Summary:   QDirStat "open installed packages" dialog
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "OpenPkgDialog.h"
#include "Logger.h"
#include "Settings.h"


using namespace QDirStat;


OpenPkgDialog::OpenPkgDialog( QWidget * parent ):
    QDialog{ parent },
    _ui{ new Ui::OpenPkgDialog }
{
    // logDebug() << "init" << Qt::endl;

    _ui->setupUi( this );

    _ui->pkgPatternField->setClearButtonEnabled( true );
    _ui->pkgPatternField->setFocus();

    connect( _ui->pkgPatternField, &QLineEdit::textChanged,
             this,                 &OpenPkgDialog::textEdited );

    Settings::readWindowSettings( this, "OpenPkgDialog" );
}


OpenPkgDialog::~OpenPkgDialog()
{
    Settings::writeWindowSettings( this, "OpenPkgDialog" );
}


PkgFilter OpenPkgDialog::pkgFilter()
{
    if ( _ui->allPkgRadioButton->isChecked() )
    {
        // logDebug() << "SelectAll" << Qt::endl;
        return PkgFilter();
    }

    const auto mode = SearchFilter::FilterMode( _ui->filterModeComboBox->currentIndex() );
    PkgFilter filter{ _ui->pkgPatternField->text(), mode };
    // logDebug() << filter << Qt::endl;

    return filter;
}


bool OpenPkgDialog::askPkgFilter( PkgFilter & pkgFilter )
{
    OpenPkgDialog dialog{};
    const int result = dialog.exec();

    const bool cancelled = (result == QDialog::Rejected );
    pkgFilter = cancelled ? PkgFilter{} : dialog.pkgFilter();

    return !cancelled;
}


void OpenPkgDialog::textEdited()
{
    if ( _ui->pkgPatternField->text().isEmpty() )
        _ui->allPkgRadioButton->setChecked( true );
    else
        _ui->useFilterRadioButton->setChecked( true );
}

