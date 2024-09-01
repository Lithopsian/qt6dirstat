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
#include <QLabel>
#include <QLocale>

#include "FormatUtil.h"


using namespace QDirStat;


QString QDirStat::formatSize( FileSize lSize, int precision )
{
    static QStringList units{ QObject::tr( " kB" ),
                              QObject::tr( " MB" ),
                              QObject::tr( " GB" ),
                              QObject::tr( " TB" ),
                              QObject::tr( " PB" ),
                              QObject::tr( " EB" ),
                              QObject::tr( " ZB" ),
                              QObject::tr( " YB" ),
                            };

    if ( lSize < 1000 )
    {
	// Exact number of bytes, no decimals
	return QString::number( lSize ) % ( lSize == 1 ? QObject::tr( " byte" ) : QObject::tr( " bytes" ) );
    }
    else
    {
	int    unitIndex = 0;
	double size = lSize / 1024.0;

	// Restrict to three digits before the decimal point
	while ( size >= 1000.0 && unitIndex < units.size() - 1 )
	{
	    size /= 1024.0;
	    ++unitIndex;
	}

	return QString::number( size, 'f', precision ) % units.at( unitIndex );
    }
}


QString QDirStat::formatTime( time_t rawTime )
{
    if ( rawTime == 0 )
	return QString{};

#if QT_VERSION < QT_VERSION_CHECK( 5, 8, 0 )
    const QDateTime time = QDateTime::fromTime_t( rawTime );
#else
    const QDateTime time = QDateTime::fromSecsSinceEpoch( rawTime );
#endif
    return QLocale().toString( time, QLocale::ShortFormat );
}


QString QDirStat::symbolicMode( mode_t mode )
{
    const QChar type = [ mode ]()
    {
	if ( S_ISDIR ( mode ) ) return u'd';
	if ( S_ISCHR ( mode ) ) return u'c';
	if ( S_ISBLK ( mode ) ) return u'b';
	if ( S_ISFIFO( mode ) ) return u'p';
	if ( S_ISLNK ( mode ) ) return u'l';
	if ( S_ISSOCK( mode ) ) return u's';
	return u' ';
    }();

    // User
    const QChar uRead    = ( mode & S_IRUSR ) ? u'r' : u'-';
    const QChar uWrite   = ( mode & S_IWUSR ) ? u'w' : u'-';
    const QChar uExecute = ( mode & S_ISUID ) ? u's' : ( mode & S_IXUSR ) ? u'x' : u'-';

    // Group
    const QChar gRead    = ( mode & S_IRGRP ) ? u'r' : u'-';
    const QChar gWrite   = ( mode & S_IWGRP ) ? u'w' : u'-';
    const QChar gExecute = ( mode & S_ISGID ) ? u's' : ( mode & S_IXGRP ) ? u'x' : u'-';

    // Other
    const QChar oRead    = ( mode & S_IROTH ) ? u'r' : u'-';
    const QChar oWrite   = ( mode & S_IWOTH ) ? u'w' : u'-';
    const QChar oExecute = ( mode & S_ISVTX ) ? u't' : ( mode & S_IXOTH ) ? u'x' : u'-';

    return type % uRead % uWrite % uExecute % gRead % gWrite % gExecute % oRead % oWrite % oExecute;
}


QString QDirStat::formatMillisec( qint64 millisec )
{
    const int hours = millisec / 3600000L; // 60*60*1000
    millisec %= 3600000L;

    const int min = millisec / 60000L;	// 60*1000
    millisec %= 60000L;

    if ( hours < 1 && min < 1 && millisec < 60000 )
    {
	// .arg doesn't offer enough control over decimal places and significant figures
	// so do it manually, 3 decimal places up to 1 sec, then 1 up to 10 secs, then none
	const int   precision = millisec > 9999 ? 0 : millisec < 1000 ? 3 : 1;
	const float sec       = millisec / 1000.0f;
	return QString::number( sec, 'f', precision) % QObject::tr( " sec" );
    }
    else
    {
	const int sec = millisec / 1000L;
	return QString{ "%1:%2:%3" }.arg( hours, 2, 10, QChar{ u'0' } )
	                            .arg( min,   2, 10, QChar{ u'0' } )
	                            .arg( sec,   2, 10, QChar{ u'0' } );
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
        default: return QString{};
    }
}


QString QDirStat::elideMiddle( const QString & text, int maxLen )
{
    if ( maxLen < 4 || text.size() < maxLen )
        return text;

    return text.left( maxLen / 2 ) % u'…' % text.right( maxLen / 2 - 1 );
}


void QDirStat::elideLabel( QLabel * label, const QString & text, int maxSize )
{
    // Calculate a width from the dialog less margins, less a bit more
    const QFontMetrics metrics{ label->font() };
    label->setText( metrics.elidedText( text, Qt::ElideMiddle, maxSize ) );
}
