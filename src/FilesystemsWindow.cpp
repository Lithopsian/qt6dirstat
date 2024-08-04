/*
 *   File name: FilesystemsWindow.cpp
 *   Summary:   QDirStat "Mounted Filesystems" window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QClipboard>
#include <QContextMenuEvent>
#include <QMenu>
#include <QPointer>

#include "FilesystemsWindow.h"
#include "DirTreeModel.h"
#include "Exception.h"
#include "FormatUtil.h"
#include "MountPoints.h"
#include "HeaderTweaker.h"
#include "MainWindow.h"
#include "PanelMessage.h"
#include "QDirStatApp.h"
#include "Settings.h"


#define WARN_PERCENT 10.0


using namespace QDirStat;


namespace
{
    /**
     * Returns the icon filename for the given type of mount point.
     **/
    const char * icon( const MountPoint * mountPoint )
    {
	if ( mountPoint->isNetworkMount() ) return "network.png";
	if ( mountPoint->isSystemMount()  ) return "system.png";
	if ( mountPoint->isDuplicate()    ) return "bind-mount.png";

	return "mount-point.png";
    }

}


FilesystemsWindow::FilesystemsWindow( QWidget * parent ):
    QDialog { parent },
    _ui { new Ui::FilesystemsWindow }
{
    setAttribute( Qt::WA_DeleteOnClose );

    _ui->setupUi( this );

    MountPoints::reload();
    initWidgets();
    readSettings();

    connect( this,                &FilesystemsWindow::readFilesystem,
             app()->mainWindow(), &MainWindow::readFilesystem );

    connect( _ui->normalCheckBox, &QCheckBox::stateChanged,
             this,                &FilesystemsWindow::refresh );

    connect( _ui->refreshButton,  &QAbstractButton::clicked,
             this,                &FilesystemsWindow::refresh );

    connect( _ui->fsTree,         &QTreeWidget::customContextMenuRequested,
              this,               &FilesystemsWindow::contextMenu);

    connect( _ui->fsTree,         &QTreeWidget::itemDoubleClicked,
             _ui->actionRead,     &QAction::triggered );

    connect( _ui->readButton,     &QAbstractButton::clicked,
             _ui->actionRead,     &QAction::triggered );

    connect( _ui->actionRead,     &QAction::triggered,
             this,                &FilesystemsWindow::readSelectedFilesystem );

    connect( _ui->actionCopy,     &QAction::triggered,
             this,                &FilesystemsWindow::copyDeviceToClipboard );

    connect( _ui->fsTree,         &QTreeWidget::itemSelectionChanged,
             this,                &FilesystemsWindow::enableActions );
}


FilesystemsWindow::~FilesystemsWindow()
{
    Settings::writeWindowSettings( this, "FilesystemsWindow" );
}


FilesystemsWindow * FilesystemsWindow::sharedInstance( QWidget * parent )
{
    static QPointer<FilesystemsWindow> _sharedInstance;

    if ( !_sharedInstance )
	_sharedInstance = new FilesystemsWindow( parent );

    return _sharedInstance;
}


void FilesystemsWindow::readSettings()
{
    Settings::readWindowSettings( this, "FilesystemsWindow" );

    Settings settings;
    settings.beginGroup( "FilesystemsWindow" );
    settings.applyActionHotkey( _ui->actionRead );
    settings.applyActionHotkey( _ui->actionCopy );
    settings.endGroup();
}


void FilesystemsWindow::showBtrfsFreeSizeWarning()
{
    PanelMessage::showFilesystemsMsg( this, _ui->vBox );
}


void FilesystemsWindow::refresh()
{
    MountPoints::reload();
    populate();
}


void FilesystemsWindow::clear()
{
    _ui->fsTree->clear();
}


void FilesystemsWindow::initWidgets()
{
    QStringList headers { tr( "Device" ), tr( "Mount Point" ), tr( "Type" ) };

    if ( MountPoints::hasSizeInfo() )
	headers << tr( "Size" ) << tr( "Used" ) << tr( "Reserved" ) << tr( "Free" ) << tr( "Free %" );

    _ui->fsTree->setHeaderLabels( headers );
    app()->dirTreeModel()->setTreeWidgetSizes( _ui->fsTree );

    // Center the column headers except the first two
    _ui->fsTree->header()->setDefaultAlignment( Qt::AlignVCenter | Qt::AlignHCenter );

    QTreeWidgetItem * hItem = _ui->fsTree->headerItem();
    hItem->setTextAlignment( FS_DeviceCol,    Qt::AlignVCenter | Qt::AlignLeft );
    hItem->setTextAlignment( FS_MountPathCol, Qt::AlignVCenter | Qt::AlignLeft );

    hItem->setToolTip( FS_ReservedSizeCol, tr( "Reserved for root" ) );
    hItem->setToolTip( FS_FreeSizeCol,     tr( "Free for unprivileged users" ) );

    HeaderTweaker::resizeToContents( _ui->fsTree->header() );
    _ui->fsTree->sortItems( FS_DeviceCol, Qt::AscendingOrder );

    enableActions();
}


FilesystemsWindow * FilesystemsWindow::populateSharedInstance( QWidget * parent )
{
    FilesystemsWindow * instance = sharedInstance( parent );
    instance->populate();
    instance->show();

    return instance;
}


void FilesystemsWindow::populate()
{
    clear();

    const bool showAll = !_ui->normalCheckBox->isChecked();
    const auto mountPoints = showAll ? MountPoints::allMountPoints() : MountPoints::normalMountPoints();
    for ( MountPoint * mountPoint : mountPoints )
    {
	CHECK_PTR( mountPoint);

	FilesystemItem * item = new FilesystemItem( mountPoint, _ui->fsTree );
	item->setIcon( 0, QIcon( app()->dirTreeModel()->treeIconDir() + icon( mountPoint ) ) );
    }

    if ( MountPoints::hasBtrfs() )
	showBtrfsFreeSizeWarning();
}


void FilesystemsWindow::enableActions()
{
    _ui->readButton->setEnabled( !selectedPath().isEmpty() );
}


void FilesystemsWindow::readSelectedFilesystem()
{
    const QString path = selectedPath();
    if ( !path.isEmpty() )
    {
	//logDebug() << "Read " << path << Qt::endl;
	emit readFilesystem( path );
    }
}


QString FilesystemsWindow::selectedPath() const
{
    const QList<QTreeWidgetItem *> sel = _ui->fsTree->selectedItems();
    if ( !sel.isEmpty() )
    {
	const FilesystemItem * item = dynamic_cast<FilesystemItem *>( sel.first() );
	if ( item )
	    return item->mountPath();
    }

    return QString();
}


void FilesystemsWindow::copyDeviceToClipboard()
{
    const FilesystemItem * currentItem = dynamic_cast<FilesystemItem *>( _ui->fsTree->currentItem() );
    if ( currentItem )
	QApplication::clipboard()->setText( currentItem->device().trimmed() );
}


void FilesystemsWindow::contextMenu( const QPoint & pos )
{
    // See if the right click was actually on an item
    if ( !_ui->fsTree->itemAt( pos ) )
	return;

    // The clicked item will always be the current item now
    _ui->actionRead->setText( tr( "Read at " ) + selectedPath() );

    QMenu menu;
    menu.addAction( _ui->actionRead );
    menu.addAction( _ui->actionCopy );
    menu.exec( _ui->fsTree->mapToGlobal( pos ) );
}


void FilesystemsWindow::keyPressEvent( QKeyEvent * event )
{
    if ( !selectedPath().isEmpty() && ( event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter ) )
	readSelectedFilesystem();
    else
	QDialog::keyPressEvent( event );
}



FilesystemItem::FilesystemItem( MountPoint * mountPoint, QTreeWidget * parent ):
    QTreeWidgetItem { parent },
    _device         { mountPoint->device()          },
    _mountPath      { mountPoint->path()            },
    _fsType         { mountPoint->filesystemType()  },
    _totalSize      { mountPoint->totalSize()       },
    _usedSize       { mountPoint->usedSize()        },
    _reservedSize   { mountPoint->reservedSize()    },
    _freeSize       { mountPoint->freeSizeForUser() },
    _isNetworkMount { mountPoint->isNetworkMount()  },
    _isReadOnly     { mountPoint->isReadOnly()      }
{
    QString dev = _device;

    // Cut off insanely long generated device mapper names
    const int limit = sizeof( "/dev/mapper/luks-123456" );
    if ( dev.size() > limit )
    {
	dev = dev.left( limit - 1 ) + "…"; // ellipsis
	setToolTip( FS_DeviceCol, _device );
    }

    set( FS_DeviceCol,    Qt::AlignLeft,    dev );
    set( FS_MountPathCol, Qt::AlignLeft,    _mountPath );
    set( FS_TypeCol,      Qt::AlignHCenter, _fsType );

    if ( parent->columnCount() >= FS_TotalSizeCol && _totalSize > 0 )
    {
	set( FS_TotalSizeCol, Qt::AlignRight, formatSize( _totalSize ) );
	set( FS_UsedSizeCol,  Qt::AlignRight, formatSize( _usedSize  ) );

	if ( _reservedSize > 0 )
	    set( FS_ReservedSizeCol, Qt::AlignRight, formatSize( _reservedSize ) );

	if ( _isReadOnly )
	{
	    set( FS_FreeSizeCol, Qt::AlignHCenter, QObject::tr( "read-only" ) );
	}
	else
	{
	    set( FS_FreeSizeCol, Qt::AlignRight, formatSize( _freeSize ) );

	    if ( _totalSize > 0 )
	    {
		const float percent = freePercent();
		set( FS_FreePercentCol, Qt::AlignRight, formatPercent( percent ) );

		if ( percent < WARN_PERCENT )
		{
		    setForeground( FS_FreeSizeCol,    Qt::red );
		    setForeground( FS_FreePercentCol, Qt::red );
		}
	    }
	}
    }
}


bool FilesystemItem::operator<( const QTreeWidgetItem & rawOther ) const
{
    if ( !treeWidget() )
	return QTreeWidgetItem::operator<( rawOther );

    const FilesystemItem & other = dynamic_cast<const FilesystemItem &>( rawOther );

    switch ( (FilesystemColumns)treeWidget()->sortColumn() )
    {
	case FS_DeviceCol:
	    if ( isNetworkMount() == other.isNetworkMount() )
		return device() < other.device();
	    return isNetworkMount() < other.isNetworkMount();

	case FS_MountPathCol:    return mountPath()    < other.mountPath();
	case FS_TypeCol:         return fsType()       < other.fsType();
	case FS_TotalSizeCol:    return totalSize()    < other.totalSize();
	case FS_UsedSizeCol:     return usedSize()     < other.usedSize();
	case FS_ReservedSizeCol: return reservedSize() < other.reservedSize();
	case FS_FreePercentCol:  return freePercent()  < other.freePercent();
	case FS_FreeSizeCol:     return freeSize()     < other.freeSize();
    }

    return QTreeWidgetItem::operator<( rawOther );
}
