/*
 *   File name: OutputWindow.cpp
 *   Summary:	Terminal-like window to watch output of an external process
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <QApplication>
#include <QCloseEvent>
#include <QTimer>

#include "OutputWindow.h"
#include "Settings.h"
#include "SettingsHelpers.h"
#include "Logger.h"
#include "Exception.h"

using QDirStat::readColorEntry;
using QDirStat::writeColorEntry;
using QDirStat::readFontEntry;
using QDirStat::writeFontEntry;


OutputWindow::OutputWindow( QWidget * parent, bool autoClose ):
    QDialog ( parent ),
    _ui { new Ui::OutputWindow }
{
    CHECK_NEW( _ui );
    _ui->setupUi( this );

    //logDebug() << "Creating" << Qt::endl;
    readSettings();

    _ui->terminal->clear();
    setAutoClose( autoClose );

    connect( _ui->actionZoomIn,       &QAction::triggered,
             this,                    &OutputWindow::zoomIn );

    connect( _ui->actionZoomOut,      &QAction::triggered,
             this,                    &OutputWindow::zoomOut );

    connect( _ui->actionResetZoom,    &QAction::triggered,
             this,                    &OutputWindow::resetZoom );

    connect( _ui->actionKillProcess,  &QAction::triggered,
             this,                    &OutputWindow::killAll );

    updateActions();
}


OutputWindow::~OutputWindow()
{
    //logDebug() << "Destructor" << Qt::endl;

    if ( !_processList.isEmpty() )
    {
	logWarning() << _processList.size() << " processes left over" << Qt::endl;

	for ( const QProcess * process : _processList )
	    logWarning() << "Left over: " << process << Qt::endl;

	qDeleteAll( _processList );
    }

//    writeSettings();  // nothing can be changed while the program is running
    delete _ui;
}


void OutputWindow::addProcess( QProcess * process )
{
    CHECK_PTR( process );

    if ( _killedAll )
    {
	logInfo() << "User killed all processes - "
                  << "no longer accepting new processes" << Qt::endl;
	process->kill();
	process->deleteLater();
    }

    _processList << process;
    // logDebug() << "Adding " << process << Qt::endl;

    connect( process, &QProcess::readyReadStandardOutput,
	     this,    &OutputWindow::readStdout );

    connect( process, &QProcess::readyReadStandardError,
	     this,    &OutputWindow::readStderr );

#if (QT_VERSION < QT_VERSION_CHECK( 5, 6, 0 ))
    connect( process, qOverload<QProcess::ProcessError >( &QProcess::error ),
	     this,    &OutputWindow::processError );
#else
    connect( process, &QProcess::errorOccurred,
	     this,    &OutputWindow::processError );
#endif
    connect( process, qOverload<int, QProcess::ExitStatus>( &QProcess::finished ),
	     this,    &OutputWindow::processFinishedSlot );

    if ( !hasActiveProcess() )
	startNextProcess();
}


void OutputWindow::addCommandLine( const QString & commandline )
{
    addText( commandline, _commandTextColor );
}


void OutputWindow::addStdout( const QString & output )
{
    addText( output, _stdoutColor );
}


void OutputWindow::addStderr( const QString & output )
{
    _errorCount++;
    addText( output, _stderrColor );
    logWarning() << output << ( output.endsWith( "\n" ) ? "" : "\n" );

    if ( _showOnStderr && !isVisible() && !_closed )
	show();
}


void OutputWindow::addText( const QString & rawText, const QColor & textColor )
{
    if ( rawText.isEmpty() )
	return;

    _ui->terminal->moveCursor( QTextCursor::End );
    QTextCursor cursor( _ui->terminal->textCursor() );

    QTextCharFormat format;
    format.setForeground( QBrush( textColor ) );
    cursor.setCharFormat( format );
    cursor.insertText( rawText.endsWith( "\n" ) ? rawText : rawText + "\n" );
}


void OutputWindow::clearOutput()
{
    _ui->terminal->clear();
}


QProcess * OutputWindow::senderProcess( const char * function ) const
{
    QProcess * process = qobject_cast<QProcess *>( sender() );
    if ( !process )
    {
	if ( sender() )
	{
	    logError() << "Expecting QProcess as sender() in " << function
		       <<" , got "
		       << sender()->metaObject()->className() << Qt::endl;
	}
	else
	{
	    logError() << "NULL sender() in " << function << Qt::endl;
	}
    }

    return process;
}


void OutputWindow::readStdout()
{
    QProcess * process = senderProcess( __FUNCTION__ );
    if ( process )
	addStdout( QString::fromUtf8( process->readAllStandardOutput() ) );
}


void OutputWindow::readStderr()
{
    QProcess * process = senderProcess( __FUNCTION__ );
    if ( process )
	addStderr( QString::fromUtf8( process->readAllStandardError() ) );
}


void OutputWindow::processFinishedSlot( int exitCode, QProcess::ExitStatus exitStatus )
{
    switch ( exitStatus )
    {
	case QProcess::NormalExit:
	    //logDebug() << "Process finished normally." << Qt::endl;
	    addCommandLine( tr( "Process finished." ) );
	    break;

	case QProcess::CrashExit:

	    if ( exitCode == 0 )
	    {
		// Don't report an exit code of 0: Since we are starting all
		// processes with a shell, that exit code would be the exit
		// code of the shell; that would only be useful if the shell
		// crashed or could not be started.

		logError() << "Process crashed." << Qt::endl;
		addStderr( tr( "Process crashed." ) );
	    }
	    else
	    {
		logError() << "Process crashed. Exit code: " << exitCode << Qt::endl;
		addStderr( tr( "Process crashed. Exit code: %1" ).arg( exitCode ) );
	    }
	    break;
    }

    QProcess * process = senderProcess( __FUNCTION__ );
    if ( process )
    {
	processFinished( process );
	closeIfDone();
    }

    startNextProcess(); // this also calls updateActions()
}


void OutputWindow::processError( QProcess::ProcessError error )
{
    QString msg;

    switch ( error )
    {
	case QProcess::FailedToStart:
	    msg = tr( "Error: Process failed to start." );
	    break;

	case QProcess::Crashed: // Already reported via processFinished()
	    break;

	case QProcess::Timedout:
	    msg = tr( "Error: Process timed out." );
	    break;

	case QProcess::ReadError:
	    msg = tr( "Error reading output from the process." );
	    break;

	case QProcess::WriteError:
	    msg = tr( "Error writing data to the process." );
	    break;

	case QProcess::UnknownError:
	    msg = tr( "Unknown error." );
	    break;
    }

    if ( !msg.isEmpty() )
    {
	logError() << msg << Qt::endl;
	addStderr( msg );
    }

    QProcess * process = senderProcess( __FUNCTION__ );
    if ( process )
	processFinished( process );

    startNextProcess(); // this also calls updateActions()

    if ( !_showOnStderr && !isVisible() )
	closeIfDone();
}


void OutputWindow::processFinished( QProcess * process )
{
    _processList.removeAll( process );

    if ( _processList.isEmpty() && _noMoreProcesses )
    {
	//logDebug() << "Emitting lastProcessFinished() err: " << _errorCount << Qt::endl;
	emit lastProcessFinished( _errorCount );
    }

    process->deleteLater();
}


void OutputWindow::closeIfDone()
{
    if ( _processList.isEmpty() && _noMoreProcesses )
    {
	if ( ( autoClose() || _closed || !isVisible() ) && _errorCount == 0 )
	{
	    //logDebug() << "No more processes to watch. Auto-closing." << Qt::endl;
	    this->deleteLater(); // It is safe to call this multiple times
	}
    }
}


void OutputWindow::noMoreProcesses()
{
    _noMoreProcesses = true;

    if ( _processList.isEmpty() )
    {
	//logDebug() << "Emitting lastProcessFinished() err: " << _errorCount << Qt::endl;
	emit lastProcessFinished( _errorCount );
    }

    closeIfDone();
}


void OutputWindow::zoom( qreal factor )
{
    QFont font = _ui->terminal->font();
    if ( font.pixelSize() != -1 )
    {
	int pixelSize = font.pixelSize() * factor;
	if ( pixelSize == font.pixelSize() ) // rounding (always down) to the same value
	    ++pixelSize;
	font.setPixelSize( pixelSize );
    }
    else
    {
	const qreal oldPointSize = font.pointSizeF();
	font.setPointSizeF( oldPointSize * factor );
	if ( font.pointSizeF() == oldPointSize ) // some platforms may round (down) to integer sizes
	    font.setPointSizeF( font.pointSizeF() + 1 );
    }

    _ui->terminal->setFont( font );
}


void OutputWindow::zoomIn()
{
    zoom( 1.1 );
}


void OutputWindow::zoomOut()
{
    zoom( 1.0/1.1 );
}


void OutputWindow::resetZoom()
{
    //logDebug() << "Resetting font to normal" << Qt::endl;
    _ui->terminal->setFont( _terminalDefaultFont );
}


void OutputWindow::killAll()
{
    int killCount = 0;

    for ( QProcess * process : _processList )
    {
	logInfo() << "Killing process " << process << Qt::endl;
	process->kill();
	_processList.removeAll( process );
	process->deleteLater();
	++killCount;
    }

    _killedAll = true;
    addCommandLine( killCount == 1 ? tr( "Process killed." ) : tr( "Killed %1 processes." ).arg( killCount ) );
}


#if 0
void OutputWindow::setTerminalBackground( const QColor & newColor )
{
    // TO DO
    // TO DO
    // TO DO
}
#endif


bool OutputWindow::hasActiveProcess() const
{
    for ( const QProcess * process : _processList )
    {
	if ( process->state() == QProcess::Starting ||
	     process->state() == QProcess::Running )
	{
	    return true;
	}
    }

    return false;
}


QProcess * OutputWindow::pickQueuedProcess()
{
    for ( QProcess * process : _processList )
    {
	if ( process->state() == QProcess::NotRunning )
	    return process;
    }

    return nullptr;
}


QProcess * OutputWindow::startNextProcess()
{
    QProcess * process = pickQueuedProcess();

    if ( process )
    {
	const QString dir = process->workingDirectory();

	if ( dir != _lastWorkingDir )
	{
	    addCommandLine( "cd " + dir );
	    _lastWorkingDir = dir;
	}

	addCommandLine( command( process ) );
	logInfo() << "Starting: " << process << Qt::endl;

	process->start();
	qApp->processEvents(); // Keep GUI responsive
    }

    updateActions();

    return process;
}


QString OutputWindow::command( QProcess * process )
{
    // The common case is to start an external command with
    //	  /bin/sh -c theRealCommand arg1 arg2 arg3 ...
    QStringList args = process->arguments();

    if ( !args.isEmpty() )
	args.removeFirst();		// Remove the "-c"

    if ( args.isEmpty() )		// Nothing left?
	return process->program();	// Ok, use the program name
    else
	return args.join( " " );	// output only the real command and its args
}


void OutputWindow::closeEvent( QCloseEvent * event )
{
    _closed = true;

    if ( _processList.isEmpty() && _noMoreProcesses )
	this->deleteLater();

    // If there are any more processes, wait until the last one is finished and
    // then deleteLater().

    event->accept();
}


void OutputWindow::updateActions()
{
    _ui->killButton->setEnabled( hasActiveProcess() );
}


void OutputWindow::showAfterTimeout( int timeoutMillisec )
{
    if ( timeoutMillisec <= 0 )
	timeoutMillisec = defaultShowTimeout();

    QTimer::singleShot( timeoutMillisec, this, &OutputWindow::timeoutShow );
}


void OutputWindow::timeoutShow()
{
    if ( !isVisible() && !_closed )
	show();
}


void OutputWindow::readSettings()
{
    QDirStat::Settings settings;
    settings.beginGroup( "OutputWindow" );

    _terminalBackground  = readColorEntry( settings, "TerminalBackground", QColor( Qt::black        ) );
    _commandTextColor    = readColorEntry( settings, "CommandTextColor"  , QColor( Qt::white        ) );
    _stdoutColor         = readColorEntry( settings, "StdoutTextColor"   , QColor( 0xff, 0xaa, 0x00 ) );
    _stderrColor         = readColorEntry( settings, "StdErrTextColor"   , QColor( Qt::red          ) );
    _terminalDefaultFont = readFontEntry ( settings, "TerminalFont"      , _ui->terminal->font()      );

    settings.endGroup();

    _ui->terminal->setFont( _terminalDefaultFont );
}

/*
void OutputWindow::writeSettings()
{
    QDirStat::Settings settings;
    settings.beginGroup( "OutputWindow" );

    writeColorEntry( settings, "TerminalBackground", _terminalBackground  );
    writeColorEntry( settings, "CommandTextColor"  , _commandTextColor    );
    writeColorEntry( settings, "StdoutTextColor"   , _stdoutColor         );
    writeColorEntry( settings, "StdErrTextColor"   , _stderrColor         );
    writeFontEntry ( settings, "TerminalFont"      , _terminalDefaultFont );
//    settings.setValue( "DefaultShowTimeoutMillisec", defaultShowTimeout() );

    settings.endGroup();
}
*/

int OutputWindow::defaultShowTimeout()
{
    QDirStat::Settings settings;

    settings.beginGroup( "OutputWindow" );
    int defaultShowTimeout = settings.value( "DefaultShowTimeoutMillisec", 500 ).toInt();
    settings.endGroup();

    return defaultShowTimeout;
}
