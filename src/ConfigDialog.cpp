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
    _ui { new Ui::ConfigDialog }
{
    CHECK_NEW( _ui );
    _ui->setupUi( this );

    _generalConfigPage = new GeneralConfigPage( this );
    CHECK_NEW( _generalConfigPage );
    _ui->pagesTabWidget->addTab( _generalConfigPage, tr( "General" ) );

    _mimeCategoryConfigPage = new MimeCategoryConfigPage( this );
    CHECK_NEW( _mimeCategoryConfigPage );
    _ui->pagesTabWidget->addTab( _mimeCategoryConfigPage, tr( "MIME Categories" ) );

    _cleanupConfigPage = new CleanupConfigPage( this );
    CHECK_NEW( _cleanupConfigPage );
    _ui->pagesTabWidget->addTab( _cleanupConfigPage, tr( "Cleanup Actions" ) );

    _excludeRulesConfigPage = new ExcludeRulesConfigPage( this );
    CHECK_NEW( _excludeRulesConfigPage );
    _ui->pagesTabWidget->addTab( _excludeRulesConfigPage, tr( "Exclude Rules" ) );

    connect( _ui->applyButton,	 &QPushButton::clicked,
	     this,		 &ConfigDialog::applyChanges );
}


ConfigDialog::~ConfigDialog()
{
    // logDebug() << "ConfigDialog destructor" << Qt::endl;
    delete _ui;
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
