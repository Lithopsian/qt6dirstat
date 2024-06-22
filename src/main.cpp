/*
 *   File name: main.cpp
 *   Summary:   QDirStat main program
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <iostream>	// cerr, endl

#include "Logger.h"
#include "MainWindow.h"
#include "QDirStatApp.h"
#include "Settings.h"
#include "Version.h"


namespace
{
    void usage()
    {
	const char * progName = "qdirstat";

	std::cerr << "\n"
	          << "Usage: \n"
	          << "\n"
	          << "  " << progName << " [--slow-update|-s] [<directory-name>]\n"
	          << "  " << progName << " pkg:/pkgpattern\n"
	          << "  " << progName << " unpkg:/dir\n"
	          << "  " << progName << " --dont-ask|-d\n"
	          << "  " << progName << " --cache|-c <cache-file-name>\n"
	          << "  " << progName << " --help|-h\n"
	          << "\n"
	          << "Supported pkg patterns:\n"
	          << "\n"
	          << "- Default: \"Starts with\" \"pkg:/mypkg\"\n"
	          << "- Wildcards with \"*\" and \"?\"\n"
	          << "- Full regexps with \".*\", \"^\",or \"$\"\n"
	          << "- Exact match: \"pkg:/=mypkg\"\n"
	          << "- All packages: \"pkg:/\"\n"
	          << "\n"
	          << "See also   man qdirstat"
	          << "\n"
	          << std::endl;
    }


    void logVersion()
    {
	logInfo() << "Qt6DirStat-" << QDIRSTAT_VERSION
	          << " built with Qt " << QT_VERSION_STR
	          << Qt::endl;

#if QT_VERSION < QT_VERSION_CHECK( 5, 12, 0 )
	logWarning() << "The supported Qt version for Qt6DirStat is Qt 5.12 or newer."
	             << " You are using Qt " << QT_VERSION_STR
	             << ". This may or may not work." << Qt::endl;
#endif
    }


    /**
     * Extract a command line switch (a command line argument without any
     * additional parameter) from the command line and remove it from 'argList'.
     **/
    bool commandLineSwitch( const QString & longName,
			    const QString & shortName,
			    QStringList   & argList )
    {
	if ( !argList.contains( longName ) && !argList.contains( shortName ) )
	{
	    // logDebug() << "No " << longName << Qt::endl;
	    return false;
	}

	//logDebug() << "Found " << longName << Qt::endl;
	argList.removeAll( longName  );
	argList.removeAll( shortName );

	return true;
    }


    /**
     * Output a message about an invalid set of command line arguments.
     * Will appear on stderr since logging is not yet started.
     **/
    void reportFatalError()
    {
	QStringList argList = QCoreApplication::arguments();
	argList.removeFirst(); // Remove program name
	std::cerr << "FATAL: Bad command line args: " << qPrintable( argList.join( ' ' ) ) << std::endl;
	usage();
    }


    /**
     * Create MainWindow, call the requested functions, and run the
     * application event loop.
     *
     * Note that MainWindow is destroyed when this function exits.
     **/
    void mainLoop( bool slowUpdate, bool openCache, bool dontAsk, const QStringList & argList )
    {
	QDirStat::MainWindow mainWin( slowUpdate );
	mainWin.show();

	if ( !argList.isEmpty() )
	{
	    const QString & arg = argList.first();
	    if ( openCache )
		mainWin.readCache( arg );
	    else
		mainWin.openUrl( arg );
	}
	else if ( !dontAsk )
	{
	    mainWin.askOpenDir();
	}

	qApp->exec();
    }

} // namespace


int main( int argc, char * argv[] )
{
    QDirStat::QDirStatApp qDirStatApp( argc, argv );
    QStringList argList = QCoreApplication::arguments();
    argList.removeFirst(); // Remove program name

    // Take out acceptable switches
    const bool dontAsk    = commandLineSwitch( "--dont-ask",    "-d", argList );
    const bool slowUpdate = commandLineSwitch( "--slow-update", "-s", argList );
    const bool openCache  = commandLineSwitch( "--cache",       "-c", argList );

    if ( commandLineSwitch( "--help", "-h", argList ) )
    {
	// treat --help anywhere as valid, even combined with other arguments
	usage();
	return 0;
    }
    else if ( openCache )
    {
	// --cache must be the only (remaining) argument and must have one value
	if ( argList.size() != 1 || argList.first().startsWith( '-' ) )
	{
	    reportFatalError();
	    return 1;
	}
    }
    else if ( !argList.isEmpty() )
    {
	// Any option other than -d, -c, -h, or -s is invalid,
	// -d can only be combined with -s and nothing else
	// More than one non-option argument is invalid
	if ( dontAsk || argList.first().startsWith( '-' ) || argList.size() > 1 )
	{
	    reportFatalError();
	    return 1;
	}
    }

    // We are definitely going to start the application now
    Logger logger( "/tmp/qdirstat-$USER", "qdirstat.log" );
    logVersion();

    // Set org/app name for QSettings
    QCoreApplication::setOrganizationName( "Qt6DirStat" );
    QCoreApplication::setApplicationName ( "Qt6DirStat" );
//    QCoreApplication::setApplicationVersion( QDIRSTAT_VERSION );

    mainLoop( slowUpdate, openCache, dontAsk, argList );

    // Give config files back to the original owner if running with sudo
    QDirStat::Settings::fixFileOwners();

    return 0;
}
