/*
 *   File name: BusyPopup.cpp
 *   Summary:	QDirStat generic widget classes
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <QEventLoop>

#include "BusyPopup.h"
#include "Logger.h"


#define PROCESS_EVENTS_MILLISEC 500

using namespace QDirStat;


BusyPopup::BusyPopup( const QString & text,
		      QWidget       * parent,
		      bool            autoPost ):
    QLabel ( text, parent, Qt::SplashScreen )
{
    setMargin( 15 );
    setWindowTitle( " " );

    if ( autoPost )
	post();
}


void BusyPopup::post()
{
    if ( _posted )
	return;

    show();
    processEvents( PROCESS_EVENTS_MILLISEC );
    _posted = true;
}


void BusyPopup::processEvents( int millisec )
{
    QEventLoop eventLoop;
    eventLoop.processEvents( QEventLoop::ExcludeUserInputEvents, millisec );
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
