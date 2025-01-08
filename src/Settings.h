/*
 *   File name: Settings.h
 *   Summary:   Specialized settings classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef Settings_h
#define Settings_h

#include <QAction>
#include <QColor>
#include <QCoreApplication>
#include <QFont>
#include <QSet>
#include <QSettings>
#include <QStringBuilder>

#include "Typedefs.h" // ColorList


namespace QDirStat
{
    typedef QSet<QString> UsedFileList;
    typedef QMap<int, QLatin1String> SettingsEnumMapping;

    /**
     * Specialized QSettings subclass for generic settings, i.e. the main
     * config file ~/.config/QDirStat/QDirStat.conf
     *
     * There are wrappers for reading and writing colours, colour lists,
     * and fonts, as well as managing window geometry settings.  Note that
     * thhe keys and groups are passed through as raw const char * to get
     * the benefit of using QAnyStringView in Qt6.  This does restrict
     * callers of the API pretty much to hard-coded string literals for
     * the settings keys.
     *
     * Three sub-classes handle the Cleanup, ExcludeRules, and MimeCategory
     * config files.
     *
     * This class also automatically takes care of cleaning up leftovers of
     * previous config file formats, when certain settings groups
     * (cleanups, mime categories) were moved out from the main config
     * file to specialized config files.
     **/
    class Settings : public QSettings
    {
    protected:

	/**
	 * Protected constructor: only used as a delegating
	 * constructor by the default constructor and derived
	 * classes The application (ie. config filename) is set
	 * to QCoreApplication::applicationName() + 'suffix'.
	 **/
	Settings( const char * suffix ):
	    QSettings{ QCoreApplication::organizationName(), QCoreApplication::applicationName() % suffix }
	{ _usedConfigFiles << fileName(); }


    public:

	/**
	 * Default constructor.  This is the public Settings constructor
	 * and it does not use a suffix, so the base config filename is
	 * used for all groups.
	 **/
	Settings():
	    Settings{ "" }
	{}

	/**
	 * Prefix used to construct section names such as Cleanup_01.
	 * Not normally used in the base class.
	 **/
	virtual const QLatin1String listGroupPrefix() const
	    { return QLatin1String{}; }

	/**
	 * Begin a group (section) by using a prefix and number, for
	 * example Cleanup_01.  This is only used normally by derived
	 * classes which  will provide a suitable prefix.
	 **/
	void beginListGroup( int num )
	    { QSettings::beginGroup( listGroupPrefix() % QString{ "_%1" }.arg( num, 2, 10, QChar{ u'0' } ) ); }

	/**
	 * Provided to pair with beginListGroup(), although it just calls
	 * endGroup() in the base class.
	 **/
	void endListGroup()
	    { QSettings::endGroup(); }

	/**
	 * Read a color in RGB format (#RRGGBB) from the settings.
	 **/
	QColor colorValue( const char * key, const QColor & fallback );

	/**
	 * Write a color in RGB format (#RRGGBB) to the settings.
	 **/
	void setColorValue( const char * key, const QColor & color );

	/**
	 * Read a font in string format  from the settings.
	 * Example: "DejaVu Sans Mono,10,-1,5,50,0,0,0,0,0"
	 **/
	QFont fontValue( const char * key, const QFont & fallback );

	/**
	 * Write a font in string format to the settings.
	 * Example: "DejaVu Sans Mono,10,-1,5,50,0,0,0,0,0"
	 *
	 * Note that this is inlined mainly because it isn't used anywhere.
	 * The only font settings is handled using fontValue() and
	 * setDefaultValue().
	 **/
	void setFontValue( const char * key, const QFont & font )
	    { setValue( key, font.toString() ); }

	/**
	 * Read a list of colors in RGB format (#RRGGBB, #RRGGBB, ...) from the
	 * settings.
	 **/
	ColorList colorListValue( const char * key, const ColorList & fallback );

	/**
	 * Write a list of colors in RGB format (#RRGGBB, #RRGGBB, ...) to the
	 * settings.
	 **/
	void setColorListValue( const char * key, const ColorList & colors );

	/**
	 * Read an enum value in string format from the settings.
	 * 'enumMapping' maps each valid enum value to the corresponding string.
	 **/
	int enumValue( const char                * key,
	               int                         fallback,
	               const SettingsEnumMapping & enumMapping );

	/**
	 * Write an enum value in string format to the settings.
	 * 'enumMapping' maps each valid enum value to the corresponding string.
	 **/
	void setEnumValue( const char                * key,
	                   int                         enumValue,
	                   const SettingsEnumMapping & enumMapping );

	/**
	 * Set a value, but only if that key is not already in the settings.
	 * There are overloads for bool, int, QString, QColor, QFont, and
	 * QColorList.
	 **/
	void setDefaultValue( const char * key, bool value )
	    { if ( !contains( key ) ) setValue( key, value ); }
	void setDefaultValue( const char * key, int  value )
	    { if ( !contains( key ) ) setValue( key, value ); }
	void setDefaultValue( const char * key, const QString   & value )
	    { if ( !contains( key ) ) setValue( key, value ); }
	void setDefaultValue( const char * key, const QColor    & value )
	    { if ( !contains( key ) ) setColorValue( key, value ); }
	void setDefaultValue( const char * key, const QFont     & value )
	    { if ( !contains( key ) ) setFontValue( key, value ); }
	void setDefaultValue( const char * key, const ColorList & value )
	    { if ( !contains( key ) ) setColorListValue( key, value ); }

	/**
	 * Read the hotkey setting for an action and apply it if it is a valid
	 * key sequence.  An empty string is valid and means there will be no
	 * hotkey for that action.
	 *
	 * If there is no empty or valid shortcut, then the hotkey already
	 * configured for the action is written to the Settings, so errors are
	 * "corrected" and the settings file will contain a list of all the
	 * configurable actions.
	 **/
	void applyActionHotkey( QAction * action );

	/**
	 * Read window settings (size and position) from the settings and apply
	 * them.
	 **/
	static void readWindowSettings( QWidget    * widget,
	                                const char * settingsGroup );

	/**
	 * Write window settings (size and position) to the settings.
	 **/
	static void writeWindowSettings( const QWidget * widget,
	                                 const char    * settingsGroup );

	/**
	 * Find all settings groups that start with the group prefix
	 * for this object.
	 **/
	QStringList findListGroups();

	/**
	 * Remove all settings groups that start with the group prefix
	 * for this object.
	 **/
	void removeListGroups();

	/**
	 * Return the filename of the main settings file
	 **/
	static QString primaryFileName();

	/**
	 * If the application is running wuth sudo and config files in the
	 * home directory of the original user are being used, set the owner of
	 * all used config files to the original user.
	 *
	 * This is necessary in relatively rare cases where programs running as
	 * root have kept some or all of the environment of the calling user.
	 * This used to happen by default in Ubunutu until 19.10.  Config files
	 * that get written in these cases become owned by root.
	 **/
	static void fixFileOwners();


    protected:

	/**
	 * Go to the settings top level
	 **/
	void ensureToplevel();

	/**
	 * Migrate settings from the common settings (the main config file) to
	 * this one.  The config file format changed nearly ten years ago, so
	 * this is more or less redundant.  The brute-force search for possible
	 * settings to migrate takes a little under 1ms for all three.
	 **/
	void migrate();


    private:

	static UsedFileList _usedConfigFiles;

    };	// class Settings



    /**
     * Specialized settings class for MIME categories.
     *
     * These settings are stored in a separate file so that the entire file
     * can easily be replaced by a site administrator.
     **/
    class MimeCategorySettings final : public Settings
    {
    public:

	/**
	 * Constructor.
	 **/
	MimeCategorySettings();

	/**
	 * Prefix used to construct section names such as MimeCategory_01.
	 **/
	const QLatin1String listGroupPrefix() const override { return "MimeCategory"_L1; }

    };	// class MimeCategorySettings



    /**
     * Specialized settings class for cleanup actions.
     *
     * These settings are stored in a separate file so that the entire file
     * can easily be replaced by a site administrator.
     **/
    class CleanupSettings final : public Settings
    {
    public:

	/**
	 * Constructor.
	 **/
	CleanupSettings();

	/**
	 * Prefix used to construct section names such as Cleanup_01.
	 **/
	const QLatin1String listGroupPrefix() const override { return "Cleanup"_L1; }

    };	// class CleanupSettings



    /**
     * Specialized settings class for exclude rules.
     *
     * These settings are stored in a separate file so that the entire file
     * can easily be replaced by a site administrator.
     **/
    class ExcludeRuleSettings final : public Settings
    {
    public:

	/**
	 * Constructor.
	 **/
	ExcludeRuleSettings();

	/**
	 * Prefix used to construct section names such as ExcludeRule_01.
	 **/
	const QLatin1String listGroupPrefix() const override { return "ExcludeRule"_L1; }

    };	// class ExcludeRuleSettings

}	// namespace QDirStat

#endif	// Settings_h
