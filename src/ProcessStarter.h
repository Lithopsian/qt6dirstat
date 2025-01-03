/*
 *   File name: ProcessStarter.h
 *   Summary:   Utilities for external processes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef ProcessStarter_h
#define ProcessStarter_h

#include <QObject>
#include <QProcess>
#include <QVector>

#include "Logger.h"

namespace QDirStat
{
    /**
     * Class to manage starting a number of external processes, but limiting
     * the number of processes running in parallel. Whenever a process
     * finishes, the next one from the list is started.
     *
     * When all processes have been started, this class will delete itself.
     **/
    class ProcessStarter final : public QObject
    {
        Q_OBJECT

    public:

        /**
         * Constructor.
         **/
        ProcessStarter( int maxParallel, QObject * parent = nullptr ):
            QObject{ parent },
            _maxParallel{ maxParallel }
        {}

        /**
         * Add a process to the _waiting list. This class does not take
         * ownership of 'process', and it must be destroyed explicitly.
         **/
        void add( QProcess * process );

        /**
         * Notification that no more processes will be submitted to this
         * ProcessStarter.  It will delete itself once all the currently-queued
         * processes have been started (but not necessarily finished).
         **/
        void noMoreProcesses();


    protected slots:

        /**
         * Notification that a process has finished.
         **/
        void processFinished( int, QProcess::ExitStatus );


    protected:

        /**
         * Start more processes until _maxParallel processes are running.
         **/
        void startProcesses();


    private:

        int  _maxParallel;
        bool _autoDelete{ false};

        QVector<QProcess *> _running;
        QVector<QProcess *> _waiting;

    };  // class ProcessStarter

}       // namespace QDDirStat

#endif  // ProcessStarter_h
