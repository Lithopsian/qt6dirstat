/*
 *   File name: Settings.cpp
 *   Summary:   Specialized settings classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <unistd.h> // chown()
#include <algorithm>

#include <QProcessEnvironment>
#include <QWidget>

#include "Settings.h"
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


QColor Settings::colorValue( const char * key, const QColor & fallback )
{
    const QColor color( value( key ).toString() );
    if ( color.isValid() )
	return color;

    //logDebug() << "Using fallback for " << key << ": " << fallback.name() << Qt::endl;

    return fallback;
}


void Settings::setColorValue( const char * key, const QColor  & color )
{
    setValue( key, color.name() );
}


QFont Settings::fontValue( const char * key, const QFont & fallback )
{
    if ( contains( key ) )
    {
	QFont font;
	if ( font.fromString( value( key ).toString() ) )
	    return font;
    }

    //logDebug() << "Using fallback for " << key << ": " << fallback.family() << Qt::endl;

    return fallback;
}


ColorList Settings::colorListValue( const char * key, const ColorList & fallback )
{
    ColorList valueList;

    const QStringList colorList = value( key ).toStringList();
    for ( const QString & rgb : colorList )
    {
	const QColor color( rgb );

	if ( color.isValid() )
	    valueList << color;
	else
	    logError() << "ERROR in " << key << ": \"" << rgb << "\" not a valid color" << Qt::endl;
    }

    return colorList.isEmpty() ? fallback : valueList;
}


void Settings::setColorListValue( const char * key, const ColorList & colors )
{
    QStringList valueList;

    for ( const QColor & color : colors )
	valueList << color.name();

    setValue( key, valueList );
}


int Settings::enumValue( const char                * key,
                         int                         fallback,
                         const SettingsEnumMapping & enumMapping )
{
    if ( !contains( key ) )
	return fallback;

    const QLatin1String str = QLatin1String( value( key ).toByteArray() );
    const int enumKey = enumMapping.key( str, -1 );
    if ( enumKey >= 0 )
	return enumKey;

    logError() << "Invalid value for " << key << ": \"" << str << '"' << Qt::endl;

    return fallback;
}


void Settings::setEnumValue( const char                * key,
                             int                         enumValue,
                             const SettingsEnumMapping & enumMapping )
{
    if ( !enumMapping.contains( enumValue ) )
    {
	logError() << "No string for enum value " << enumValue << Qt::endl;
	return;
    }

    setValue( key, enumMapping.value( enumValue ) );
}


void Settings::applyActionHotkey( QAction * action )
{
    const QString actionName { action->objectName() };
    if ( actionName.isEmpty() ) // separators, menus, Cleanups, etc., just in case
	return;

    if ( contains( actionName ) )
    {
	const QString shortcut { value( actionName ).toString() };
	const QKeySequence hotkey { shortcut };
	if ( shortcut.isEmpty() || !hotkey.isEmpty() )
	{
	    // Put any empty or valid key sequence on the action even if it's already set
	    action->setShortcut( hotkey.toString() );
	    return;
	}
    }

    // Use the .ui shortcut as the default setting
    setValue( actionName, action->shortcut().toString() );
}


void Settings::readWindowSettings( QWidget * widget, const char * settingsGroup )
{
    QDirStat::Settings settings;

    settings.beginGroup( settingsGroup );
    const QPoint winPos  = settings.value( "WindowPos",  QPoint( -99, -99 ) ).toPoint();
    const QSize  winSize = settings.value( "WindowSize", QSize (   0,   0 ) ).toSize();
    settings.endGroup();

    if ( winSize.height() > 100 && winSize.width() > 100 )
	widget->resize( winSize );

    if ( winPos.x() != -99 && winPos.y() != -99 )
	widget->move( winPos );
}


void Settings::writeWindowSettings( QWidget * widget, const char * settingsGroup )
{
    QDirStat::Settings settings;

    settings.beginGroup( settingsGroup );
    settings.setValue( "WindowPos",  widget->pos()  );
    settings.setValue( "WindowSize", widget->size() );
    settings.endGroup();
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
    Settings { "-cleanup" }
{
    migrate();
}




ExcludeRuleSettings::ExcludeRuleSettings():
    Settings { "-exclude" }
{
    migrate();
}




MimeCategorySettings::MimeCategorySettings():
    Settings { "-mime" }
{
    migrate();
}


