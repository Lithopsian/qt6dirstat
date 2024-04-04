/*
 *   File name: main.cpp
 *   Summary:	QDirStat main program
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <iostream>	// cerr

#include <QApplication>
#include "QDirStatApp.h"
#include "MainWindow.h"
#include "DirTreeModel.h"
#include "Settings.h"
#include "Logger.h"
#include "Exception.h"
#include "Version.h"


using std::cerr;
static const char * progName = "qt6dirstat";


void usage()
{
    cerr << "\n"
	 << "Usage: \n"
	 << "\n"
	 << "  " << progName << " [--slow-update|-s] [<directory-name>]\n"
	 << "  " << progName << " pkg:/pkgpattern\n"
	 << "  " << progName << " unpkg:/dir\n"
	 << "  " << progName << " --dont-ask|-d\n"
	 << "  " << progName << " --cache|-c <cache-file-name>\n"
	 << "  " << progName << " --help|-h\n"
	 << "\n"
	 << "\n"
         << "Supported pkg patterns:\n"
	 << "\n"
         << "- Default: \"Starts with\" \"pkg:/mypkg\"\n"
         << "- Wildcards with '*' and '?'\n"
         << "- Full regexps with \".*\" and/or \"^\" and/or \"$\"\n"
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

#if (QT_VERSION < QT_VERSION_CHECK( 5, 12, 0 ))
    logWarning() << "WARNING: You are using Qt " << QT_VERSION_STR
                 << ". This may or may not work." << Qt::endl;
    logWarning() << "The supported Qt version for Qt6DirStat is Qt 5.12 or newer." << Qt::endl;
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
    if ( argList.contains( longName  ) ||
	 argList.contains( shortName )	 )
    {
	argList.removeAll( longName  );
	argList.removeAll( shortName );
        //logDebug() << "Found " << longName << Qt::endl;
	return true;
    }
    else
    {
        // logDebug() << "No " << longName << Qt::endl;
	return false;
    }
}


/**
 * Output a message about an invalid set of command line arguments.
 * Will appear on stderr since logging is not yet started.
 **/
void reportFatalError()
{
    QStringList argList = QCoreApplication::arguments();
    argList.removeFirst(); // Remove program name
    cerr << "FATAL: Bad command line args: " << qPrintable( argList.join( " " ) ) << std::endl;
}


int main( int argc, char *argv[] )
{
    QApplication qtApp( argc, argv );
    QStringList argList = QCoreApplication::arguments();
    argList.removeFirst(); // Remove program name

    // Take out acceptable switches
    const bool dontAsk = commandLineSwitch( "--dont-ask", "-d", argList );
    const bool slowUpdate = commandLineSwitch( "--slow-update", "-s", argList );
    const bool openCache = commandLineSwitch( "--cache", "-c", argList );

    // treat --help anywhere as valid, even combined with other arguments
    if ( commandLineSwitch( "--help", "-h", argList ) )
    {
	usage();
	return 0;
    }
    // --cache must be the only (remaining) argument and must have one value
    else if ( openCache )
    {
	if ( argList.size() != 1 || argList.first().startsWith( "-" ) )
	{
	    reportFatalError();
	    usage();
	    return 1;
	}
    }
    // Any option other than -d, -c, -h, or -s is invalid,
    // -d can only be combined with -s and nothing else
    // More than one non-option argument is invalid
    else if ( !argList.isEmpty() )
    {
	if ( dontAsk || argList.first().startsWith( "-" ) || argList.size() > 1 )
	{
	    reportFatalError();
	    usage();
	    return 1;
	}
    }

    // We are definitely going to start the application now
    Logger logger( "/tmp/qdirstat-$USER", "qdirstat.log" );
    logVersion();

    // Set org/app name for QSettings
    QCoreApplication::setOrganizationName  ( "Qt6DirStat" );
    QCoreApplication::setApplicationName   ( "Qt6DirStat" );
//    QCoreApplication::setApplicationVersion( QDIRSTAT_VERSION );

    MainWindow * mainWin = new MainWindow( slowUpdate );
    CHECK_PTR( mainWin );
    mainWin->show();

    if ( argList.isEmpty() )
    {
        if ( !dontAsk )
            mainWin->askOpenDir();
    }
    else
    {
	if ( openCache )
	    mainWin->readCache( argList.at(1) );
	else
	    mainWin->openUrl( argList.first() );
    }

    qtApp.exec();

    delete mainWin;

    // If running with 'sudo', this would leave all config files behind owned
    // by root which means that the real user can't write to those files
    // anymore if once invoking QDirStat with 'sudo'. Fixing the file owner for
    // our config files if possible.
    QDirStat::Settings::fixFileOwners();

    return 0;
}
