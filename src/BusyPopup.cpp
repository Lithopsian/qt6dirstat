/*
 *   File name: BusyPopup.cpp
 *   Summary:   QDirStat generic widget classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QEventLoop>

#include "BusyPopup.h"
#include "Logger.h"
#include "MainWindow.h"
#include "QDirStatApp.h"


#define PROCESS_EVENTS_MILLISEC 500


using namespace QDirStat;


namespace
{
    /**
     * Process events (except user input events) for the specified time.
     **/
    void processEvents()
    {
	QEventLoop{}.processEvents( QEventLoop::ExcludeUserInputEvents, PROCESS_EVENTS_MILLISEC );
    }

} // namespace


BusyPopup::BusyPopup( const QString & text ):
    QLabel{ text, app()->mainWindow(), Qt::SplashScreen }
{
    setMargin( 15 );
    setWindowTitle( QString{} );
    show();
    processEvents();
}


void BusyPopup::showEvent( QShowEvent * )
{
    if ( parentWidget() )
    {
	const int x = ( parentWidget()->width()  - width()  ) / 2;
	const int y = ( parentWidget()->height() - height() ) / 2;

	move( parentWidget()->x() + x, parentWidget()->y() + y );
    }
}
