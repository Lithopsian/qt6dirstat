/*
 *   File name: RpmPkgManager.cpp
 *   Summary:   RPM package manager support for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#define LONG_CMD_TIMEOUT_SEC    30	// for SysUtil.h
#define DEFAULT_WARNING_SEC      7	// for SysUtil.h

#include <iostream>	// cerr/endl;

#include <QElapsedTimer>
#include <QPointer>

#include "RpmPkgManager.h"
#include "MainWindow.h"
#include "PanelMessage.h"
#include "PkgFileListCache.h"
#include "QDirStatApp.h"
#include "Settings.h"
#include "SysUtil.h"
#include "Logger.h"
#include "Exception.h"


using namespace QDirStat;

using SysUtil::runCommand;
using SysUtil::tryRunCommand;
using SysUtil::haveCommand;


namespace
{
    /**
     * Determine the path to the rpm command for this system - may not
     * actually exist.
     **/
    QString rpmCommand()
    {
	// Note that it is not enough to rely on a symlink /bin/rpm ->
	// /usr/bin/rpm: While recent SUSE distros have that symlink (and maybe Red
	// Hat and Fedora as well?), rpm as a secondary package manager on Ubuntu
	// does not have such a link; they only have /usr/bin/rpm.
	if ( haveCommand( "/usr/bin/rpm" ) )
	    return "/usr/bin/rpm";

	// Return something to try although it may not exist
	return "/bin/rpm"; // for old SUSE / Red Hat distros
    }

} // namespace


RpmPkgManager::RpmPkgManager()
{
    readSettings();
}


bool RpmPkgManager::isPrimaryPkgManager() const
{
    return tryRunCommand( QString( "%1 -qf %1" ).arg( rpmCommand() ),
			  QRegularExpression( "^rpm.*" ) );
}


bool RpmPkgManager::isAvailable() const
{
    return haveCommand( rpmCommand() );
}


QString RpmPkgManager::owningPkg( const QString & path ) const
{
    int exitCode = -1;
    const QString output = runCommand(	rpmCommand(),
					{ "-qf", "--queryformat", "%{name}", path },
					&exitCode );

    if ( exitCode != 0 || output.contains( "not owned by any package" ) )
	return "";

    return output;
}


PkgInfoList RpmPkgManager::installedPkg() const
{
    int exitCode = -1;
    QElapsedTimer timer;
    timer.start();

    const QString output = runCommand( rpmCommand(),
				       { "-qa", "--queryformat", "%{name} | %{version}-%{release} | %{arch}\n" },
				       &exitCode,
				       LONG_CMD_TIMEOUT_SEC );

    if ( timer.hasExpired( _getPkgListWarningSec * 1000 ) )
	rebuildRpmDbWarning();

    return exitCode == 0 ? parsePkgList( output ) : PkgInfoList();
}


PkgInfoList RpmPkgManager::parsePkgList( const QString & output ) const
{
    PkgInfoList pkgList;

    const QStringList splitOutput = output.split( "\n" );
    for ( const QString & line : splitOutput )
    {
	if ( !line.isEmpty() )
	{
	    QStringList fields = line.split( " | " );

	    if ( fields.size() != 3 )
		logError() << "Invalid rpm -qa output: " << line << "\n" << Qt::endl;
	    else
	    {
		const QString name    = fields.takeFirst();
		const QString version = fields.takeFirst(); // includes release

		QString arch = fields.takeFirst();
		if ( arch == "(none)" )
		    arch = "";

		PkgInfo * pkg = new PkgInfo( name, version, arch, this );
		CHECK_NEW( pkg );

		pkgList << pkg;
	    }
	}
    }

    return pkgList;
}


QString RpmPkgManager::fileListCommand( const PkgInfo * pkg ) const
{
    return QString( "%1 -ql %2" ).arg( rpmCommand() ).arg( queryName( pkg ) );
}


QStringList RpmPkgManager::parseFileList( const QString & output ) const
{
    QStringList fileList = output.split( "\n" );
    fileList.removeAll( "(contains no files)" );

    return fileList;
}


QString RpmPkgManager::queryName( const PkgInfo * pkg ) const
{
    CHECK_PTR( pkg );

    QString name = pkg->baseName();

    if ( !pkg->version().isEmpty() )
	name += "-" + pkg->version();

    if ( !pkg->arch().isEmpty() )
	name += "." + pkg->arch();

    return name;
}


PkgFileListCache * RpmPkgManager::createFileListCache( PkgFileListCache::LookupType lookupType ) const
{
    int exitCode = -1;
    QString output = runCommand( rpmCommand(),
				 { "-qa", "--qf", "[%{=NAME}-%{=VERSION}-%{=RELEASE}.%{=ARCH} | %{FILENAMES}\n]" },
				 &exitCode,
				 LONG_CMD_TIMEOUT_SEC );

    if ( exitCode != 0 )
	return nullptr;

    const QStringList lines = output.split( "\n" );
    output.clear(); // Free all that text ASAP
    logDebug() << lines.size() << " output lines" << Qt::endl;

    PkgFileListCache * cache = new PkgFileListCache( this, lookupType );
    CHECK_NEW( cache );

    // Sample output:
    //
    //	   zsh-5.6-lp151.1.3.x86_64 | /bin/zsh
    //	   zsh-5.6-lp151.1.3.x86_64 | /etc/zprofile
    //	   zsh-5.6-lp151.1.3.x86_64 | /etc/zsh_completion.d

    for ( const QString & line : lines )
    {
	if ( line.isEmpty() )
	    continue;

	QStringList fields = line.split( " | " );

	if ( fields.size() != 2 )
	{
	    logError() << "Unexpected file list line: \"" << line << "\"" << Qt::endl;
	}
	else
	{
	    const QString pkgName = fields.takeFirst();
	    const QString path    = fields.takeFirst();

	    if ( !pkgName.isEmpty() && !path.isEmpty() )
		cache->add( pkgName, path );
	}
    }

    logDebug() << "file list cache finished." << Qt::endl;

    return cache;
}


void RpmPkgManager::readSettings()
{
    Settings settings;
    settings.beginGroup( "Pkg" );
    _getPkgListWarningSec = settings.value( "GetRpmPkgListWarningSec", DEFAULT_WARNING_SEC ).toInt();

    // Write the value right back to the settings if it isn't there already:
    // Since package manager objects are never destroyed, this can't
    // reliably be done in the destructor.
    settings.setDefaultValue( "GetRpmPkgListWarningSec", _getPkgListWarningSec );
    settings.endGroup();
}


void RpmPkgManager::rebuildRpmDbWarning() const
{
    static bool issuedWarning = false;

    if ( !issuedWarning )
    {
	std::cerr << "WARNING: rpm is very slow. Run	  sudo rpm --rebuilddb\n" << std::endl;
	logWarning()  << "rpm is very slow. Run	  sudo rpm --rebuilddb"	  << Qt::endl;
    }

    // Add a panel message so the user is sure to see this message.
    MainWindow * mainWindow = (MainWindow *)app()->findMainWindow();
    if ( mainWindow )
	PanelMessage::showRpmMsg( mainWindow, mainWindow->messagePanelContainer() );

    issuedWarning = true;
}
