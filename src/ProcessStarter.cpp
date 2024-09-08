/*
 *   File name: ProcessStarter.h
 *   Summary:   Utilities for external processes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "ProcessStarter.h"
#include "Exception.h"


using namespace QDirStat;


void ProcessStarter::start()
{
//    logDebug() << "Starting. Processes in queue: " << _waiting.count() << Qt::endl;
//    logDebug() << "Maximum parallel processes: " << _maxParallel << Qt::endl;

    _started = true;
    startProcesses();
}


void ProcessStarter::add( QProcess * process )
{
    CHECK_PTR( process );

    _waiting.append( process );

    connect( process, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ),
             this,    &ProcessStarter::processFinished );

    if ( _started )
        startProcesses();
}


void ProcessStarter::startProcesses()
{
    while ( _running.size() < _maxParallel && !_waiting.isEmpty() )
    {
        QProcess * process = _waiting.takeFirst();
        if ( process )
        {
            process->start();
            _running.append( process );
        }
    }
}


void ProcessStarter::processFinished( int, QProcess::ExitStatus )
{
    QProcess * process = qobject_cast<QProcess *>( sender() );
    if ( !process )
    {
        logError() << "Ignoring non-process QObject " << (void *) sender() << Qt::endl;
        return;
    }

    _running.removeAll( process );
    _waiting.removeAll( process ); // It shouldn't be in _waiting; just making sure

    if ( _started )
    {
        if ( !_waiting.isEmpty() )
            startProcesses();
        else if ( _autoDelete )
            deleteLater();
    }
}
