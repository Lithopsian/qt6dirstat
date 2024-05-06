/*
 *   File name: DirTreeCache.cpp
 *   Summary:   QDirStat cache reader / writer
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <ctype.h>

#include <QUrl>

#include "DirTreeCache.h"
#include "DirInfo.h"
#include "DirTree.h"
#include "DotEntry.h"
#include "MountPoints.h"
#include "Logger.h"
#include "Exception.h"


#define KB 1024LL
#define MB (1024LL*1024)
#define GB (1024LL*1024*1024)
#define TB (1024LL*1024*1024*1024)

#define MAX_ERROR_COUNT			1000

#define VERBOSE_READ			0
#define VERBOSE_CACHE_DIRS		0
#define VERBOSE_CACHE_FILE_INFOS	0
#define DEBUG_LOCATE_PARENT		0


using namespace QDirStat;


namespace
{
    /**
     * Format a file size as string - with trailing "G", "M", "K" for
     * "Gigabytes", "Megabytes, "Kilobytes", respectively (provided there
     * is no fractional part - 27M is OK, 27.2M is not).
     **/
    QString formatSize( FileSize size )
    {
	// Multiples of 1024 are common, any larger multiple freakishly rare
	return size >= KB && size % KB == 0 ? QString( "%1K" ).arg( size / KB ) : QString::number( size );
    }


    /**
     * Return a string representing the type of file.
     **/
    const char * fileType( const FileInfo * item )
    {
	if ( item->isFile()        ) return "F";
	if ( item->isDir()         ) return "D";
	if ( item->isSymLink()     ) return "L";
	if ( item->isBlockDevice() ) return "BlockDev";
	if ( item->isCharDevice()  ) return "CharDev";
	if ( item->isFifo()        ) return "FIFO";
	if ( item->isSocket()      ) return "Socket";

	return "";
    }


    /**
     * Return the 'path' in an URL-encoded form, i.e. with some special
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
	gzprintf( cache, fileType( item ) );

	// Write name
	if ( item->isDirInfo() && !item->isDotEntry() )
	    gzprintf( cache, " %-40s", urlEncoded( item->url() ).data() ); // absolute path
	else
	    gzprintf( cache, "\t%-36s", urlEncoded( item->name() ).data() ); // relative path

	// Write size
	gzprintf( cache, "\t%s", formatSize( item->rawByteSize() ).toUtf8().data() );

	// For uid, gid, and permissions (mode also identifies the object type)
	gzprintf( cache, "\t%4d\t%4d\t%06o", item->uid(), item->gid(), item->mode() );

	// Write mtime
	gzprintf( cache, "\t0x%lx", (unsigned long)item->mtime() );

	// Write allocated size (and dummy to maintain compatibility with earlier formats)
	gzprintf( cache, "\t%s\t|", formatSize( item->rawAllocatedSize() ).toUtf8().data() );

	// Optional fields
	if ( item->isExcluded() )
	    gzprintf( cache, "\tunread: %s", "excluded" );
	if ( item->readState() == DirPermissionDenied )
	    gzprintf( cache, "\tunread: %s", "permissions" );
	if ( item->isMountPoint() && item->readState() == DirOnRequestOnly )
	    gzprintf( cache, "\tunread: %s", "mountpoint" );
	if ( item->isSparseFile() )
	    gzprintf( cache, "\tblocks: %lld", item->blocks() );
	if ( item->isFile() && item->links() > 1 )
	    gzprintf( cache, "\tlinks: %u", (unsigned) item->links() );

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

	// Write file children
	if ( item->dotEntry() )
	    writeTree( cache, item->dotEntry() );

	// Recurse through subdirectories
	for ( FileInfo * child = item->firstChild(); child; child = child->next() )
	    writeTree( cache, child );
    }

} // namespace



bool CacheWriter::writeCache( const QString & fileName, const DirTree *tree )
{
    if ( !tree || !tree->root() )
	return false;

    gzFile cache = gzopen( (const char *) fileName.toUtf8(), "w" );
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
	     "\n", CACHE_FORMAT_VERSION );

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
	    cptr++;

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
	    cptr++;

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
     * Split up a file name with path into its path and its name component
     * and return them in path_ret and name_ret, respectively.
     *
     * Example:
     *     "/some/dir/somewhere/myfile.obj"
     * ->  "/some/dir/somewhere", "myfile.obj"
     **/
    void splitPath( const QString & fileNameWithPath,
		    QString       & path_ret,
		    QString       & name_ret )
    {
	const bool  absolutePath = fileNameWithPath.startsWith( '/' );
	QStringList components   = fileNameWithPath.split( '/', Qt::SkipEmptyParts );

	if ( components.isEmpty() )
	{
	    path_ret = "";
	    name_ret = absolutePath ? "/" : "";
	}
	else
	{
	    name_ret = components.takeLast();
	    path_ret = components.join( '/' );

	    if ( absolutePath )
		path_ret.prepend( '/' );
	}
    }


    /**
     * Build a full path from path + file name (without path).
     **/
    QString buildPath( const QString & path, const QString & name )
    {
	if ( path.isEmpty() )
	    return name;

	if ( name.isEmpty() )
	    return path;

	if ( path == QLatin1String( "/" ) )
	    return path + name;

	return path + '/' + name;
    }

} // namespace



CacheReader::CacheReader( const QString & fileName,
			  DirTree       * tree,
			  DirInfo       * parent,
			  bool            markFromCache ):
    _cache { gzopen( fileName.toUtf8(), "r" ) },
    _markFromCache { markFromCache },
    _tree { tree },
    _parent { parent }
{
    if ( _cache == 0 )
    {
	logError() << "Can't open " << fileName << ": " << formatErrno() << Qt::endl;
	_ok = false;
	return;
    }

    //logDebug() << fileName << " opened OK" << Qt::endl;
    checkHeader();
}


CacheReader::CacheReader( const QString & fileName,
			  DirTree       * tree,
			  DirInfo       * dir,
			  DirInfo       * parent ):
    CacheReader ( fileName, tree, parent, true )
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
    if ( _cache )
	gzclose( _cache );

    //logDebug() << "Cache reading finished" << Qt::endl;

    // Only finalize if we actually created anything
    if ( _toplevel )
    {
	// logDebug() << "Finalizing recursive for " << _toplevel << Qt::endl;
	finalizeRecursive( _toplevel );
	_toplevel->finalizeAll();
    }
}


bool CacheReader::read( int maxLines )
{
    while ( !gzeof( _cache ) && _ok && ( maxLines == 0 || --maxLines > 0 ) )
    {
	if ( readLine() )
	{
	    splitLine();
	    addItem();
	}
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

	setReadError( _latestDir );

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
	if ( strcasecmp( keyword, "links:"  ) == 0 ) links_str	= val_str;
    }

    mode_t mode;
    if ( mode_str )
    {
	mode = strtoll( mode_str, 0, 8 );
    }
    else
    {
	// No mode in old file formats,
	// get the object type from the first field, but no permissions
	mode = [ type, mode_str ]()
	{
	    switch ( toupper( *type ) )
	    {
		// 'F' is ambiguous unfortunately
		case 'F': return *(mode_str+1) == '\0' ? S_IFREG : S_IFIFO;
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
    QString path;
    QString name;
    splitPath( fullPath, path, name );

    // Size
    FileSize size = readSize( size_str );

    // uid/gid
    const uid_t uid = uid_str ? strtol( uid_str, 0, 10 ) : 0;
    const gid_t gid = gid_str ? strtol( gid_str, 0, 10 ) : 0;

    // MTime
    const time_t mtime = strtol( mtime_str, 0, 0 );

    // Consider it a sparse file if the blocks field is present
    const bool isSparseFile = blocks_str;

    // Allocated size
    FileSize alloc = readSize( alloc_str );

    // Blocks: only stored for sparse files, otherwise just guess from the file size
    const FileSize blocks = blocks_str ?
			    strtoll( blocks_str, 0, 10 ) :
			    qCeil( (float)alloc / STD_BLOCK_SIZE );

    // Links
    const int links = links_str ? atoi( links_str ) : 1;

    //  The last file loaded from the cache should be the parent of any files
    DirInfo * parent = _latestDir;

    // The next directory might not be a child of the previous one
    if ( !parent && _tree->root() )
    {
	// The trivial case of an empty tree
	if ( _tree->root()->isEmpty() )
	    parent = _tree->root();

	// Try the easy way first - the starting point of this cache
	if ( !parent && _parent )
	    parent = dynamic_cast<DirInfo *> ( _parent->locate( path, false ) );

#if DEBUG_LOCATE_PARENT
	if ( parent )
	    logDebug() << "Using cache starting point as parent for " << fullPath << Qt::endl;
#endif

	if ( !parent )
	{
	    // Fallback: Search the entire tree
	    parent = dynamic_cast<DirInfo *> ( _tree->locate( path ) );

#if DEBUG_LOCATE_PARENT
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

#if DEBUG_LOCATE_PARENT
	    THROW( Exception( "Could not locate cache item parent" ) );
#endif
	    return;	// Ignore this cache line completely
	}
    }

    if ( S_ISDIR( mode ) ) // directory
    {
	QString url = ( parent == _tree->root() ) ? buildPath( path, name ) : name;
#if VERBOSE_CACHE_DIRS
	logDebug() << "Creating DirInfo for " << url << " with parent " << parent << Qt::endl;
#endif
	DirInfo * dir = new DirInfo( parent, _tree, url, mode, size, alloc, hasUidGidPerm, uid, gid, mtime );
	dir->setReadState( DirReading );

	// Allow automatically-loaded cache files to be flagged up to the user
	if ( _markFromCache )
	    dir->setFromCache();

	_latestDir = dir;

	if ( parent )
	    parent->insertChild( dir );

	if ( !_tree->root() )
	{
	    _tree->setRoot( dir );
	    _parent = dir;
	}

	if ( !_toplevel )
	    _toplevel = _parent ? _parent : dir;

	_tree->childAddedNotify( dir );

	if ( dir != _toplevel )
	{
	    // Don't treat the top level of our tree as a mount point even if it is
	    if ( !MountPoints::device( dir->url() ).isEmpty() )
		dir->setMountPoint();

	    // Don't try to exclude anything ourselves, just mark directories
	    // that are flagged in the cache file.
	    if ( unread_str )
	    {
		switch ( tolower( *unread_str ) )
		{
		    case 'e':
			dir->setExcluded();
			dir->setReadState( DirOnRequestOnly );
			break;

		    case 'p':
			dir->setReadState( DirPermissionDenied );
			break;

		    case 'm':
			dir->setReadState( DirOnRequestOnly );
			break;
		}

		dir->finalizeLocal();
		dir->readJobFinished( dir ); // propagates the unread count
//		_latestDir = 0;
	    }
	}
    }
    else if ( parent ) // not directory
    {
#if VERBOSE_CACHE_FILE_INFOS
	logDebug() << "Creating FileInfo for " << buildPath( parent->debugUrl(), name ) << Qt::endl;
#endif

	FileInfo * item = new FileInfo( parent, _tree, name,
					mode, size, alloc, hasUidGidPerm, uid, gid, mtime,
					isSparseFile, blocks, links );
	parent->insertChild( item );
	_tree->childAddedNotify( item );
    }
    else
    {
	logError() << "Line " << _lineNo << ": " << "No parent for item " << name << Qt::endl;
    }
}


FileSize CacheReader::readSize( const char * size_str )
{
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


bool CacheReader::eof() const
{
    if ( !_ok || !_cache )
	return true;

    return gzeof( _cache );
}


bool CacheReader::isDir( const QString & dirName )
{
    while ( !gzeof( _cache ) && _ok )
    {
	if ( !readLine() )
	    return false;

	splitLine();

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

	return QString( path ) == dirName;
    }

    return false;
}


void CacheReader::checkHeader()
{
    _ok = true;

    if ( !_ok || !readLine() )
	return;

    //logDebug() << "Checking cache file header" << Qt::endl;
    splitLine();

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
	const QString version = field( 1 );

	// currently not checking version number
	// for future use

	if ( !_ok )
	    logError() << "Line " << _lineNo << ": Incompatible cache file version" << Qt::endl;
    }

    //logDebug() << "Cache file header check OK: " << _ok << Qt::endl;
}


bool CacheReader::readLine()
{
    if ( !_ok || !_cache )
	return false;

    _fieldsCount = 0;

    do
    {
	_lineNo++;

	const char * buf = gzgets( _cache, _buffer, MAX_CACHE_LINE_LEN );
	if ( buf == Z_NULL || buf[ strlen( buf ) - 1 ] != '\n' )
	{
	    _buffer[0]	= '\0';
	    _line	= _buffer;

	    if ( !gzeof( _cache ) )
	    {
		_ok = false;
		logError() << "Line " << _lineNo <<
			( buf == Z_NULL ? ": read error" : ": line too long" ) << Qt::endl;
	    }

	    return false;
	}

	_line = skipWhiteSpace( _buffer );
	killTrailingWhiteSpace( _line );

	// logDebug() << "line[ " << _lineNo << "]: \"" << _line<< "\"" << Qt::endl;

    } while ( !gzeof( _cache ) &&
	      ( *_line == '\0'   ||	// empty line
		*_line == '#'	  ) );	// comment line

    return true;
}


void CacheReader::splitLine()
{
    _fieldsCount = 0;

    if ( !_ok || !_line )
	return;

    if ( *_line == '#' )	// skip comment lines
	*_line = '\0';

    char * current = _line;
    const char * end = _line + strlen( _line );

    while ( current && current < end && *current && _fieldsCount < MAX_FIELDS_PER_LINE-1 )
    {
	_fields[ _fieldsCount++ ] = current;
	current = findNextWhiteSpace( current );

	if ( current && current < end )
	{
	    *current++ = '\0';
	    current = skipWhiteSpace( current );
	}
    }
}


const char * CacheReader::field( int no ) const
{
    if ( no >= 0 && no < _fieldsCount )
	return _fields[ no ];
    else
	return nullptr;
}


void CacheReader::finalizeRecursive( DirInfo * dir )
{
    if ( dir->readState() != DirOnRequestOnly )
    {
	if ( !dir->readError() )
	    dir->setReadState( DirFinished );

	dir->finalizeLocal();
	_tree->sendReadJobFinished( dir );
    }

    for ( FileInfo * child = dir->firstChild(); child; child = child->next() )
    {
	if ( child->isDirInfo() )
	    finalizeRecursive( child->toDirInfo() );
    }

}


void CacheReader::setReadError( DirInfo * dir ) const
{
    //logDebug() << "Setting read error for " << dir << Qt::endl;

    while ( dir )
    {
	dir->setReadState( DirError );

	if ( dir == _toplevel )
	    return;

	dir = dir->parent();
    }
}


QString CacheReader::cleanPath( const QString & rawPath ) const
{
    QString clean = rawPath;
    return clean.replace( _multiSlash, "/" );
}


QString CacheReader::unescapedPath( const QString & rawPath ) const
{
    // Using a protocol part to avoid directory names with a colon ":"
    // being cut off because it looks like a URL protocol.
    const QString protocol = "foo:";
    const QString url = protocol + cleanPath( rawPath );

    return QUrl::fromEncoded( url.toUtf8() ).path();
}
