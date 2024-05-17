/*
 *   File name: Settings.cpp
 *   Summary:   Specialized settings classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#define DONT_DEPRECATE_STRERROR // for Logger,h

#include <unistd.h> // chown()
#include <errno.h>

#include <QCoreApplication>
#include <QProcessEnvironment>

#include "Settings.h"
#include "SettingsHelpers.h"
#include "SysUtil.h"
#include "Logger.h"
#include "Exception.h"


using namespace QDirStat;


QSet<QString> Settings::_usedConfigFiles;


namespace
{
    /**
     * Move all settings groups starting with 'groupPrefix' from settings
     * object 'from' to settings object 'to'.
     **/
    void moveGroups( const QString & groupPrefix,
		     Settings      * from,
		     Settings      * to )
    {
	CHECK_PTR( from );
	CHECK_PTR( to   );

	if ( !to->hasGroup( groupPrefix ) )
	{
#if 0
	    logInfo() << "Migrating " << groupPrefix << "* to " << to->name() << Qt::endl;
#endif
	    const QStringList groups = from->findGroups( groupPrefix );
	    for ( const QString & group : groups )
	    {
		// logVerbose() << "  Migrating " << group << Qt::endl;

		from->beginGroup( group );
		to->beginGroup( group );

		const QStringList keys = from->allKeys();
		for ( const QString & key : keys )
		{
		    // logVerbose() << "	Copying " << key << Qt::endl;
		    to->setValue( key, from->value( key ) );
		}

		to->endGroup();
		from->endGroup();
	    }
	}
	else
	{
#if 0
	    logVerbose() << "Target settings " << to->name()
			 << " have group " << groupPrefix
			 << " - nothing to migrate"
			 << Qt::endl;
#endif
	}

	from->removeGroups( groupPrefix );
    }


    /**
     * Change the owner of the config file to the user in the $SUDO_UID /
     * $SUDO_GID environment variables (if set).
     **/
    void fixFileOwner( const QString & filename )
    {
	const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	const QString sudoUid = env.value( "SUDO_UID", QString() );
	const QString sudoGid = env.value( "SUDO_GID", QString() );

	if ( !sudoUid.isEmpty() && !sudoGid.isEmpty() )
	{
	    const uid_t   uid     = sudoUid.toInt();
	    const gid_t   gid     = sudoGid.toInt();
	    const QString homeDir = SysUtil::homeDir( uid );
	    if ( homeDir.isEmpty() )
	    {
		logWarning() << "Can't get home directory for UID " << uid << Qt::endl;
		return;
	    }

	    if ( filename.startsWith( homeDir ) )
	    {
		const int result = ::chown( filename.toUtf8(), uid, gid );
		if ( result != 0 )
		{
		    logError() << "Can't chown " << filename
			       << " to UID "  << uid
			       << " and GID " << gid
			       << ": " << strerror( errno )
			       << Qt::endl;
		}
		else
		{
#if 1
		    logDebug() << "Success: chown " << filename
			       << " to UID "  << uid
			       << " and GID " << gid
			       << Qt::endl;
#endif
		}
	    }
	    else
	    {
		// logInfo() << "Not touching " << filename << Qt::endl;
	    }
	}
	else
	{
	    logWarning() << "$SUDO_UID / $SUDO_GID not set" << Qt::endl;
	}
    }

} // namespace


Settings::Settings( const QString & name ):
    QSettings ( QCoreApplication::organizationName(),
		name.isEmpty()? QCoreApplication::applicationName() : name ),
    _name { name }
{
    _usedConfigFiles << fileName();
}


void Settings::beginGroup( const QString & prefix, int no )
{
    _groupPrefix = prefix;

    QSettings::beginGroup( prefix + QString( "_%1" ).arg( no, 2, 10, QLatin1Char( '0' ) ) );
}


void Settings::fixFileOwners()
{
    if ( SysUtil::runningWithSudo() )
    {
        for ( const QString & filename : _usedConfigFiles )
            fixFileOwner( filename );
    }
}


void Settings::ensureToplevel()
{
    while ( !group().isEmpty() )
	endGroup();
}


QStringList Settings::findGroups( const QString & groupPrefix )
{
    ensureToplevel();

    QStringList result;

    const auto groups = childGroups();
    for ( const QString & group : groups )
    {
	if ( group.startsWith( groupPrefix ) )
	    result << group;
    }

    return result;
}


bool Settings::hasGroup( const QString & groupPrefix )
{
    ensureToplevel();

    const auto groups = childGroups();
    for ( const QString & group : groups )
    {
	if ( group.startsWith( groupPrefix ) )
	    return true;
    }

    return false;
}


void Settings::removeGroups( const QString & groupPrefix )
{
    ensureToplevel();

    const auto groups = childGroups();
    for ( const QString & group : groups )
    {
	if ( group.startsWith( groupPrefix ) )
	    remove( group );
    }
}


QString Settings::primaryFileName()
{
    QSettings settings;
    return settings.fileName();
}




CleanupSettings::CleanupSettings():
    Settings( QCoreApplication::applicationName() + "-cleanup" )
{
    setGroupPrefix( "Cleanup_" );
    migrate();
}


void CleanupSettings::migrate()
{
    Settings commonSettings;
    moveGroups( groupPrefix(), &commonSettings, this );
}




MimeCategorySettings::MimeCategorySettings():
    Settings( QCoreApplication::applicationName() + "-mime" )
{
    setGroupPrefix( "MimeCategory_" );
    migrate();
}


void MimeCategorySettings::migrate()
{
    Settings commonSettings;
    moveGroups( groupPrefix(), &commonSettings, this );
}




ExcludeRuleSettings::ExcludeRuleSettings():
    Settings( QCoreApplication::applicationName() + "-exclude" )
{
    setGroupPrefix( "ExcludeRule_" );
    migrate();
}


void ExcludeRuleSettings::migrate()
{
    Settings commonSettings;
    moveGroups( groupPrefix(), &commonSettings, this );
}
