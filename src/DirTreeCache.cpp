/*
 *   File name: DirTreeCache.cpp
 *   Summary:   QDirStat cache reader / writer
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <cmath>  // ceil()
#include <cctype> // isspace(), toupper()

#include "DirTreeCache.h"
#include "DirTree.h"
#include "Exception.h"
#include "FileInfoIterator.h"
#include "MountPoints.h"
#include "SysUtil.h"


#define KB 1024LL
#define MB (1024LL*1024)
#define GB (1024LL*1024*1024)
#define TB (1024LL*1024*1024*1024)

#define MAX_ERROR_COUNT			1000

#define VERBOSE_READ			0
#define VERBOSE_CACHE_DIRS		0
#define VERBOSE_CACHE_FILE_INFOS	0
#define VERBOSE_LOCATE_PARENT		0


using namespace QDirStat;


namespace
{
    /**
     * Format a file size as a string.  Abbreviate exact multiples of 1024 to
     * nK, otherwise just convert the number to a string.
     **/
    QString formatSize( FileSize size )
    {
	// Exact multiples of 1024 are fairly common, any larger multiple freakishly rare
	if ( size >= KB && size % KB == 0 )
	    return QString::number( size / KB ) % 'K';

	return QString::number( size );
    }


    /**
     * Return a string representing the type of file.
     **/
    const char * fileType( const FileInfo * item )
    {
	if ( item->isFile()        ) return "F";
	if ( item->isDir()         ) return "D";
	if ( item->isSymlink()     ) return "L";
	if ( item->isBlockDevice() ) return "BlockDev";
	if ( item->isCharDevice()  ) return "CharDev";
	if ( item->isFifo()        ) return "FIFO";
	if ( item->isSocket()      ) return "Socket";

	return "X";
    }


    /**
     * Return 'path' in a URL-encoded form, i.e. with some special
     * characters escaped in percent notation (" " -> "%20").
     **/
    QByteArray urlEncoded( const QString & path )
    {
	// Using a protocol ("scheme") part to avoid directory names with a colon
	// ":" being cut off because it looks like a URL protocol.
	QUrl url;
	url.setScheme( "foo" );
	url.setPath( path );

	const QByteArray encoded = url.toEncoded( QUrl::RemoveScheme );
	if ( encoded.isEmpty() )
	    logError() << "Invalid file/dir name: " << path << Qt::endl;

	return encoded;

    }


    /**
     * Write 'item' to cache file 'cache' without recursion.
     * Uses zlib to write gzip-compressed files.
     **/
    void writeItem( gzFile cache, const FileInfo * item )
    {
	if ( !item )
	    return;

	// Write file type
	gzputs( cache, fileType( item ) );

	// Write name
	if ( item->isDirInfo() )
	    gzprintf( cache, " %-40s", urlEncoded( item->url() ).constData() ); // absolute path
	else
	    gzprintf( cache, "\t%-36s", urlEncoded( item->name() ).constData() ); // relative path

	// Write size
	gzprintf( cache, "\t%s", formatSize( item->rawByteSize() ).toUtf8().constData() );

	// For uid, gid, and permissions (mode also identifies the object type)
	gzprintf( cache, "\t%4d\t%4d\t%06o", item->uid(), item->gid(), item->mode() );

	// Write mtime
	gzprintf( cache, "\t0x%lx", (unsigned long)item->mtime() );

	// Write allocated size (and dummy to maintain compatibility with earlier formats)
	gzprintf( cache, "\t%s\t|", formatSize( item->rawAllocatedSize() ).toUtf8().constData() );

	// Optional fields
	if ( item->isExcluded() )
	    gzputs( cache, "\tunread: excluded" );
	else if ( item->readState() == DirNoAccess )
	    gzputs( cache, "\tunread: noaccess" );
	else if ( item->readState() == DirPermissionDenied )
	    gzputs( cache, "\tunread: permissions" );
	else if ( item->readState() == DirError )
	    gzputs( cache, "\tunread: readerror" );
	else if ( item->isMountPoint() && item->readState() == DirOnRequestOnly )
	    gzputs( cache, "\tunread: mountpoint" );
	if ( item->isSparseFile() )
	    gzprintf( cache, "\tblocks: %lld", item->blocks() );
	if ( item->isFile() && item->links() > 1 )
	    gzprintf( cache, "\tlinks: %u", (unsigned)item->links() );

	// One item per line
	gzputc( cache, '\n' );
    }


    /**
     * Write 'item' recursively to cache file 'cache'.
     * Uses zlib to write gzip-compressed files.
     **/
    void writeTree( gzFile cache, const FileInfo * item )
    {
	if ( !item )
	    return;

	// Write entry for this item
	if ( !item->isDotEntry() )
	    writeItem( cache, item );

	// Write file children immediately following the parent entry
	if ( item->dotEntry() )
	    writeTree( cache, item->dotEntry() );

	// Recurse through subdirectories, but not the dot entry
	for ( FileInfoIterator it{ item }; *it; ++it )
	    writeTree( cache, *it );
    }

} // namespace



bool CacheWriter::writeCache( const QString & fileName, const DirTree * tree )
{
    if ( !tree || !tree->root() )
	return false;

    if ( !tree->firstToplevel()->isDirInfo() )
    {
	logWarning() << "No toplevel directory, can't write a valid cache file" << Qt::endl;
	return false;
    }

    gzFile cache = gzopen( fileName.toUtf8().constData(), "w" );
    if ( cache == 0 )
    {
	logError() << "Can't open " << fileName << ": " << formatErrno() << Qt::endl;
	return false;
    }

    gzprintf( cache,
             "[qdirstat %s cache file]\n"
             "#Generated file - do not edit!\n"
             "#\n"
             "# Type\tpath                              \tsize\tuid\tgid\tmode\tmtime\t\talloc\t\t<optional fields>\n"
             "\n",
             CACHE_FORMAT_VERSION );
    writeTree( cache, tree->firstToplevel() );
    gzclose( cache );

    return true;
}




namespace
{
    /**
     * Skip leading whitespace from a string.
     * Returns a pointer to the first character that is non-whitespace.
     **/
    char * skipWhiteSpace( char * cptr )
    {
	if ( cptr == nullptr )
	    return nullptr;

	while ( *cptr != '\0' && isspace( *cptr ) )
	    ++cptr;

	return cptr;
    }


    /**
     * Find the next whitespace in a string.
     *
     * Returns a pointer to the next whitespace character
     * or a null pointer if there is no more whitespace in the string.
     **/
    char * findNextWhiteSpace( char * cptr )
    {
	if ( cptr == nullptr )
	    return nullptr;

	while ( *cptr != '\0' && !isspace( *cptr ) )
	    ++cptr;

	return *cptr == '\0' ? nullptr : cptr;
    }


    /**
     * Remove all trailing whitespace from a string - overwrite it with 0
     * bytes.
     *
     * Returns the new string length.
     **/
    void killTrailingWhiteSpace( char * cptr )
    {
	if ( cptr == nullptr )
	    return;

	const char * start = cptr;
	cptr += strlen( start ) - 1;

	while ( cptr >= start && isspace( *cptr ) )
	    *cptr-- = '\0';
    }


    /**
     * Split the current input line into fields separated by whitespace.
     **/
    int splitLine( char * line, char * fields[] )
    {
	int fieldsCount = 0;

	const char * end = line + strlen( line );

	while ( line && line < end && *line && fieldsCount < MAX_FIELDS_PER_LINE-1 )
	{
	    fields[ fieldsCount++ ] = line;
	    line = findNextWhiteSpace( line );

	    if ( line && line < end )
	    {
		*line++ = '\0';
		line = skipWhiteSpace( line );
	    }
	}

	return fieldsCount;
    }


    /**
     * Converts a string representing a number of bytes into a FileSize
     * return value.
     *
     * Note that the CacheWriter in this file only uses the 'K' suffix, but
     * older versions may use 'M', 'F', or 'T'.
     **/
    FileSize readSize( const char * size_str )
    {
	if ( !size_str )
	    return 0;

	char * end = nullptr;
	FileSize size = strtoll( size_str, &end, 10 );
	if ( end )
	{
	    switch ( toupper( *end ) )
	    {
		case 'K': return size * KB;
		case 'M': return size * MB;
		case 'G': return size * GB;
		case 'T': return size * TB;
	    }
	}

	return size;
    }


    /**
     * Recursively set the read status of all dirs from 'dir' on, send tree
     * signals and finalize local (i.e. clean up empty or unneeded dot
     * entries).
     **/
    void finalizeRecursive( DirInfo * dir, DirTree * tree )
    {
	if ( dir->readState() != DirOnRequestOnly )
	{
	    if ( !dir->subtreeReadError() )
		dir->setReadState( DirFinished );

	    dir->finalizeLocal();
	    tree->sendReadJobFinished( dir );
	}

	for ( DirInfoIterator it{ dir }; *it; ++it )
	    finalizeRecursive( *it, tree );
    }


    /**
     * Cascade a read error up to the 'toplevel' directory node.
     **/
    void setReadError( DirInfo * dir, const DirInfo * toplevel )
    {
	//logDebug() << "Setting read error for " << dir << Qt::endl;

	while ( dir )
	{
	    dir->setReadState( DirError );

	    if ( dir == toplevel )
		return;

	    dir = dir->parent();
	}
    }

} // namespace



CacheReader::CacheReader( const QString & fileName,
                          DirTree       * tree,
                          DirInfo       * parent,
                          bool            markFromCache ):
    _cache{ gzopen( fileName.toUtf8().constData(), "r" ) },
    _markFromCache{ markFromCache },
    _tree{ tree },
    _parent{ parent }
{
    if ( !tree )
	return;

    if ( _cache == 0 )
    {
	logError() << "Can't open " << fileName << ": " << formatErrno() << Qt::endl;
	return;
    }

    //logDebug() << fileName << " opened OK" << Qt::endl;
    checkHeader();
}


CacheReader::CacheReader( const QString & fileName,
                          DirTree       * tree,
                          DirInfo       * dir,
                          DirInfo       * parent ):
    CacheReader{ fileName, tree, parent, true }
{
    if ( dir && !isDir( dir->url() ) ) // Does this cache file match this directory?
    {
	_ok = false;
    }
    else
    {
	gzrewind( _cache );	// so the file is ready for reading again
	checkHeader();		// skip cache header
    }
}

CacheReader::~CacheReader()
{
    //logDebug() << "Cache reading finished" << Qt::endl;

    if ( _ok && !gzeof( _cache ) )
    {
	// Treat this as a user abort, although it might conceivably be an error
	if ( _toplevel )
	{
	    // Mark the top level of the cache file as aborted, which will propagate up
	    _toplevel->readJobAborted();

	    // No way to know what is complete, so remove everything else
	    _tree->clearSubtree( _toplevel );
	}
    }

    if ( _cache )
	gzclose( _cache );

    // Only finalize if we actually created anything
    if ( _toplevel )
    {
	// Flag this read job as finished since there is no actual read job
	_toplevel->readJobFinished( _toplevel );

	// Need to finalize the parent when replacing a subtree, as it will have been marked DirReading
	DirInfo * toplevel = _parent ? _parent : _toplevel;
	finalizeRecursive( toplevel, _tree );
	toplevel->finalizeAll();
    }
}


bool CacheReader::read( int maxLines )
{
    while ( !gzeof( _cache ) && _ok && ( maxLines == 0 || --maxLines > 0 ) )
    {
	if ( readLine() )
	    addItem();
    }

    return _ok && !gzeof( _cache );
}


void CacheReader::addItem()
{
    if ( _fieldsCount < 4 )
    {
	logError() << "Syntax error at line " << _lineNo
	           << ": Expected at least 4 fields, saw only " << _fieldsCount
	           << Qt::endl;

	setReadError( _latestDir, _toplevel );

	if ( ++_errorCount > MAX_ERROR_COUNT )
	{
	    logError() << "Too many syntax errors. Giving up." << Qt::endl;
	    _ok = false;
	}

	return;
    }

    int n = 0;
    const char * type     = field( n++ );
    const char * raw_path = field( n++ );
    const char * size_str = field( n++ );

    const char * mtime_str = field( n++ );
    const char * uid_str   = nullptr;
    const char * gid_str   = nullptr;
    const char * mode_str  = nullptr;

    // Adjust for the current file version with uid, gid, and mode before mtime
    bool hasUidGidPerm = false;
    if ( *mtime_str && !( *mtime_str == '0' && *mtime_str+1 == 'x' ) )
    {
	hasUidGidPerm = true;
	uid_str   = mtime_str;
	gid_str   = field( n++ );
	mode_str  = field( n++ );
	mtime_str = field( n++ );
    }

    const char * alloc_str = field( n );
    n += 2;

    // Optional key/value field pairs
    const char * unread_str = nullptr;
    const char * blocks_str = nullptr;
    const char * links_str  = nullptr;
    while ( _fieldsCount > n+1 )
    {
	const char * keyword = field( n++ );
	const char * val_str = field( n++ );

	// unread: is used for directories that have not been read for some reason
	if ( strcasecmp( keyword, "unread:" ) == 0 ) unread_str = val_str;

	// blocks: is used for sparse files to give the allocation
	if ( strcasecmp( keyword, "blocks:" ) == 0 ) blocks_str = val_str;

	// links: is used when there is more than one hard link
	if ( strcasecmp( keyword, "links:"  ) == 0 ) links_str  = val_str;
    }

    mode_t mode;
    if ( mode_str )
    {
	mode = strtoul( mode_str, 0, 8 );
    }
    else
    {
	// No mode in old file formats,
	// get the object type from the first field, but no permissions
	mode = [ type ]()
	{
	    switch ( toupper( *type ) )
	    {
		// 'F' is ambiguous unfortunately
		case 'F': return *(type+1) == '\0' ? S_IFREG : S_IFIFO;
		case 'D': return S_IFDIR;
		case 'L': return S_IFLNK;
		case 'B': return S_IFBLK;
		case 'C': return S_IFCHR;
		case 'S': return S_IFSOCK;
		default:  return S_IFREG;
	    }
	}();
    }

    // Path
    if ( *raw_path == '/' )
	_latestDir = nullptr;

    const QString fullPath = unescapedPath( raw_path );

    QString pathRet;
    QString nameRet;
    SysUtil::splitPath( fullPath, pathRet, nameRet );
    const QString & path = pathRet;
    const QString & name = nameRet;

    // Size
    FileSize size = readSize( size_str );

    // uid/gid
    const uid_t uid = uid_str ? strtoul( uid_str, 0, 10 ) : 0;
    const gid_t gid = gid_str ? strtoul( gid_str, 0, 10 ) : 0;

    // MTime
    const time_t mtime = strtol( mtime_str, 0, 0 );

    // Consider it a sparse file if the blocks field is present
    const bool isSparseFile = blocks_str;

    // Allocated size
    FileSize alloc = readSize( alloc_str );

    // Blocks: only stored for sparse files, otherwise just guess from the file size
    const FileSize blocks = blocks_str ?
                            strtoll( blocks_str, 0, 10 ) :
                            static_cast<FileSize>( std::ceil( 1.0 * alloc ) / STD_BLOCK_SIZE );

    // Links
    const int links = links_str ? atoi( links_str ) : 1;

    //  The last file loaded from the cache should be the parent of any files
    DirInfo * parent = _latestDir;

    // The next directory might not be a child of the previous one
    if ( !parent )
    {
	// The trivial case of an empty tree
	if ( _tree->root()->isEmpty() )
	    parent = _tree->root();

#if VERBOSE_LOCATE_PARENT
	if ( parent )
	    logDebug() << "Using empty tree root as parent" << Qt::endl;
#endif

	// Try the easy way first - the starting point of this cache
	if ( _parent && !parent )
	    parent = _parent->locate( path )->toDirInfo();

#if VERBOSE_LOCATE_PARENT
	if ( parent )
	    logDebug() << "Using cache starting point as parent for " << fullPath << Qt::endl;
#endif

	if ( !parent )
	{
	    // Fallback: Search the entire tree
	    parent = _tree->locate( path )->toDirInfo();

#if VERBOSE_LOCATE_PARENT
	    if ( parent )
		logDebug() << "Located parent " << path << " in tree" << Qt::endl;
#endif
	}

	if ( !parent ) // Still nothing?
	{
	    logError() << "Line " << _lineNo << ": "
	               << "Could not locate parent \"" << path << "\" for " << name << Qt::endl;

	    if ( ++_errorCount > MAX_ERROR_COUNT )
	    {
		logError() << "Too many consistency errors. Giving up." << Qt::endl;
		_ok = false;
	    }

#if VERBOSE_LOCATE_PARENT
	    THROW( Exception{ "Could not locate cache item parent" } );
#endif
	    return; // Ignore this cache line completely
	}
    }

    // Treat unread items as directories even if the mode is bad
    if ( unread_str || S_ISDIR( mode ) ) // directory
    {
	QString url = ( parent == _tree->root() ) ? fullPath : name;
#if VERBOSE_CACHE_DIRS
	logDebug() << "Creating DirInfo for " << url << " with parent " << parent << Qt::endl;
#endif
	DirInfo * dir = new DirInfo{ parent, _tree, url,
	                             mode, size, alloc, _markFromCache, hasUidGidPerm, uid, gid, mtime };
	dir->setReadState( DirReading );

	_latestDir = dir;
	parent->insertChild( dir );

	if ( !_toplevel )
	{
	    _toplevel = dir;
	    dir->readJobAdded(); // just to show 1 pending read job
	    if ( !_parent )
		_tree->setUrl( dir->url() );
	}

	_tree->childAddedNotify( dir );

	// Don't finalize the top level of a complete tree until the whole read is done
	if ( dir != _toplevel || _parent )
	{
	    // Don't treat the top level of the entire tree as a mount point even if it is
	    if ( !MountPoints::device( dir->url() ).isEmpty() )
	    {
		//logDebug() << dir << " is mountpoint" << Qt::endl;
		dir->setMountPoint();
	    }

	    // Don't try to exclude anything ourselves, just mark directories
	    // that are flagged in the cache file.
	    if ( unread_str )
	    {
		dir->readJobAdded(); // balances the pending read jobs count

		const DirReadState readState = [ &dir, unread_str ]()
		{
		    switch ( tolower( *unread_str ) )
		    {
			case 'e':
			    dir->setExcluded();
			    return DirOnRequestOnly;

			case 'n':
			    return DirNoAccess;

			case 'p':
			    return DirPermissionDenied;

			case 'm':
			    return DirOnRequestOnly;

			default:
			    return DirError;
		    }
		}();

		dir->setReadState( readState );
		dir->finalizeLocal();
		dir->readJobFinished( dir ); // propagates the unread count up the tree
	    }
	}
    }
    else if ( parent && parent != _tree->root() ) // not directory, must have a valid parent first
    {
#if VERBOSE_CACHE_FILE_INFOS
	const QString parentUrl = parent->debugUrl();
	const QString debugPath = [ &parentUrl, &name ]() -> QString
	{
	    if ( parentUrl.isEmpty() )
		return name;

	    if ( name.isEmpty() )
		return parentUrl;

	    if ( parentUrl == "/"_L1 )
		return parentUrl % name;

	    return parentUrl % '/' % name;
	}();

	logDebug() << "Creating FileInfo for " << debugPath << Qt::endl;
#endif

	FileInfo * item = new FileInfo{ parent, _tree, name,
	                                mode, size, alloc, hasUidGidPerm, uid, gid, mtime,
	                                isSparseFile, blocks, static_cast<nlink_t>( links ) };
	parent->insertChild( item );
	_tree->childAddedNotify( item );
    }
    else
    {
	logError() << "Line " << _lineNo << ": " << "no parent for item " << name << Qt::endl;
    }
}


bool CacheReader::isDir( const QString & dirName )
{
    while ( !gzeof( _cache ) && _ok )
    {
	if ( !readLine() )
	    return false;

	if ( _fieldsCount < 2 )
	    return false;

	int n = 0;
	const char * type = field( n++ );
	const char * path = field( n++ );

	// should be a directory first, but double-check
	if ( strcasecmp( type, "D" ) != 0 )
	    return false;

	// no point reading if the cache toplevel is unread
	while ( _fieldsCount > n )
	{
	    const char * keyword = field( n++ );
	    if ( strcasecmp( keyword, "unread:" ) == 0 )
		return false;
	}

	return QString{ path } == dirName;
    }

    return false;
}


void CacheReader::checkHeader()
{
    _ok = true;

    if ( !readLine() || !_ok )
	return;

    //logDebug() << "Checking cache file header" << Qt::endl;

    // Check for    [qdirstat <version> cache file]
    // or	    [kdirstat <version> cache file]

    if ( _fieldsCount != 4 )
	_ok = false;

    if ( _ok )
    {
	if ( ( strcmp( field( 0 ), "[qdirstat" ) != 0 && strcmp( field( 0 ), "[kdirstat" ) != 0 ) ||
	       strcmp( field( 2 ), "cache"     ) != 0 ||
	       strcmp( field( 3 ), "file]"     ) != 0 )
	{
	    _ok = false;
	    logError() << "Line " << _lineNo << ": Unknown file format" << Qt::endl;
	}
    }

    if ( _ok )
    {
	const char * version = field( 1 );

	// currently not checking version number
	// for future use

	if ( !_ok )
	{
	    logError() << "Line " << _lineNo
	               << ": incompatible cache file version " << version
	               << Qt::endl;
	}
    }

    //logDebug() << "Cache file header check OK: " << _ok << Qt::endl;
}


bool CacheReader::readLine()
{
    if ( !_ok || !_cache )
	return false;

//    _fieldsCount = 0;

    char * line;

    do
    {
	++_lineNo;

	const char * buf = gzgets( _cache, _buffer, MAX_CACHE_LINE_LEN );
	if ( buf == Z_NULL || buf[ strlen( buf ) - 1 ] != '\n' )
	{
	    _buffer[0] = '\0';
	    line       = _buffer;

	    if ( !gzeof( _cache ) )
	    {
		_ok = false;
		logError() << "Line " << _lineNo
		           << ( buf == Z_NULL ? ": read error" : ": line too long" )
		           << Qt::endl;
	    }

	    return false;
	}

	line = skipWhiteSpace( _buffer );
	killTrailingWhiteSpace( line );

	// logDebug() << "line[ " << _lineNo << "]: \"" << _line << '"' << Qt::endl;

    } while ( !gzeof( _cache ) && ( *line == '\0' || *line == '#' ) );

    _fieldsCount = _ok && line ? splitLine( line, _fields ) : 0;

    return true;
}
