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
#include "ExcludeRules.h"
#include "FileInfo.h"
#include "FileInfoIterator.h"
#include "FileInfoSet.h"
#include "FormatUtil.h"
#include "PkgFilter.h"
#include "Settings.h"
#include "SettingsHelpers.h"
#include "Logger.h"
#include "Exception.h"


// Number of clusters up to which a file will be considered small and will also
// display the allocated size like "(4k)".
#define SMALL_FILE_CLUSTERS     2


using namespace QDirStat;


namespace
{
    // Debug helpers: static so they should disappear if they aren't being used.
    // Verbose logging functions, all callers might be commented out
    [[gnu::unused]] QStringList modelTreeAncestors( const QModelIndex & index )
    {
	QStringList parents;

	for ( QModelIndex parent = index; parent.isValid(); parent = index.model()->parent( parent ) )
	{
	    const QVariant data = index.model()->data( parent, 0 );
	    if ( data.isValid() )
		parents.prepend( data.toString() );
	}

	return parents;
    }


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

	for ( const QModelIndex & index : persistentList)
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
	const QString text = QDirStat::formatPercent( percent );

	return text.isEmpty() ? QVariant() : text;
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
	const QString allocText = QString( " (%1)" ).arg( formatSize( item->rawAllocatedSize() ) );
	const QString linksText = formatLinksInline( item->links() );

	return { sizeText, allocText, linksText };
    }


    /**
     * Return text formatted as "42.0kB / 4 links".  This would normally only
     * be called if the number of hard links is more than one.
     **/
    QString linksSizeText( FileInfo * item )
    {
	return QString( formatSize( item->rawByteSize() ) + formatLinksInline( item->links() ) );
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
	if ( !item || item->blocks() == 0 || !( item->isFile() || item->isSymLink() ) )
	    return false;

	// If no cluster size has been determined yet, we can't say what is a small file
	if ( !item->tree() || !item->tree()->haveClusterSize() )
	    return false;

	// More than 3 allocated clusters isn't "small"
	const FileSize clusterSize = item->tree()->clusterSize();
	const FileSize allocated = item->allocatedSize();
	const int numClusters = allocated / clusterSize;
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
	if ( item->isSpecial() )
	    return QVariant();

	if ( item->isDirInfo() )
	    return QString( item->sizePrefix() + formatSize( item->totalAllocatedSize() ) );

	if ( item->isSparseFile() ) // delegate will use SizeTextRole
	    return QVariant();

	if ( item->links() > 1 )
	    return linksSizeText( item );

	if ( useSmallFileSizeText( item ) ) // delegate will use SizeTextRole
	    return QVariant();

	// ... and standard formatting for everything else
	return formatSize( item->size() );
    }

    /**
     * Return the column text for the data() override function.
     **/
    QVariant columnText( DirTree * tree, FileInfo * item, int col )
    {
	if ( item->isPkgInfo() && item->readState() == DirAborted )
	{
	    if ( col == PercentBarCol )                  return QObject::tr( "[aborted]" );
	    if ( !item->firstChild() && col != NameCol ) return "?";
	}

	switch ( col )
	{
	    case PercentBarCol:
	    {
		// Leave to delegate except for special cases
		if ( item->isBusy() )
		    return item->pendingReadJobs() == 1 ?
			   QObject::tr( "[1 read job]" ) :
			   QObject::tr( "[%1 read jobs]" ).arg( item->pendingReadJobs() );

		if ( item->isExcluded() )
		    return QObject::tr( "[excluded]" );

		return QVariant();
	    }

	    case PercentNumCol:
	    {
		if ( item->isAttic() || item == tree->firstToplevel() )
		    return QVariant();

		return formatPercentQVariant( item->subtreeAllocatedPercent() );
	    }

	    case NameCol:             return item->name();
	    case SizeCol:             return sizeColText( item );
	    case LatestMTimeCol:      return formatTime( item->latestMtime() );
	    case UserCol:             return item->userName();
	    case GroupCol:            return item->groupName();
	    case PermissionsCol:      return item->symbolicPermissions();
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
		case TotalItemsCol:      return item->sizePrefix() + QString::number( item->totalItems() );
		case TotalFilesCol:      return item->sizePrefix() + QString::number( item->totalFiles() );
		case TotalSubDirsCol:    return item->sizePrefix() + QString::number( item->totalSubDirs() );
		case OldestFileMTimeCol: return formatTime( item->oldestFileMtime() );
	    }
	}

	return QVariant();
    }


    /**
     * Return the column alignment for the data() override function.
     **/
    QVariant columnAlignment( FileInfo *, int col )
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


    /**
     * Return the column font for the data() override function.
     **/
    QVariant columnFont( const QFont & baseFont,
			 FileInfo    * item,
			 int           col,
			 bool          useBoldForDominantItems )
    {
	switch ( col )
	{
	    case PermissionsCol:
	    {
		QFont font( baseFont );
		font.setFamily( QStringLiteral( "monospace" ) );
		return font;
	    }

	    case PercentNumCol:
	    case SizeCol:
	    {
		if ( useBoldForDominantItems && item && item->isDominant() )
		{
		    QFont font( baseFont );
		    font.setWeight( QFont::Bold );
		    return font;
		}
	    }
	}

	return baseFont;
    }


    /**
     * Return the number of direct children (plus the attic if there is
     * one) of a subtree.
     **/
    int directChildrenCount( FileInfo * subtree )
    {
	if ( !subtree )
	    return 0;

	const int count = subtree->directChildrenCount();
	return subtree->attic() ? count + 1 : count;
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
     * Return the formatted tooltip for the size column.
     **/
    QVariant sizeColTooltip( FileInfo * item )
    {
	if ( item->isDirInfo() )
	    return QVariant( item->sizePrefix() + formatByteSize( item->totalAllocatedSize() ) );

	QString text = item->sizePrefix() + formatByteSize( item->rawByteSize() );

	if ( item->allocatedSize() != item->rawByteSize() || item->isSparseFile() )
	{
	    text += QObject::tr( " %1<br/>%2 allocated" )
		    .arg( item->isSparseFile() ? QObject::tr( "sparse data" ) : QObject::tr( "used" ) )
		    .arg( formatByteSize( item->rawAllocatedSize() ) );
	}

	return whitespacePre( text + formatLinksRichText( item->links() ) );
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
	    return QVariant();
	}

	return item->subtreeAllocatedPercent();
    }


    /**
     * QStringList for direct communication with the SizeColDelegate
     **/
    QVariant sizeTextData( FileInfo * item )
    {
	// List of 3 strings, the third (links) may be empty
	if ( item->isSparseFile() )
	    return sparseSizeText( item );

	// List of 2 strings
	if ( useSmallFileSizeText( item ) && item->links() == 1 )
	    return smallSizeText( item );

	return QVariant();
    }

} // namespace


DirTreeModel::DirTreeModel():
    QAbstractItemModel ()
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
    writeSettings();
}


void DirTreeModel::readSettings()
{
    Settings settings;

    settings.beginGroup( "DirectoryTree" );
    _crossFilesystems        = settings.value( "CrossFilesystems",    false ).toBool();
    _useBoldForDominantItems = settings.value( "UseBoldForDominant",  true  ).toBool();
    _updateTimerMillisec     = settings.value( "UpdateTimerMillisec", 250  ).toInt();
    _slowUpdateMillisec      = settings.value( "SlowUpdateMillisec",  3000 ).toInt();
    _tree->setIgnoreHardLinks( settings.value( "IgnoreHardLinks",     _tree->ignoreHardLinks() ).toBool() );
    _treeItemSize = dirTreeItemSize( settings.value( "TreeIconDir",   DirTreeModel::treeIconDir( DTIS_Small ) ).toString() );
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


void DirTreeModel::updateSettings( bool crossFilesystems,
                                   bool useBoldForDominant,
				   DirTreeItemSize dirTreeItemSize,
				   int updateTimerMillisec )
{
    _crossFilesystems = crossFilesystems;
    _useBoldForDominantItems = useBoldForDominant;
    _treeItemSize = dirTreeItemSize;
    _updateTimerMillisec = updateTimerMillisec;
    _updateTimer.setInterval( _slowUpdate ? _slowUpdateMillisec : updateTimerMillisec );

    loadIcons();
    setBaseFont( _themeFont );
    emit layoutChanged();
}


void DirTreeModel::setSlowUpdate()
{
    _slowUpdate = true;
    _updateTimer.setInterval( _slowUpdateMillisec );

    logInfo() << "Display update every " << _slowUpdateMillisec << " millisec" << Qt::endl;
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
    _tree = new DirTree( this );
    _tree->setExcludeRules();

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

/*
void DirTreeModel::openUrl( const QString & url )
{
    _updateTimer.start();
    _tree->startReading( url );
}


void DirTreeModel::readPkg( const PkgFilter & pkgFilter )
{
    //logDebug() << "Reading " << pkgFilter << Qt::endl;

    _updateTimer.start();
    _tree->readPkg( pkgFilter );
}
*/

void DirTreeModel::loadIcons()
{
    const QString iconDir = treeIconDir();

    _dirIcon           = QIcon( iconDir % "dir.png"            );
    _dotEntryIcon      = QIcon( iconDir % "dot-entry.png"      );
    _fileIcon          = QIcon( iconDir % "file.png"           );
    _symlinkIcon       = QIcon( iconDir % "symlink.png"        );
    _unreadableDirIcon = QIcon( iconDir % "unreadable-dir.png" );
    _mountPointIcon    = QIcon( iconDir % "mount-point.png"    );
    _stopIcon          = QIcon( iconDir % "stop.png"           );
    _excludedIcon      = QIcon( iconDir % "excluded.png"       );
    _blockDeviceIcon   = QIcon( iconDir % "block-device.png"   );
    _charDeviceIcon    = QIcon( iconDir % "char-device.png"    );
    _specialIcon       = QIcon( iconDir % "special.png"        );
    _pkgIcon           = QIcon( iconDir % "folder-pkg.png"     );
}


FileInfo * DirTreeModel::findChild( DirInfo * parent, int childNo ) const
{
    CHECK_PTR( parent );

    const FileInfoList & sortedChildren = parent->sortedChildren( _sortCol, _sortOrder );
    if ( childNo >= 0 && childNo < sortedChildren.size() )
	return sortedChildren.at( childNo );

    logError() << "Child #" << childNo << " is out of range (0..." << sortedChildren.size()-1
	       << ") for " << parent << Qt::endl;

    dumpDirectChildren( parent );

    return nullptr;
}


//
// Reimplemented from QAbstractItemModel
//

QModelIndex DirTreeModel::index( int row, int column, const QModelIndex & parentIndex ) const
{
    if ( !_tree  || !_tree->root() || !hasIndex( row, column, parentIndex ) )
	return QModelIndex();

    FileInfo * parent = parentIndex.isValid() ? internalPointerCast( parentIndex ) : _tree->root();
    CHECK_MAGIC( parent );

    if ( parent->isDirInfo() )
	return createIndex( row, column, findChild( parent->toDirInfo(), row ) );

    return QModelIndex();
}


QModelIndex DirTreeModel::parent( const QModelIndex & index ) const
{
    if ( !index.isValid() )
	return QModelIndex();

    const FileInfo * child = internalPointerCast( index );
    if ( !checkMagicNumber( child ) )
	return QModelIndex();

    FileInfo * parent = child->parent();
    if ( !parent || parent == _tree->root() )
	return QModelIndex();

    const int row = rowNumber( parent );
    // logDebug() << "Parent of " << child << " is " << parent << " #" << row << Qt::endl;

    return createIndex( row, 0, parent );
}


int DirTreeModel::rowCount( const QModelIndex & parentIndex ) const
{
    FileInfo * item = parentIndex.isValid() ? internalPointerCast( parentIndex ) : _tree->root();
    CHECK_MAGIC( item );

    if ( !item->isDirInfo() || item->toDirInfo()->isLocked() )
	return 0;

    switch ( item->readState() )
    {
	case DirQueued:
	case DirReading:
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
    FileInfo * item = internalPointerCast( index );
    CHECK_MAGIC( item );

    switch ( role )
    {
	case Qt::DisplayRole: // text
	    if ( item && item->isDirInfo() )
		item->toDirInfo()->touch();

	    return columnText( _tree, item, col );

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
	    return columnFont( _baseFont, item, col, _useBoldForDominantItems );

	case Qt::BackgroundRole:
	    if ( col == NameCol && item->isDirInfo() && item->toDirInfo()->isFromCache() )
		return QGuiApplication::palette().color( QPalette::Active, QPalette::AlternateBase );
	    break;

	case PercentRole: // Send percent data to the PercentBarDelegate
	    return percentData( _tree, item );

	case TreeLevelRole: // Send the tree level (ignoring the top two) to the PercentBarDelegate
	    return item->treeLevel() - 2;

	case SizeTextRole: // Send an array of size text strings to the SizeColDelegate
	    return sizeTextData( item );
    }

    return QVariant();
}


QVariant DirTreeModel::headerData( int             section,
				   Qt::Orientation orientation,
				   int             role ) const
{
    // Vertical header should never be visible, but ...
    if ( orientation != Qt::Horizontal )
	return QVariant();

    switch ( role )
    {
	case Qt::DisplayRole:
	    switch ( DataColumns::fromViewCol( section ) )
	    {
		case NameCol:             return tr( "Name"               );
		case PercentBarCol:       return tr( "Subtree Percentage" );
		case PercentNumCol:       return tr( "%"                  );
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
		default:                  return QVariant();
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

    logDebug() << "Sorting by " << DataColumns::fromViewCol( column )
	       << ( order == Qt::AscendingOrder ? " ascending" : " descending" )
	       << Qt::endl;

    // logDebug() << "Before layoutAboutToBeChanged()" << Qt::endl;
    //dumpPersistentIndexList( persistentIndexList() );

    emit layoutAboutToBeChanged( QList<QPersistentModelIndex>(), QAbstractItemModel::VerticalSortHint );
    _sortCol = DataColumns::fromViewCol( column );
    _sortOrder = order;
    updatePersistentIndexes();
    emit layoutChanged( QList<QPersistentModelIndex>(), QAbstractItemModel::VerticalSortHint );

    //logDebug() << "After layoutChanged()" << Qt::endl;
    // dumpPersistentIndexList( persistentIndexList() );
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

    if ( item->readState() == DirAborted )
	return _stopIcon;

    if ( item->isDotEntry() ) return _dotEntryIcon;
    if ( item->isAttic()    ) return _atticIcon;
    if ( item->isPkgInfo()  ) return _pkgIcon;
    if ( item->isExcluded() ) return _excludedIcon;
    if ( item->readError()  ) return _unreadableDirIcon;
    if ( item->isDir()      ) return item->isMountPoint() ? _mountPointIcon : _dirIcon;
    // else FileInfo
    if ( item->isFile()        ) return _fileIcon;
    if ( item->isSymLink()     ) return _symlinkIcon;
    if ( item->isBlockDevice() ) return _blockDeviceIcon;
    if ( item->isCharDevice()  ) return _charDeviceIcon;
    if ( item->isSpecial()     ) return _specialIcon;

    return QIcon();
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

    // dumpDirectChildren( dir );
    const int count = directChildrenCount( dir );
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
	    _pendingUpdates.append( dir );

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
	const int count = directChildrenCount( subtree );
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
		   << " (" << QDirStat::DataColumns::fromViewCol( index.column() ) << ")"
		   << "\t" << item
	 	   << " data(0): " << index.model()->data( index, 0 ).toString()
		   << Qt::endl;
	logDebug() << "Ancestors: " << modelTreeAncestors( index ).join( QLatin1String( " -> " ) ) << Qt::endl;
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

    return QModelIndex();
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

	dumpDirectChildren( parent );
    }

    return row;
}
