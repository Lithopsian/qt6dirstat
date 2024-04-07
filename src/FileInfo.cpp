/*
 *   File name: FileInfo.cpp
 *   Summary:	Support classes for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>       // gmtime()
#include <unistd.h>

#include <QFileInfo>

#include "FileInfo.h"
#include "DirInfo.h"
#include "DotEntry.h"
#include "Attic.h"
#include "DirTree.h"
#include "PkgInfo.h"
#include "FormatUtil.h"
#include "SysUtil.h"
#include "Logger.h"
#include "Exception.h"

// Some filesystems (NTFS seems to be among them) may handle block fragments
// well. Don't report files as "sparse" files if the block size is only a few
// bytes less than the byte size - it may be due to intelligent fragment
// handling.

#define FRAGMENT_SIZE 2048

using namespace QDirStat;


FileInfo::FileInfo( DirInfo           * parent,
		    DirTree           * tree,
		    const QString     & filename,
		    const struct stat & statInfo ):
    /**
     * Constructor from a stat buffer (i.e. based on an lstat() call).
     * This is the standard case.
     **/
    _parent { parent },
    _tree { tree },
    _name { filename },
    _isLocalFile { true },
    _isIgnored { false },
    _hasUidGidPerm { true }
{
//    CHECK_PTR( statInfo );

    _device	   = statInfo.st_dev;
    _mode	   = statInfo.st_mode;
    _links	   = statInfo.st_nlink;
    _uid	   = statInfo.st_uid;
    _gid	   = statInfo.st_gid;
    _mtime	   = statInfo.st_mtime;

    if ( isSpecial() )
    {
	_size		= 0;
	_allocatedSize	= 0;
	_blocks		= 0;
	_isSparseFile	= false;
    }
    else
    {
	_size		= statInfo.st_size;
	_blocks		= statInfo.st_blocks;

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
		       << " (" << (int) _blocks << " blocks)"
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

    /**
     * The destructor should also take care about unlinking this object from
     * its parent's children list, but regrettably that just doesn't work: At
     * this point (within the destructor) parts of the object are already
     * destroyed, e.g., the virtual table - virtual methods don't work any
     * more. Thus, somebody from outside must call deletingChild() just prior
     * to the actual "delete".
     *
     * This sucks, but it's the C++ standard.
     **/
}


FileSize FileInfo::size() const
{
    const FileSize sz = _isSparseFile ? _allocatedSize : _size;

    if ( _links > 1 && !_tree->ignoreHardLinks() && isFile() )
	return sz / _links;

    return sz;
}


FileSize FileInfo::allocatedSize() const
{
    const FileSize sz = _allocatedSize;

    if ( _links > 1 && !_tree->ignoreHardLinks() && isFile() )
	return sz / _links;

    return sz;
}


QString FileInfo::url() const
{
    if ( !_parent )
	return _name;

    QString parentUrl = _parent->url();

    if ( isPseudoDir() ) // don't append "/." for dot entries and attics
	return parentUrl;

    if ( !parentUrl.endsWith( "/" ) && !_name.startsWith( "/" ) )
	parentUrl += "/";

    return parentUrl + _name;
}


QString FileInfo::path() const
{
    if ( isPkgInfo() )
	return "";

    if ( !_parent )
	return _name;

    QString parentPath = _parent->isPkgInfo() ? "/" : _parent->path();

    if ( isPseudoDir() )
	return parentPath;

    if ( !parentPath.endsWith( "/" ) && !_name.startsWith( "/" ) )
	parentPath += "/";

    return parentPath + _name;
}


QString FileInfo::debugUrl() const
{
    if ( _tree && this == _tree->root() )
	return "<root>";

    QString result = url();

    if ( isDotEntry() )
    {
	result += "/" + dotEntryName();
    }
    else if ( isAttic() )
    {
	if ( _parent )
	{
	    if ( _tree && _parent != _tree->root() )
		result = _parent->debugUrl() + "/" + atticName();
	}
        else
            result += "/" + atticName();
    }

    result.replace( "//", "/" );

    return result;
}


int FileInfo::treeLevel() const
{
    int level = 0;

    for ( const FileInfo * parent = _parent; parent; parent = parent->parent() )
	level++;

    return level;
}


bool FileInfo::isInSubtree( const FileInfo *subtree ) const
{
    for ( const FileInfo * ancestor = this; ancestor; ancestor = ancestor->parent() )
    {
	if ( ancestor == subtree )
	    return true;
    }

    return false;
}


FileInfo * FileInfo::locate( const QString & locateUrl, bool findPseudoDirs )
{
    if ( !_tree || ( !locateUrl.startsWith( _name ) && this != _tree->root() ) )
	return nullptr;

    QString url = locateUrl;

    if ( this != _tree->root() )		// The root item is invisible
    {
	url.remove( 0, _name.length() );	// Remove leading name of this node

	if ( url.length() == 0 )		// Nothing left?
	    return this;			// Hey! That's us!

	if ( url.startsWith( "/" ) )	// If the next thing is a path delimiter,
	{
	    url.remove( 0, 1 );		// remove that leading delimiter.
	}
	else				// No path delimiter at the beginning
	{
	    if ( _name.right(1) != "/" &&	// and this is not the root directory
		 !isDotEntry() )		// or a dot entry:
	    {
		return nullptr;			// This can't be any of our children.
	    }
	}
    }

    // Search all children
    for ( FileInfo * child = firstChild(); child; child = child->next() )
    {
	FileInfo * foundChild = child->locate( url, findPseudoDirs );
	if ( foundChild )
	    return foundChild;
    }

    // Special case: One of the pseudo directories is requested.
    if ( findPseudoDirs )
    {
	if ( dotEntry() && url == dotEntryName() )
	    return dotEntry();

	if ( attic() && url == atticName() )
	    return attic();

	if ( url == dotEntryName() + "/" + atticName() && dotEntry() && dotEntry()->attic() )
	    return dotEntry()->attic();
    }

    // Search the dot entry if there is one - but only if there is no more
    // path delimiter left in the URL. The dot entry contains files only,
    // and their names may not contain the path delimiter, nor can they
    // have children. This check is not strictly necessary, but it may
    // speed up things a bit if we don't search the non-directory children
    // if the rest of the URL consists of several pathname components.
    if ( dotEntry() && !url.contains( "/" ) )  // No (more) "/" in this URL
    {
	// logDebug() << "Searching DotEntry for " << url << " in " << this << Qt::endl;

	for ( FileInfo * child = dotEntry()->firstChild(); child; child = child->next() )
	{
	    if ( child->name() == url )
	    {
		// logDebug() << "Found " << url << " in " << dotEntry() << Qt::endl;
		return child;
	    }
	}

	// logDebug() << "Cant find " << url << " in DotEntry" << Qt::endl;
    }

    if ( attic() )
	return attic()->locate( url, findPseudoDirs );

    return nullptr;
}


float FileInfo::subtreePercent()
{
    if ( !parent()			 ||	// only if there is a parent as calculation base
	 parent()->pendingReadJobs() > 0 ||	// not before subtree is finished reading
	 parent()->totalSize() == 0	 ||	// avoid division by zero
	 isExcluded() )				// not if this is an excluded object (dir)
    {
	return -1.0;
    }

    return ( 100.0 * totalSize() ) / (float) parent()->totalSize();
}


float FileInfo::subtreeAllocatedPercent()
{
    if ( !parent()			     ||	// only if there is a parent as calculation base
	 parent()->pendingReadJobs() > 0     ||	// not before subtree is finished reading
	 parent()->totalAllocatedSize() == 0 ||	// avoid division by zero
	 isExcluded() )				// not if this is an excluded object (dir)
    {
	return -1.0;
    }

    return ( 100.0 * totalAllocatedSize() ) / (float) parent()->totalAllocatedSize();
}


QString FileInfo::userName() const
{
    return hasUid() ? SysUtil::userName( uid() ) : QString();
}


QString FileInfo::groupName() const
{
    return hasGid() ? SysUtil::groupName( gid() ) : QString();
}


QString FileInfo::symbolicPermissions() const
{
    return hasPerm() ? symbolicMode( _mode ) : QString(); // omitTypeForRegularFiles
}


QString FileInfo::octalPermissions() const
{
    return hasPerm() ? octalMode( _mode ) : QString();
}


QString FileInfo::baseName() const
{
    return SysUtil::baseName( _name );
}


DirInfo * FileInfo::toDirInfo()
{
    DirInfo * dirInfo = dynamic_cast<DirInfo *>( this );

    return dirInfo;
}


DotEntry * FileInfo::toDotEntry()
{
    DotEntry * dotEntry = dynamic_cast<DotEntry *>( this );

    return dotEntry;
}


Attic * FileInfo::toAttic()
{
    Attic * attic = dynamic_cast<Attic *>( this );

    return attic;
}


PkgInfo * FileInfo::toPkgInfo()
{
    PkgInfo * pkgInfo = dynamic_cast<PkgInfo *>( this );

    return pkgInfo;
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
    const FileInfo * dir = this;

    // Find the nearest real directory from here;
    // do not use a DotEntry or an Attic because they always have 0 blocks.

    while ( !dir->isDirInfo() || dir->isPseudoDir() )
    {
	dir = dir->parent();

	if ( !dir )
	    return false;
    }

    // logDebug() << "Checking block size of " << dir << ": " << (int) dir->blocks() << Qt::endl;

    // A real directory never has a size == 0, so we can skip this check.
    return dir->blocks() > 0;
}


QString FileInfo::symLinkTarget() const
{
    return isSymLink() ? SysUtil::symLinkTarget( path() ) : QString();
}


QPair<short, short> FileInfo::yearAndMonth() const
{
    if ( isPseudoDir() || isPkgInfo() )
        return { 0, 0 };

    // Using gmtime() which is standard C/C++
    // unlike gmtime_r() which is not
    const struct tm * mtime_tm = gmtime( &_mtime );

    const short year  = mtime_tm->tm_year + 1900;
    const short month = mtime_tm->tm_mon  + 1;

    return { year, month };
}


bool FileInfo::isDominant()
{
    return _parent ? _parent->isDominantChild( this ) : false;
}
