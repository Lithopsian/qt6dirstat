/*
 *   File name: OutputWindow.cpp
 *   Summary:   Terminal-like window to watch output of an external process
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QTimer>
#include <QtMath> // qFloor()

#include "OutputWindow.h"
#include "ActionManager.h"
#include "Exception.h"
#include "Settings.h"


using namespace QDirStat;


namespace
{
    /**
     * Add one or more lines of text in text color 'textColor' to 'terminal'.
     **/
    void addText( QTextEdit * terminal, const QString & rawText, const QColor & textColor )
    {
	if ( rawText.isEmpty() )
	    return;

	terminal->moveCursor( QTextCursor::End );
	QTextCursor cursor{ terminal->textCursor() };

	QTextCharFormat format;
	format.setForeground( QBrush{ textColor } );
	cursor.setCharFormat( format );

	cursor.insertText( rawText.endsWith( u'\n' ) ? rawText : rawText % '\n' );
    }


    /**
     * Zoom the 'terminal' font by the specified factor.  The operation is
     * performed in pixels from QFontInfo, with a minimum of 1 pixel.
     **/
    void zoom( QTextEdit * terminal, qreal factor )
    {
	QFont font = terminal->font();
	QFontInfo fontInfo{ font };

	const int oldPixelSize = fontInfo.pixelSize();
	const int newPixelSize = qFloor( oldPixelSize * factor );
	font.setPixelSize( newPixelSize == oldPixelSize ? newPixelSize + 1 : qMax( newPixelSize, 1 ) );

	terminal->setFont( font );
    }

}


OutputWindow::OutputWindow( QWidget * parent, bool autoClose ):
    QDialog{ parent },
    _ui{ new Ui::OutputWindow }
{
    _ui->setupUi( this );
    _ui->actionZoomIn->setShortcuts( QKeySequence::ZoomIn );
    _ui->actionZoomOut->setShortcuts( QKeySequence::ZoomOut );
    _ui->actionResetZoom->setShortcut( Qt::CTRL | Qt::Key_0 );

    //logDebug() << "Creating with parent " << parent << Qt::endl;
    readSettings();

    setAutoClose( autoClose );
    clearOutput();

    connect( _ui->zoomInButton,    &QPushButton::clicked,
             this,                 &OutputWindow::zoomIn );

    connect( _ui->zoomOutButton,   &QPushButton::clicked,
             this,                 &OutputWindow::zoomOut );

    connect( _ui->resetZoomButton, &QPushButton::clicked,
             this,                 &OutputWindow::resetZoom );

    connect( _ui->killButton,      &QPushButton::clicked,
             this,                 &OutputWindow::killAll );

    updateActions();
}


OutputWindow::~OutputWindow()
{
    //logDebug() << "Destructor" << Qt::endl;

    Settings::writeWindowSettings( this, "OutputWindow" );

    if ( !_processList.isEmpty() )
    {
	logWarning() << _processList.size() << " entries still in process list" << Qt::endl;

	// Kill any active processes and destroy them all
	killAll();
    }
}


void OutputWindow::addProcess( QProcess * process )
{
    CHECK_PTR( process );

    if ( _killedAll )
    {
	logInfo() << "User killed all processes - no longer accepting new processes" << Qt::endl;
	process->kill();
	process->deleteLater();
    }

    // logDebug() << "Adding " << process << Qt::endl;
    _processList << process;

    connect( process, &QProcess::readyReadStandardOutput,
             this,    &OutputWindow::readStdout );

    connect( process, &QProcess::readyReadStandardError,
             this,    &OutputWindow::readStderr );

#if QT_VERSION < QT_VERSION_CHECK( 5, 6, 0 )
    connect( process, QOverload<QProcess::ProcessError>::of( &QProcess::error ),
             this,    &OutputWindow::processError );
#else
    connect( process, &QProcess::errorOccurred,
             this,    &OutputWindow::processError );
#endif

    connect( process, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ),
             this,    &OutputWindow::processFinishedSlot );

    if ( !hasActiveProcess() )
	startNextProcess();
}


void OutputWindow::addCommandLine( const QString & commandline )
{
    addText( _ui->terminal, commandline, _commandTextColor );
}


void OutputWindow::addStdout( const QString & output )
{
    addText( _ui->terminal, output, _stdoutColor );
}


void OutputWindow::addStderr( const QString & output )
{
    ++_errorCount;
    addText( _ui->terminal, output, _stderrColor );
    logWarning() << output.trimmed() << Qt::endl;

    if ( _showOnStderr && !isVisible() && !_closed )
	show();
}


void OutputWindow::readStdout()
{
    QProcess * process = senderProcess();
    if ( process )
	addStdout( QString{ process->readAllStandardOutput() } );
}


void OutputWindow::readStderr()
{
    QProcess * process = senderProcess();
    if ( process )
	addStderr( QString{ process->readAllStandardError() } );
}


void OutputWindow::processFinishedSlot( int exitCode, QProcess::ExitStatus exitStatus )
{
    // A crash exit status has already been handled and reported in errorOccurred()
    if ( exitStatus == QProcess::CrashExit )
	return;

    //logDebug() << "Process finished normally with exit code " << exitCode << Qt::endl;
    addCommandLine( tr( "Process finished with exit code %1." ).arg( exitCode ) );

    QProcess * process = senderProcess();
    if ( process )
    {
	processFinished( process );
	closeIfDone();
    }

    startNextProcess(); // this also calls updateActions()
}


void OutputWindow::processError( QProcess::ProcessError error )
{
    const QString msg = [ error ]()
    {
	switch ( error )
	{
	    case QProcess::FailedToStart: return tr( "Error: Process failed to start." );
	    case QProcess::Timedout:      return tr( "Error: Process timed out." );
	    case QProcess::ReadError:     return tr( "Error reading output from the process." );
	    case QProcess::WriteError:    return tr( "Error writing data to the process." );
	    case QProcess::UnknownError:  return tr( "Unknown error." );
	    case QProcess::Crashed:       return tr( "Crashed" );
	    default:                      return QString{};
	}
    }();

    if ( !msg.isEmpty() )
    {
	logError() << msg << Qt::endl;
	addStderr( msg );
    }

    QProcess * process = senderProcess();
    if ( process )
	processFinished( process );

    startNextProcess(); // this also calls updateActions()

    if ( !_showOnStderr && !isVisible() )
	closeIfDone();
}


void OutputWindow::processFinished( QProcess * process )
{
    _processList.removeAll( process );
    process->deleteLater();

    if ( _processList.isEmpty() && _noMoreProcesses )
    {
	//logDebug() << "Emitting lastProcessFinished() with error count: " << _errorCount << Qt::endl;
	emit lastProcessFinished( _errorCount );
    }
}


void OutputWindow::closeIfDone()
{
    if ( _processList.isEmpty() && _noMoreProcesses )
    {
	if ( ( autoClose() && _errorCount == 0 ) || _closed || !isVisible() )
	{
	    //logDebug() << "No more processes to watch. Auto-closing." << Qt::endl;
	    deleteLater(); // It is safe to call this multiple times
	}
    }
}


void OutputWindow::noMoreProcesses()
{
    _noMoreProcesses = true;

    if ( _processList.isEmpty() )
    {
	//logDebug() << "Emitting lastProcessFinished() with error count=" << _errorCount << Qt::endl;
	emit lastProcessFinished( _errorCount );
    }

    closeIfDone();
}


void OutputWindow::zoomIn()
{
    zoom( _ui->terminal, 1.1_qr );
}


void OutputWindow::zoomOut()
{
    zoom( _ui->terminal, 1.0_qr / 1.1_qr );
}


void OutputWindow::resetZoom()
{
    //logDebug() << "Resetting font to normal" << Qt::endl;
    _ui->terminal->setFont( _terminalDefaultFont );
}


void OutputWindow::killAll()
{
    const int killCount = _processList.size();

    for ( QProcess * process : asConst( _processList ) )
    {
	//logInfo() << "Killing process " << process << Qt::endl;
	process->kill();
	process->deleteLater();
    }

    _processList.clear();

    _killedAll = true;
    addCommandLine( killCount == 1 ? tr( "Process killed." ) : tr( "Killed %L1 processes." ).arg( killCount ) );
}


bool OutputWindow::hasActiveProcess() const
{
    for ( const QProcess * process : asConst( _processList ) )
    {
	QProcess::ProcessState state = process->state();
	if ( state == QProcess::Starting || state == QProcess::Running )
	    return true;
    }

    return false;
}


void OutputWindow::startNextProcess()
{
    QProcess * process = []( const ProcessList & processList ) -> QProcess *
    {
	for ( QProcess * process : processList )
	{
	    if ( process->state() == QProcess::NotRunning )
		return process;
	}

	return nullptr;
    }( _processList );

    if ( process )
    {
	const QString dir = process->workingDirectory();
	if ( dir != _lastWorkingDir )
	{
	    addCommandLine( "cd "_L1 % dir );
	    _lastWorkingDir = dir;
	}

	addCommandLine( command( process ) );
	logInfo() << "Starting: " << process << Qt::endl;

	process->start();
    }

    updateActions();
}


QString OutputWindow::command( const QProcess * process )
{
    // Detect the case where an external command is started in a shell:
    //	  /bin/sh -c theRealCommand arg1 arg2 arg3 ...
    QStringList args = process->arguments();
    if ( args.size() < 2 || args.first() != shellCommandArg() )
    {
	// Not a shell, just use the program name
	return process->program();
    }
    else
    {
	// Likely shell, output only the real command and its args
	args.removeFirst();
	return args.join( u' ' );
    }
}


void OutputWindow::showAfterTimeout( int timeoutMillisec )
{
    // Show immediately if there is an error
    _showOnStderr = true;

    // Show after the configured timeout if processes are still running
    const int millisec = timeoutMillisec > 0 ? timeoutMillisec : defaultShowTimeout();
    QTimer::singleShot( millisec, this, &OutputWindow::timeoutShow );
}


void OutputWindow::timeoutShow()
{
    if ( !isVisible() && !_closed )
	show();
}


void OutputWindow::readSettings()
{
    Settings settings;

    settings.beginGroup( "OutputWindow" );

    _terminalBackground  = settings.colorValue( "TerminalBackground", QColor{ Qt::black        } );
    _commandTextColor    = settings.colorValue( "CommandTextColor",   QColor{ Qt::white        } );
    _stdoutColor         = settings.colorValue( "StdoutTextColor",    QColor{ 0xff, 0xaa, 0x00 } );
    _stderrColor         = settings.colorValue( "StdErrTextColor",    QColor{ 0xff, 0x00, 0x55 } );
    _terminalDefaultFont = settings.fontValue ( "TerminalFont",       _ui->terminal->font()      );

    settings.setDefaultValue( "TerminalBackground", _terminalBackground  );
    settings.setDefaultValue( "CommandTextColor",   _commandTextColor    );
    settings.setDefaultValue( "StdoutTextColor",    _stdoutColor         );
    settings.setDefaultValue( "StdErrTextColor",    _stderrColor         );
    settings.setDefaultValue( "TerminalFont",       _terminalDefaultFont );

    settings.endGroup();

    QPalette newPalette{ _ui->terminal->palette() };
    newPalette.setBrush( QPalette::Base, _terminalBackground );
    _ui->terminal->setPalette( newPalette );
    _ui->terminal->setFont( _terminalDefaultFont );

    Settings::readWindowSettings( this, "OutputWindow" );
    ActionManager::actionHotkeys( this, "OutputWindow" );
}


int OutputWindow::defaultShowTimeout()
{
    Settings settings;

    settings.beginGroup( "OutputWindow" );
    int defaultShowTimeout = settings.value( "DefaultShowTimeoutMillisec", 500 ).toInt();
    settings.setDefaultValue( "DefaultShowTimeoutMillisec", defaultShowTimeout );
    settings.endGroup();

    return defaultShowTimeout;
}


void OutputWindow::hideEvent( QHideEvent * event )
{
    // Ignore iconification or placing in another workspace
    if ( event->spontaneous() )
	return;

    // Flag as "logically" closed
    _closed = true;

    // Wait until the last process is finished and then delete this window
    if ( _processList.isEmpty() && _noMoreProcesses )
	this->deleteLater();
}
