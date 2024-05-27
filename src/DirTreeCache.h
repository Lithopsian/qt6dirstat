/*
 *   File name: DirTreeCache.h
 *   Summary:   QDirStat cache reader / writer
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef DirTreeCache_h
#define DirTreeCache_h

#include <zlib.h>

#include <QRegularExpression>

#include "Typedefs.h" //FileSize


#define DEFAULT_CACHE_NAME	".qdirstat.cache.gz"
#define CACHE_FORMAT_VERSION	"2.1"
#define MAX_CACHE_LINE_LEN	5000  // 4096 plus some
#define MAX_FIELDS_PER_LINE	32


namespace QDirStat
{
	class DirInfo;
	class DirTree;
	class FileInfo;

    class CacheWriter
    {
    public:

	/**
	 * Write 'tree' to file 'fileName' in gzip format (using zlib).
	 *
	 * Check CacheWriter::ok() to see if writing the cache file went OK.
	 **/
	CacheWriter( const QString & fileName, DirTree * tree ):
	    _ok { writeCache( fileName, tree ) }
	{}

	/**
	 * Returns true if writing the cache file went OK.
	 **/
	bool ok() const { return _ok; }


    protected:

	/**
	 * Write cache file in gzip format.
	 * Returns 'true' if OK, 'false' upon error.
	 **/
	static bool writeCache( const QString & fileName, const DirTree * tree );


    private:

	//
	// Data members
	//

	bool _ok;

    };



    class CacheReader
    {
	/**
	 * Private constructor.  Opens the cache file and checks that it is
	 * a valid cache file.
	 **/
	CacheReader( const QString & fileName,
		     DirTree       * tree,
		     DirInfo       * parent,
		     bool            markFromCache );

    public:

	/**
	 * Public constructor with only a filename and tree.  The contents of
	 * the cache file will be placed at the root of the tree.
	 **/
	CacheReader( const QString & fileName,
		     DirTree       * tree ):
	    CacheReader ( fileName, tree, nullptr, false )
	{}

	/**
	 * Public constructor with a DirInfo object, used to automatically
	 * fill a portion of a tree while it is being read.  The cache file
	 * is tested to see if its first entry matches the given directory.
	 * Directories read from the cache file will be marked so the user
	 * can be made aware of what has happened.
	 **/
	CacheReader( const QString & fileName,
		     DirTree       * tree,
		     DirInfo       * dir,
		     DirInfo       * parent );

	/**
	 * Destructor
	 **/
	~CacheReader();

	/**
	 * Suppress copy and assignment constructors (would not do anything sensible)
	 **/
	CacheReader( const CacheReader & ) = delete;
	CacheReader & operator=( const CacheReader & ) = delete;

	/**
	 * Read at most maxLines from the cache file (check with eof() if the
	 * end of file is reached yet) or the entire file (if maxLines is 0).
	 *
	 * Returns true if OK and there is more to read, false otherwise.
	 **/
	bool read( int maxLines );

	/**
	 * Returns true if the end of the cache file is reached (or if there
	 * was an error).
	 **/
	bool eof() const;

	/**
	 * Returns true if reading the cache file went OK.
	 **/
	bool ok() const { return _ok; }

	/**
	 * Returns the tree associated with this reader.
	 **/
//	DirTree * tree() const { return _tree; }


    protected:

	/**
	 * Returns whether the absolute path of the first directory in this
	 * cache file matches the given directory.
	 *
	 * This method expects the cache file to be just opened without any
	 * previous read() operations on the file. If this is not the case,
	 * call rewind() immediately before firstDir().
	 **/
	bool isDir( const QString & dirName );

	/**
	 * Converts a string representing a number of bytes into a FileSize
	 * return value.
	 **/
	FileSize readSize( const char * size_str );

	/**
	 * Check this cache's header (see if it is a QDirStat cache at all)
	 **/
	void checkHeader();

	/**
	 * Use _fields to add one item to _tree.
	 **/
	void addItem();

	/**
	 * Read the next line that is not empty or a comment and store it in
	 * _line.
         *
         * Returns true if OK, false if error.
	 **/
	bool readLine();

	/**
	 * split the current input line into fields separated by whitespace.
	 **/
	void splitLine();

	/**
	 * Returns the start of field no. 'no' in the current input line
	 * after splitLine().
	 **/
	const char * field( int no ) const;

	/**
	 * Return an unescaped version of 'rawPath'.
	 **/
	QString unescapedPath( const QString & rawPath ) const;

	/**
	 * Clean a path: Replace duplicate (or triplicate or more) slashes with
	 * just one. QUrl doesn't seem to handle those well.
	 **/
	QString cleanPath( const QString & rawPath ) const;

	/**
	 * Recursively set the read status of all dirs from 'dir' on, send tree
	 * signals and finalize local (i.e. clean up empty or unneeded dot
	 * entries).
	 **/
	void finalizeRecursive( DirInfo * dir );

	/**
	 * Cascade a read error up to the toplevel directory node read by this
	 * cache file.
	 **/
	void setReadError( DirInfo * dir ) const;


    private:

	//
	// Data members
	//

	gzFile    _cache;
	char      _buffer[ MAX_CACHE_LINE_LEN + 1 ];
	char    * _line		{ _buffer };
	int       _lineNo	{ 0 };
	char    * _fields[ MAX_FIELDS_PER_LINE ];
	int       _fieldsCount;
	bool      _markFromCache;
	bool      _ok		{ false };
        int       _errorCount	{ 0 };

	DirTree	* _tree;
	DirInfo * _parent; // parent directory if there is one
	DirInfo * _toplevel	{ nullptr }; // the parent if there is one, otherwise the top level of the cache file
	DirInfo * _latestDir	{ nullptr }; // the latest drectory read from the cache file, parent to subsequent file children

        QRegularExpression	_multiSlash { "//+" };
    };

}	// namespace QDirStat


#endif // ifndef DirTreeCache_h

