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

#include <QCoreApplication>
#include <QSettings>
#include <QStringList>
#include <QSet>


namespace QDirStat
{
    typedef QSet<QString> UsedFileList;

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

    protected:

	/**
	 * Protected constructor: only used as a delegating
	 * constructor by the default constructor and derived
	 * classes The application (ie. config filename) is set
	 * to QCoreApplication::applicationName() + 'suffix'.
	 **/
	Settings( const char * suffix );


    public:

	/**
	 * Default constructor.  This is the public Settings constructor
	 * and it does not use a suffix, so the base config filename is
	 * used for all groups.
	 **/
	Settings():
	    Settings ( "" )
	{}

	/**
	 * Prefix used to construct section names such as Cleanup_01.
	 * Not normally used in the base class.
	 **/
	virtual const QLatin1String listGroupPrefix() const
	    { return QLatin1String(); }

	/**
	 * Begin a group (section) by using a prefix and number, for
	 * example Cleanup_01.  This is only used normally by derived
	 * classes which  will provide a suitable prefix.
	 **/
	void beginListGroup( int num )
	    { QSettings::beginGroup( listGroupPrefix() + QString( "_%1" ).arg( num, 2, 10, QLatin1Char( '0' ) ) ); }

	/**
	 * Provided to pair with beginListGroup(), although it just calls
	 * QSettings::endGroup().
	 **/
	void endListGroup()
	    { QSettings::endGroup(); }

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
	 * Set the owner of all used config files to the correct one if this
	 * program was started with 'sudo'.
	 **/
	static void fixFileOwners();


    protected:

	/**
	 * Go to the settings top level
	 **/
	void ensureToplevel();

	/**
	 * Migrate settings of the common settings (the main config file) to
	 * this one.  The config file format changed nearly ten years ago, so
	 * this is more or less redundant.
	 **/
	void migrate();


    private:

	// Data members

	static UsedFileList _usedConfigFiles;
    };


    /**
     * Specialized settings class for cleanup actions.
     *
     * Those settings are stored in a separate file so that the entire file
     * can easily be replaced by a site administrator.
     **/
    class CleanupSettings: public Settings
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	CleanupSettings();

	/**
	 * Prefix used to construct section names such as Cleanup_01.
	 **/
	const QLatin1String listGroupPrefix() const override { return QLatin1String( "Cleanup" ); }

    };


    /**
     * Specialized settings class for exclude rules.
     *
     * Those settings are stored in a separate file so that the entire file
     * can easily be replaced by a site administrator.
     **/
    class ExcludeRuleSettings: public Settings
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	ExcludeRuleSettings();

	/**
	 * Prefix used to construct section names such as ExcludeRule_01.
	 **/
	const QLatin1String listGroupPrefix() const override { return QLatin1String( "ExcludeRule" ); }

    };


    /**
     * Specialized settings class for MIME categories.
     *
     * Those settings are stored in a separate file so that the entire file
     * can easily be replaced by a site administrator.
     **/
    class MimeCategorySettings: public Settings
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	MimeCategorySettings();

	/**
	 * Prefix used to construct section names such as MimeCategory_01.
	 **/
	const QLatin1String listGroupPrefix() const override { return QLatin1String( "MimeCategory" ); }

    };

}	// namespace QDirStat

#endif	// Settings_h
