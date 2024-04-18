/*
 *   File name: UnpkgSettings.h
 *   Summary:   Parameters for "unpackaged files" view
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef UnpkgSettings_h
#define UnpkgSettings_h

#include <QString>
#include <QStringList>
#include <QTextStream>


namespace QDirStat
{
    /**
     * Settings for the "unpackages files" view
     **/
    class UnpkgSettings
    {
    public:

        /**
         * Default constructor, reads the configured settings.
         **/
        UnpkgSettings();

        /**
         * Alternate constructor, uses the four values passed in as
	 * the settings.
         **/
	UnpkgSettings( const QString & startingDir,
		       const QStringList & excludeDirs,
		       const QStringList & ignorePatterns,
		       bool crossFilesystems ):
	    _startingDir { startingDir },
	    _excludeDirs { excludeDirs },
	    _ignorePatterns { ignorePatterns },
	    _crossFilesystems { crossFilesystems }
	{}

        /**
         * Alternate constructor, uses the string passed in as the
	 * startingDir instead of the value from the config file.
         **/
	UnpkgSettings( const QString & startingDir );

        /**
         * Returns the default settings in the form of an UnpkgSettings
	 * object.
         **/
	static UnpkgSettings defaultSettings();

        /**
         * Return the startingDir setting.
         **/
        const QString & startingDir() const { return _startingDir; }

        /**
         * Return the excludeDirs setting.
         **/
        const QStringList & excludeDirs() const { return _excludeDirs; }

        /**
         * Return the ignorePatterns setting.
         **/
        const QStringList & ignorePatterns() const { return _ignorePatterns; }

        /**
         * Return the crossFilesystems setting.  This is entirely independant
	 * of the global cross-filesystems setting and the cross-filesystems
	 * flag used in the Open Directory dialog.
         **/
        bool crossFilesystems() const { return _crossFilesystems; }

	/**
	 * Write settings to the config file
	 **/
	void write();

        /**
         * Dump the values to the log, may be if-def'd out in release code.
         **/
        void dump() const;


    protected:

	/**
	 * Read settings from the config file
	 **/
	void read();


    private:

        //
        // Data members
        //

        QString     _startingDir;
        QStringList _excludeDirs;
        QStringList _ignorePatterns;
	bool        _crossFilesystems;

    };  // UnpkgSettings


}       // namespace QDirStat

#endif  // UnpkgSettings_h
