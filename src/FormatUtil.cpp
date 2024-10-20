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
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QScrollBar>
#include <QTextDocument>
#include <QToolTip>
#include <QTreeView>
#include <QTreeWidgetItem>

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
	return lSize == 1 ? oneByte() : QString{ "%1 " }.arg( lSize ) % bytes();
    }

    int    unitIndex = 0;
    double size      = lSize / 1024.0;

    // Restrict to three digits before the decimal point, if possible
    while ( size >= 1000.0 && unitIndex < units.size() - 1 )
    {
	size /= 1024.0;
	++unitIndex;
    }

    return QString::number( size, 'f', precision ) % units.at( unitIndex );
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
    return QLocale{}.toString( time, QLocale::ShortFormat );
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

    const int min = millisec / 60000L; // 60*1000
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


QString QDirStat::replaceCrLf( const QString & text )
{
    if ( !hasLineBreak( text ) )
	return text;

    return QString{ text }.replace( QRegularExpression{ "\r|\n" }, " " );
}


QString & QDirStat::pathTooltip( QString & path )
{
    // Add possible line break points in case a tooltip with no spaces is wider than the screen
    for ( int i = 25; i < path.size(); i += 25 )
	path.insert( i, u'\u200B' ); // zero-width space

    // Stop the tooltip being treated as rich text even if it contains html
    if ( Qt::mightBeRichText( path ) )
	path.replace( u'<', "<\u200B" ); // this can't start an html tag

    return path;
}


void QDirStat::elideLabel( QLabel * label, const QString & text, int lastPixel )
{
    const QFont font = label->font();

    // The text in a frame is indented in addition to the frame width
    const int frameWidth = label->frameWidth();
    const int indent = frameWidth > 0 ? labelFrameIndent( font ) + frameWidth : 0;

    // Fit the text into the space between the left-hand pixel and the right-hand pixel
    const int roomToResize = ellipsisWidth( font );
    label->setText( elidedText( font, text, lastPixel - label->x() - 2 * indent - roomToResize ) );
}


void QDirStat::resizeTreeColumns( QTreeView * tree )
{
    // Try to resize everything to contents
    QHeaderView * header = tree->header();
    header->resizeSections( QHeaderView::ResizeToContents );
    const int contentsWidth = header->sectionSize( 0 );

    // Get the width of the vertical scrollbar if it is visible
    const QScrollBar * scrollbar = tree->verticalScrollBar();
    const bool scrollbarVisible = scrollbar->minimum() != scrollbar->maximum();
    const int scrollbarWidth = scrollbarVisible ? scrollbar->sizeHint().width() : 0;

    // Calculate the space that is available for this column by setting a minimum and then stretching
    const int headerWidth = header->sectionSizeHint( 0 );
    header->resizeSection( 0, headerWidth );
    header->setSectionResizeMode( 0, QHeaderView::Stretch );
    const int stretchedWidth =
	qMax( headerWidth, header->sectionSize( 0 ) - scrollbarWidth - tree->indentation() );

    // Set the width to the minimum of the contents width or the available width
    header->setSectionResizeMode( 0, QHeaderView::Interactive );
    header->resizeSection( 0, qMin( contentsWidth, stretchedWidth ) );
}


QString QDirStat::tooltipForElided( const QTreeWidgetItem * item, int column, int treeLevel )
{
    QTreeWidget * tree = item->treeWidget();

    // Mock up a style option for the item
    QStyleOptionViewItem opt;
    if ( item->icon( column ).isNull() )
	opt.features = QStyleOptionViewItem::HasDisplay;
    else
	opt.features = QStyleOptionViewItem::HasDisplay | QStyleOptionViewItem::HasDecoration;
    opt.decorationSize = tree->iconSize();
    opt.font           = tree->font();
    opt.text           = item->text( column );

    // No tooltip if the column is wider than the item
    const int sectionWidth = tree->header()->sectionSize( column );
    const QSize itemSize = tree->style()->sizeFromContents( QStyle::CT_ItemViewItem, &opt, QSize{} );
    if ( itemSize.width() + tree->indentation() * treeLevel <= sectionWidth )
	return QString{};

    return pathTooltip( opt.text );
}


void QDirStat::tooltipForElided( QRect                      visualRect,
                                 QSize                      sizeHint,
                                 const QAbstractItemModel * model,
                                 const QModelIndex        & index,
                                 QPoint                     pos )
{
    if ( model )
    {
	const QString tooltipText = [ visualRect, sizeHint, model, &index ]()
	{
	    QString modelTooltip = model->data( index, Qt::ToolTipRole ).toString();
	    if ( !modelTooltip.isEmpty() )
		return modelTooltip;

	    if ( visualRect.width() < sizeHint.width() || visualRect.height() < sizeHint.height() )
	    {
		QString text = model->data( index, Qt::DisplayRole ).toString();
		return pathTooltip( text );
	    }

	    return QString{};
	}();

	if ( !tooltipText.isEmpty() )
	{
	    QToolTip::showText( pos, tooltipText );
	    return;
	}
    }

    QToolTip::hideText();
}
