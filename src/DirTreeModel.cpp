/*
 *   File name: DirTreeModel.cpp
 *   Summary:   Qt data model for directory tree
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "DirTreeModel.h"
#include "DirInfo.h"
#include "DirTree.h"
#include "Exception.h"
#include "ExcludeRules.h"
#include "FileInfoIterator.h"
#include "FileInfoSet.h"
#include "FormatUtil.h"
#include "PkgFilter.h"
#include "Settings.h"


// Number of clusters up to which a file will be considered small and will also
// display the allocated size like "(4k)".
#define SMALL_FILE_CLUSTERS     2


using namespace QDirStat;


namespace
{
    // Debug helpers: should be optimised away if they aren't being used.
    // Verbose logging functions, all callers might be commented out
    [[gnu::unused]] QStringList modelTreeAncestors( const QModelIndex & index )
    {
	QStringList parents;

	for ( QModelIndex parent = index; parent.isValid(); parent = parent.parent() )
	{
	    const QVariant data = parent.data( 0 );
	    if ( data.isValid() )
		parents.prepend( data.toString() );
	}

	return parents;
    }


    void dumpChildren( const FileInfo * dir )
    {
	if ( ! dir )
	    return;


	if ( dir->hasChildren() )
	{
	    logDebug() << "Children of " << dir << "  (" << (void *)dir << ")" << Qt::endl;

	    int count = 0;
	    for ( AtticIterator it{ dir }; *it; ++it )
		logDebug() << "	   #" << count++ << ": " << (void *)*it << "	 " << *it << Qt::endl;
	}
	else
	{
	    logDebug() << "    No children in " << dir << Qt::endl;
	}
    }


    [[gnu::unused]] void dumpPersistentIndexList( const QModelIndexList & persistentList )
    {
	logDebug() << persistentList.size() << " persistent indexes" << Qt::endl;

	for ( const QModelIndex & index : persistentList )
	    logDebug() << index << Qt::endl;
    }


    /**
     * Cast a QModelIndex internal pointer to a FileInfo pointer.
     **/
    FileInfo * internalPointerCast( const QModelIndex & index )
    {
	return static_cast<FileInfo *>( index.internalPointer() );
    }


    /**
     * Return whether the FileInfo item exists and is still valid.
     **/
    bool checkMagicNumber( const FileInfo * item )
    {
	return item && item->checkMagicNumber();
    }


    /**
     * Format a percentage value as string if it is non-negative.
     **/
    QVariant formatPercentQVariant( float percent )
    {
	const QString text = formatPercent( percent );

	return text.isEmpty() ? QVariant{} : text;
    }


    /**
     * For sparse files, return a list of three strings for the delegate:
     * text describing the size, eg. "1.0MB "; text describing the allocated
     * size, eg. "(1.0kB)"; and text describing the number of hard links, eg.
     * " / 3 links", which will be empty if there are not at least 2
     * hard links.
     **/
    QStringList sparseSizeText( FileInfo * item )
    {
	const QString sizeText = formatSize( item->rawByteSize() );
	const QString allocText = " ("_L1 % formatSize( item->rawAllocatedSize() ) % ')';
	const QString linksText = formatLinksInline( item->links() );

	return { sizeText, allocText, linksText };
    }


    /**
     * Return text formatted as "42.0kB / 4 links".  This would normally only
     * be called if the number of hard links is more than one.
     **/
    QString linksSizeText( FileInfo * item )
    {
	return formatSize( item->rawByteSize() ) % formatLinksInline( item->links() );
    }


    /**
     * Return a list containing two strings for the delegate: the size formatted
     * with special for individual bytes, eg "137 B "; and the allocated size in
     * whole kilobytes, eg. "(8k)". This is only intended to be called if
     * useSmallFilSizeText() returns true; in particular, the allocated size is
     * known to be an exact multiple of 1024.
     **/
    QStringList smallSizeText( FileInfo * item )
    {
	const FileSize size = item->size();
	const QString sizeText = size < 1000 ? formatShortByteSize( size ) : formatSize( size );
	const QString allocText = QObject::tr( " (%1k)" ).arg( item->allocatedSize() / 1024 );

	return { sizeText, allocText };
    }


    /**
     * Return 'true' if this is considered a small file or symlink,
     * i.e. non-null, but 2 clusters allocated or less.
     **/
    bool useSmallFileSizeText( FileInfo * item )
    {
	// Not a regular file or symlink doesn't count
	if ( !item || item->blocks() == 0 || !item->isFileOrSymlink() )
	    return false;

	// If no cluster size has been determined yet, we can't say what is a small file
	if ( !item->tree() || !item->tree()->haveClusterSize() )
	    return false;

	// More than 3 allocated clusters isn't "small"
	const FileSize clusterSize = item->tree()->clusterSize();
	const FileSize allocated = item->allocatedSize();
	const FileSize numClusters = allocated / clusterSize;
	if ( numClusters > SMALL_FILE_CLUSTERS + 1 )
	    return false;

	// 3 allocated clusters, but less than 2.5 actually used is "small"
	// 'unused' might be negative for sparse files, but the check will still be valid
	const FileSize unused = allocated - item->rawByteSize();
	if ( numClusters > SMALL_FILE_CLUSTERS && unused <= clusterSize / 2 )
	    return false;

	// Mostly just a sanity-check at this point
	return allocated < 1024 * 1024 &&     // below 1 MB
	       allocated >= 1024 &&           // at least 1k so the (?k) makes sense
	       allocated % 1024 == 0;         // exact number of kB
    }

    /**
     * Return the text for the size column for 'item'
     **/
    QVariant sizeColText( FileInfo * item )
    {
	if ( item->isSpecial() || item->size() < 0 )
	    return QVariant{};

	if ( item->isDirInfo() )
	    return QString{ item->sizePrefix() % formatSize( item->totalAllocatedSize() ) };

	if ( item->isSparseFile() ) // delegate will use SizeTextRole
	    return QVariant{};

	if ( item->links() > 1 )
	    return linksSizeText( item );

	if ( useSmallFileSizeText( item ) ) // delegate will use SizeTextRole
	    return QVariant{};

	// ... and standard formatting for everything else
	return formatSize( item->size() );
    }


    /**
     * Return the formatted tooltip for the size column.
     **/
    QVariant sizeColTooltip( FileInfo * item )
    {
	if ( item->isDirInfo() )
	    return QVariant{ item->sizePrefix() % formatByteSize( item->totalAllocatedSize() ) };

	const QString sizeText = formatByteSize( item->rawByteSize() );
	const QString allocText = [ item ]()
	{
	    if ( item->allocatedSize() == item->rawByteSize() && !item->isSparseFile() )
		return QString{};

	    const QString usedText = item->isSparseFile() ? QObject::tr( "sparse data" ) : QObject::tr( "used" );
	    const QString allocText = formatByteSize( item->rawAllocatedSize() );
	    return QString{ ' ' % usedText % '\n' % allocText % ' ' % QObject::tr( "allocated" ) };
	}();
	const QString linksText = formatLinksRichText( item->links() );

	return whitespacePre( item->sizePrefix() % sizeText % allocText % linksText );
    }


    /**
     * Return the column text for the data() override function.
     **/
    QVariant columnText( DirTree * tree, FileInfo * item, int col )
    {
	if ( item->isPkgInfo() || item->isPseudoDir() )
	{
	    if ( col == UserCol || col == GroupCol || col == PermissionsCol || col == OctalPermissionsCol )
		return QVariant{};
	}

	if ( item->isPkgInfo() && item->readState() == DirAborted )
	{
	    if ( col == PercentBarCol )
		return QObject::tr( "[aborted]" );

	    if ( !item->firstChild() && col != NameCol )
		return "?";
	}

	switch ( col )
	{
	    case NameCol:
	    {
		// Hack to avoid (very rare) line breaks blowing up the (uniform) line size
		if ( item == tree->firstToplevel() )
		    return replaceCrLf( item->name() );

		return item->name();
	    }

	    case PercentBarCol:
	    {
		// Leave to delegate except for special cases
		if ( item->isBusy() )
		    return item->pendingReadJobs() == 1 ?
		           QObject::tr( "[1 read job]" ) :
		           QObject::tr( "[%1 read jobs]" ).arg( item->pendingReadJobs() );

		if ( item->isExcluded() )
		    return QObject::tr( "[excluded]" );

		return QVariant{};
	    }

	    case PercentNumCol:
	    {
		if ( !item->isAttic() && item != tree->firstToplevel() )
		    return formatPercentQVariant( item->subtreeAllocatedPercent() );

		return QVariant{};
	    }

	    case SizeCol:             return sizeColText( item );
	    case LatestMTimeCol:      return formatTime( item->latestMTime() );
	    case UserCol:             return item->userName();
	    case GroupCol:            return item->groupName();
	    case PermissionsCol:      return item->symbolicPermissions();
	    case OctalPermissionsCol: return item->octalPermissions();
	}

	if ( item->isDirInfo() )
	{
	    if ( item->isDotEntry() && col == TotalSubDirsCol )
		return QVariant{};

	    if ( item->subtreeReadError() )
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
		case TotalItemsCol:
		    return QString{ item->sizePrefix() % QString::number( item->totalItems() ) };
		case TotalFilesCol:
		    return QString{ item->sizePrefix() % QString::number( item->totalFiles() ) };
		case TotalSubDirsCol:
		    return QString{ item->sizePrefix() % QString::number( item->totalSubDirs() ) };
		case OldestFileMTimeCol:
		    return formatTime( item->oldestFileMTime() );
	    }
	}

	return QVariant{};
    }


    /**
     * Return the column alignment for the data() override function.
     **/
    QVariant columnAlignment( FileInfo *, int col )
    {
	switch ( col )
	{
	    case NameCol:
		return QVariant{ Qt::AlignVCenter | Qt::AlignLeft };

	    case PercentBarCol:         // just for the special text, the bar aligns itself
	    case LatestMTimeCol:
	    case OldestFileMTimeCol:
		return QVariant{ Qt::AlignVCenter | Qt::AlignHCenter };

//	    case UserCol:
//	    case GroupCol:
//	    case PercentNumCol:
//	    case SizeCol:
//	    case TotalItemsCol:
//	    case TotalFilesCol:
//	    case TotalSubDirsCol:
//	    case PermissionsCol:
//	    case OctalPermissionsCol:
//	    case LatestMTimeCol:
//	    case OldestFileMTimeCol:
//	    default:
	}

	return QVariant{ Qt::AlignVCenter | Qt::AlignRight };
    }


    /**
     * Return the column tooltip for the data() override function.
     **/
    QVariant columnTooltip( FileInfo * item, int col )
    {
	switch ( col )
	{
	    case NameCol:
	    {
		// CR/LF will have been removed from the root name display role so provide the original
		QString name = item->name();
		return hasLineBreak( name ) ? pathTooltip( name ) : QVariant{};
	    }

	    case PercentBarCol:
		return formatPercentQVariant( item->subtreeAllocatedPercent() );

	    case SizeCol:
		return sizeColTooltip( item );

	    case PermissionsCol:
		return item->octalPermissions();

	    case OctalPermissionsCol:
		return item->symbolicPermissions();

	    default:
		return QVariant{};
	}
    }


    /**
     * Return the column font for the data() override function.
     **/
    QVariant columnFont( const QFont & baseFont, FileInfo * item, int col, bool useBoldForDominant )
    {
	switch ( col )
	{
	    case PermissionsCol:
	    {
		QFont font{ baseFont };
		font.setFamily( QStringLiteral( "monospace" ) );
		return font;
	    }

	    case PercentNumCol:
	    case SizeCol:
	    {
		if ( useBoldForDominant && item && item->isDominant() )
		{
		    QFont font{ baseFont };
		    font.setWeight( QFont::Bold );
		    return font;
		}
	    }
	}

	return baseFont;
    }


    /**
     * Return the number of children of a FileInfo item.
     **/
    int childCount( FileInfo * item )
    {
	if ( !item )
	    return 0;

	return item->childCount();
    }


    /**
     * Return 'true' if 'item' or any ancestor (parent or parent's parent
     * etc.) is still busy, i.e. the read job for the directory itself (not
     * any children!) is still queued or currently reading.
     **/
    bool anyAncestorBusy( const FileInfo * item )
    {
	while ( item )
	{
	    if ( item->readState() == DirQueued || item->readState() == DirReading )
		return true;

	    item = item->parent();
	}

	return false;
    }


    /**
     * Percent float value for direct communication with the PercentBarDelegate
     **/
    QVariant percentData( const DirTree * tree, FileInfo * item )
    {
	if ( ( item->parent() && item->parent()->isBusy() ) ||
	     item == tree->firstToplevel() ||
	     item->isAttic() )
	{
	    return QVariant{};
	}

	return item->subtreeAllocatedPercent();
    }


    /**
     * QStringList for direct communication with the SizeColDelegate
     **/
    QVariant sizeTextData( FileInfo * item )
    {
	// List of 3 strings, for sparse files, the third (links) may be empty
	if ( item->isSparseFile() )
	    return sparseSizeText( item );

	// List of 2 strings, for small files with allocations in exact multiples of 1024
	if ( useSmallFileSizeText( item ) && item->links() == 1 )
	    return smallSizeText( item );

	return QVariant{};
    }

} // namespace


DirTreeModel::DirTreeModel( QObject * parent ):
    QAbstractItemModel{ parent }
{
    CHECK_PTR( parent ); // no MainWindow!

    createTree();
    readSettings();
    loadIcons();

    _updateTimer.setInterval( _updateTimerMillisec );

    connect( &_updateTimer, &QTimer::timeout,
             this,          &DirTreeModel::sendPendingUpdates );
}


DirTreeModel::~DirTreeModel()
{
    writeSettings();
}


void DirTreeModel::readSettings()
{
    Settings settings;

    settings.beginGroup( "DirectoryTree" );
    _crossFilesystems         = settings.value( "CrossFilesystems",    false ).toBool();
    _useBoldForDominantItems  = settings.value( "UseBoldForDominant",  true  ).toBool();
    _updateTimerMillisec      = settings.value( "UpdateTimerMillisec", 250  ).toInt();
    _slowUpdateMillisec       = settings.value( "SlowUpdateMillisec",  3000 ).toInt();
    const bool ignoreLinks    = settings.value( "IgnoreHardLinks",     _tree->ignoreHardLinks() ).toBool();
    const bool trustNtfsLinks = settings.value( "TrustNtfsHardLinks",  _tree->trustNtfsHardLinks() ).toBool();
    _treeItemSize =
	dirTreeItemSize( settings.value( "TreeIconDir", DirTreeModel::treeIconDir( DTIS_Small ) ).toString() );
    settings.endGroup();

    settings.beginGroup( "TreeTheme-light" );
    _dirReadErrLightTheme     = settings.colorValue( "DirReadErrColor",     QColor{ 0xdd, 0x00, 0x00 } );
    _subtreeReadErrLightTheme = settings.colorValue( "SubtreeReadErrColor", QColor{ 0xaa, 0x44, 0x44 } );
    settings.endGroup();

    settings.beginGroup( "TreeTheme-dark" );
    _dirReadErrDarkTheme     = settings.colorValue( "DirReadErrColor",     QColor{ 0xff, 0x44, 0xcc } );
    _subtreeReadErrDarkTheme = settings.colorValue( "SubtreeReadErrColor", QColor{ 0xff, 0xaa, 0xdd } );
    settings.endGroup();

    _tree->setCrossFilesystems( _crossFilesystems );
    _tree->setIgnoreHardLinks( ignoreLinks );
    _tree->setTrustNtfsHardLinks( trustNtfsLinks );
}


void DirTreeModel::writeSettings()
{
    Settings settings;

    settings.beginGroup( "DirectoryTree" );
    settings.setValue( "SlowUpdateMillisec",  _slowUpdateMillisec         );
    settings.setValue( "CrossFilesystems",    _crossFilesystems           );
    settings.setValue( "UseBoldForDominant",  _useBoldForDominantItems    );
    settings.setValue( "IgnoreHardLinks",     _tree->ignoreHardLinks()    );
    settings.setValue( "TrustNtfsHardLinks",  _tree->trustNtfsHardLinks() );
    settings.setValue( "TreeIconDir",         treeIconDir()               );
    settings.setValue( "UpdateTimerMillisec", _updateTimerMillisec        );
    settings.endGroup();

    settings.beginGroup( "TreeTheme-light" );
    settings.setColorValue( "DirReadErrColor",     _dirReadErrLightTheme     );
    settings.setColorValue( "SubtreeReadErrColor", _subtreeReadErrLightTheme );
    settings.endGroup();

    settings.beginGroup( "TreeTheme-dark" );
    settings.setColorValue( "DirReadErrColor",     _dirReadErrDarkTheme     );
    settings.setColorValue( "SubtreeReadErrColor", _subtreeReadErrDarkTheme );
    settings.endGroup();
}


void DirTreeModel::updateSettings( bool crossFilesystems,
                                   bool useBoldForDominant,
                                   DirTreeItemSize dirTreeItemSize,
                                   int updateTimerMillisec )
{
    // layoutChanged() is used just to refresh the display, the indexes and order don't change
    // a matching layoutAboutToBechanged() is supposed to be sent, but nobody is actually listening for it
    emit layoutAboutToBeChanged();

    _crossFilesystems = crossFilesystems;
    _useBoldForDominantItems = useBoldForDominant;
    _treeItemSize = dirTreeItemSize;
    _updateTimerMillisec = updateTimerMillisec;
    _updateTimer.setInterval( _slowUpdate ? _slowUpdateMillisec : updateTimerMillisec );

    loadIcons();

    emit layoutChanged();
}


void DirTreeModel::setSlowUpdate()
{
    _slowUpdate = true;
    _updateTimer.setInterval( _slowUpdateMillisec );

    logInfo() << "Display update every " << _slowUpdateMillisec << " millisec" << Qt::endl;
}


void DirTreeModel::createTree()
{
    _tree = new DirTree{ this };

    connect( _tree, &DirTree::startingReading,
             this,  &DirTreeModel::startingRead );

    connect( _tree, &DirTree::startingRefresh,
             this,  &DirTreeModel::startingRead );

    connect( _tree, &DirTree::finished,
             this,  &DirTreeModel::readingFinished );

    connect( _tree, &DirTree::aborted,
             this,  &DirTreeModel::readingFinished );

    connect( _tree, &DirTree::readJobFinished,
             this,  &DirTreeModel::readJobFinished );

    connect( _tree, &DirTree::deletingChildren,
             this,  &DirTreeModel::deletingChildren );

    connect( _tree, &DirTree::childrenDeleted,
             this,  &DirTreeModel::endRemoveRows );

    connect( _tree, &DirTree::clearing,
             this,  &DirTreeModel::beginResetModel );

    connect( _tree, &DirTree::cleared,
             this,  &DirTreeModel::endResetModel );

    connect( _tree, &DirTree::clearingSubtree,
             this,  &DirTreeModel::clearingSubtree );

    connect( _tree, &DirTree::subtreeCleared,
             this,  &DirTreeModel::endRemoveRows );
}


void DirTreeModel::loadIcons()
{
    const QString iconDir = treeIconDir();

    _dirIcon           = QIcon( iconDir % "dir.png"_L1            );
    _dotEntryIcon      = QIcon( iconDir % "dot-entry.png"_L1      );
    _fileIcon          = QIcon( iconDir % "file.png"_L1           );
    _symlinkIcon       = QIcon( iconDir % "symlink.png"_L1        );
    _unreadableDirIcon = QIcon( iconDir % "unreadable-dir.png"_L1 );
    _mountPointIcon    = QIcon( iconDir % "mount-point.png"_L1    );
    _stopIcon          = QIcon( iconDir % "stop.png"_L1           );
    _excludedIcon      = QIcon( iconDir % "excluded.png"_L1       );
    _blockDeviceIcon   = QIcon( iconDir % "block-device.png"_L1   );
    _charDeviceIcon    = QIcon( iconDir % "char-device.png"_L1    );
    _specialIcon       = QIcon( iconDir % "special.png"_L1        );
    _pkgIcon           = QIcon( iconDir % "folder-pkg.png"_L1     );
}


FileInfo * DirTreeModel::findChild( DirInfo * parent, int childNo ) const
{
    CHECK_MAGIC( parent );

    const FileInfoList & sortedChildren = parent->sortedChildren( _sortCol, _sortOrder );
    if ( childNo >= 0 && childNo < sortedChildren.size() )
	return sortedChildren.at( childNo );

    logError() << "Child #" << childNo << " is out of range (0..." << sortedChildren.size()-1
	       << ") for " << parent << Qt::endl;
    dumpChildren( parent );

    return nullptr;
}


//
// Reimplemented from QAbstractItemModel
//

QModelIndex DirTreeModel::index( int row, int column, const QModelIndex & parentIndex ) const
{
    if ( !_tree  || !_tree->root() || !hasIndex( row, column, parentIndex ) )
	return QModelIndex{};

    FileInfo * parent = parentIndex.isValid() ? internalPointerCast( parentIndex ) : _tree->root();
    CHECK_MAGIC( parent );

    if ( parent->isDirInfo() )
	return createIndex( row, column, findChild( parent->toDirInfo(), row ) );

    return QModelIndex{};
}


QModelIndex DirTreeModel::parent( const QModelIndex & index ) const
{
    if ( !index.isValid() )
	return QModelIndex{};

    const FileInfo * child = internalPointerCast( index );
    if ( !checkMagicNumber( child ) )
	return QModelIndex{};

    FileInfo * parent = child->parent();
    if ( !parent || parent == _tree->root() )
	return QModelIndex{};

    const int row = rowNumber( parent );
    // logDebug() << "Parent of " << child << " is " << parent << " #" << row << Qt::endl;

    return createIndex( row, 0, parent );
}


int DirTreeModel::rowCount( const QModelIndex & parentIndex ) const
{
    FileInfo * item = parentIndex.isValid() ? internalPointerCast( parentIndex ) : _tree->root();
    CHECK_MAGIC( item );

    if ( parentIndex.column() <= 0 && item->isDirInfo() && !item->toDirInfo()->isLocked() )
    {
	switch ( item->readState() )
	{
	    case DirQueued:
	    case DirReading:
		// Better keep it simple: Don't report any children until they
		// are complete.
		break;

	    case DirError:
	    case DirNoAccess:
	    case DirPermissionDenied:
		// This is a hybrid case: depending on the dir reader, the dir may
		// or may not be finished at this time. For a local dir, it most
		// likely is; for a cache reader, there might be more to come.
		if ( !_tree->isBusy() )
		    return childCount( item );
		break;

	    case DirFinished:
	    case DirOnRequestOnly:
//	    case DirCached:
	    case DirAborted:
		return childCount( item );

	    // intentionally omitting 'default' case so the compiler can report
	    // missing enum values
	}
    }

    return 0;
}


QVariant DirTreeModel::data( const QModelIndex & index, int role ) const
{
    if ( !index.isValid() )
	return QVariant{};

    FileInfo * item = internalPointerCast( index );
    CHECK_MAGIC( item );

    switch ( role )
    {
	case Qt::DisplayRole: // text
	    if ( item && item->isDirInfo() )
		item->toDirInfo()->touch();

	    return columnText( _tree, item, index.column() );

	case Qt::ForegroundRole: // text colour
	    if ( item->isIgnored() || item->isAttic() )
		return QGuiApplication::palette().brush( QPalette::Disabled, QPalette::WindowText );

	    if ( item->subtreeReadError() )
		return dirReadErrColor();

	    if ( item->errSubDirs() > 0 )
		return subtreeReadErrColor();

	    return QVariant{};

	case Qt::DecorationRole: // icon
	    return columnIcon( item, index.column() );

	case Qt::TextAlignmentRole:
	    return columnAlignment( item, index.column() );

	case Qt::ToolTipRole:
	    return columnTooltip( item, index.column() );

	case Qt::FontRole:
	    return columnFont( _baseFont, item, index.column(), _useBoldForDominantItems );

	case Qt::BackgroundRole:
	    if ( index.column() == NameCol && item->isDirInfo() && item->toDirInfo()->isFromCache() )
		return QGuiApplication::palette().color( QPalette::Active, QPalette::AlternateBase );

	    return QVariant{};

	case PercentRole: // Send percent data to the PercentBarDelegate
	    return percentData( _tree, item );

	case TreeLevelRole: // Send the tree level (ignoring the top two) to the PercentBarDelegate
	    return item->treeLevel() - 2;

	case SizeTextRole: // Send an array of size text strings to the SizeColDelegate
	    return sizeTextData( item );

	default:
	    return QVariant{};
    }
}


QVariant DirTreeModel::headerData( int             section,
                                   Qt::Orientation orientation,
                                   int             role ) const
{
    // Vertical header should never be visible, but ...
    if ( orientation != Qt::Horizontal )
	return QVariant{};

    switch ( role )
    {
	case Qt::DisplayRole:
	    switch ( DataColumns::fromViewCol( section ) )
	    {
		case NameCol:             return tr( "Name"               );
		case PercentBarCol:       return tr( "Subtree Percentage" );
		case PercentNumCol:       return "%";
		case SizeCol:             return tr( "Size"               );
		case TotalItemsCol:       return tr( "Items"              );
		case TotalFilesCol:       return tr( "Files"              );
		case TotalSubDirsCol:     return tr( "Subdirs"            );
		case LatestMTimeCol:      return tr( "Last Modified"      );
		case OldestFileMTimeCol:  return tr( "Oldest File"        );
		case UserCol:             return tr( "User"               );
		case GroupCol:            return tr( "Group"              );
		case PermissionsCol:      return tr( "Permissions"        );
		case OctalPermissionsCol: return tr( "Perm."              );
		default:                  return QVariant{};
	    }

	case Qt::TextAlignmentRole:
	    // Default is AlignHCenter, but use align left for the name header
	    if ( DataColumns::fromViewCol( section ) == NameCol )
		return QVariant{ Qt::AlignVCenter | Qt::AlignLeft };
	    return QVariant{};

	default:
	    return QVariant{};
    }
}


Qt::ItemFlags DirTreeModel::flags( const QModelIndex & index ) const
{
    if ( !index.isValid() )
	return Qt::NoItemFlags;

    const FileInfo * item = internalPointerCast( index );
    CHECK_MAGIC( item );

    if ( item->isDirInfo() )
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    else
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
}


void DirTreeModel::sort( int column, Qt::SortOrder order )
{
    if ( column == _sortCol && order == _sortOrder )
        return;

    // logDebug() << "Before layoutAboutToBeChanged()" << Qt::endl;
    //dumpPersistentIndexList( persistentIndexList() );

    emit layoutAboutToBeChanged( QList<QPersistentModelIndex>(), QAbstractItemModel::VerticalSortHint );
    _sortCol = DataColumns::fromViewCol( column );
    _sortOrder = order;
    updatePersistentIndexes();
    emit layoutChanged( QList<QPersistentModelIndex>(), QAbstractItemModel::VerticalSortHint );

    logInfo() << "Sorting by " << _sortCol
              << ( order == Qt::AscendingOrder ? " ascending" : " descending" )
              << Qt::endl;

    //logDebug() << "After layoutChanged()" << Qt::endl;
    //dumpPersistentIndexList( persistentIndexList() );
}


QVariant DirTreeModel::columnIcon( FileInfo * item, int col ) const
{
    if ( col != NameCol )
	return QVariant{};

    const QIcon icon = itemTypeIcon( item );
    if ( icon.isNull() )
	return QVariant{};

    const bool useDisabled = item->isIgnored() || item->isAttic();
    return icon.pixmap( dirTreeIconSize(), useDisabled ? QIcon::Disabled : QIcon::Normal );
}


QIcon DirTreeModel::itemTypeIcon( FileInfo * item ) const
{
    if ( !item )
        return QIcon{};

    if ( item->readState() == DirAborted )
	return _stopIcon;

    if ( item->isDotEntry()       ) return _dotEntryIcon;
    if ( item->isAttic()          ) return _atticIcon;
    if ( item->isPkgInfo()        ) return _pkgIcon;
    if ( item->isExcluded()       ) return _excludedIcon;
    if ( item->subtreeReadError() ) return _unreadableDirIcon;
    if ( item->isDir()            ) return item->isMountPoint() ? _mountPointIcon : _dirIcon;
    // else FileInfo
    if ( item->isFile()        ) return _fileIcon;
    if ( item->isSymlink()     ) return _symlinkIcon;
    if ( item->isBlockDevice() ) return _blockDeviceIcon;
    if ( item->isCharDevice()  ) return _charDeviceIcon;
    if ( item->isSpecial()     ) return _specialIcon;

    return QIcon{};
}


void DirTreeModel::startingRead()
{
    _updateTimer.start();
}


void DirTreeModel::readJobFinished( DirInfo * dir )
{
    // logDebug() << dir << Qt::endl;
    delayedUpdate( dir );

    if ( !anyAncestorBusy( dir ) )
	newChildrenNotify( dir );
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

    // dumpChildren( dir );
    const int count = childCount( dir );
    if ( count > 0 )
    {
	// logDebug() << "Notifying view about " << count << " new children of " << dir << Qt::endl;

	dir->lock();
	beginInsertRows( modelIndex( dir ), 0, count - 1 );
	dir->unlock();
	endInsertRows();
    }

    // If any readJobFinished signals were ignored because a parent was not
    // finished yet, now is the time to notify the view about those children.
    for ( DirInfoIterator it{ dir }; *it; ++it )
    {
	if ( it->readState() != DirReading && it->readState() != DirQueued )
	    newChildrenNotify( *it );
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

    for ( DirInfo * dir : asConst( _pendingUpdates ) )
    {
	// Not checking the magic numbers as they turn out to be unreliable after a tree clear
	// as the next read may overwrite the same memory with new FileInfos -
	// so just make sure we never get here with stale pointers
	if ( dir && dir != _tree->root() && dir->isTouched() )
	{
	    const int row = rowNumber( dir );
	    const QModelIndex topLeft     = createIndex( row, 0, dir );
	    const QModelIndex bottomRight = createIndex( row, DataColumns::lastCol(), dir );
	    emit dataChanged( topLeft, bottomRight );

	    //logDebug() << "Data changed for " << dir << Qt::endl;

	    // If the view is still interested in this dir, it will fetch data, and touch again
	    dir->clearTouched();
	}
    }

    _pendingUpdates.clear();
}


void DirTreeModel::readingFinished()
{
    // Just abandon any remaining updates
    // The whole display is going to get refreshed when it is sorted
    _updateTimer.stop();
    _pendingUpdates.clear();

    // dumpPersistentIndexList( persistentIndexList() );
    // dumpChildren( _tree->root() );
}


void DirTreeModel::updatePersistentIndexes()
{
    const QModelIndexList persistentList = persistentIndexList();
    for ( const QModelIndex & oldIndex : persistentList )
    {
	if ( oldIndex.isValid() )
	{
	    FileInfo * item = internalPointerCast( oldIndex );
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


void DirTreeModel::beginResetModel()
{
    _updateTimer.stop();
    _pendingUpdates.clear(); // these are dangerous if they arrive from a dead tree

    QAbstractItemModel::beginResetModel();
}


void DirTreeModel::beginRemoveRows( const QModelIndex & parent, int first, int last )
{
    if ( _removingRows )
    {
	logError() << "Removing rows already in progress" << Qt::endl;
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


void DirTreeModel::deletingChildren( DirInfo * parent, const FileInfoSet & children )
{
    const QModelIndex parentIndex = modelIndex( parent );
    int firstRow = rowCount( parentIndex );

    int lastRow = 0;
    for ( const FileInfo * child : children )
    {
	if ( child->parent() == parent ) // just a sanity check, should always be true
	{
	    const int row = rowNumber( child );
	    if ( row > lastRow )
		lastRow = row;
	    if ( row < firstRow )
		firstRow = row;
	}
    }

    //logDebug() << "beginRemoveRows for " << parent << ": rows " << firstRow << "-" << lastRow << Qt::endl;

    beginRemoveRows( parentIndex, firstRow, lastRow );
}


void DirTreeModel::clearingSubtree( DirInfo * subtree )
{
    //logDebug() << "Deleting all children of " << subtree << Qt::endl;

    if ( subtree == _tree->root() || subtree->isTouched() )
    {
	const int count = childCount( subtree );
	if ( count > 0 )
	    beginRemoveRows( modelIndex( subtree ), 0, count - 1 );

	//logDebug() << "beginRemoveRows for " << subtree << " row 0 to " << count - 1 << Qt::endl;
    }
}

#if 0
void DirTreeModel::itemClicked( const QModelIndex & index )
{
    if ( index.isValid() )
    {
	const FileInfo * item = internalPointerCast( index );

	logDebug() << "Clicked row " << index.row()
	           << " col " << index.column()
	           << " (" << DataColumns::fromViewCol( index.column() ) << ")"
	           << "\t" << item
	            << " data(0): " << this->data( index, 0 ).toString()
	           << Qt::endl;
	logDebug() << "Ancestors: " << modelTreeAncestors( index ).join( " -> "_L1 ) << Qt::endl;
    }
    else
    {
	logDebug() << "Invalid model index" << Qt::endl;
    }

    dumpPersistentIndexList( persistentIndexList() );
}
#endif


QModelIndex DirTreeModel::modelIndex( FileInfo * item, int column ) const
{
    if ( checkMagicNumber( item ) && item != _tree->root() )
    {
	const int row = rowNumber( item );
	//logDebug() << item << " is row #" << row << " of " << item->parent() << Qt::endl;
	if ( row >= 0 )
	    return createIndex( row, column, item );
    }

    return QModelIndex{};
}


int DirTreeModel::rowNumber( const FileInfo * child ) const
{
    DirInfo * parent = child->parent();
    if ( !parent )
	return 0;

    const int row = parent->childNumber( _sortCol, _sortOrder, child );
    if ( row < 0 )
    {
	// Not found, should never happen
	logError() << "Child " << child << " (" << (void *)child << ")"
	           << " not found in " << parent << Qt::endl;

	dumpChildren( parent );
    }

    return row;
}
