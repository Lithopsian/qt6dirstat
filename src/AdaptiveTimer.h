/*
 *   File name: AdaptiveTimer.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef AdaptiveTimer_h
#define AdaptiveTimer_h

#include <QElapsedTimer>
#include <QList>
#include <QTimer>


namespace QDirStat
{
    /**
     * Timer for delivering signals that each obsolete each previous one, for
     * example for an expensive blocking operation to retrieve the contents of
     * a widget.
     *
     * Infrequent requests can be delivered on a zero timer, as soon as the Qt
     * event loop processes them.
     *
     * When requests are made more quickly, a delay can be introduced before a
     * request is delivered for execution, and previous requests still waiting
     * will be discarded as they are now obsolete.
     *
     * The delay in delivering a request is configurable as a multiple of the
     * time that it took the previous request to complete.  This avoids pointlessly
     * penalising users with fast hardware or when lookups are coming from cache,
     * but reduces lockups with real lookups on slower hardware.
     **/
    class AdaptiveTimer: public QObject
    {
        Q_OBJECT

        typedef QList<float>          Delays;
        typedef QList<int>            Cooldowns;
        typedef std::function<void()> Payload;

    public:

        /**
         * Constructor supplying custom delays and cooldown periods.
         *
         * The delay stage will only escalate as many times as the number of delays
         * in the list.  The number of cooldowns will typically be the same as the
         * number of delays.  If there are more cooldowns than delays, the extra delays
         * will never be reached.  If there are fewer cooldowns, the extra delay stages
         * will get a cooldown period of zero, meaing they will effectively not be
         * reached.
         *
         * An empty cooldowns list effectively means only the first stage will ever
         * be used.  This will typically deliver the requests with a zero delay.  An
         * empty delays list means that a fixed default delay of 0 will be used.
         *
         * The delays will typically be longer for higher stages, while the cooldowns
         * will be shorter so that the longer delays are only used for repidly-repeated
         * requests while shorter delays will be used for less frequent requests.
         **/
        AdaptiveTimer( QObject * parent, Delays delays, Cooldowns cooldowns );

        /**
         * Return the current delay stage.
         **/
        int delayStage() const { return _delayStage; }

        /**
         * Incoming request with optional user-defined payload. The payload
         * will be delivered in the deliverRequest() signal.
         *
         * If requests arrive very rapidly, only the latest one will be
         * delivered, all others will be discarded.
         **/
        void request( Payload payload );


    protected:

        /**
         * Returns the number of milliseconds that should be used for the delay timer.
         * This is the value from the delays list corresponding to the current stage, or
         * a default value if the delays list is empty.
         **/
        int currentDelay() const;

        /**
         * Returns the cooldown period for the current stage.
         **/
        int cooldownPeriod() const;

        /**
         * Increase the delivery delay.
         **/
        void increaseDelay();

        /**
         * Decrease the delivery delay.
         **/
        void decreaseDelay();

        /**
         * Timeout for the delivery timer.
         **/
        void deliveryTimeout();


    private:

        // Data members

        Payload       _payload;

        QElapsedTimer _payloadStopwatch;
        qint64        _payloadTime      { 0 };

        int           _delayStage       { 0 };
        Delays        _delays;
        Cooldowns     _cooldowns;

        QTimer        _deliveryTimer;
        QTimer        _cooldownTimer;

    }; // class AdaptiveTimer

} // namespace QDirStat

#endif // AdaptiveTimer_h
