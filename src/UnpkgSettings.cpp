/*
 *   File name: UnpkgSettings.cpp
 *   Summary:	Parameters for "unpackaged files" view
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include "UnpkgSettings.h"
#include "Settings.h"
#include "SettingsHelpers.h"
#include "Logger.h"


using namespace QDirStat;


static QString defaultStartingDir()
    { return "/"; }

static QStringList defaultExcludeDirs()
    { return { "/home", "/root", "/tmp", "/var", "/snap", "/usr/lib/sysimage/rpm", "/usr/local" }; }

static QStringList defaultIgnorePatterns()
    { return { "*.pyc" }; }

static bool defaultCrossFilesystems()
    { return false; }


UnpkgSettings::UnpkgSettings()
{
    read();
}


UnpkgSettings::UnpkgSettings( const QString & startingDir ):
    UnpkgSettings ()
{
    // Override the settings starting dir
    _startingDir = startingDir;
}


void UnpkgSettings::read()
{
    QDirStat::Settings settings;

    settings.beginGroup( "UnpkgSettings" );

    _startingDir      = settings.value( "StartingDir",      defaultStartingDir()      ).toString();
    _excludeDirs      = settings.value( "ExcludeDirs",      defaultExcludeDirs()      ).toStringList();
    _ignorePatterns   = settings.value( "IgnorePatterns",   defaultIgnorePatterns()   ).toStringList();
    _crossFilesystems = settings.value( "CrossFilesystems", defaultCrossFilesystems() ).toBool();

    settings.endGroup();
}


void UnpkgSettings::write()
{
    QDirStat::Settings settings;

    settings.beginGroup( "UnpkgSettings" );

    settings.setValue( "StartingDir",	   _startingDir      );
    settings.setValue( "ExcludeDirs",	   _excludeDirs      );
    settings.setValue( "IgnorePatterns",   _ignorePatterns   );
    settings.setValue( "CrossFilesystems", _crossFilesystems );

    settings.endGroup();
}


UnpkgSettings UnpkgSettings::defaultSettings()
{
    return UnpkgSettings( defaultStartingDir(),
                          defaultExcludeDirs(),
                          defaultIgnorePatterns(),
                          defaultCrossFilesystems() );
}


void UnpkgSettings::dump() const
{
#if 0
    logDebug() << "startingDir:      " << _startingDir << Qt::endl;
    logDebug() << "excludeDirs:      " << _excludeDirs << Qt::endl;
    logDebug() << "ignorePatterns:   " << _ignorePatterns << Qt::endl;
    logDebug() << "crossFilesystems: " << _crossFilesystems << Qt::endl;
#endif
}
