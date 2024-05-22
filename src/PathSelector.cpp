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
#include "MountPoints.h"
#include "FormatUtil.h"
#include "Logger.h"
#include "Exception.h"


#define SHOW_SIZES_IN_TOOLTIP 0


using namespace QDirStat;


PathSelector::PathSelector( QWidget * parent ):
    QListWidget ( parent )
{
    connect( this, &PathSelector::currentItemChanged,
	     this, &PathSelector::slotItemSelected );

    connect( this, &PathSelector::itemClicked,
	     this, &PathSelector::slotItemSelected );

    connect( this, &PathSelector::itemActivated,
	     this, &PathSelector::slotItemDoubleClicked );
}


PathSelectorItem * PathSelector::addPath( const QString & path,
                                          const QIcon   & icon )
{
    PathSelectorItem * item = new PathSelectorItem( path, this );
    CHECK_NEW( item );

    if ( !icon.isNull() )
	item->setIcon( icon );

    return item;
}


void PathSelector::addHomeDir()
{
    PathSelectorItem * item = addPath( QDir::homePath(), QIcon( ":/icons/48x48/home-dir.png" ) );
    item->setToolTip( tr( "Your home directory" ) );
}


void PathSelector::addMountPoint( MountPoint * mountPoint )
{
    CHECK_PTR( mountPoint );

    PathSelectorItem * item = new PathSelectorItem( mountPoint, this );
    CHECK_NEW( item );

    const auto type = mountPoint->isNetworkMount() ? QFileIconProvider::Network : QFileIconProvider::Drive;
    item->setIcon( _iconProvider.icon( type ) );
}


void PathSelector::addMountPoints( const QList<MountPoint *> & mountPoints )
{
    for ( MountPoint * mountPoint : mountPoints )
	addMountPoint( mountPoint );
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

/*
void PathSelector::selectParentMountPoint( const QString & wantedPath )
{
    PathSelectorItem * bestMatch = nullptr;

    for ( int i=0; i < count(); ++i )
    {
        PathSelectorItem * current = dynamic_cast<PathSelectorItem *>( item( i ) );

        if ( current && wantedPath.startsWith( current->path() ) )
        {
            if ( !bestMatch || current->path().length() > bestMatch->path().length() )
                bestMatch = current;
        }
    }

    if ( bestMatch )
    {
        // logDebug() << "Best match: " << bestMatch->path() << Qt::endl;
        setCurrentItem( bestMatch );
    }
}
*/




PathSelectorItem::PathSelectorItem( MountPoint   * mountPoint,
				    PathSelector * parent ):
    QListWidgetItem ( parent ),
    _path { mountPoint->path() }
{
    QString text = _path + "\n";

    if ( mountPoint->hasSizeInfo() && mountPoint->totalSize() > 0 )
	text += formatSize( mountPoint->totalSize() ) + "  ";

    text += mountPoint->filesystemType();
    setText( text );

    QString tooltip = mountPoint->device();

#if SHOW_SIZES_IN_TOOLTIP
    if ( mountPoint->hasSizeInfo() )
    {
        tooltip += QObject::tr( "<br/><tr><td>Used:</td><td align='right'>%1</td></tr>" )
		.arg( formatSize( mountPoint->usedSize() ) );
        tooltip += QObject::tr( "<tr><td>Free for users: </td><td align='right'>%1</td></tr>" )
		.arg( formatSize( mountPoint->freeSizeForUser() ) );
        tooltip += QObject::tr( "<tr><td>Free for root:</td><td align='right'>%1</td></tr>" )
		.arg( formatSize( mountPoint->freeSizeForRoot() ) );
    }
#endif

    setToolTip( tooltip );
}
