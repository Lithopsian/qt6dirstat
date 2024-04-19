/*
 *   File name: FormatUtil.cpp
 *   Summary:   String formatting utilities for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QObject>
#include <QDateTime>
#include <QTextStream>

#include "FormatUtil.h"


using namespace QDirStat;


QString QDirStat::elideMiddle( const QString & text, int maxLen )
{
    if ( maxLen < 4 || text.size() < maxLen )
        return text;

    return text.left( maxLen / 2 ) + "…" + text.right( maxLen / 2 - 1 );
}


QString QDirStat::formatSize( FileSize lSize, int precision )
{
    static QStringList units = { QObject::tr( " bytes" ),
	                         QObject::tr( " kB" ),
	                         QObject::tr( " MB" ),
	                         QObject::tr( " GB" ),
	                         QObject::tr( " TB" ),
	                         QObject::tr( " PB" ),
	                         QObject::tr( " EB" ),
	                         QObject::tr( " ZB" ),
	                         QObject::tr( " YB" )
                               };

    if ( lSize < 1000 )
    {
	// Exact number of bytes, no decimals
	return QString::number( lSize ) + units.at( 0 );
    }
    else
    {
	int    unitIndex = 1;
	double size = lSize / 1024.0;

	// Restrict to three digits before the decimal point
	while ( size >= 1000.0 && unitIndex < units.size() - 1 )
	{
	    size /= 1024.0;
	    ++unitIndex;
	}

	return QString::number( size, 'f', precision ) + units.at( unitIndex );
    }
}


QString QDirStat::formatTime( time_t rawTime )
{
    if ( rawTime == (time_t)0 )
	return "";

#if QT_VERSION < QT_VERSION_CHECK( 5, 8, 0 )
    const QDateTime time = QDateTime::fromTime_t( rawTime );
#else
    const QDateTime time = QDateTime::fromSecsSinceEpoch( rawTime );
#endif
    return QLocale().toString( time, QLocale::ShortFormat );
}


QString QDirStat::symbolicMode( mode_t mode )
{
    QString result;

    // Type
    if	    ( S_ISDIR ( mode ) )	  result = "d";
    else if ( S_ISCHR ( mode ) )	  result = "c";
    else if ( S_ISBLK ( mode ) )	  result = "b";
    else if ( S_ISFIFO( mode ) )	  result = "p";
    else if ( S_ISLNK ( mode ) )	  result = "l";
    else if ( S_ISSOCK( mode ) )	  result = "s";

    // User
    result += ( mode & S_IRUSR ) ? "r" : "-";
    result += ( mode & S_IWUSR ) ? "w" : "-";

    if ( mode & S_ISUID )
	result += "s";
    else
	result += ( mode & S_IXUSR ) ? "x" : "-";

    // Group
    result += ( mode & S_IRGRP ) ? "r" : "-";
    result += ( mode & S_IWGRP ) ? "w" : "-";

    if ( mode & S_ISGID )
	result += "s";
    else if ( mode & S_IXGRP )
	result += "x";
    else
	result += "-";

    // Other
    result += ( mode & S_IROTH ) ? "r" : "-";
    result += ( mode & S_IWOTH ) ? "w" : "-";

    if ( mode & S_ISVTX )
	result += "t";
    else if ( mode & S_IXOTH )
	result += "x";
    else
	result += "-";

    return result;
}


QString QDirStat::formatMillisec( qint64 millisec )
{
    const int hours = millisec / 3600000L;	// 60*60*1000
    millisec %= 3600000L;

    const int min = millisec / 60000L;	// 60*1000
    millisec %= 60000L;

    if ( hours < 1 && min < 1 && millisec < 60000 )
    {
	// .arg doesn't offer enough control over decimal places and significant figures
	// so do it manually, 3 decimal places up to 1 sec, then 1 up to 10 secs, then none
	const int   precision = millisec > 9999 ? 0 : millisec < 1000 ? 3 : 1;
	const float sec       = (float)millisec / 1000.0;
	return QObject::tr( "%1 sec" ).arg( sec, 0, 'f', precision );
    }
    else
    {
	const int sec = millisec / 1000L;
	return QString( "%1:%2:%3" ).arg( hours, 2, 10, QChar( '0' ) )
				    .arg( min,   2, 10, QChar( '0' ) )
				    .arg( sec,   2, 10, QChar( '0' ) );
    }
}


QString QDirStat::monthAbbreviation( short month )
{
    switch ( month )
    {
        case  1: return QObject::tr( "Jan" );
        case  2: return QObject::tr( "Feb" );
        case  3: return QObject::tr( "Mar" );
        case  4: return QObject::tr( "Apr" );
        case  5: return QObject::tr( "May" );
        case  6: return QObject::tr( "Jun" );
        case  7: return QObject::tr( "Jul" );
        case  8: return QObject::tr( "Aug" );
        case  9: return QObject::tr( "Sep" );
        case 10: return QObject::tr( "Oct" );
        case 11: return QObject::tr( "Nov" );
        case 12: return QObject::tr( "Dec" );
	default: return "";
    }
}


void QDirStat::elideLabel( QLabel * label, const QString & text, int maxSize )
{
    // Calculate a width from the dialog less margins, less a bit more
    const QFontMetrics metrics( label->font() );
    label->setText( metrics.elidedText( text, Qt::ElideMiddle, maxSize ) );
}
