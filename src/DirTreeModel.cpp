/*
 *   File name: DirTreeModel.cpp
 *   Summary:	Qt data model for directory tree
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <QStringBuilder>

#include "DirTreeModel.h"
#include "DirInfo.h"
#include "DirTree.h"
#include "ExcludeRules.h"
#include "FileInfo.h"
#include "FileInfoIterator.h"
#include "Settings.h"
#include "SettingsHelpers.h"
#include "Logger.h"
#include "FormatUtil.h"
#include "Exception.h"


// Number of clusters up to which a file will be considered small and will also
// display the allocated size like (4k).
#define SMALL_FILE_CLUSTERS     2


using namespace QDirStat;


namespace
{
    // Debug helpers: static so they should disappear if they aren't being used.
    [[gnu::unused]] void dumpDirectChildren( const FileInfo * dir )
    {
	if ( ! dir )
	    return;

	FileInfoIterator it( dir );

	if ( dir->hasChildren() )
	{
	    logDebug() << "Children of " << dir
		       << "  (" << (void *) dir << ")"
		       << Qt::endl;
	    int count = 0;

	    while ( *it )
	    {
		logDebug() << "	   #" << count++ << ": "
			   << (void *) *it
			   << "	 " << *it
			   << Qt::endl;
		++it;
	    }
	}
	else
	{
	    logDebug() << "    No children in " << dir << Qt::endl;
	}
    }


    [[gnu::unused]] void dumpPersistentIndexList( QModelIndexList persistentList )
    {
	logDebug() << persistentList.size() << " persistent indexes" << Qt::endl;

	for ( int i=0; i < persistentList.size(); ++i )
	{
	    const QModelIndex index = persistentList.at(i);

	    FileInfo * item = static_cast<FileInfo *>( index.internalPointer() );
	    CHECK_MAGIC( item );

	    logDebug() << "#" << i
		       << " Persistent index "
		       << index
		       << Qt::endl;
	}
    }


    /**
     * Format a percentage value as string if it is non-negative.
     **/
    QVariant formatPercentQVariant( float percent )
    {
	const QString text = QDirStat::formatPercent( percent );

	return text.isEmpty() ? QVariant() : text;
    }


    /**
     * For sparse files, return a list of three strings for the delegate:
     * text describing the size, eg. "1.0MB "; text describing the allocated
     * size, eg. "(1.0kB)"; and text descriving the number of hard links, eg.
     * " / 3 links", which will be empty if there are not at least 2
     * hard links.
     **/
    QStringList sparseSizeText( FileInfo * item )
    {
	const QString sizeText = formatSize( item->rawByteSize() );
	const QString allocText = " (" % formatSize( item->rawAllocatedSize() ) % ")";
	const QString linksText = formatLinksInline( item->links() );
	return { sizeText, allocText, linksText };
    }


    /**
     * Return text formatted as "42.0kB / 4 links".  This would normally only
     * ve called if the number of hard links is more than one.
     **/
    QString linksSizeText( FileInfo * item )
    {
	return QString( formatSize( item->rawByteSize() ) % formatLinksInline( item->links() ) );
    }


    /**
     * Return a list containing two strings for the delegate: the size formatted
     * with special for individual bytes, eg "137 B "; and the allocated size in
     * whole kilobytes, eg. "(8k)". This is only intended to be called if
     * useSmallFilSizeText() returns true.
     **/
    QStringList smallSizeText( FileInfo * item )
    {
	const FileSize size = item->size();
	const QString sizeText = size < 1000 ? formatShortByteSize( size ) : formatSize( size );
	const QString allocText = QString( " (%1k)" ).arg( item->allocatedSize() / 1024 );
	return { sizeText, allocText };
    }


    /**
     * Return 'true' if this is considered a small file or symlink,
     * i.e. non-null, but 2 clusters allocated or less.
     **/
    bool useSmallFileSizeText( FileInfo * item )
    {
	if ( !item || !item->tree() || item->blocks() == 0 || !( item->isFile() || item->isSymLink() ) )
	    return false;

	const FileSize clusterSize = item->tree()->clusterSize();
	if ( clusterSize == 0 )
	    return false;

	// More than 3 allocated clusters isn't "small"
	const FileSize allocated = item->allocatedSize();
	const int numClusters = allocated / clusterSize;
	if ( numClusters > SMALL_FILE_CLUSTERS + 1 )
	    return false;

	// 3 allocated clusters, but less than 2.5 actually used is "small"
	// 'unused' might be negative for sparse files, but the check will still be valid
	const FileSize unused = allocated - item->rawByteSize();
	if ( numClusters > SMALL_FILE_CLUSTERS && unused <= clusterSize / 2 )
	    return false;

	return allocated < 1024 * 1024 &&     // below 1 MB
	       allocated >= 1024 &&           // at least 1k so the (?k) makes sense
	       allocated % 1024 == 0;         // exact number of kB
    }

} // namespace


DirTreeModel::DirTreeModel( QObject * parent ):
    QAbstractItemModel ( parent )
{
    createTree();
    readSettings();
    loadIcons();

    _updateTimer.setInterval( _updateTimerMillisec );

    connect( &_updateTimer, &QTimer::timeout,
	     this,          &DirTreeModel::sendPendingUpdates );
}


DirTreeModel::~DirTreeModel()
{
    //logDebug() << Qt::endl;
    writeSettings();

    delete _tree;
}


void DirTreeModel::updateSettings( bool crossFilesystems,
                                   bool useBoldForDominant,
				   DirTreeItemSize dirTreeItemSize,
				   int updateTimerMillisec )
{
    // Avoid overwriting the dialog setting unless there is an actual change
    if ( _crossFilesystems != crossFilesystems )
	_tree->setCrossFilesystems( _crossFilesystems );
    _crossFilesystems = crossFilesystems;
    _useBoldForDominantItems = useBoldForDominant;
    _treeItemSize = dirTreeItemSize;
    _updateTimerMillisec = updateTimerMillisec;
    _updateTimer.setInterval( _slowUpdate ? _slowUpdateMillisec : updateTimerMillisec );

    loadIcons();
    setBaseFont( _themeFont );
    emit layoutChanged();
}


void DirTreeModel::readSettings()
{
    Settings settings;

    settings.beginGroup( "DirectoryTree" );
    _crossFilesystems	      = settings.value( "CrossFilesystems",    false ).toBool();
    _useBoldForDominantItems  = settings.value( "UseBoldForDominant",  true  ).toBool();
    _tree->setIgnoreHardLinks(  settings.value( "IgnoreHardLinks",     _tree->ignoreHardLinks() ).toBool() );
    const QString treeIconDir = settings.value( "TreeIconDir",         DirTreeModel::treeIconDir( DTIS_Medium ) ).toString();
    _updateTimerMillisec      = settings.value( "UpdateTimerMillisec", 250  ).toInt();
    _slowUpdateMillisec	      = settings.value( "SlowUpdateMillisec",  3000 ).toInt();
    settings.endGroup();

    settings.beginGroup( "TreeTheme-light" );
    _dirReadErrLightTheme     = readColorEntry( settings, "DirReadErrColor",     QColor( 0xdd, 0x00, 0x00 ) );
    _subtreeReadErrLightTheme = readColorEntry( settings, "SubtreeReadErrColor", QColor( 0xaa, 0x44, 0x44 ) );
    settings.endGroup();

    settings.beginGroup( "TreeTheme-dark" );
    _dirReadErrDarkTheme     = readColorEntry( settings, "DirReadErrColor",     QColor( 0xff, 0x44, 0xcc ) );
    _subtreeReadErrDarkTheme = readColorEntry( settings, "SubtreeReadErrColor", QColor( 0xff, 0xaa, 0xdd ) );
    settings.endGroup();

    _tree->setCrossFilesystems( _crossFilesystems );
    _treeItemSize = dirTreeItemSize( treeIconDir );
}


void DirTreeModel::writeSettings()
{
    Settings settings;

    settings.beginGroup( "DirectoryTree" );
    settings.setValue( "SlowUpdateMillisec",  _slowUpdateMillisec      );
    settings.setValue( "CrossFilesystems",    _crossFilesystems        );
    settings.setValue( "UseBoldForDominant",  _useBoldForDominantItems );
    settings.setValue( "IgnoreHardLinks",     _tree->ignoreHardLinks() );
    settings.setValue( "TreeIconDir",         treeIconDir()            );
    settings.setValue( "UpdateTimerMillisec", _updateTimerMillisec     );
    settings.endGroup();

    settings.beginGroup( "TreeTheme-light" );
    writeColorEntry( settings, "DirReadErrColor",     _dirReadErrLightTheme     );
    writeColorEntry( settings, "SubtreeReadErrColor", _subtreeReadErrLightTheme );
    settings.endGroup();

    settings.beginGroup( "TreeTheme-dark" );
    writeColorEntry( settings, "DirReadErrColor",     _dirReadErrDarkTheme     );
    writeColorEntry( settings, "SubtreeReadErrColor", _subtreeReadErrDarkTheme );
    settings.endGroup();
}


void DirTreeModel::setSlowUpdate()
{
    logInfo() << "Display update every " << _updateTimer.interval() << " millisec" << Qt::endl;

    _slowUpdate = true;
    _updateTimer.setInterval( _slowUpdateMillisec );
}


void DirTreeModel::setBaseFont( const QFont & font )
{
    _themeFont = font;
    _baseFont = font;

    if ( _treeItemSize == DTIS_Medium )
	_baseFont.setPointSizeF( font.pointSizeF() * 1.1 );
}


void DirTreeModel::createTree()
{
    _tree = new DirTree();
    CHECK_NEW( _tree );

    _tree->setExcludeRules();

    connect( _tree, qOverload<>( &DirTree::startingReading ),
	     this,  &DirTreeModel::busyDisplay );

    connect( _tree, &DirTree::finished,
	     this,  &DirTreeModel::readingFinished );

    connect( _tree, &DirTree::aborted,
	     this,  &DirTreeModel::readingFinished );

    connect( _tree, &DirTree::readJobFinished,
	     this,  &DirTreeModel::readJobFinished );

    connect( _tree, &DirTree::deletingChild,
	     this,  &DirTreeModel::deletingChild );

    connect( _tree, &DirTree::clearingSubtree,
	     this,  &DirTreeModel::clearingSubtree );

    connect( _tree, &DirTree::subtreeCleared,
	     this,  &DirTreeModel::subtreeCleared );

    connect( _tree, &DirTree::childDeleted,
	     this,  &DirTreeModel::childDeleted );
}


void DirTreeModel::clear()
{
    if ( _tree )
    {
	beginResetModel();
	// logDebug() << "After beginResetModel()" << Qt::endl;
	// dumpPersistentIndexList( persistentIndexList() );

	_updateTimer.stop();
	_pendingUpdates.clear(); // these are dangerous if they arrive for a new tree
	_tree->clear();

	endResetModel();
	// logDebug() << "After endResetModel()" << Qt::endl;
	// dumpPersistentIndexList( persistentIndexList() );
    }
}


void DirTreeModel::openUrl( const QString & url )
{
    CHECK_PTR( _tree );

    // Need to get rid of pending updates even if there are no tree children
//    if ( _tree->root() && _tree->root()->hasChildren() )
	clear();

    _updateTimer.start();

    _tree->startReading( url );
}


void DirTreeModel::readPkg( const PkgFilter & pkgFilter )
{
    // logDebug() << "Reading " << pkgFilter << Qt::endl;
    CHECK_PTR( _tree );

    // Need to get rid of pending updates even if there are no tree children
//    if ( _tree->root() && _tree->root()->hasChildren() )
	clear();

    _updateTimer.start();

    _tree->readPkg( pkgFilter );
}


void DirTreeModel::loadIcons()
{
    QString iconDir = treeIconDir();

    _dirIcon	       = QIcon( iconDir + "dir.png"            );
    _dotEntryIcon      = QIcon( iconDir + "dot-entry.png"      );
    _fileIcon	       = QIcon( iconDir + "file.png"           );
    _symlinkIcon       = QIcon( iconDir + "symlink.png"        );
    _unreadableDirIcon = QIcon( iconDir + "unreadable-dir.png" );
    _mountPointIcon    = QIcon( iconDir + "mount-point.png"    );
    _stopIcon	       = QIcon( iconDir + "stop.png"           );
    _excludedIcon      = QIcon( iconDir + "excluded.png"       );
    _blockDeviceIcon   = QIcon( iconDir + "block-device.png"   );
    _charDeviceIcon    = QIcon( iconDir + "char-device.png"    );
    _specialIcon       = QIcon( iconDir + "special.png"        );
    _pkgIcon	       = QIcon( iconDir + "folder-pkg.png"     );
}

/*
void DirTreeModel::setColumns( const DataColumnList & columns )
{
    beginResetModel();
    DataColumns::setColumns( columns );
    endResetModel();
}
*/

FileInfo * DirTreeModel::findChild( DirInfo * parent, int childNo ) const
{
    CHECK_PTR( parent );

    const FileInfoList & childrenList = parent->sortedChildren( _sortCol,
								_sortOrder,
								true );	    // includeAttic

    if ( childNo < 0 || childNo >= childrenList.size() )
    {
	logError() << "Child #" << childNo << " is out of range: 0.."
		   << childrenList.size()-1 << " children for "
		   << parent << Qt::endl;

	dumpDirectChildren( parent );

	return nullptr;
    }

    return childrenList.at( childNo );
}


int DirTreeModel::rowNumber( FileInfo * child ) const
{
    if ( !child->parent() )
	return 0;

    const FileInfoList & childrenList = child->parent()->sortedChildren( _sortCol,
									 _sortOrder,
									 true ); // includeAttic

    const int row = childrenList.indexOf( child );
    if ( row < 0 )
    {
	// Not found
	logError() << "Child " << child
		   << " (" << (void *) child << ")"
		   << " not found in \""
		   << child->parent() << "\"" << Qt::endl;

	dumpDirectChildren( child->parent() );
    }

    return row;
}


FileInfo * DirTreeModel::itemFromIndex( const QModelIndex & index )
{
    if ( !index.isValid() )
	return nullptr;

    FileInfo * item = static_cast<FileInfo *>( index.internalPointer() );
    CHECK_MAGIC( item );

    return item;
}



//
// Reimplemented from QAbstractItemModel
//

int DirTreeModel::rowCount( const QModelIndex & parentIndex ) const
{
    if ( !_tree )
	return 0;

    FileInfo * item = nullptr;

    if ( parentIndex.isValid() )
    {
	item = static_cast<FileInfo *>( parentIndex.internalPointer() );
	CHECK_MAGIC( item );
    }
    else
	item = _tree->root();

    if ( !item->isDirInfo() || item->toDirInfo()->isLocked() )
	return 0;

    switch ( item->readState() )
    {
	case DirQueued:
	case DirReading:

	    // Don't mess with directories that are currently being read: If we
	    // tell our view about them, the view might begin fetching model
	    // indexes for them, and when the tree later sends the
	    // readJobFinished() signal, the beginInsertRows() call in our
	    // readJobFinished() slot will confuse the view; it would assume
	    // that the number of children reported in that beginInsertRows()
	    // call needs to be added to the number reported here. We'd have to
	    // keep track how many children we already reported, and how many
	    // new ones to report later.
	    //
	    // Better keep it simple: Don't report any children until they
	    // are complete.

	    break;

	case DirError:
	case DirPermissionDenied:

	    // This is a hybrid case: Depending on the dir reader, the dir may
	    // or may not be finished at this time. For a local dir, it most
	    // likely is; for a cache reader, there might be more to come.

	    if ( !_tree->isBusy() )
		return directChildrenCount( item );
	    break;

	case DirFinished:
	case DirOnRequestOnly:
//	case DirCached:
	case DirAborted:
	    return directChildrenCount( item );

	// intentionally omitting 'default' case so the compiler can report
	// missing enum values
    }

    return 0;
}


QVariant DirTreeModel::data( const QModelIndex & index, int role ) const
{
    if ( !index.isValid() )
	return QVariant();

    const DataColumn col  = DataColumns::fromViewCol( index.column() );
    FileInfo * item = static_cast<FileInfo *>( index.internalPointer() );
    CHECK_MAGIC( item );

    switch ( role )
    {
	case Qt::DisplayRole: // text
	    if ( item && item->isDirInfo() )
		item->toDirInfo()->touch();

	    return columnText( item, col );

	case Qt::ForegroundRole: // text colour
	    if ( item->isIgnored() || item->isAttic() )
		return QGuiApplication::palette().brush( QPalette::Disabled, QPalette::WindowText );

	    if ( item->readError() )
		return dirReadErrColor();

	    if ( item->errSubDirCount() > 0 )
		return subtreeReadErrColor();

	    return QVariant();

	case Qt::DecorationRole: // icon
	    return columnIcon( item, col );

	case Qt::TextAlignmentRole:
	    return columnAlignment( item, col );

	case RawDataRole: // Send raw data to item delegates PercentBarDelegate and SizeColDelegate
	    return columnRawData( item, col );

	case Qt::ToolTipRole:
	    switch ( col )
	    {
		case PercentBarCol:
		    return formatPercentQVariant( item->subtreeAllocatedPercent() );

		case SizeCol:
		    return sizeColTooltip( item );

		case PermissionsCol:
		    return item->octalPermissions();

		case OctalPermissionsCol:
		    return item->symbolicPermissions();

		default:
		    return QVariant();
	    }

	case Qt::FontRole:
	    return columnFont( item, col );

	case Qt::BackgroundRole:
	    if ( col == NameCol && item->isDirInfo() && item->toDirInfo()->isFromCache() )
		return QGuiApplication::palette().color( QPalette::Active, QPalette::AlternateBase );
    }

    return QVariant();
}



QVariant DirTreeModel::sizeColTooltip( FileInfo * item ) const
{
    if ( item->isDirInfo() )
	return item->sizePrefix() + formatByteSize( item->totalAllocatedSize() );

    QString text = item->sizePrefix() + formatByteSize( item->rawByteSize() );

    if ( item->allocatedSize() != item->rawByteSize() || item->isSparseFile() )
    {
	text += QString( " %1<br/>%2 allocated" )
	    .arg( item->isSparseFile() ? "sparse data" : "used" )
	    .arg( formatByteSize( item->rawAllocatedSize() ) );
    }

    return whitespacePre( text + formatLinksRichText( item->links() ) );
}


QVariant DirTreeModel::headerData( int		   section,
				   Qt::Orientation orientation,
				   int		   role ) const
{
    // Vertical header should never be visible, but ...
    if ( orientation != Qt::Horizontal )
	return QVariant();

    switch ( role )
    {
	case Qt::DisplayRole:
	    switch ( DataColumns::fromViewCol( section ) )
	    {
		case NameCol:		  return tr( "Name"		  );
		case PercentBarCol:	  return tr( "Subtree Percentage" );
		case PercentNumCol:	  return tr( "%"		  );
		case SizeCol:		  return tr( "Size"		  );
		case TotalItemsCol:	  return tr( "Items"		  );
		case TotalFilesCol:	  return tr( "Files"		  );
		case TotalSubDirsCol:	  return tr( "Subdirs"		  );
		case LatestMTimeCol:	  return tr( "Last Modified"	  );
		case OldestFileMTimeCol:  return tr( "Oldest File"	  );
		case UserCol:		  return tr( "User"		  );
		case GroupCol:		  return tr( "Group"		  );
		case PermissionsCol:	  return tr( "Permissions"	  );
		case OctalPermissionsCol: return tr( "Perm."		  );
		default: 		  return QVariant();
	    }

	case Qt::TextAlignmentRole:
	    // Default is AlignHCenter, but use align left for the name header
	    if ( DataColumns::fromViewCol( section ) == NameCol )
		return QVariant( Qt::AlignVCenter | Qt::AlignLeft );
	    break;

	// Theme standard font, adjusted for the configured item size
	case Qt::FontRole:
	    return _baseFont;
    }

    return QVariant();
}


Qt::ItemFlags DirTreeModel::flags( const QModelIndex & index ) const
{
    if ( !index.isValid() )
	return Qt::NoItemFlags;

    FileInfo * item = static_cast<FileInfo *>( index.internalPointer() );
    CHECK_MAGIC( item );

    if ( item->isDirInfo() )
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    else
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
}


QModelIndex DirTreeModel::index( int row, int column, const QModelIndex & parentIndex ) const
{
    if ( !_tree  || !_tree->root() || !hasIndex( row, column, parentIndex ) )
	return QModelIndex();

    FileInfo *parent;

    if ( parentIndex.isValid() )
    {
	parent = static_cast<FileInfo *>( parentIndex.internalPointer() );
	CHECK_MAGIC( parent );
    }
    else
	parent = _tree->root();

    if ( parent->isDirInfo() )
    {
	FileInfo * child = findChild( parent->toDirInfo(), row );
	CHECK_PTR( child );

	if ( child )
	    return createIndex( row, column, child );
    }

    return QModelIndex();
}


QModelIndex DirTreeModel::parent( const QModelIndex & index ) const
{
    if ( !index.isValid() )
	return QModelIndex();

    const FileInfo * child = static_cast<FileInfo*>( index.internalPointer() );
    if ( !checkMagicNumber( child ) )
	return QModelIndex();

    FileInfo * parent = child->parent();

    if ( !parent || parent == _tree->root() )
	return QModelIndex();

    const int row = rowNumber( parent );
    // logDebug() << "Parent of " << child << " is " << parent << " #" << row << Qt::endl;

    return createIndex( row, 0, parent );
}


void DirTreeModel::sort( int column, Qt::SortOrder order )
{
    if ( column == _sortCol && order == _sortOrder )
        return;

    logDebug() << "Sorting by " << static_cast<DataColumn>( column )
	       << ( order == Qt::AscendingOrder ? " ascending" : " descending" )
	       << Qt::endl;

    // logDebug() << "Before layoutAboutToBeChanged()" << Qt::endl;
    //dumpPersistentIndexList( persistentIndexList() );

    emit layoutAboutToBeChanged();
    _sortCol = DataColumns::fromViewCol( column );
    _sortOrder = order;
    updatePersistentIndexes();
    emit layoutChanged( QList<QPersistentModelIndex>(), QAbstractItemModel::VerticalSortHint );

    //logDebug() << "After layoutChanged()" << Qt::endl;
    // dumpPersistentIndexList( persistentIndexList() );
}


//---------------------------------------------------------------------------


void DirTreeModel::busyDisplay()
{
    emit layoutAboutToBeChanged();

//    _sortCol = SizeCol;
//    _sortOrder = Qt::DescendingOrder;
//    logDebug() << "Sorting by " << _sortCol << " during reading" << Qt::endl;

    updatePersistentIndexes();
    emit layoutChanged();
}


void DirTreeModel::idleDisplay()
{
    emit layoutAboutToBeChanged();

//    _sortCol = NameCol;
//    _sortOrder = Qt::AscendingOrder;
//    logDebug() << "Sorting by " << _sortCol << " after reading is finished" << Qt::endl;

    updatePersistentIndexes();
    emit layoutChanged();
}


QModelIndex DirTreeModel::modelIndex( FileInfo * item, int column ) const
{
    CHECK_PTR( _tree );
    CHECK_PTR( _tree->root() );

    if ( checkMagicNumber( item ) && item != _tree->root() )
    {
	const int row = rowNumber( item );
	// logDebug() << item << " is row #" << row << " of " << item->parent() << Qt::endl;
	if ( row >= 0 )
	    return createIndex( row, column, item );
    }

    return QModelIndex();
}


QVariant DirTreeModel::columnText( FileInfo * item, int col ) const
{
    CHECK_PTR( item );

    if ( item->isPkgInfo() && item->readState() == DirAborted && !item->firstChild() && col != NameCol )
	return "?";

    switch ( col )
    {
	case PercentBarCol:
	{
	    if ( item->isBusy() )     return tr( "[%1 read jobs]" ).arg( item->pendingReadJobs() );
	    if ( item->isExcluded() ) return tr( "[excluded]" );
	    return QVariant();
	}

	case PercentNumCol:
	{
	    if ( item->isAttic() || item == _tree->firstToplevel() )
		return QVariant();

	    return formatPercentQVariant( item->subtreeAllocatedPercent() );
	}

	case NameCol:		  return item->name();
	case SizeCol:		  return sizeColText( item );
	case LatestMTimeCol:	  return formatTime( item->latestMtime() );
	case UserCol:		  return item->userName();
	case GroupCol:		  return item->groupName();
	case PermissionsCol:	  return item->symbolicPermissions();
	case OctalPermissionsCol: return item->octalPermissions();
    }

    if ( item->isDirInfo() )
    {
	if ( item->isDotEntry() && col == TotalSubDirsCol )
	    return QVariant();

	if ( item->readError() )
	{
	    switch ( col )
	    {
		case TotalItemsCol:
		case TotalFilesCol:
		case TotalSubDirsCol:
		    return "?";
	    }
	}

	switch ( col )
	{
	    case TotalItemsCol:      return QVariant( item->sizePrefix() % QString::number( item->totalItems() ) );
	    case TotalFilesCol:      return QVariant( item->sizePrefix() % QString::number( item->totalFiles() ) );
	    case TotalSubDirsCol:    return QVariant( item->sizePrefix() % QString::number( item->totalSubDirs() ) );
	    case OldestFileMTimeCol: return formatTime( item->oldestFileMtime() );
	}
    }

    return QVariant();
}


QVariant DirTreeModel::columnAlignment( FileInfo *, int col ) const
{
    switch ( col )
    {
	case NameCol:
	    return QVariant( Qt::AlignVCenter | Qt::AlignLeft );

	case PercentBarCol:         // just for the special text, the bar aligns itself
	case LatestMTimeCol:
	case OldestFileMTimeCol:
	    return QVariant( Qt::AlignVCenter | Qt::AlignHCenter );

//	case UserCol:
//	case GroupCol:
//	case PercentNumCol:
//	case SizeCol:
//	case TotalItemsCol:
//	case TotalFilesCol:
//	case TotalSubDirsCol:
//	case PermissionsCol:
//	case OctalPermissionsCol:
//	case LatestMTimeCol:
//	case OldestFileMTimeCol:
//	default:
    }

    return QVariant( Qt::AlignVCenter | Qt::AlignRight );
}


QVariant DirTreeModel::columnFont( FileInfo * item, int col ) const
{
    switch ( col )
    {
	case PermissionsCol:
	{
	    QFont font( _baseFont );
	    font.setFamily( "monospace" );
	    return font;
	}

	case PercentNumCol:
	case SizeCol:
	{
	    if ( _useBoldForDominantItems && item && item->isDominant() )
	    {
		QFont font( _baseFont );
		font.setWeight( QFont::Bold );
		return font;
	    }
	}
    }

    return _baseFont;
}


QVariant DirTreeModel::columnRawData( FileInfo * item, int col ) const
{
    // This is for the delegates, just for two columns

    switch ( col )
    {
//	case NameCol:		  return item->name();
	case PercentBarCol:
	    {
		if ( ( item->parent() && item->parent()->isBusy() ) ||
		     item == _tree->firstToplevel() ||
		     item->isAttic() )
		{
		    return -1.0;
		}

		return item->subtreeAllocatedPercent();
	    }
//	case PercentNumCol:	  return item->subtreeAllocatedPercent();
	case SizeCol:
	    {
		if ( item->isSparseFile() )
		    return sparseSizeText( item );

		if ( useSmallFileSizeText( item ) && item->links() == 1 )
		    return smallSizeText( item );
	    }
//	case TotalItemsCol:	  return item->totalItems();
//	case TotalFilesCol:	  return item->totalFiles();
//	case TotalSubDirsCol:	  return item->totalSubDirs();
//	case LatestMTimeCol:	  return (qulonglong)item->latestMtime();
//	case OldestFileMTimeCol:  return (qulonglong)item->oldestFileMtime();
//	case UserCol:		  return item->uid();
//	case GroupCol:		  return item->gid();
//	case PermissionsCol:	  return item->mode();
//	case OctalPermissionsCol: return item->mode();
//	default:		  return QVariant();
    }

    return QVariant();
}


int DirTreeModel::directChildrenCount( FileInfo * subtree ) const
{
    if ( !subtree )
	return 0;

    int count = subtree->directChildrenCount();

    if ( subtree->attic() )
	++count;

    return count;
}


QVariant DirTreeModel::sizeColText( FileInfo * item ) const
{
    if ( item->isSpecial() )
	return QVariant();

    if ( item->isDirInfo() )
	return QString( item->sizePrefix() % formatSize( item->totalAllocatedSize() ) );

    if ( item->isSparseFile() ) // delegate will use RawDataRole
	return QVariant();

    if ( item->links() > 1 )
	return linksSizeText( item );

    if ( useSmallFileSizeText( item ) ) // delegate will use RawDataRole
	return QVariant();

    // ... and standard formatting for everything else
    return formatSize( item->size() );
}


QVariant DirTreeModel::columnIcon( FileInfo * item, int col ) const
{
    if ( col != NameCol )
	return QVariant();

    const QIcon icon = itemTypeIcon( item );
    if ( icon.isNull() )
	return QVariant();

    const bool useDisabled = item->isIgnored() || item->isAttic();
    return icon.pixmap( dirTreeIconSize(), useDisabled ? QIcon::Disabled : QIcon::Normal );
}


QIcon DirTreeModel::itemTypeIcon( FileInfo * item ) const
{
    if ( !item )
        return QIcon();

    if ( item->readState() == DirAborted ) return _stopIcon;

    if ( item->isDotEntry() ) return _dotEntryIcon;
    if ( item->isAttic()    ) return _atticIcon;
    if ( item->isPkgInfo()  ) return _pkgIcon;
    if ( item->isExcluded() ) return _excludedIcon;
    if ( item->readError()  ) return _unreadableDirIcon;
    if ( item->isDir()      ) return item->isMountPoint() ? _mountPointIcon : _dirIcon;
    // else FileInfo
    if ( item->isFile()	       ) return _fileIcon;
    if ( item->isSymLink()     ) return _symlinkIcon;
    if ( item->isBlockDevice() ) return _blockDeviceIcon;
    if ( item->isCharDevice()  ) return _charDeviceIcon;
    if ( item->isSpecial()     ) return _specialIcon;

    return QIcon();
}


void DirTreeModel::readJobFinished( DirInfo * dir )
{
    // logDebug() << dir << Qt::endl;
    delayedUpdate( dir );

    if ( !anyAncestorBusy( dir ) )
	newChildrenNotify( dir );
//    else if ( dir && !dir->isMountPoint() )
//	logDebug() << "Ancestor busy - ignoring readJobFinished for " << dir << Qt::endl;
}


bool DirTreeModel::anyAncestorBusy( FileInfo * item ) const
{
    while ( item )
    {
	if ( item->readState() == DirQueued || item->readState() == DirReading )
	    return true;

	item = item->parent();
    }

    return false;
}


void DirTreeModel::newChildrenNotify( DirInfo * dir )
{
    if ( !dir )
    {
	logError() << "NULL DirInfo *" << Qt::endl;
	return;
    }

    if ( !dir->isTouched() && dir != _tree->root() && dir != _tree->firstToplevel() )
    {
	// logDebug() << "Remaining silent about untouched dir " << dir << Qt::endl;
	return;
    }

    const QModelIndex index = modelIndex( dir );
    const int count = directChildrenCount( dir );
    // dumpDirectChildren( dir );

    if ( count > 0 )
    {
	// logDebug() << "Notifying view about " << count << " new children of " << dir << Qt::endl;

	dir->lock();
	beginInsertRows( index, 0, count - 1 );
	dir->unlock();
	endInsertRows();
    }

    // If any readJobFinished signals were ignored because a parent was not
    // finished yet, now is the time to notify the view about those children,
    // too.
    for ( FileInfoIterator it( dir ); *it; ++it )
    {
	if ( (*it)->isDirInfo() && (*it)->readState() != DirReading && (*it)->readState() != DirQueued )
	    newChildrenNotify( (*it)->toDirInfo() );
    }
}


void DirTreeModel::delayedUpdate( DirInfo * dir )
{
    while ( dir && dir != _tree->root() )
    {
	if ( dir->isTouched() )
	    _pendingUpdates.insert( dir );

	dir = dir->parent();
    }
}


void DirTreeModel::sendPendingUpdates()
{
    //logDebug() << "Sending " << _pendingUpdates.size() << " updates" << Qt::endl;

    for ( DirInfo * dir : _pendingUpdates )
	dataChangedNotify( dir );

    _pendingUpdates.clear();
}


void DirTreeModel::dataChangedNotify( DirInfo * dir )
{
    // Could check magic number in case any pending updates survived being cleared, but turns out it
    // is unreliable as a tree clear and re-read may overwrite the same memory with some new FileInfos
    if ( !dir || dir == _tree->root() || !dir->isTouched() )
	return;

    const QModelIndex topLeft     = modelIndex( dir );
    const QModelIndex bottomRight = createIndex( topLeft.row(), DataColumns::lastCol(), dir );

    emit dataChanged( topLeft, bottomRight, { Qt::DisplayRole } );

    // logDebug() << "Data changed for " << dir << Qt::endl;

    // If the view is still interested in this dir, it will fetch data, and touch again
    dir->clearTouched();
}


void DirTreeModel::readingFinished()
{
    _updateTimer.stop();
    idleDisplay();
    sendPendingUpdates();

    // dumpPersistentIndexList( persistentIndexList() );
    // dumpDirectChildren( _tree->root(), "root" );
}


void DirTreeModel::updatePersistentIndexes()
{
    const QModelIndexList persistentList = persistentIndexList();
    for ( const QModelIndex oldIndex : persistentList )
    {
	if ( oldIndex.isValid() )
	{
	    FileInfo * item = static_cast<FileInfo *>( oldIndex.internalPointer() );
	    const QModelIndex newIndex = modelIndex( item, oldIndex.column() );
#if 0
	    logDebug() << "Updating " << item
		       << " col " << oldIndex.column()
		       << " row " << oldIndex.row()
		       << " --> " << newIndex.row()
		       << Qt::endl;
#endif
	    changePersistentIndex( oldIndex, newIndex );
	}
    }
}


void DirTreeModel::beginRemoveRows( const QModelIndex & parent, int first, int last )
{
    if ( _removingRows )
    {
	logError() << "Removing rows already in progress" << Qt::endl;
	return;
    }

    if ( !parent.isValid() )
    {
	logError() << "Invalid QModelIndex" << Qt::endl;
	return;
    }

    _removingRows = true;
    QAbstractItemModel::beginRemoveRows( parent, first, last );
}


void DirTreeModel::endRemoveRows()
{
    if ( _removingRows )
    {
	QAbstractItemModel::endRemoveRows();
	_removingRows = false;
    }
}


void DirTreeModel::deletingChild( FileInfo * child )
{
    // logDebug() << "Deleting child " << child << Qt::endl;

    if ( child->parent() && ( child->parent() == _tree->root() || child->parent()->isTouched() ) )
    {
	const QModelIndex parentIndex = modelIndex( child->parent() );
	const int row = rowNumber( child );
	//logDebug() << "beginRemoveRows for " << child << " row " << row << Qt::endl;
	beginRemoveRows( parentIndex, row, row );
    }

    invalidatePersistent( child, true );
}


void DirTreeModel::childDeleted()
{
    endRemoveRows();
}


void DirTreeModel::clearingSubtree( DirInfo * subtree )
{
    //logDebug() << "Deleting all children of " << subtree << Qt::endl;

    if ( subtree == _tree->root() || subtree->isTouched() )
    {
	const QModelIndex subtreeIndex = modelIndex( subtree );
	const int count = directChildrenCount( subtree );
	if ( count > 0 )
	{
	    //logDebug() << "beginRemoveRows for " << subtree << " row 0 to " << count - 1 << Qt::endl;
	    beginRemoveRows( subtreeIndex, 0, count - 1 );
	}
    }

    invalidatePersistent( subtree, false );
}


void DirTreeModel::subtreeCleared( DirInfo * )
{
    endRemoveRows();
}


void DirTreeModel::invalidatePersistent( FileInfo * subtree,
					 bool	    includeParent )
{
    const auto indexList = persistentIndexList();
    for ( const QModelIndex & index : indexList )
    {
	FileInfo * item = static_cast<FileInfo *>( index.internalPointer() );
	CHECK_PTR( item );

	if ( !item->checkMagicNumber() || item->isInSubtree( subtree ) )
	{
	    if ( item != subtree || includeParent )
	    {
		//logDebug() << "Invalidating " << index << Qt::endl;
		changePersistentIndex( index, QModelIndex() );
	    }
	}
    }

}


bool DirTreeModel::checkMagicNumber( const FileInfo * item )
{
    return item && item->checkMagicNumber();
}
