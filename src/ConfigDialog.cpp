/*
 *   File name: ConfigDialog.cpp
 *   Summary:	QDirStat configuration dialog classes
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include "ConfigDialog.h"
#include "CleanupConfigPage.h"
#include "ExcludeRulesConfigPage.h"
#include "GeneralConfigPage.h"
#include "MimeCategoryConfigPage.h"
#include "Logger.h"
#include "Exception.h"

using namespace QDirStat;


ConfigDialog::ConfigDialog( QWidget * parent ):
    QDialog ( parent ),
    _ui { new Ui::ConfigDialog },
    _generalConfigPage { new GeneralConfigPage( this ) },
    _mimeCategoryConfigPage { new MimeCategoryConfigPage( this ) },
    _cleanupConfigPage { new CleanupConfigPage( this ) },
    _excludeRulesConfigPage { new ExcludeRulesConfigPage( this ) }
{
    setAttribute(Qt::WA_DeleteOnClose);

    CHECK_NEW( _ui );
    _ui->setupUi( this );

    CHECK_NEW( _generalConfigPage );
    CHECK_NEW( _mimeCategoryConfigPage );
    CHECK_NEW( _cleanupConfigPage );
    CHECK_NEW( _excludeRulesConfigPage );

    _ui->pagesTabWidget->addTab( _generalConfigPage, tr( "General" ) );
    _ui->pagesTabWidget->addTab( _mimeCategoryConfigPage, tr( "MIME Categories" ) );
    _ui->pagesTabWidget->addTab( _cleanupConfigPage, tr( "Cleanup Actions" ) );
    _ui->pagesTabWidget->addTab( _excludeRulesConfigPage, tr( "Exclude Rules" ) );

    connect( _ui->applyButton, &QPushButton::clicked,
	     this,             &ConfigDialog::applyChanges );
}


ConfigDialog::~ConfigDialog()
{
    // logDebug() << "ConfigDialog destructor" << Qt::endl;
    delete _ui;
}


ConfigDialog * ConfigDialog::sharedInstance( QWidget * parent )
{
    static QPointer<ConfigDialog> _sharedInstance = nullptr;

    if ( !_sharedInstance )
    {
	_sharedInstance = new ConfigDialog( parent );
	CHECK_NEW( _sharedInstance );
    }

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
