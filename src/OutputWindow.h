/*
 *   File name: OutputWindow.h
 *   Summary:   Terminal-like window to watch output of an external process
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef OutputWindow_h
#define OutputWindow_h

#include <memory>

#include <QDialog>
#include <QHideEvent>
#include <QProcess>
#include <QTextStream>
#include <QVector>

#include "ui_output-window.h"


namespace QDirStat
{
    /**
     * Terminal-like window to watch output of external processes started via
     * QProcess. The command invoked by the process, as well as its stdout and
     * stderr output are displayed in different colors.  An OutputWindow may
     * also be created and no processes added, as in moveToTrash().
     *
     * This class can watch more than one process: it can watch a sequence of
     * processes, such as cleanup actions as they are invoked for each selected
     * item one after another.
     *
     * This dialog can be configured to show immediately, after a timeout (but
     * only if there are still running processes), only if there is output on
     * stderr, or never.  If the dialog is configured to show after a timeout,
     * it will (by default) show itself immediately if there is output on
     * stderr, although this can be overridden to not show on error.
     **/
    class OutputWindow final : public QDialog
    {
        Q_OBJECT

        typedef QVector<QProcess *> ProcessList;


    public:

        /**
         * Constructor.  Initialises the dialog window, buttons, actions,
         * and settings.  The class is created with an empty process list.
         **/
        OutputWindow( QWidget * parent, bool autoClose );

        /**
         * Destructor.  Saves the window geometry, empties the process
         * queue, and forcibly kills any processes still running,
         **/
        ~OutputWindow() override;

        /**
         * Add a process to watch. Ownership of the process is transferred to
         * this object. If the process is not started yet, it will be started
         * as soon as there is no other process running.  Processes are started
         * in the ordet they are added.
         *
         * Note that starting or running processes may be added.  These will
         * continue to run, in parallel with any other already-running process,
         * but more processes will not be started by this class until there are
         * no longer any starting or running processes.
         **/
        void addProcess( QProcess * process );

        /**
         * Tell this dialog that no more processes will be added, so when the
         * last one is finished and the "auto close" checkbox is checked, it
         * may close itself.
         **/
        void noMoreProcesses();

        /**
         * Set if this dialog should show itself if there is any output on
         * stderr. This means an application can create the dialog and leave it
         * hidden, and if there is any error output, it will automatically show
         * itself -- with all previous output of the watched processes on stdout
         * and stderr.
         *
         * If the user explicitly closes an open dialog, it will remain closed
         * even if output appears on stderr afterwards, and even if this
         * setting is true.
         **/
        void setShowOnStderr( bool show ) { _showOnStderr = show; }

        /**
         * Show window (if not already shown) after the specified timeout has
         * elapsed. This is useful for operations that might be very short, so
         * no output window is desired, but that sometimes might also take a
         * long time.  If this is set, then the dialog will immediately show
         * if there is output on stderr.
         *
         * If 'timeoutMillisec' is 0, defaultShowTimeout() is used.
         **/
        void showAfterTimeout( int timeoutMillisec = 0 );

        /**
         * Return the default window show timeout in milliseconds.  This is
         * used by CleanupConfigPage as well as within this class.
         **/
        static int defaultShowTimeout();

        /**
         * Return the argument used with a shell command to indicate that
         * it should use the following arguments as input to the shell.
         **/
        static QLatin1String shellCommandArg() { return QLatin1String{ "-c" }; }

        /**
         * Get the command of 'process'. Since processes are usually started
         * via a shell ("/bin/sh -c theRealCommand arg1 arg2 ..."), this is
         * typically not QProcess::program(), but the arguments minus the "-c".
         *
         * Note that while it is possible in some shells that "-c" may not be
         * the first argument, or that arguments following "-c" may not be
         * part of the underlying program, the Cleanup class always constructs
         * processes with a first argument of "-c" and a second argument
         * containing the program and all of its arguments.
         **/
        static QString command( const QProcess * process );


    signals:

        /**
         * Emitted when the last process finished, whether that was successful
         * or with an error. 'totalErrorCount' is the accumulated error count
         * of all processes this OutputWindow watched.
         **/
        void lastProcessFinished( int totalErrorCount );


    public slots:

        /**
         * Add one or more lines of stdout to show in the output area. This
         * is typically displayed in amber, but the colour is configurable.
         **/
        void addStdout( const QString & output );

        /**
         * Add one or more lines of stderr to show in the output area. This
         * is typically displayed in red, but the colour is configurable.
         **/
        void addStderr( const QString & output );


    protected slots:

        /**
         * Kill all processes this class watches.
         **/
        void killAll();

        /**
         * Read output on one of the watched process's stdout channel.
         **/
        void readStdout();

        /**
         * Read output on one of the watched process's stderr channel.
         **/
        void readStderr();

        /**
         * One of the watched processes finished.
         **/
        void processFinishedSlot( int exitCode, QProcess::ExitStatus exitStatus );

        /**
         * One of the watched processes terminated with an error.
         **/
        void processError( QProcess::ProcessError error );

        /**
         * Zoom the output area in, i.e. make its font larger.
         **/
        void zoomIn();

        /**
         * Zoom the output area out, i.e. make its font smaller.
         **/
        void zoomOut();

        /**
         * Reset the output area zoom, i.e. restore its default font.
         **/
        void resetZoom();

        /**
         * Show after timeout has elapsed, unless the user closed this dialog
         * already.
         **/
        void timeoutShow();


    protected:

        /**
         * Read parameters from the settings.
         **/
        void readSettings();

        /**
         * Clear the output area, i.e. remove all previous output and commands.
         **/
        void clearOutput()
            { _ui->terminal->clear(); }

        /**
         * Set the auto-close checkbox to the given state.
         **/
        void setAutoClose( bool autoClose )
            { _ui->autoCloseCheckBox->setChecked( autoClose ); }

        /**
         * Enable or disable actions based on the internal status of this
         * object.
         **/
        void updateActions()
            { _ui->killButton->setEnabled( hasActiveProcess() ); }

        /**
         * Return 'true' if any process in the internal process is still
         * active.
         **/
        bool hasActiveProcess() const;

        /**
         * Remove a finished process and signal it is done.
         **/
        void processFinished( QProcess * process );

        /**
         * Close if there are no more processes and there is no error to show.
         **/
        void closeIfDone();

        /**
         * Obtain the process to use from sender(). Return 0 if this is not a
         * QProcess.
         **/
        QProcess * senderProcess( const char * callingFunctionName ) const;

        /**
         * Try to start the next inactive process, if there is any.
         **/
        void startNextProcess();

        /**
         * Return whether the auto-close checkbox is checked.
         **/
        bool autoClose() const { return _ui->autoCloseCheckBox->isChecked(); }

        /**
         * Add a command line to show in the output area.  This is typically
         * displayed in white, but the colour is configurable.
         **/
        void addCommandLine( const QString & commandline );

        /**
         * Hide event: invoked upon QDialog::close() (i.e. the "Close" button),
         * the window manager close button (the [x] at the top right), when
         * this dialog decides to auto-close itself after the last process
         * finishes successfully, or when the dialog is hidden
         * "non-spontaneously".  Before Qt 6.3, reject(), for example by
         * pressing the Escape key, hides the window but does not close it.
         *
         * This object will delete itself in this event if there are no more
         * processes to watch, or set a flag that the dialog can be deleted
         * when the last process ends.
         *
         * Reimplemented from QWidget.
         **/
        void hideEvent( QHideEvent * ) override;


    private:

        std::unique_ptr<Ui::OutputWindow > _ui;

        ProcessList _processList;

        bool    _noMoreProcesses{ false };
        bool    _closed{ false };
        bool    _killedAll{ false };
        int     _errorCount{ 0 };
        QString _lastWorkingDir;

        bool   _showOnStderr{ false };
        QColor _terminalBackground;
        QColor _commandTextColor;
        QColor _stdoutColor;
        QColor _stderrColor;
        QFont  _terminalDefaultFont;

    };  // class OutputWindow



    inline QTextStream & operator<<( QTextStream & stream, const QProcess * process )
    {
        if ( process )
            stream << OutputWindow::command( process );
        else
            stream << "<NULL QProcess>";

        return stream;
    }

}       // namespace QDirStat

#endif  // OutputWindow_h
