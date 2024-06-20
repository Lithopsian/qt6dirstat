/*
 *   File name: Settings.cpp
 *   Summary:   Specialized settings classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <unistd.h> // chown()

#include <QCoreApplication>
#include <QProcessEnvironment>

#include "Settings.h"
#include "SettingsHelpers.h"
#include "Logger.h"
#include "SysUtil.h"


#define VERBOSE_MIGRATE 0


using namespace QDirStat;


UsedFileList Settings::_usedConfigFiles;


namespace
{
    /**
     * Return true if the settings object has any settings group that
     * starts with 'groupPrefix'.
     **/
    bool hasGroup( const QStringList & groups, QLatin1String groupPrefix )
    {
	for ( const QString & group : groups )
	{
	    if ( group.startsWith( groupPrefix ) )
		return true;
	}

	return false;
    }


    QStringList findGroups( const QStringList & groups, QLatin1String groupPrefix )
    {
	QStringList result;

	for ( const QString & group : groups )
	{
	    if ( group.startsWith( groupPrefix ) )
		result << group;
	}

	return result;
    }


    void removeGroups( QSettings * settings, QLatin1String groupPrefix )
    {
	const auto groups = settings->childGroups();
	for ( const QString & group : groups )
	{
	    if ( group.startsWith( groupPrefix ) )
	    {
#if VERBOSE_MIGRATE
		logVerbose() << "	Removing " << group << Qt::endl;
#endif
		settings->remove( group );
	    }
	}
    }


    /**
     * Move all settings groups starting with 'groupPrefix' from settings
     * object 'from' to settings object 'to'.
     **/
    void moveGroups( Settings * from, Settings * to )
    {
	if ( hasGroup( to->childGroups(), to->listGroupPrefix() ) )
	{
#if VERBOSE_MIGRATE
	    logVerbose() << "Target settings " << to->applicationName()
	                 << " have group starting with \"" << to->listGroupPrefix()
	                 << "\" - not migrating"
	                 << Qt::endl;
#endif
	}
	else
	{
	    logInfo() << "Migrating " << to->listGroupPrefix()
	              << "* to " << to->applicationName()
	              << Qt::endl;

	    const auto groups = findGroups( from->childGroups(), to->listGroupPrefix() );
	    for ( const QString & group : groups )
	    {
#if VERBOSE_MIGRATE
		logVerbose() << "	Migrating " << group << Qt::endl;
#endif
		from->beginGroup( group );
		to->beginGroup( group );

		const auto keys = from->allKeys();
		for ( const QString & key : keys )
		{
#if VERBOSE_MIGRATE
		    logVerbose() << "		Copying " << key << Qt::endl;
#endif
		    to->setValue( key, from->value( key ) );
		}

		to->endGroup();
		from->endGroup();
	    }
	}

	removeGroups( from, to->listGroupPrefix() );
    }


    /**
     * Change the owner of the config file to the user in the $SUDO_UID /
     * $SUDO_GID environment variables (if set).
     **/
    inline void fixFileOwner( const UsedFileList & filenames )
    {
	const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	const QString sudoUid = env.value( "SUDO_UID", QString() );
	const QString sudoGid = env.value( "SUDO_GID", QString() );

	if ( sudoUid.isEmpty() || sudoGid.isEmpty() )
	{
	    logWarning() << "$SUDO_UID / $SUDO_GID not set" << Qt::endl;
	    return;
	}

	const uid_t   uid     = sudoUid.toInt();
	const QString homeDir = SysUtil::homeDir( uid );
	if ( homeDir.isEmpty() )
	{
	    logWarning() << "Can't get home directory for UID " << uid << Qt::endl;
	    return;
	}

	for ( const QString & filename : filenames )
	{
	    if ( filename.startsWith( homeDir ) )
	    {
		const gid_t gid = sudoGid.toInt();

		if ( ::chown( filename.toUtf8(), uid, gid ) != 0 )
		{
		    logError() << "Can't chown " << filename
			       << " to UID "  << uid
			       << " and GID " << gid
			       << ": " << formatErrno()
			       << Qt::endl;
		}
		else
		{
#if 0
		    logDebug() << "Success: chown " << filename
			       << " to UID "  << uid
			       << " and GID " << gid
			       << Qt::endl;
#endif
		}
	    }
	    else
	    {
		logInfo() << "Don't chown " << filename << Qt::endl;
	    }
	}
    }

} // namespace


Settings::Settings( const char * suffix ):
    QSettings ( QCoreApplication::organizationName(), QCoreApplication::applicationName() + suffix )
{
    _usedConfigFiles << fileName();
}


void Settings::fixFileOwners()
{
    if ( SysUtil::runningWithSudo() )
	fixFileOwner( _usedConfigFiles );
}


void Settings::ensureToplevel()
{
    while ( !group().isEmpty() )
	endGroup();
}


QStringList Settings::findListGroups()
{
    ensureToplevel();

    return findGroups( childGroups(), listGroupPrefix() );
}


void Settings::removeListGroups()
{
    ensureToplevel();

    removeGroups( this, listGroupPrefix() );
}


QString Settings::primaryFileName()
{
    QSettings settings;

    return settings.fileName();
}


void Settings::migrate()
{
    Settings commonSettings;

    // Silently skip this if there are no legacy groups in the common settings
    if ( hasGroup( commonSettings.childGroups(), listGroupPrefix() ) )
	moveGroups( &commonSettings, this );
}




CleanupSettings::CleanupSettings():
    Settings( "-cleanup" )
{
    migrate();
}




ExcludeRuleSettings::ExcludeRuleSettings():
    Settings( "-exclude" )
{
    migrate();
}




MimeCategorySettings::MimeCategorySettings():
    Settings( "-mime" )
{
    migrate();
}


