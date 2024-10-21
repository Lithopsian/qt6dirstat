/*
 *   File name: FilesystemsWindow.cpp
 *   Summary:   QDirStat "Mounted Filesystems" window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QClipboard>
#include <QKeyEvent>
#include <QPointer>

#include "FilesystemsWindow.h"
#include "ActionManager.h"
#include "DirTreeModel.h"
#include "FormatUtil.h"
#include "Logger.h"
#include "MountPoints.h"
#include "MainWindow.h"
#include "PanelMessage.h"
#include "QDirStatApp.h"
#include "Settings.h"


#define WARN_PERCENT 10.0


using namespace QDirStat;


namespace
{
    /**
     * Returns the current item in 'tree', cast as a
     * FilesystemItem, or 0 if there is no current item or it is
     * not a FilesystemItem.
     **/
    const FilesystemItem * currentItem( const QTreeWidget * treeWidget)
    {
	return dynamic_cast<const FilesystemItem *>( treeWidget->currentItem() );
    }


    /**
     * Returns the icon filename for the given type of mount point.
     **/
    const char * iconName( const MountPoint * mountPoint )
    {
	if ( mountPoint->isNetworkMount() ) return "network.png";
	if ( mountPoint->isSystemMount()  ) return "system.png";
	if ( mountPoint->isDuplicate()    ) return "bind-mount.png";

	return "mount-point.png";
    }


    /**
     * One-time initialization of the tree widget.
     **/
    void initTree( QTreeWidget * tree )
    {
	app()->dirTreeModel()->setTreeIconSize( tree );

	QTreeWidgetItem * headerItem = tree->headerItem();
	headerItem->setText( FS_DeviceCol,    QObject::tr( "Device" ) );
	headerItem->setText( FS_MountPathCol, QObject::tr( "Mount Point" ) );
	headerItem->setText( FS_TypeCol,      QObject::tr( "Type" ) );
	headerItem->setTextAlignment( FS_DeviceCol,    Qt::AlignVCenter | Qt::AlignLeft );
	headerItem->setTextAlignment( FS_MountPathCol, Qt::AlignVCenter | Qt::AlignLeft );

	if ( MountPoints::hasSizeInfo() )
	{
	    headerItem->setText( FS_TotalSizeCol,    QObject::tr( "Size" ) );
	    headerItem->setText( FS_UsedSizeCol,     QObject::tr( "Used" ) );
	    headerItem->setText( FS_ReservedSizeCol, QObject::tr( "Reserved" ) );
	    headerItem->setText( FS_FreeSizeCol,     QObject::tr( "Free" ) );
	    headerItem->setText( FS_FreePercentCol,  QObject::tr( "Free %" ) );

	    headerItem->setToolTip( FS_ReservedSizeCol, QObject::tr( "Reserved for root" ) );
	    headerItem->setToolTip( FS_FreeSizeCol,     QObject::tr( "Free for unprivileged users" ) );
	}

	// Center the column headers except the first two
	tree->header()->setDefaultAlignment( Qt::AlignCenter );

	tree->sortItems( FS_DeviceCol, Qt::AscendingOrder );
    }

} // namespace


FilesystemsWindow::FilesystemsWindow( QWidget * parent ):
    QDialog{ parent },
    _ui{ new Ui::FilesystemsWindow }
{
    setAttribute( Qt::WA_DeleteOnClose );

    _ui->setupUi( this );

    initTree( _ui->fsTree );

    Settings::readWindowSettings( this, "FilesystemsWindow" );
    enableActions();

    connect( _ui->normalCheckBox, &QCheckBox::toggled,
             this,                &FilesystemsWindow::populate );

    connect( _ui->refreshButton,  &QAbstractButton::clicked,
             this,                &FilesystemsWindow::populate );

    connect( _ui->fsTree,         &QTreeWidget::itemSelectionChanged,
             this,                &FilesystemsWindow::enableActions );

    connect( _ui->readButton,     &QAbstractButton::clicked,
             this,                &FilesystemsWindow::readSelectedFilesystem );

    show();
}


FilesystemsWindow::~FilesystemsWindow()
{
    Settings::writeWindowSettings( this, "FilesystemsWindow" );
}


FilesystemsWindow * FilesystemsWindow::sharedInstance( QWidget * parent )
{
    static QPointer<FilesystemsWindow> _sharedInstance;

    if ( !_sharedInstance )
	_sharedInstance = new FilesystemsWindow{ parent };

    return _sharedInstance;
}


void FilesystemsWindow::populate()
{
    clear();

    MountPoints::reload();

    const bool showAll = !_ui->normalCheckBox->isChecked();
    for ( MountPointIterator it{ showAll }; *it; ++it )
	_ui->fsTree->addTopLevelItem( new FilesystemItem{ *it } );

    _ui->fsTree->setCurrentItem( _ui->fsTree->topLevelItem( 0 ) );

    if ( MountPoints::hasBtrfs() && !_warnedAboutBtrfs )
    {
	_warnedAboutBtrfs = true;
	PanelMessage::showFilesystemsMsg( this, _ui->vBox );
    }

    resizeTreeColumns( _ui->fsTree );
}


void FilesystemsWindow::enableActions()
{
    _ui->readButton->setEnabled( !selectedPath().isEmpty() );
}


void FilesystemsWindow::readSelectedFilesystem()
{
    const QString path = selectedPath();
    if ( !path.isEmpty() )
	app()->mainWindow()->readFilesystem( path );
}


QString FilesystemsWindow::selectedPath() const
{
    const FilesystemItem * item = currentItem( _ui->fsTree );
    if ( item )
	return item->mountPath();

    return QString{};
}


void FilesystemsWindow::keyPressEvent( QKeyEvent * event )
{
    // Let return/enter trigger itemActivated instead of buttons that don't have focus
    if ( ( event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter ) )
	return;

    QDialog::keyPressEvent( event );
}



FilesystemItem::FilesystemItem( MountPoint * mountPoint ):
    QTreeWidgetItem{ UserType },
    _device        { mountPoint->device()          },
    _mountPath     { mountPoint->path()            },
    _fsType        { mountPoint->filesystemType()  },
    _totalSize     { mountPoint->totalSize()       },
    _usedSize      { mountPoint->usedSize()        },
    _reservedSize  { mountPoint->reservedSize()    },
    _freeSize      { mountPoint->freeSizeForUser() },
    _isNetworkMount{ mountPoint->isNetworkMount()  },
    _isReadOnly    { mountPoint->isReadOnly()      }
{
    /**
     * Helper function to set the text and text alignment for a column.
     **/
    const auto set = [ this ]( int col, Qt::Alignment alignment, const QString & text )
    {
	setText( col, text );
	setTextAlignment( col, alignment | Qt::AlignVCenter );
    };

    setIcon( FS_DeviceCol, QIcon{ app()->dirTreeModel()->treeIconDir() % iconName( mountPoint ) } );

    set( FS_DeviceCol,    Qt::AlignLeft,    _device );
    set( FS_TypeCol,      Qt::AlignHCenter, _fsType );
    set( FS_MountPathCol, Qt::AlignLeft,    _mountPath );

    if ( MountPoints::hasSizeInfo() && _totalSize > 0 )
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


QVariant FilesystemItem::data( int column, int role ) const
{
    // This is just for the tooltip on columns that are likely to be long and elided
//    if ( role != Qt::ToolTipRole || ( column != FS_DeviceCol && column != FS_MountPathCol ) )
    if ( role != Qt::ToolTipRole )
	return QTreeWidgetItem::data( column, role );

    return tooltipForElided( this, column, column == FS_DeviceCol ? 1 : 0 );
}


bool FilesystemItem::operator<( const QTreeWidgetItem & rawOther ) const
{
    if ( !treeWidget() )
	return QTreeWidgetItem::operator<( rawOther );

    const FilesystemItem & other = dynamic_cast<const FilesystemItem &>( rawOther );

    switch ( treeWidget()->sortColumn() )
    {
	case FS_DeviceCol:
	    return isNetworkMount() < other.isNetworkMount() || QTreeWidgetItem::operator<( rawOther );

//	case FS_TypeCol:         return fsType()       < other.fsType();
	case FS_TotalSizeCol:    return totalSize()    < other.totalSize();
	case FS_UsedSizeCol:     return usedSize()     < other.usedSize();
	case FS_ReservedSizeCol: return reservedSize() < other.reservedSize();
	case FS_FreeSizeCol:     return freeSize()     < other.freeSize();
	case FS_FreePercentCol:  return freePercent()  < other.freePercent();
//	case FS_MountPathCol:    return mountPath()    < other.mountPath();

	default: return QTreeWidgetItem::operator<( rawOther );
    }
}
