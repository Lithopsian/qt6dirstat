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


namespace QDirStat
{
    /**
     * Class to manage starting a number of external processes, but limiting
     * the number of processes running in parallel. Whenever a process
     * finishes, the next one from the list is started.
     *
     * When all processes are started and the 'autoDelete' flag is set, this
     * class will delete itself.
     **/
    class ProcessStarter final : public QObject
    {
        Q_OBJECT

    public:

        /**
         * Constructor.
         **/
        ProcessStarter( int maxParallel, bool autoDelete, QObject * parent = nullptr ):
            QObject{ parent },
            _maxParallel{ maxParallel },
            _autoDelete{ autoDelete }
        {}

        /**
         * Add a process to the _waiting list. This class does not take
         * ownership of 'process'.
         **/
        void add( QProcess * process );

        /**
         * Begin starting processes.
         *
         * If 'autoDelete' is set, make sure to call this AFTER all processes
         * are added; otherwise this object might be deleted already when the
         * first processes finish very quickly.
         **/
        void start();


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
        bool _autoDelete;
        bool _started{ false };

        QVector<QProcess *> _running;
        QVector<QProcess *> _waiting;

    };  // class ProcessStarter

}       // namespace QDDirStat

#endif  // ProcessStarter_h
