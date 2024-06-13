/*
 *   File name: AdaptiveTimer.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "AdaptiveTimer.h"
#include "Logger.h"


#define VERBOSE_DELAY 0


using namespace QDirStat;


AdaptiveTimer::AdaptiveTimer( QObject * parent, Delays delays, Cooldowns cooldowns ):
    QObject ( parent ),
    _delays { delays },
    _cooldowns { cooldowns }
{
    connect( &_deliveryTimer, &QTimer::timeout,
             this,            &AdaptiveTimer::deliveryTimeout );

    connect( &_cooldownTimer, &QTimer::timeout,
             this,            &AdaptiveTimer::decreaseDelay );

    _deliveryTimer.setSingleShot( true );
    _cooldownTimer.setSingleShot( true );
}


void AdaptiveTimer::request( Payload payload )
{
    //logDebug() << "Received request" << Qt::endl;
    _payload = payload;

    if ( _cooldownTimer.isActive() )
        increaseDelay();

    _deliveryTimer.start( currentDelay() );
}


void AdaptiveTimer::deliveryTimeout()
{
    //logDebug() << "Delivering request" << Qt::endl;

    _payloadStopwatch.start();
    _payload();

    // Average the payload time to smooth out any stray spikes
    _payloadTime = ( _payloadTime + _payloadStopwatch.elapsed() ) / 2;

    //logDebug() << "deliveryTime=" << _deliveryTime << Qt::endl;

    _cooldownTimer.start( cooldownPeriod() );
}


int AdaptiveTimer::currentDelay() const
{
    if ( _delays.isEmpty() )
        return _defaultDelay;

    return _payloadTime * _delays[ _delayStage ];
}


int AdaptiveTimer::cooldownPeriod() const
{
    if ( _cooldowns.isEmpty() )
        return 0;

    // Might be more delays than cooldowns
    const int cooldownStage = qMin( _cooldowns.size() - 1, _delayStage );
    return _cooldowns[ cooldownStage ];
}


void AdaptiveTimer::increaseDelay()
{
    if ( _delayStage >= _delays.size() - 1 )
        return;

    ++_delayStage;

#if VERBOSE_DELAY
    logDebug() << "Increasing delay to stage " << _delayStage
               << ": " << currentDelay() << " millisec"
               << Qt::endl;
#endif
}


void AdaptiveTimer::decreaseDelay()
{
    if ( _delayStage == 0 )
        return;

    --_delayStage;

#if VERBOSE_DELAY
    logDebug() << "Decreasing delay to stage " << _delayStage
               << ": " << currentDelay() << " millisec"
               << Qt::endl;
#endif

    // continue to cool down even if there are no further requests
    _cooldownTimer.start( cooldownPeriod() );
}
