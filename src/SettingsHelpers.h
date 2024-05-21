/*
 *   File name: SettingsHelpers.h
 *   Summary:   Helper functions for QSettings for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef SettingsHelpers_h
#define SettingsHelpers_h

#include <QColor>
#include <QFont>
#include <QList>
#include <QSettings>

#include "Typedefs.h" // ColorList


class QSettings;


namespace QDirStat
{
    typedef QMap<int, QString> SettingsEnumMapping;

    /**
     * Read a color in RGB format (#RRGGBB) from the settings.
     **/
    QColor readColorEntry( QSettings     & settings,
			   const QString & entryName,
			   const QColor  & fallback );

    /**
     * Write a color in RGB format (#RRGGBB) to the settings.
     **/
    void writeColorEntry( QSettings     & settings,
			  const QString & entryName,
			  const QColor  & color );

    /**
     * Read a list of colors in RGB format (#RRGGBB, #RRGGBB, ...) from the
     * settings.
     **/
    ColorList readColorListEntry( const QSettings  & settings,
				  const QString    & entryName,
				  const ColorList  & fallback );

    /**
     * Write a list of colors in RGB format (#RRGGBB, #RRGGBB, ...) to the
     * settings.
     **/
    void writeColorListEntry( QSettings       & settings,
			      const QString   & entryName,
			      const ColorList & colors );

    /**
     * Read a font in string format  from the settings.
     * Example: "DejaVu Sans Mono,10,-1,5,50,0,0,0,0,0"
     **/
    QFont readFontEntry( QSettings     & settings,
                         const QString & entryName,
                         const QFont   & fallback );

    /**
     * Write a font in string format to the settings.
     * Example: "DejaVu Sans Mono,10,-1,5,50,0,0,0,0,0"
     **/
    void writeFontEntry( QSettings     & settings,
                         const QString & entryName,
                         const QFont   & font );


    /**
     * Read an enum value in string format from the settings.
     * 'enumMapping' maps each valid enum value to the corresponding string.
     **/
    int readEnumEntry( const QSettings           & settings,
		       const QString             & entryName,
		       int                         fallback,
		       const SettingsEnumMapping & enumMapping );

    /**
     * Write an enum value in string format to the settings.
     * 'enumMapping' maps each valid enum value to the corresponding string.
     **/
    void writeEnumEntry( QSettings                 & settings,
			 const QString             & entryName,
			 int                         enumValue,
			 const SettingsEnumMapping & enumMapping );

    /**
     * Set a value, but only if that key is not already in the settings.
     **/
    void setDefaultValue( QSettings & settings, const QString & key, const QColor    & value );
//	{ if ( !settings.contains( key ) ) writeColorEntry( settings, key, value ); }
    void setDefaultValue( QSettings & settings, const QString & key, const QFont     & value );
//	{ if ( !settings.contains( key ) ) writeFontEntry( settings, key, value ); }
    void setDefaultValue( QSettings & settings, const QString & key, const ColorList & value );
//	{ if ( !settings.contains( key ) ) writeColorListEntry( settings, key, value ); }

    /**
     * Read window settings (size and position) from the settings and apply
     * them.
     **/
    void readWindowSettings( QWidget       * widget,
                             const QString & settingsGroup );

    /**
     * Write window settings (size and position) to the settings.
     **/
    void writeWindowSettings( QWidget       * widget,
                              const QString & settingsGroup );

}	// namespace QDirStat

#endif	// SettingsHelpers_h
