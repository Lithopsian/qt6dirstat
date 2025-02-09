/*
 *   File name: PathSelector.cpp
 *   Summary:   Path selection list widget for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QDir>

#include "PathSelector.h"
#include "FormatUtil.h"
#include "MountPoints.h"


#define SHOW_SIZES_IN_TOOLTIP 0


using namespace QDirStat;


PathSelectorItem::PathSelectorItem( MountPoint * mountPoint, QListWidget * parent ):
    QListWidgetItem{ parent },
    _path{ mountPoint->path() }
{
    QString text = _path % '\n';

    if ( mountPoint->hasSizeInfo() && mountPoint->totalSize() > 0 )
	text += formatSize( mountPoint->totalSize() ) % "  "_L1;

    text += mountPoint->filesystemType();
    setText( text );

    QString tooltip = mountPoint->device();

#if SHOW_SIZES_IN_TOOLTIP
    if ( mountPoint->hasSizeInfo() )
    {
	const QString{ boilerplate = "<tr><td>%1: </td><td align='right'>%2</td></tr>" };
	tooltip += "<br/>" %
	    boilerplate.arg( QObject::tr( "Used" ), formatSize( mountPoint->usedSize() ) ) %
	    boilerplate.arg( QObject::tr( "Free for users" ), formatSize( mountPoint->freeSizeForUser() ) ) %
	    boilerplate.arg( QObject::tr( "Free for root" ), formatSize( mountPoint->freeSizeForRoot() ) );
    }
#endif

    setToolTip( tooltip );
}




PathSelector::PathSelector( QWidget * parent ):
    QListWidget{ parent }
{
    connect( this, &PathSelector::currentItemChanged,
             this, &PathSelector::slotItemSelected );

    connect( this, &PathSelector::itemClicked,
             this, &PathSelector::slotItemSelected );

    connect( this, &PathSelector::itemActivated,
             this, &PathSelector::slotItemDoubleClicked );
}


void PathSelector::addHomeDir()
{
    PathSelectorItem * item = new PathSelectorItem{ QDir::homePath(), this };

    QIcon icon{ ":/icons/48x48/home-dir.png" };
    if ( !icon.isNull() )
	item->setIcon( icon );

    item->setToolTip( tr( "Your home directory" ) );
}


void PathSelector::addNormalMountPoints()
{
    QFileIconProvider iconProvider;

    MountPoints::reload();

    for ( MountPointIterator it{ false } ; *it ; ++it )
    {
	PathSelectorItem * item = new PathSelectorItem{ *it, this };
	const auto type = it->isNetworkMount() ? QFileIconProvider::Network : QFileIconProvider::Drive;
	item->setIcon( iconProvider.icon( type ) );
    }
}


void PathSelector::slotItemSelected( const QListWidgetItem * widgetItem )
{
    const PathSelectorItem * item = dynamic_cast<const PathSelectorItem *>( widgetItem );
    if ( item )
    {
	// logVerbose() << "Selected path " << item->path() << Qt::endl;
	emit pathSelected( item->path() );
    }
}


void PathSelector::slotItemDoubleClicked( const QListWidgetItem * widgetItem )
{
    const PathSelectorItem * item = dynamic_cast<const PathSelectorItem *>( widgetItem );
    if ( item )
    {
	// logVerbose() << "Double-clicked path " << item->path() << Qt::endl;
	emit pathDoubleClicked( item->path() );
    }
}
