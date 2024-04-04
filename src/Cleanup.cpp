/*
 *   File name: Cleanup.cpp
 *   Summary:	QDirStat classes to reclaim disk space
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <QRegularExpression>
#include <QProcess>
#include <QProcessEnvironment>
#include <QFileInfo>

#include "Cleanup.h"
#include "DirInfo.h"
#include "FileInfoSet.h"
#include "OutputWindow.h"
#include "SysUtil.h"
#include "Logger.h"
#include "Exception.h"


using namespace QDirStat;


Cleanup::Cleanup( const Cleanup * other ):
    Cleanup ( 0, other->_active, other->_title, other->_command,
	      other->_recurse, other->_askForConfirmation, other->_refreshPolicy,
	      other->_worksForDir, other->_worksForFile, other->_worksForDotEntry,
	      other->_outputWindowPolicy, other->_outputWindowTimeout, other->_outputWindowAutoClose,
	      other->_shell )
{
    setIcon( other->iconName() );     // carried on both the Cleanup and QAction (as a pixmap)
    setShortcut( other->shortcut() ); // only carried on the underlying QAction
}


void Cleanup::setTitle( const QString &title )
{
    _title = title;
    QAction::setText( _title );
}


void Cleanup::setIcon( const QString & iconName )
{
    _iconName = iconName;
    QAction::setIcon( QPixmap( _iconName ) );
}


bool Cleanup::worksFor( FileInfo *item ) const
{
    if ( !_active || !item )
	return false;

    if ( item->isPseudoDir() )
	return worksForDotEntry();

    if ( item->isDir() )
	return worksForDir();

    return worksForFile();
}


void Cleanup::execute( FileInfo *item, OutputWindow * outputWindow )
{
    if ( _recurse )
    {
	// Process any children
	for ( FileInfo * subdir = item->firstChild(); subdir; subdir = subdir->next() )
	{
	    /**
	     * Recursively execute in this subdirectory, but only if it
	     * really is a directory: file children might have been
	     * reparented to the directory (normally, they reside in
	     * the dot entry) if there are no real subdirectories on
	     * this directory level.
	     **/
	    if ( subdir->isDir() )
		execute( subdir, outputWindow );
	}
    }

    // Perform cleanup for this item
    if ( worksFor( item ) )
	runCommand( item, _command, outputWindow );
}


const QString Cleanup::itemDir( const FileInfo *item ) const
{
    QString dir = item->path();

    if ( !item->isDir() && !item->isPseudoDir() )
	dir.replace( QRegularExpression( "/[^/]*$" ), "" );

    return dir;
}

/*
static bool isExecutable( const QString & programName )
{
    if ( programName.isEmpty() )
	return false;

    const QFileInfo fileInfo( programName );
    return fileInfo.isExecutable();
}
*/

static const QString loginShell()
{
    const QString shell = QProcessEnvironment::systemEnvironment().value( "SHELL", QString() );

    if ( !SysUtil::haveCommand( shell ) )
    {
	logError() << "ERROR: Shell \"" << shell << "\" is not executable" << Qt::endl;
	return QString();
    }

    return shell;
}


const QStringList & Cleanup::defaultShells()
{
    static QStringList shells = []()
    {
	QStringList shells;
	const QStringList candidates = { loginShell(), "/bin/bash", "/bin/sh" };
	for ( const QString & shell : candidates )
	{
	    if ( SysUtil::haveCommand( shell ) )
		 shells << shell;
	    else if ( !shell.isEmpty() )
		logWarning() << "Shell " << shell << " is not executable" << Qt::endl;
	}

	if ( !shells.isEmpty() )
	    logDebug() << "Default shell: " << shells.first() << Qt::endl;

	return shells;
    }();

    if ( shells.isEmpty() )
	logError() << "ERROR: No usable shell" << Qt::endl;

    return shells;
}


QString Cleanup::chooseShell( OutputWindow * outputWindow ) const
{
    QString errMsg;

    QString shell = _shell;
    if ( !shell.isEmpty() )
    {
	//logDebug() << "Using custom shell " << shell << Qt::endl;

	if ( !SysUtil::haveCommand( shell ) )
	{
	    errMsg = tr( "ERROR: Shell %1 is not executable" ).arg( shell );
	    shell = defaultShell();

	    if ( !shell.isEmpty() )
		errMsg += "\n" + tr( "Using fallback %1" ).arg( shell );
	}
    }

    if ( shell.isEmpty() )
    {
	shell = defaultShell();
	//logDebug() << "No custom shell configured - using " << shell << Qt::endl;
    }

    if ( !errMsg.isEmpty() )
    {
	outputWindow->show(); // Show error regardless of user settings
	outputWindow->addStderr( errMsg );
    }

    return shell;
}


SettingsEnumMapping Cleanup::refreshPolicyMapping()
{
    return { { NoRefresh,     "NoRefresh"     },
	     { RefreshThis,   "RefreshThis"   },
	     { RefreshParent, "RefreshParent" },
	     { AssumeDeleted, "AssumeDeleted" },
	   };
}


SettingsEnumMapping Cleanup::outputWindowPolicyMapping()
{
    return { { ShowAlways,        "ShowAlways"        },
	     { ShowIfErrorOutput, "ShowIfErrorOutput" },
	     { ShowAfterTimeout,  "ShowAfterTimeout"  },
	     { ShowNever,         "ShowNever"         },
	   };
}


static bool haveApp( const QString & app )
{
    if ( app.isEmpty() )
	return false;

    const QString path = QProcessEnvironment::systemEnvironment().value( "PATH", QString() );
    const QStringList paths = path.split( ':', Qt::SkipEmptyParts );

    // If we don't have a PATH, just assume the program exists although it will probably never run
    if ( paths.isEmpty() )
	return true;

    for ( const QString & appPath : paths )
    {
	if ( SysUtil::haveCommand( appPath + '/' + app.section( ' ', 0, 0 ) ) )
	    return true;
    }

    return false;
}


/**
 * Return the fallback terminal apps, default first.
 **/
static QStringList terminalApps()
{
#ifdef Q_OS_MAC
    return { "open -a Terminal.app ." };
#else
    return { "gnome-terminal",
	     "xfce4-terminal",
	     "lxterminal",
	     "eterm",
	     "terminology",
	     "mate-terminal",
	     "tilix",
	     "qterminal",
	     "konsole --workdir",
	   };
#endif
}


/**
 * Return the fallback file manager apps, default first.
 **/
static QStringList fileManagerApps()
{
#ifdef Q_OS_MAC
    return { "open" };
#else
    return { "dolphin",
	     "nautilus",
	     "thunar",
	     "pcmanfm",
	     "pcmanfm-qt",
	     "spacefm",
	     "caja",
	     "nemo",
	   };
#endif
}


static QString fallbackTerminalApp()
{
    return "xterm";
}


static QString fallbackFileManagerApp()
{
    return "xdg-open";
}


static QString envDesktop()
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString desktop = env.value( "QDIRSTAT_DESKTOP", QString() );
    if ( !desktop.isEmpty() )
    {
	//logDebug() << "Overriding $XDG_CURRENT_DESKTOP with $QDIRSTAT_DESKTOP (\"" << desktop << "\")" << Qt::endl;

	return desktop.toLower();
    }

    const QString xdgDesktop = env.value( "XDG_CURRENT_DESKTOP", QString() );
    if ( !xdgDesktop.isEmpty() )
	return xdgDesktop.toLower();

    //logWarning() << "$XDG_CURRENT_DESKTOP is not set" << Qt::endl;

    return QString();
}


static QString desktopSpecificTerminal()
{
    const QString desktop = envDesktop();
    if ( !desktop.isEmpty() )
    {
	logInfo() << "Detected desktop \"" << desktop << "\"" << Qt::endl;

	    // KDE konsole misbehaves in every way possible:
	    //
	    // It cannot be started in the background from a cleanup action,
	    // it will terminate when QDirStat terminates,
	    // and it doesn't give a shit about its current working directory.
	    // So all the other terminals need to be explicitly started in
	    // the background, but konsole not.
	const QString desktopApp = [ &desktop ]()
	{
	    if ( desktop == "gnome" ||
	         desktop == "unity" ||
		 desktop == "cinnamon"      ) return "gnome-terminal";
	    if ( desktop == "xfce"          ) return "xfce4-terminal";
	    if ( desktop == "lxde"          ) return "lxterminal";
	    if ( desktop == "enlightenment" ) return "terminology";
	    if ( desktop == "mate"          ) return "mate-terminal";
	    if ( desktop == "budgie"        ) return "tilix";
	    if ( desktop == "lxqt"          ) return "qterminal";
	    if ( desktop == "kde"           ) return "konsole --workdir %d";

	    return "";
	}();

	//  See if the standard desktop terminal is installed on this system
	if ( haveApp( desktopApp ) )
	    return desktopApp;
    }

    // Now just try any terminal emulator
    for ( const QString & app : terminalApps() )
    {
	if ( haveApp( app ) )
	    return app;
    }

    // We didn't find anything positive, will look again next time
    return QString();
}


static QString desktopSpecificFileManager()
{
    const QString desktop = envDesktop();
    if ( !desktop.isEmpty() )
    {
	logInfo() << "Detected desktop \"" << desktop << "\"" << Qt::endl;

	const QString desktopApp = [ &desktop ]()
	{
	    if ( desktop == "gnome" ||
	         desktop == "unity"         ) return "nautilus";
	    if ( desktop == "xfce"          ) return "thunar";
	    if ( desktop == "lxde"          ) return "pcmanfm";
	    if ( desktop == "enlightenment" ) return "enlightenment-open";
	    if ( desktop == "mate"          ) return "caja";
	    if ( desktop == "cinnamon" ||
	         desktop == "budgie"        ) return "nemo";
	    if ( desktop == "lxqt"          ) return "pcmanfm-qt";
	    if ( desktop == "kde"           ) return "dolphin";

	    return "";
	}();

	//  See if the standard desktop file manager is installed on this system
	if ( haveApp( desktopApp ) )
	    return desktopApp;
    }

    // Now just try any file manager
    for ( const QString & app : fileManagerApps() )
    {
	if ( haveApp( app ) )
	    return app;
    }

    // We didn't find anything positive, will look again next time
    return QString();
}


static QString terminalApp()
{
    static QString app;

    if ( app.isEmpty() )
	app = desktopSpecificTerminal();

    return app.isEmpty() ? fallbackTerminalApp() : app;
}


static QString fileManagerApp()
{
    static QString app;

    if ( app.isEmpty() )
	app = desktopSpecificFileManager();

    return app.isEmpty() ? fallbackFileManagerApp() : app;
}


static QString expandDesktopSpecificApps( const QString & unexpanded )
{
    QString expanded = unexpanded;
    expanded.replace( DESKTOP_APP_TERMINAL, terminalApp() );
    expanded.replace( DESKTOP_APP_FILE_MANAGER, fileManagerApp() );

    return expanded;
}


FileInfoSet Cleanup::deDuplicateParents( const FileInfoSet & sel )
{
    // Only change when the command doesn't act on individual files
    if ( _command.contains( "%n" ) || _command.contains( "%p" ) )
	return sel;

    FileInfoSet parents;
    for ( FileInfo * item : sel )
    {
	DirInfo * parent = item->isDir() ? item->toDirInfo() : item->parent();
	while ( parent && parent->isPseudoDir() )
	    parent = parent->parent();

	if ( parent )
	    parents << parent;
    }

    return parents;
}


static QString expandVariables( const FileInfo * item,
				const QString  & unexpanded )
{
    QString expanded = expandDesktopSpecificApps( unexpanded );

    expanded.replace( "%p", SysUtil::quoted( SysUtil::escaped( item->path() ) ) );
    expanded.replace( "%n", SysUtil::quoted( SysUtil::escaped( item->name() ) ) );

    const QString dirName = [ item ]()
    {
	if ( item->isDir() )
	    return item->path();

	if ( item->parent() )
	    return item->parent()->path();

	return QString();
    }();

    if ( !dirName.isEmpty() )
	expanded.replace( "%d", SysUtil::quoted( SysUtil::escaped( dirName ) ) );

    // logDebug() << "Expanded: \"" << expanded << "\"" << Qt::endl;
    return expanded;
}


void Cleanup::runCommand( const FileInfo * item,
			  const QString  & command,
			  OutputWindow   * outputWindow ) const
{
    const QString shell = chooseShell( outputWindow );

    if ( shell.isEmpty() )
    {
	outputWindow->show(); // Regardless of user settings
	outputWindow->addStderr( tr( "No usable shell - aborting cleanup action" ) );
	logError() << "ERROR: No usable shell" << Qt::endl;
	return;
    }

    QProcess * process = new QProcess( parent() );
    CHECK_NEW( process );

    process->setProgram( shell );
    process->setArguments( { "-c", expandVariables( item, command ) } );
    process->setWorkingDirectory( itemDir( item ) );
    // logDebug() << "New process \"" << process << Qt::endl;

    outputWindow->addProcess( process );

    // The CleanupCollection will take care about refreshing if this is
    // configured for this cleanup.
}
