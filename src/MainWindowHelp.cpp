/*
 *   File name: MainWindowHelp.cpp
 *   Summary:   Help menu actions in the QDirStat main window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QDesktopServices>
#include <QMessageBox>

#include "MainWindow.h"
#include "Version.h"
#include "Exception.h"
#include "Logger.h"


using namespace QDirStat;


void MainWindow::showAboutDialog()
{
    const QString text = QString( "<h2>Qt6DirStat %1</h2>" ).arg( QDIRSTAT_VERSION ) +
        "<p>" +
        tr( "Qt-based directory statistics -- showing where all your disk space has gone "
            "and trying to help you to clean it up." ) +
        "</p><p>" +
        "(c) 2015-2024 Stefan Hundhammer and 2023-2024 Ian Nartowicz" +
        "</p><p>" +
        tr( "License: GPL V2 (GNU General Public License Version 2)" ) +
        "</p><p>" +
        tr( "This is free Open Source software, provided to you hoping that it might be "
            "useful for you. It does not cost you anything, but on the other hand there "
            "is no warranty or promise of anything." ) +
        "</p><p>" +
        tr( "This software was made with the best intentions and greatest care, but still "
            "there is the off chance that something might go wrong which might damage "
            "data on your computer. Under no circumstances will the authors of this program "
            "be held responsible for anything like that. Use this program at your own risk." ) +
        "</p>";

    QMessageBox::about( this, tr( "About Qt6DirStat" ), text );
}


void MainWindow::showDonateDialog()
{
    const QString url = "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=EYJXAVLGNRR5W";

    const QString text = QString( "<h2>Donate</h2>" ) + \
        "<p><nobr>" + \
        tr( "QDirStat is Free Open Source Software." ) + \
        "</nobr></p><p><nobr>" + \
        tr( "If you find it useful, please consider donating." ) + \
        "</nobr>\n<nobr>" + \
        tr( "You can donate any amount of your choice:" ) + \
        "</nobr></p><p>" + \
        QString( "<a href=\"%1\">QDirStat at PayPal</a>" ).arg( url ) + \
        "</p><p>" + \
        tr( "(opens in external browser window)" ) + \
        "</p>";


    QMessageBox::about( this, tr( "Donate" ), text );
}


void MainWindow::showAboutQtDialog()
{
    QApplication::aboutQt();
}


void MainWindow::openActionUrl()
{
    // Use a QAction that was set up in Qt Designer to just open an URL in an
    // external web browser.  The url is stored in the status tip, and so
    // it also appears in the status bar.
    QAction * action = qobject_cast<QAction *>( sender() );
    if ( !action )
	return;

    const QString url = action->statusTip();
    if ( url.isEmpty() )
	logError() << "No URL in statusTip() for action " << action->objectName() << Qt::endl;
    else
	QDesktopServices::openUrl( url );
//	SysUtil::openInBrowser( url );
}
