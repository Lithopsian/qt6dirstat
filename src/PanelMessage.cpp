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


PanelMessage * PanelMessage::createMsg( QWidget * parent, QVBoxLayout * vBox )
{
    CHECK_PTR( vBox );

    PanelMessage * msg = new PanelMessage{ parent };
    vBox->addWidget( msg );

    return msg;
}


void PanelMessage::showPermissionsMsg( QWidget * parent, QVBoxLayout * vBox )
{
    static QPointer<PanelMessage> permissionsMsg;
    if ( !permissionsMsg )
    {
	permissionsMsg = createMsg( parent, vBox );
	permissionsMsg->_ui->stackedWidget->setCurrentWidget( permissionsMsg->_ui->permissionsPage );

	connect( permissionsMsg->_ui->detailsLinkLabel, &QLabel::linkActivated,
		 qobject_cast<MainWindow *>( parent ),  &MainWindow::showUnreadableDirs );
    }
}


void PanelMessage::showFilesystemsMsg( QWidget * parent, QVBoxLayout * vBox )
{
    static QPointer<PanelMessage> filesystemsMsg;
    if ( !filesystemsMsg )
    {
	filesystemsMsg = createMsg( parent, vBox );
	filesystemsMsg->_ui->stackedWidget->setCurrentWidget( filesystemsMsg->_ui->filesystemsPage );
    }
}


void PanelMessage::showRpmMsg( QWidget * parent, QVBoxLayout * vBox )
{
    static QPointer<PanelMessage> rpmMsg;
    if ( !rpmMsg )
    {
	rpmMsg = createMsg( parent, vBox );
	rpmMsg->_ui->stackedWidget->setCurrentWidget( rpmMsg->_ui->rpmPage );
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
