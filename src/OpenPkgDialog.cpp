/*
 *   File name: OpenPkgDialog.cpp
 *   Summary:	QDirStat "open installed packages" dialog
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include "OpenPkgDialog.h"
#include "Logger.h"
#include "Exception.h"


using namespace QDirStat;


OpenPkgDialog::OpenPkgDialog( QWidget * parent ):
    QDialog ( parent ),
    _ui { new Ui::OpenPkgDialog }
{
    // logDebug() << "init" << Qt::endl;

    CHECK_NEW( _ui );
    _ui->setupUi( this );
    _ui->pkgPatternField->setClearButtonEnabled( true );
    _ui->pkgPatternField->setFocus();

    connect( _ui->pkgPatternField, &QLineEdit::textChanged,
             this,                 &OpenPkgDialog::textEdited );

}


OpenPkgDialog::~OpenPkgDialog()
{
    delete _ui;
}


PkgFilter OpenPkgDialog::pkgFilter()
{
    if ( _ui->allPkgRadioButton->isChecked() )
    {
        // logDebug() << "SelectAll" << Qt::endl;
        return PkgFilter();
    }

    const auto mode       = PkgFilter::FilterMode( _ui->filterModeComboBox->currentIndex() );
    const QString pattern = _ui->pkgPatternField->text();
    PkgFilter filter( pattern, mode );
    // logDebug() << filter << Qt::endl;

    return filter;
}


PkgFilter OpenPkgDialog::askPkgFilter( bool    * canceled_ret,
                                       QWidget * parent )
{
    OpenPkgDialog dialog( parent );
    const int result = dialog.exec();

    const bool canceled = (result == QDialog::Rejected );

    const PkgFilter pkgFilter = canceled ? PkgFilter() : dialog.pkgFilter();

    if ( canceled_ret )
        *canceled_ret = canceled;

    return pkgFilter;
}


void OpenPkgDialog::textEdited()
{
    if ( _ui->pkgPatternField->text().isEmpty() )
        _ui->allPkgRadioButton->setChecked( true );
    else
        _ui->useFilterRadioButton->setChecked( true );
}

