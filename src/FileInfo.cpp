/*
 *   File name: FileInfo.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <ctime> // gmtime()

#include "FileInfo.h"
#include "Attic.h"
#include "DirTree.h"
#include "FileInfoIterator.h"
#include "Logger.h"
#include "PkgInfo.h"
#include "FormatUtil.h"
#include "SysUtil.h"


// Some filesystems (NTFS seems to be among them) may handle block fragments
// well. Don't report files as "sparse" files if the block size is only a few
// bytes less than the byte size - it may be due to intelligent fragment
// handling.
#define FRAGMENT_SIZE 2048


using namespace QDirStat;


namespace
{
    /**
     * Returns whether a FileInfo item is in a state where there can
     * be meaningful percentages of its size and allocated size.
     **/
    bool hasPercent( const FileInfo * item )
    {
	// not before subtree is finished reading
	if ( !item->parent() || item->parent()->pendingReadJobs() > 0 )
	    return false;

	// no meaningful percent for aborted package reads
	if ( item->isPkgInfo() && item->readState() == DirAborted )
	    return false;

	// not if this is an excluded object (dir)
	if ( item->isExcluded() )
	    return false;

	return true;
    }

    /**
     * Returns the percentage value based on a size and the parent's size.
     **/
    float percent( FileSize size, FileSize parentSize)
    {
	return parentSize == 0 ? 0.0 : 100.0 * size / parentSize;
    }

} // namespace


FileInfo::FileInfo( DirInfo           * parent,
                    DirTree           * tree,
                    const QString     & filename,
                    const struct stat & statInfo ):
    _name{ filename },
    _parent{ parent },
    _tree{ tree },
    _isLocalFile{ true },
    _isIgnored{ false },
    _hasUidGidPerm{ true },
    _device{ statInfo.st_dev },
    _mode{ statInfo.st_mode },
    _links{ statInfo.st_nlink },
    _uid{ statInfo.st_uid },
    _gid{ statInfo.st_gid },
    _mtime{ statInfo.st_mtime }
{
    if ( _device == 0 )
	logDebug() << filename << Qt::endl;
    if ( isSpecial() )
    {
	_size          = 0;
	_allocatedSize = 0;
	_blocks        = 0;
	_isSparseFile  = false;
    }
    else
    {
	_size   = statInfo.st_size;
	_blocks = statInfo.st_blocks;

	if ( _blocks == 0 && _size > 0 )
	{
	    // Do not make any assumptions about fragment handling: The
	    // last block of the file might be partially unused, or the
	    // filesystem might do clever fragment handling, or it's an
	    // exported kernel table like /dev, /proc, /sys. So let's
	    // simply use the size reported by stat() for _allocatedSize.
	    _allocatedSize = filesystemCanReportBlocks() ? 0 : _size;
	}
	else
	{
	    _allocatedSize = _blocks * STD_BLOCK_SIZE;
	}

	// allow for intelligent fragment handling
	_isSparseFile = isFile() && _blocks >= 0 && _allocatedSize + FRAGMENT_SIZE < _size;

#if 0
	if ( _isSparseFile )
	    logDebug() << "Found sparse file: " << this
	               << "    Byte size: "     << formatSize( _size )
	               << "  Allocated: "       << formatSize( _allocatedSize )
	               << " (" << _blocks << " blocks)"
	               << Qt::endl;
#endif

#if 0
	if ( isFile() && _links > 1 )
	    logDebug() << _links << " hard links: " << this << Qt::endl;
#endif
    }
}


FileInfo::~FileInfo()
{
    _magic = 0;
}


FileSize FileInfo::size() const
{
    const FileSize size = _isSparseFile ? _allocatedSize : _size;

    if ( _links > 1 && !_tree->ignoreHardLinks() && isFile() )
	return size / _links;

    return size;
}


FileSize FileInfo::allocatedSize() const
{
    const FileSize size = _allocatedSize;

    if ( _links > 1 && !_tree->ignoreHardLinks() && isFile() )
	return size / _links;

    return size;
}


QString FileInfo::url() const
{
    if ( !_parent )
	return _name;

    QString parentUrl = _parent->url();

    if ( isPseudoDir() ) // don't append "/." for dot entries and attics
	return parentUrl;

    if ( !parentUrl.endsWith( u'/' ) && !_name.startsWith( u'/' ) )
	parentUrl += u'/';

    return parentUrl % _name;
}


QString FileInfo::path() const
{
    if ( isPkgInfo() )
	return QString{};

    if ( !_parent )
	return _name;

    QString parentPath = _parent->isPkgInfo() ? "/" : _parent->path();

    if ( isPseudoDir() )
	return parentPath;

    if ( !parentPath.endsWith( u'/' ) && !_name.startsWith( u'/' ) )
	parentPath += u'/';

    return parentPath % _name;
}


QString FileInfo::debugUrl() const
{
    if ( _tree && this == _tree->root() )
	return _tree->rootDebugUrl();

    QString result = url();

    // Add a pseudo-dir identifier (or two), but only if that is the leaf item
    if ( isPseudoDir() )
    {
	 // Make sure any parent pseudo-dir is in the url
	if ( _tree && _parent != _tree->root() )
	    result = _parent->debugUrl();

	result += '/' % ( isAttic() ? atticName() : dotEntryName() );
    }

    result.replace( "//"_L1, "/"_L1 );

    return result;
}


int FileInfo::treeLevel() const
{
    int level = 0;

    for ( const DirInfo * parent = _parent; parent; parent = parent->parent() )
	++level;

    return level;
}


bool FileInfo::isInSubtree( const FileInfo * subtree ) const
{
    for ( const FileInfo * ancestor = this; ancestor; ancestor = ancestor->parent() )
    {
	if ( ancestor == subtree )
	    return true;
    }

    return false;
}


FileInfo * FileInfo::locate( const QString & locateUrl )
{
    if ( !_tree || ( !locateUrl.startsWith( _name ) && this != _tree->root() ) )
	return nullptr;

    QString url = locateUrl;

    // The root item is invisible so don't try to search for it
    if ( this != _tree->root() )
    {
	// Remove leading name of this node
	url.remove( 0, _name.length() );

	if ( url.length() == 0 ) // nothing left? That's us!
	    return this;

	if ( url.startsWith( u'/' ) )
	    url.remove( 0, 1 ); // remove leading delimiters, we're not matching on those
	else if ( !_name.endsWith( u'/' ) && !isPseudoDir() )
	    return nullptr; // not directory, not root, not pseudo-dir, url can't be one of our children
    }

    // Recursively search all children, including the dot entry and attic
    for ( AtticIterator it{ this }; *it; ++it )
    {
	FileInfo * foundChild = it->locate( url );
	if ( foundChild )
	    return foundChild;
    }

    return nullptr;
}


float FileInfo::subtreePercent()
{
    if ( hasPercent( this ) )
	return percent( totalSize(), parent()->totalSize() );

    return -1.0f;
}


float FileInfo::subtreeAllocatedPercent()
{
    if ( hasPercent( this ) )
	return percent( totalAllocatedSize(), parent()->totalAllocatedSize() );

    return -1.0f;
}


QString FileInfo::userName() const
{
    return hasUid() ? SysUtil::userName( uid() ) : QString{};
}


QString FileInfo::groupName() const
{
    return hasGid() ? SysUtil::groupName( gid() ) : QString{};
}


QString FileInfo::symbolicPermissions() const
{
    return hasPerm() ? symbolicMode( _mode ) : QString{};
}


QString FileInfo::octalPermissions() const
{
    return hasPerm() ? octalMode( _mode ) : QString{};
}


DirInfo * FileInfo::toDirInfo()
{
    return dynamic_cast<DirInfo *>( this );
}


DotEntry * FileInfo::toDotEntry()
{
    return dynamic_cast<DotEntry *>( this );
}


Attic * FileInfo::toAttic()
{
    return dynamic_cast<Attic *>( this );
}


PkgInfo * FileInfo::toPkgInfo()
{
    return dynamic_cast<PkgInfo *>( this );
}


PkgInfo * FileInfo::pkgInfoParent() const
{
    for ( FileInfo * pkg = _parent; pkg; pkg = pkg->parent() )
    {
	if ( pkg->isPkgInfo() )
	    return pkg->toPkgInfo();
    }

    return nullptr;
}


bool FileInfo::filesystemCanReportBlocks() const
{
    // Find the nearest ancestor that is a real directory
    for ( const FileInfo * dir = this; dir; dir = dir->parent() )
    {
	// Do not use a DotEntry or an Attic because they always have 0 blocks
	if ( dir->isDirInfo() && !dir->isPseudoDir() )
	    return dir->blocks() > 0; // real directories should never have blocks == 0
    }

    return false;
}


QString FileInfo::symLinkTarget() const
{
    return isSymLink() ? SysUtil::symLinkTarget( path() ) : QString{};
}


YearAndMonth FileInfo::yearAndMonth() const
{
    if ( isPseudoDir() || isPkgInfo() )
	return { 0, 0 };

    // Using gmtime() which is standard C/C++
    // unlike gmtime_r() which is not
    const struct tm * mtime_tm = gmtime( &_mtime );

    const short year  = mtime_tm->tm_year + 1900; // works up to year 34668
    const short month = mtime_tm->tm_mon  + 1;

    return { year, month };
}


bool FileInfo::isDominant()
{
    return _parent ? _parent->isDominantChild( this ) : false;
}
