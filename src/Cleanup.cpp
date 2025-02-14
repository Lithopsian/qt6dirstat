/*
 *   File name: Cleanup.cpp
 *   Summary:   QDirStat classes to reclaim disk space
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>

#include "Cleanup.h"
#include "FileInfoIterator.h"
#include "FileInfoSet.h"
#include "Logger.h"
#include "OutputWindow.h"
#include "SysUtil.h"


using namespace QDirStat;


namespace
{
    /**
     * Return the full path name to the user's login shell.
     * The $SHELL environment variable is used to obtain this value.
     **/
    const QString loginShell()
    {
	const QString shell = QProcessEnvironment::systemEnvironment().value( "SHELL", QString{} );

	if ( !SysUtil::haveCommand( shell ) )
	{
	    logError() << "ERROR: Shell \"" << shell << "\" is not executable" << Qt::endl;
	    return QString{};
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

	const QString path = QProcessEnvironment::systemEnvironment().value( "PATH", QString{} );
	const QStringList paths = path.split( u':', Qt::SkipEmptyParts );

	// If we don't have a PATH, just assume the program exists although it will probably never run
	if ( paths.isEmpty() )
	    return true;

	const auto firstSpaceIndex = app.indexOf( u' ' );
	const QString appName = firstSpaceIndex < 0 ? app : app.left( firstSpaceIndex );

	for ( const QString & appPath : paths )
	{
	    if ( SysUtil::haveCommand( appPath % '/' % appName ) )
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
	const QString desktop = env.value( "QDIRSTAT_DESKTOP", QString{} );
	if ( !desktop.isEmpty() )
	{
	    //logDebug() << "Overriding $XDG_CURRENT_DESKTOP with $QDIRSTAT_DESKTOP (\"" << desktop << "\")" << Qt::endl;

	    return desktop.toLower();
	}

	const QString xdgDesktop = env.value( "XDG_CURRENT_DESKTOP", QString{} );
	if ( !xdgDesktop.isEmpty() )
	    return xdgDesktop.toLower();

	//logWarning() << "$XDG_CURRENT_DESKTOP is not set" << Qt::endl;

	return QString{};
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
	    logInfo() << "Detected desktop \"" << desktop << '"' << Qt::endl;

	    // KDE konsole misbehaves in every way possible:
	    //
	    // It cannot be started in the background from a cleanup action,
	    // it will terminate when QDirStat terminates,
	    // and it doesn't give a shit about its current working directory.
	    // So all the other terminals need to be explicitly started in
	    // the background, but konsole not.
	    const QString desktopApp = [ &desktop ]()
	    {
		if ( desktop == "gnome"_L1 ||
		     desktop == "unity"_L1 ||
		     desktop == "cinnamon"_L1      ) return "gnome-terminal";
		if ( desktop == "xfce"_L1          ) return "xfce4-terminal";
		if ( desktop == "lxde"_L1          ) return "lxterminal";
		if ( desktop == "enlightenment"_L1 ) return "terminology";
		if ( desktop == "mate"_L1          ) return "mate-terminal";
		if ( desktop == "budgie"_L1        ) return "tilix";
		if ( desktop == "lxqt"_L1          ) return "qterminal";
		if ( desktop == "kde"_L1           ) return "konsole --workdir %d";

		return "";
	    }();

	    //  See if the standard desktop terminal is installed on this system
	    if ( haveApp( desktopApp ) )
		return desktopApp;
	}

	// Now just try any terminal emulator
	const auto apps = terminalApps();
	for ( const QString & app : apps )
	{
	    if ( haveApp( app ) )
		return app;
	}

	// We didn't find anything positive, will look again next time
	return QString{};
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
	    logInfo() << "Detected desktop \"" << desktop << '"' << Qt::endl;

	    const QString desktopApp = [ &desktop ]()
	    {
		if ( desktop == "gnome"_L1 ||
		     desktop == "unity"_L1         ) return "nautilus";
		if ( desktop == "xfce"_L1          ) return "thunar";
		if ( desktop == "lxde"_L1          ) return "pcmanfm";
		if ( desktop == "enlightenment"_L1 ) return "enlightenment-open";
		if ( desktop == "mate"_L1          ) return "caja";
		if ( desktop == "cinnamon"_L1 ||
		     desktop == "budgie"_L1        ) return "nemo";
		if ( desktop == "lxqt"_L1          ) return "pcmanfm-qt";
		if ( desktop == "kde"_L1           ) return "dolphin";

		return "";
	    }();

	    //  See if the standard desktop file manager is installed on this system
	    if ( haveApp( desktopApp ) )
		return desktopApp;
	}

	// Now just try any file manager
	const auto apps = fileManagerApps();
	for ( const QString & app : apps )
	{
	    if ( haveApp( app ) )
		return app;
	}

	// We didn't find anything positive, will look again next time
	return QString{};
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
    void expandDesktopSpecificApps( QString & apps )
    {
	apps.replace( "%terminal"_L1, terminalApp() );
	apps.replace( "%filemanager"_L1, fileManagerApp() );
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
     *     '/usr/local/bin'       for that directory
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
     *   %terminal	"konsole" or "gnome-terminal" or "xfce4-terminal" ...
     *
     *   %filemanager	"konqueror" or "nautilus" or "thunar" ...
     *
     * For commands that are to be executed from within the 'Clean up'
     * menu, you might specify something like:
     *
     *     "xdg-open %p"
     *     "tar cjvf %n.tar.bz2 && rm -rf %n"
     **/
    QString expandVariables( const FileInfo * item, QString command )
    {
	expandDesktopSpecificApps( command );

	command.replace( "%p"_L1, SysUtil::shellQuoted( item->path() ) );
	command.replace( "%n"_L1, SysUtil::shellQuoted( item->name() ) );

	const QString dirName = [ item ]()
	{
	    if ( item->isDir() )
		return item->path();

	    if ( item->parent() )
		return item->parent()->path();

	    return QString{};
	}();

	if ( !dirName.isEmpty() )
	    command.replace( "%d"_L1, SysUtil::shellQuoted( dirName ) );

	// logDebug() << "Expanded: \"" << expanded << '"' << Qt::endl;
	return command;
    }


    /**
     * Retrieve the directory part of a FileInfo's path.
     **/
    QString itemDir( const FileInfo * item )
    {
	QString dir = item->path();

	if ( !item->isDir() && !item->isPseudoDir() )
	{
	    const auto lastSlashIndex = dir.lastIndexOf( u'/' );
	    if ( lastSlashIndex >= 0 )
		dir.truncate( lastSlashIndex );
	}

	return dir;
    }


    /**
     * Run a command with 'item' as base to expand variables.
     **/
    void runCommand( const QString  & shell,
                     const FileInfo * item,
                     const QString  & command,
                     OutputWindow   * outputWindow )
    {
	if ( shell.isEmpty() )
	{
	    outputWindow->show(); // Regardless of user settings
	    outputWindow->addStderr( QObject::tr( "No usable shell - aborting cleanup action.\n" ) );
	    return;
	}

	// Deliberately create with no parent so they aren't destroyed untidily at shutdown
	QProcess * process = new QProcess{};
	process->setProgram( shell );
	process->setArguments( { OutputWindow::shellCommandArg(), expandVariables( item, command ) } );
	process->setWorkingDirectory( itemDir( item ) );
	// logDebug() << "New process \"" << process << Qt::endl;

	outputWindow->addProcess( process );

	// CleanupCollection will take care of refreshing if it is
	// configured for this cleanup
    }

} // namespace


bool Cleanup::worksFor( FileInfo * item ) const
{
    if ( !_active || !item )
	return false;

    if ( item->isPseudoDir() )
	return worksForDotEntry();

    if ( item->isDir() )
	return worksForDir();

    return worksForFile();
}


void Cleanup::execute( FileInfo * item, OutputWindow * outputWindow )
{
    if ( _recurse )
    {
	// Recursively process any children, including dot entries
	for ( DotEntryIterator it{ item }; *it; ++it )
	    execute( *it, outputWindow );
    }

    // Perform cleanup for this item
    if ( worksFor( item ) )
	runCommand( chooseShell( outputWindow ), item, _command, outputWindow );
}


const QStringList & Cleanup::defaultShells()
{
    static const QStringList shells = []()
    {
	QStringList shells;
	const QStringList candidates{ loginShell(), "/bin/bash", "/bin/sh" };
	for ( const QString & shell : candidates )
	{
	    if ( SysUtil::haveCommand( shell ) )
		 shells << shell;
	    else if ( !shell.isEmpty() )
		logWarning() << "Shell " << shell << " is not executable" << Qt::endl;
	}

	if ( !shells.isEmpty() )
	    logInfo() << "Default shell: " << shells.first() << Qt::endl;

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
	    errMsg = tr( "ERROR: Shell %1 is not executable.\n" ).arg( shell );
	    shell = defaultShell();

	    if ( !shell.isEmpty() )
		errMsg += tr( "Using fallback %1.\n" ).arg( shell );
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


FileInfoSet Cleanup::deDuplicateParents( const FileInfoSet & sel )
{
    // Only change the list when the command doesn't act on individual files
    if ( _command.contains( "%n"_L1 ) || _command.contains( "%p"_L1 ) )
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


bool Cleanup::operator!=( const Cleanup & other ) const
{
    if ( other.isActive()              != isActive()              ||
         other.title()                 != title()                 ||
         other.command()               != command()               ||
         other.iconName()              != iconName()              ||
         other.shortcut()              != shortcut()              ||
         other.recurse()               != recurse()               ||
         other.askForConfirmation()    != askForConfirmation()    ||
         other.refreshPolicy()         != refreshPolicy()         ||
         other.worksForDir()           != worksForDir()           ||
         other.worksForFile()          != worksForFile()          ||
         other.worksForDotEntry()      != worksForDotEntry()      ||
         other.outputWindowPolicy()    != outputWindowPolicy()    ||
         other.outputWindowTimeout()   != outputWindowTimeout()   ||
         other.outputWindowAutoClose() != outputWindowAutoClose() ||
         other.shell()                 != shell() )
    {
        return true;
    }

    return false;
}
