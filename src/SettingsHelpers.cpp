/*
 *   File name: SettingsHelpers.cpp
 *   Summary:   Helper functions for QSettings for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QColor>
#include <QWidget>

#include "SettingsHelpers.h"
#include "Settings.h"
#include "Logger.h"


namespace QDirStat
{
    QColor readColorEntry( QSettings     & settings,
			   const QString & entryName,
			   const QColor  & fallback )
    {
	const QColor color( settings.value( entryName ).toString() );
	if ( color.isValid() )
	    return color;

	//logDebug() << "Using fallback for " << entryName << ": " << fallback.name() << Qt::endl;

	return fallback;
    }


    void writeColorEntry( QSettings     & settings,
			  const QString & entryName,
			  const QColor  & color )
    {
	settings.setValue( entryName, color.name() );
    }


    ColorList readColorListEntry( const QSettings & settings,
				  const QString   & entryName,
				  const ColorList & fallback )
    {
	ColorList colorList;

	const QStringList strList = settings.value( entryName ).toStringList();
	for ( const QString & rgb : strList )
	{
	    const QColor color( rgb );

	    if ( color.isValid() )
		colorList << color;
	    else
		logError() << "ERROR in " << entryName << ": \"" << rgb << "\" not a valid color" << Qt::endl;
	}

	return colorList.isEmpty() ? fallback : colorList;
    }


    void writeColorListEntry( QSettings       & settings,
			      const QString   & entryName,
			      const ColorList & colors )
    {
	QStringList strList;

	for ( const QColor & color : colors )
	    strList << color.name();

	settings.setValue( entryName, strList );
    }


    QFont readFontEntry( QSettings     & settings,
			 const QString & entryName,
			 const QFont   & fallback )
    {
	if ( settings.contains( entryName ) )
	{
	    const QString fontName = settings.value( entryName ).toString();
	    QFont font;

	    if ( font.fromString( fontName ) )
		return font;
	}

	//logDebug() << "Using fallback for " << entryName << ": " << fallback.family() << Qt::endl;

	return fallback;
    }


    void writeFontEntry( QSettings     & settings,
			 const QString & entryName,
			 const QFont   & font )
    {
	settings.setValue( entryName, font.toString() );
    }



    int readEnumEntry( const QSettings & settings,
		       const QString   & entryName,
		       int               fallback,
		       const SettingsEnumMapping & enumMapping )
    {
	if ( !settings.contains( entryName ) )
	    return fallback;

	const QString str = settings.value( entryName ).toString();
	for ( auto it = enumMapping.constBegin(); it != enumMapping.constEnd(); ++it )
	{
	    if ( it.value() == str )
		return it.key();
	}

	logError() << "Invalid value for " << entryName << ": \"" << str << "\"" << Qt::endl;

	return fallback;
    }


    void writeEnumEntry( QSettings     & settings,
			 const QString & entryName,
			 int             enumValue,
			 const SettingsEnumMapping & enumMapping )
    {
	if ( !enumMapping.contains( enumValue ) )
	{
	    logError() << "No string for enum value " << enumValue << Qt::endl;
	    return;
	}

	settings.setValue( entryName, enumMapping.value( enumValue ) );
    }


    void readWindowSettings( QWidget * widget, const QString & settingsGroup )
    {
        QDirStat::Settings settings;
        settings.beginGroup( settingsGroup );

        const QPoint winPos  = settings.value( "WindowPos" , QPoint( -99, -99 ) ).toPoint();
        const QSize  winSize = settings.value( "WindowSize", QSize (   0,   0 ) ).toSize();

        if ( winSize.height() > 100 && winSize.width() > 100 )
            widget->resize( winSize );

        if ( winPos.x() != -99 && winPos.y() != -99 )
            widget->move( winPos );

        settings.endGroup();
    }


    void writeWindowSettings( QWidget * widget, const QString & settingsGroup )
    {
        QDirStat::Settings settings;
        settings.beginGroup( settingsGroup );

        settings.setValue( "WindowPos" , widget->pos()  );
        settings.setValue( "WindowSize", widget->size() );

        settings.endGroup();
    }

    void setDefaultValue( QSettings & settings, const QString & key, const QColor & value )
    {
	if ( !settings.contains( key ) )
	    writeColorEntry( settings, key, value );
    }


    void setDefaultValue( QSettings & settings, const QString & key, const QFont & value )
    {
	if ( !settings.contains( key ) )
	    writeFontEntry( settings, key, value );
    }


    void setDefaultValue( QSettings & settings, const QString & key, const ColorList & value )
    {
	if ( !settings.contains( key ) )
	    writeColorListEntry( settings, key, value );
    }

} // namespace QDirStat

