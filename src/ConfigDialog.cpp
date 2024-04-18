/*
 *   File name: ConfigDialog.cpp
 *   Summary:   QDirStat configuration dialog classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
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
    _ui { new Ui::ConfigDialog }
{
    setAttribute(Qt::WA_DeleteOnClose);

    CHECK_NEW( _ui );
    _ui->setupUi( this );

    GeneralConfigPage * generalConfigPage = new GeneralConfigPage( this );
    CHECK_NEW( generalConfigPage );
    _ui->pagesTabWidget->addTab( generalConfigPage, tr( "General" ) );

    MimeCategoryConfigPage * mimeCategoryConfigPage = new MimeCategoryConfigPage( this );
    CHECK_NEW( mimeCategoryConfigPage );
    _ui->pagesTabWidget->addTab( mimeCategoryConfigPage, tr( "MIME Categories" ) );

    CleanupConfigPage * cleanupConfigPage = new CleanupConfigPage( this );
    CHECK_NEW( cleanupConfigPage );
    _ui->pagesTabWidget->addTab( cleanupConfigPage, tr( "Cleanup Actions" ) );

    ExcludeRulesConfigPage * excludeRulesConfigPage = new ExcludeRulesConfigPage( this );
    CHECK_NEW( excludeRulesConfigPage );
    _ui->pagesTabWidget->addTab( excludeRulesConfigPage, tr( "Exclude Rules" ) );

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
