/*
 *   File name: UnpkgSettings.cpp
 *   Summary:   Parameters for "unpackaged files" view
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "UnpkgSettings.h"
#include "Settings.h"
#include "Logger.h"


using namespace QDirStat;


namespace
{
    // Default value getters
    QString defaultStartingDir()
        { return "/"; }

    QStringList defaultExcludeDirs()
        { return { "/home", "/root", "/tmp", "/var", "/snap", "/usr/lib/sysimage/rpm", "/usr/local" }; }

    QStringList defaultIgnorePatterns()
        { return { "*.pyc" }; }

    bool defaultCrossFilesystems()
        { return false; }

} // namespace


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


void UnpkgSettings::write() const
{
    QDirStat::Settings settings;

    settings.beginGroup( "UnpkgSettings" );
    settings.setValue( "StartingDir",      _startingDir      );
    settings.setValue( "ExcludeDirs",      _excludeDirs      );
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
    logDebug() << "startingDir:      " << _startingDir      << Qt::endl;
    logDebug() << "excludeDirs:      " << _excludeDirs      << Qt::endl;
    logDebug() << "ignorePatterns:   " << _ignorePatterns   << Qt::endl;
    logDebug() << "crossFilesystems: " << _crossFilesystems << Qt::endl;
#endif
}
