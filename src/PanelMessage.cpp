/*
 *   File name: PanelMessage.cpp
 *   Summary:   Message in a panel with icon and close button
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QPointer>

#include "PanelMessage.h"
#include "Exception.h"
#include "MainWindow.h"
#include "SysUtil.h"


using namespace QDirStat;


PanelMessage::PanelMessage():
    QWidget{},
    _ui{ new Ui::PanelMessage }
{
    _ui->setupUi( this );
}


PanelMessage * PanelMessage::createMsg( QVBoxLayout * vBox )
{
    CHECK_PTR( vBox );

    PanelMessage * msg = new PanelMessage{};
    vBox->addWidget( msg );

    return msg;
}


void PanelMessage::showPermissionsMsg( MainWindow * mainWin, QVBoxLayout * vBox )
{
    static QPointer<PanelMessage> permissionsMsg;
    if ( !permissionsMsg )
    {
	permissionsMsg = createMsg( vBox );
	permissionsMsg->_ui->stackedWidget->setCurrentWidget( permissionsMsg->_ui->permissionsPage );

	connect( permissionsMsg->_ui->detailsLinkLabel, &QLabel::linkActivated,
	         mainWin,                               &MainWindow::showUnreadableDirs );

	connect( permissionsMsg->_ui->closeButton,      &QAbstractButton::clicked,
	         mainWin,                               &MainWindow::focusDirTree );
    }
}


void PanelMessage::showFilesystemsMsg( QVBoxLayout * vBox )
{
    static QPointer<PanelMessage> filesystemsMsg;
    if ( !filesystemsMsg )
    {
	filesystemsMsg = createMsg( vBox );
	filesystemsMsg->_ui->stackedWidget->setCurrentWidget( filesystemsMsg->_ui->filesystemsPage );
    }
}


void PanelMessage::showRpmMsg( MainWindow * mainWin, QVBoxLayout * vBox )
{
    static QPointer<PanelMessage> rpmMsg;
    if ( !rpmMsg )
    {
	rpmMsg = createMsg( vBox );
	rpmMsg->_ui->stackedWidget->setCurrentWidget( rpmMsg->_ui->rpmPage );

	connect( rpmMsg->_ui->closeButton, &QAbstractButton::clicked,
	         mainWin,                  &MainWindow::focusDirTree );
    }
}


void PanelMessage::deletePermissionsMsg( const QWidget * parent )
{
    const auto children = parent->findChildren<PanelMessage *>();
    for ( PanelMessage * msg : children )
    {
	if ( msg->_ui->stackedWidget->currentWidget() == msg->_ui->permissionsPage )
	    delete msg;
    }
}
