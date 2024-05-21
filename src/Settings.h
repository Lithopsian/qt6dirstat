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

#include <QSettings>
#include <QStringList>
#include <QSet>


namespace QDirStat
{
    /**
     * Specialized QSettings subclass for generic settings, i.e. the main
     * config file ~/.config/QDirStat/QDirStat.conf
     *
     * This class takes care of cleaning up leftovers of previous config file
     * formats, for example when certain settings groups (cleanups, mime
     * categories) were moved out from the main config file to specialized
     * config files.
     **/
    class Settings: public QSettings
    {
	Q_OBJECT

    public:

	/**
	 * Create a settings object with the specified name. If 'name' is
	 * empty, the application name is used, i.e. the settings are stored in
	 * the main config file.
	 **/
	Settings( const QString & name = QString() );

	/**
	 * Name of this settings object. This returns an empty string for the
	 * generic settings, i.e. the main config file.
	 **/
	const QString & name() const { return _name; }

	/**
	 * Name of the group prefix of this settings object, e.g. "Cleanup_"
	 * for a derived class that uses settings groups [Cleanup_01],
	 * [Cleanup_02] etc.
	 **/
	const QString & groupPrefix() const { return _groupPrefix; }

	/**
	 * The
	 **/
	void setGroupPrefix( const QString & prefix ) { _groupPrefix = prefix; }

	/**
	 * Overwritten version of beginGroup( const QString & groupName ):
	 *
	 * Begin a settings group with a name with a prefix and a numeric part,
	 * e.g. "Cleanup_01". This also sets the group prefix.
	 * End this with endGroup().
	 **/
	void beginGroup( const QString & groupPrefix, int no );

	/**
	 * Original inherited version of beginGroup
	 **/
	void beginGroup( const QString & groupName )
	    { QSettings::beginGroup( groupName ); }

	/**
	 * Set a value, but only if that key is not already in the settings.
	 **/
	void setDefaultValue( const QString & key, bool value )
	    { if ( !contains( key ) ) setValue( key, value ); }
	void setDefaultValue( const QString & key, int value )
	    { if ( !contains( key ) ) setValue( key, value ); }
	void setDefaultValue( const QString & key, const QString & value )
	    { if ( !contains( key ) ) setValue( key, value ); }

	/**
	 * Find all settings groups that start with 'groupPrefix'.
	 **/
	QStringList findGroups( const QString & groupPrefix );

	/**
	 * Return true if this settings object has any settings group that
	 * starts with 'groupPrefix'.
	 **/
	bool hasGroup( const QString & groupPrefix );

	/**
	 * Remove all settings groups that start with 'groupPrefix'.
	 **/
	void removeGroups( const QString & groupPrefix );

	/**
	 * Go to the settings top level
	 **/
	void ensureToplevel();

	/**
	 * Return the filename of the main settings file
	 **/
	static QString primaryFileName();

	/**
	 * Set the owner of all used config files to the correct one if this
	 * program was started with 'sudo'.
	 **/
	static void fixFileOwners();


    private:

	// Data members

	QString _name;
	QString _groupPrefix;

	static QSet<QString> _usedConfigFiles;
    };


    /**
     * Specialized settings class for cleanup actions.
     *
     * The general idea is that those settings are stored in a separate file so
     * that entire file can easily replaced by a site administrator.
     **/
    class CleanupSettings: public Settings
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	CleanupSettings();


    protected:
	/**
	 * Migrate settings of the common settings (the main config file) to
	 * this one.
	 **/
	void migrate();
    };


    /**
     * Specialized settings class for exclude rules.
     *
     * The general idea is that those settings are stored in a separate file so
     * that entire file can easily replaced by a site administrator.
     **/
    class ExcludeRuleSettings: public Settings
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	ExcludeRuleSettings();


    protected:
	/**
	 * Migrate settings of the common settings (the main config file) to
	 * this one.
	 **/
	void migrate();
    };


    /**
     * Specialized settings class for MIME categories.
     *
     * The general idea is that those settings are stored in a separate file so
     * that entire file can easily replaced by a site administrator.
     **/
    class MimeCategorySettings: public Settings
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	MimeCategorySettings();


    protected:
	/**
	 * Migrate settings of the common settings (the main config file) to
	 * this one.
	 **/
	void migrate();
    };

}	// namespace QDirStat

#endif	// Settings_h
