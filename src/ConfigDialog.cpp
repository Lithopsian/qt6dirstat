/*
 *   File name: ConfigDialog.cpp
 *   Summary:   QDirStat configuration dialog classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QPointer>

#include "ConfigDialog.h"
#include "CleanupConfigPage.h"
#include "ExcludeRulesConfigPage.h"
#include "GeneralConfigPage.h"
#include "HotkeyConfigPage.h"
#include "MimeCategoryConfigPage.h"
#include "Settings.h"


using namespace QDirStat;


ConfigDialog::ConfigDialog( QWidget * parent ):
    QDialog { parent },
    _ui { new Ui::ConfigDialog }
{
    setAttribute(Qt::WA_DeleteOnClose);

    _ui->setupUi( this );
    Settings::readWindowSettings( this, "ConfigDialog" );

    _ui->pagesTabWidget->addTab( new GeneralConfigPage( this ), tr( "General" ) );
    _ui->pagesTabWidget->addTab( new MimeCategoryConfigPage( this ), tr( "MIME Categories" ) );
    _ui->pagesTabWidget->addTab( new CleanupConfigPage( this ), tr( "Cleanup Actions" ) );
    _ui->pagesTabWidget->addTab( new ExcludeRulesConfigPage( this ), tr( "Exclude Rules" ) );

    connect( _ui->buttonBox->button( QDialogButtonBox::Ok ),     &QPushButton::clicked,
             this,                                               &ConfigDialog::accept );

    connect( _ui->buttonBox->button( QDialogButtonBox::Apply ),  &QPushButton::clicked,
             this,                                               &ConfigDialog::applyChanges );

    connect( _ui->buttonBox->button( QDialogButtonBox::Cancel ), &QPushButton::clicked,
             this,                                               &ConfigDialog::reject );
}


ConfigDialog::~ConfigDialog()
{
    Settings::writeWindowSettings( this, "ConfigDialog" );
}


ConfigDialog * ConfigDialog::sharedInstance( QWidget * parent )
{
    static QPointer<ConfigDialog> _sharedInstance;

    if ( !_sharedInstance )
	_sharedInstance = new ConfigDialog( parent );

    return _sharedInstance;
}


void ConfigDialog::showSharedInstance( QWidget * parent )
{
    // Get the shared instance, creating it if necessary
    ConfigDialog * instance = sharedInstance( parent );
    instance->show();
    instance->raise();
}


void ConfigDialog::accept()
{
    emit applyChanges();
    done( Accepted );
}


void ConfigDialog::reject()
{
//    emit discardChanges(); // currently no takers for this signal
    done( Rejected );
}
