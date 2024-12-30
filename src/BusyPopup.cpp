/*
 *   File name: BusyPopup.cpp
 *   Summary:   QDirStat generic widget classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QEventLoop>
#include <QTimer>

#include "BusyPopup.h"
#include "Logger.h"
#include "MainWindow.h"
#include "QDirStatApp.h"


using namespace QDirStat;


namespace
{
    /**
     * Process events until the label has been painted.  This will block
     * the main event loop, so exit after a second even if the label hasn't
     * been painted, just in case it is hidden or otherwise never painted.
     **/
    void processEvents( const BusyPopup * busyPopup )
    {
	QEventLoop eventLoop;
	QObject::connect( busyPopup, &BusyPopup::painted, &eventLoop, &QEventLoop::quit );
	QTimer::singleShot( 1000, &eventLoop, &QEventLoop::quit );
	eventLoop.exec();
	eventLoop.processEvents(); // make sure the main window gets reset to blank panels
    }

} // namespace


BusyPopup::BusyPopup( const QString & text ):
    QLabel{ text, app()->mainWindow(), Qt::SplashScreen }
{
    setMargin( 16 );
    setWindowTitle( QString{} );
    show();
    processEvents( this );
}


bool BusyPopup::event( QEvent * event )
{
    switch ( event->type() )
    {
	case QEvent::Paint:
	    // Signal to exit the BusyPopup event loop
	    emit painted();
	    break;

	case QEvent::Show:
	    // Try to put the popup in the middle of the main window
	    if ( parentWidget() )
	    {
		const int x = ( parentWidget()->width()  - width()  ) / 2;
		const int y = ( parentWidget()->height() - height() ) / 2;

		move( parentWidget()->x() + x, parentWidget()->y() + y );
	    }
	    break;

	default:
	    break;
    }

    return QLabel::event( event );
}
