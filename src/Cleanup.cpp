/*
 *   File name: Cleanup.cpp
 *   Summary:   QDirStat classes to reclaim disk space
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
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


namespace
{
    /**
     * Return 'true' if programName is non-empty and executable.
     **/
/*    bool isExecutable( const QString & programName )
    {
	if ( programName.isEmpty() )
	    return false;

	const QFileInfo fileInfo( programName );
	return fileInfo.isExecutable();
    }
*/

    /**
     * Return the full path name to the user's login shell.
     * The $SHELL environment variable is used to obtain this value.
     **/
    const QString loginShell()
    {
	const QString shell = QProcessEnvironment::systemEnvironment().value( "SHELL", QString() );

	if ( !SysUtil::haveCommand( shell ) )
	{
	    logError() << "ERROR: Shell \"" << shell << "\" is not executable" << Qt::endl;
	    return QString();
	}

	return shell;
    }


    /**
     * Checks whether a given app is executable in any of the environment paths.
     **/
    bool haveApp( const QString & app )
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
    QStringList terminalApps()
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
    QStringList fileManagerApps()
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


    /**
     * Return the fallback terminal app, used when nothing can be found
     * from either the desktop or full list.
     **/
    QString fallbackTerminalApp()
    {
	return "xterm";
    }


    /**
     * Return the fallback file manager app, used when nothing can be found
     * from either the desktop or full list.
     **/
    QString fallbackFileManagerApp()
    {
	return "xdg-open";
    }


    /**
     * Get the environment desktop.
     **/
    QString envDesktop()
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


    /**
     * Return a mapping from macros to applications that may be specific
     * for different desktops (KDE, GNOME, Xfce, Unity, LXDE).
     * Incomplete list:
     *
     *   %terminal
     *	KDE:	"konsole --workdir %d"
     *	GNOME:	"gnome-terminal"
     *	Unity:	"gnome-terminal"
     *	Xfce:	"xfce4-terminal"
     *	LXDE:	"lxterminal"
     *
     * Which desktop is currently used is guessed from $XDG_CURRENT_DESKTOP.
     **/
    QString desktopSpecificTerminal()
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
		if ( desktop == QLatin1String( "gnome" ) ||
		     desktop == QLatin1String( "unity" ) ||
		     desktop == QLatin1String( "cinnamon"      ) ) return "gnome-terminal";
		if ( desktop == QLatin1String( "xfce"          ) ) return "xfce4-terminal";
		if ( desktop == QLatin1String( "lxde"          ) ) return "lxterminal";
		if ( desktop == QLatin1String( "enlightenment" ) ) return "terminology";
		if ( desktop == QLatin1String( "mate"          ) ) return "mate-terminal";
		if ( desktop == QLatin1String( "budgie"        ) ) return "tilix";
		if ( desktop == QLatin1String( "lxqt"          ) ) return "qterminal";
		if ( desktop == QLatin1String( "kde"           ) ) return "konsole --workdir %d";

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


    /**
     * Return a mapping from macros to applications that may be specific
     * for different desktops (KDE, GNOME, Xfce, Unity, LXDE).
     * Incomplete list:
     *
     *   %filemanager
     *	KDE:	"konqueror --profile filemanagement" // not that dumbed-down Dolphin
     *	GNOME:	"nautilus"
     *	Unity:	"nautilus"
     *	Xfce:	"thunar"
     *	LXDE:	"pcmanfm"
     *
     * Which desktop is currently used is guessed from $XDG_CURRENT_DESKTOP.
     **/
    QString desktopSpecificFileManager()
    {
	const QString desktop = envDesktop();
	if ( !desktop.isEmpty() )
	{
	    logInfo() << "Detected desktop \"" << desktop << "\"" << Qt::endl;

	    const QString desktopApp = [ &desktop ]()
	    {
		if ( desktop == QLatin1String( "gnome" ) ||
		     desktop == QLatin1String( "unity"         ) ) return "nautilus";
		if ( desktop == QLatin1String( "xfce"          ) ) return "thunar";
		if ( desktop == QLatin1String( "lxde"          ) ) return "pcmanfm";
		if ( desktop == QLatin1String( "enlightenment" ) ) return "enlightenment-open";
		if ( desktop == QLatin1String( "mate"          ) ) return "caja";
		if ( desktop == QLatin1String( "cinnamon" ) ||
		     desktop == QLatin1String( "budgie"        ) ) return "nemo";
		if ( desktop == QLatin1String( "lxqt"          ) ) return "pcmanfm-qt";
		if ( desktop == QLatin1String( "kde"           ) ) return "dolphin";

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


    /**
     * Return the currently-configured and available terminal app
     * to be substituted for %terminal.  Store in a static variable
     * as the process for obtaining it is non-trivial.  The variable
     * may remain empty if there is no certain identification of a valid
     * app, and identification will be attempted again next time.
     **/
    QString terminalApp()
    {
	static QString app;

	if ( app.isEmpty() )
	    app = desktopSpecificTerminal();

	return app.isEmpty() ? fallbackTerminalApp() : app;
    }


    /**
     * Return the currently-configured and available file manager app
     * to be substituted for %filemanager.  Store in a static variable
     * as the process for obtaining it is non-trivial.  The variable
     * may remain empty if there is no certain identification of a valid
     * app, and identification will be attempted again next time.
     **/
    QString fileManagerApp()
    {
	static QString app;

	if ( app.isEmpty() )
	    app = desktopSpecificFileManager();

	return app.isEmpty() ? fallbackFileManagerApp() : app;
    }


    /**
     * Expand some variables in string 'unexpanded' to application that are
     * typically different from one desktop (KDE, Gnome, Xfce) to the next:
     *
     *   %terminal	  "konsole" or "gnome-terminal" or "xfce4-terminal" ...
     *   %filemanager "konqueror" or "nautilus" or "thunar" ...
     **/
    QString expandDesktopSpecificApps( const QString & unexpanded )
    {
	QString expanded = unexpanded;
	expanded.replace( DESKTOP_APP_TERMINAL, terminalApp() );
	expanded.replace( DESKTOP_APP_FILE_MANAGER, fileManagerApp() );

	return expanded;
    }


    /**
     * Expand some variables in string 'unexpanded' to information from
     * within 'item'. Multiple expansion is performed as needed, i.e. the
     * string may contain more than one variable to expand. The resulting
     * string is returned.
     *
     *   %p expands to item->path() (in single quotes), i.e. the item's
     *   full path name.
     *
     *     '/usr/local/bin'	      for that directory
     *     '/usr/local/bin/doit'  for a file within it
     *
     *   %n expands to item->name() (in single quotes), i.e. the last
     *   component of the pathname. The examples above would expand to:
     *
     *     'bin'
     *     'doit'
     *
     *   %d expands to the directory name with full path. For directories,
     *   this is the same as %p. For files or dot entries, this is the same
     *   as their parent's %p:
     *
     *    '/usr/local/bin'	for a file /usr/local/bin/doit
     *    '/usr/local/bin'	for directory /usr/local/bin.
     *
     *
     *   %terminal	  "konsole" or "gnome-terminal" or "xfce4-terminal" ...
     *
     *   %filemanager "konqueror" or "nautilus" or "thunar" ...
     *
     *
     * For commands that are to be executed from within the 'Clean up'
     * menu, you might specify something like:
     *
     *     "xdg-open %p"
     *     "tar cjvf %n.tar.bz2 && rm -rf %n"
     **/
    QString expandVariables( const FileInfo * item,
				    const QString  & unexpanded )
    {
	QString expanded = expandDesktopSpecificApps( unexpanded );

	expanded.replace( QLatin1String( "%p" ), SysUtil::quoted( SysUtil::escaped( item->path() ) ) );
	expanded.replace( QLatin1String( "%n" ), SysUtil::quoted( SysUtil::escaped( item->name() ) ) );

	const QString dirName = [ item ]()
	{
	    if ( item->isDir() )
		return item->path();

	    if ( item->parent() )
		return item->parent()->path();

	    return QString();
	}();

	if ( !dirName.isEmpty() )
	    expanded.replace( QLatin1String( "%d" ), SysUtil::quoted( SysUtil::escaped( dirName ) ) );

	// logDebug() << "Expanded: \"" << expanded << "\"" << Qt::endl;
	return expanded;
    }

} // namespace



Cleanup::Cleanup( const Cleanup * other ):
    Cleanup ( nullptr, other->_active, other->_title, other->_command,
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


QString Cleanup::itemDir( const FileInfo *item ) const
{
    QString dir = item->path();

    if ( !item->isDir() && !item->isPseudoDir() )
	dir.replace( QRegularExpression( "/[^/]*$" ), "" );

    return dir;
}


const QStringList & Cleanup::defaultShells()
{
    static const QStringList shells = []()
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
		errMsg += '\n' + tr( "Using fallback %1" ).arg( shell );
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


FileInfoSet Cleanup::deDuplicateParents( const FileInfoSet & sel )
{
    // Only change the list when the command doesn't act on individual files
    if ( _command.contains( QLatin1String( "%n" ) ) || _command.contains( QLatin1String( "%p" ) ) )
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


bool Cleanup::operator!=( const Cleanup * other ) const
{
    if ( !other )
        return true;

    if ( other == this )
        return false;

    if ( other->isActive()              != isActive()              ||
         other->title()                 != title()                 ||
         other->command()               != command()               ||
//         other->iconName()              != iconName()              || // not currently in the
//         other->shortcut()              != shortcut()              || // config dialog
         other->recurse()               != recurse()               ||
         other->askForConfirmation()    != askForConfirmation()    ||
         other->refreshPolicy()         != refreshPolicy()         ||
         other->worksForDir()           != worksForDir()           ||
         other->worksForFile()          != worksForFile()          ||
         other->worksForDotEntry()      != worksForDotEntry()      ||
         other->outputWindowPolicy()    != outputWindowPolicy()    ||
         other->outputWindowTimeout()   != outputWindowTimeout()   ||
         other->outputWindowAutoClose() != outputWindowAutoClose() ||
         other->shell()                 != shell() )
    {
        return true;
    }

    return false;
}
