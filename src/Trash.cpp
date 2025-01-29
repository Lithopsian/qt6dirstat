/*
 *   File name: Trash.h
 *   Summary:   Implementation of the XDG Trash spec for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <sys/stat.h> // mkdir(), struct stat, S_ISDIR(), etc.
#include <stdio.h> // FILE, fdopen, etc.
#include <unistd.h> // getuid()

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStringBuilder>
#include <QUrl>

#include "Trash.h"
#include "Exception.h"
#include "MountPoints.h"
#include "SysUtil.h"


using namespace QDirStat;


namespace
{
    /**
     * Returns whether the trash directory, as well as the "files" and "info"
     * directories, and their contents, can be read and modified.
     **/
    bool isTrashAccessible( const QString & trashPath )
    {
	if ( !SysUtil::canAccess( trashPath ) )
	    return false;

	if ( !SysUtil::canAccess( Trash::filesDirPath( trashPath ) ) )
	    return false;

	if ( !SysUtil::canAccess( Trash::infoDirPath( trashPath ) ) )
	    return false;

	return true;
    }


    /**
     * Return the device of file or directory 'path'.
     **/
    dev_t device( const QString & path )
    {
	struct stat statInfo;
	if ( SysUtil::stat( path, statInfo ) < 0 )
	{
	    logError() << "stat( " << path << " ) failed: " << formatErrno() << Qt::endl;

	    return static_cast<dev_t>( -1 );
	}

	return statInfo.st_dev;
    }


    /**
     * Return the path of the files directory for the trash directory
     * 'trashDir'.
     **/
    QString homeTrash( const QString & homePath )
    {
	const auto homeTrashParent = [ &homePath ]()
	{
	    const QString xdgHome = QProcessEnvironment::systemEnvironment().value( "XDG_DATA_HOME", QString{} );
	    return xdgHome.isEmpty() ? homePath % "/.local/share"_L1 : xdgHome;
	};

	return homeTrashParent() % "/Trash"_L1;
    }


    /**
     * Return the path of the main trash directory for a filesystem with
     * top directory 'topDir'.
     **/
    QString mainTrash( const QString & trashRoot )
    {
	return trashRoot % '/' % QString::number( getuid() );
    }


    /**
     * Return the path of the main trash directory for a filesystem with
     * top directory 'topDir'.
     **/
    QString userTrash( const QString & trashRoot )
    {
	return trashRoot % '-' % QString::number( getuid() );
    }


    /**
     * Find the toplevel directory (the mount point) for the device that 'path'
     * is on.
     **/
    QString toplevel( const QString & rawPath, dev_t dev )
    {
	const QFileInfo fileInfo{ rawPath };
	QString path = fileInfo.canonicalPath();
	QStringList components = path.split( u'/', Qt::SkipEmptyParts );

	// Work up the directory tree
	while ( !components.isEmpty() )
	{
	    // See if we are the top level on this device
	    components.removeLast();
	    const QString nextPath = '/' % components.join( u'/' );
	    if ( device( nextPath ) != dev )
		return path;

	    path = nextPath;
	}

	// Return an empty string for "/" to avoid a double slash
	return "";
    }


    TrashDir * createTrashDir( const QString & path, dev_t dev )
    {
	const QString trashPath = [ &path, dev ]()
	{
	    const QString topDir = toplevel( path, dev );

	    // Check if there is $TOPDIR/.Trash
	    const QString trashRoot = Trash::trashRoot( topDir );
	    if ( Trash::isValidMainTrash( trashRoot ) )
	    {
		// Use $TOPDIR/.Trash/$UID
		return mainTrash( trashRoot );
	    }

	    if ( errno != ENOENT )
	    {
		// stat() failed for some other reason (not "no such file or directory")
		logError() << "stat failed for " << trashRoot << ": " << formatErrno() << Qt::endl;
		return QString{};
	    }

	    // No valid $TOPDIR/.Trash: use $TOPDIR/.Trash-$UID
	    return userTrash( trashRoot );
	}();

	if ( trashPath.isEmpty() )
	    return nullptr;

	logInfo() << "Using " << trashPath << Qt::endl;

	TrashDir * newTrashDir;
	try
	{
	    newTrashDir = new TrashDir{ trashPath };
	}
	catch ( const FileException & ex )
	{
	    CAUGHT( ex );
	    logWarning() << "Failed to create trash directory " << trashPath << Qt::endl;

	    return nullptr;
	}

	return newTrashDir;
    }

} // namespace


Trash::Trash()
{
    // Best guess for the home trash path
    const QString homePath  = QDir::homePath();
    const QString homeTrashPath = homeTrash( homePath );

    // new TrashDir can throw, although very unlikely for the home device
    try
    {
	_homeTrashDir = new TrashDir{ homeTrashPath };
    }
    catch ( const FileException & ex )
    {
	CAUGHT( ex );
	logWarning() << "Cannot create home trash dir " << homeTrashPath << Qt::endl;

	_homeTrashDir = nullptr;
    }

    // Store it even if it is null
    _trashDirs[ device( homePath ) ] = _homeTrashDir;
}


TrashDir * Trash::trashDir( const QString & path )
{
    const dev_t dev = device( path );

    if ( _trashDirs.contains( dev ) )
	return _trashDirs[ dev ];

    TrashDir * newTrashDir = createTrashDir( path, dev );
    if ( newTrashDir )
    {
	_trashDirs[ dev ] = newTrashDir;
	return newTrashDir;
    }

    logWarning() << "Falling back to home trash dir: " << _homeTrashDir->path() << Qt::endl;

    return _homeTrashDir;
}


bool Trash::trash( const QString & path )
{
    try
    {
	TrashDir * dir = trashDir( path );
	if ( !dir )
	    return false;

	dir->trash( path );
    }
    catch ( const FileException & ex )
    {
	CAUGHT( ex );
	return false;
    }

    logInfo() << "Successfully moved to trash: " << path << Qt::endl;

    return true;
}


QStringList Trash::trashRoots()
{
    QStringList trashRoots;

    const QString homeTrashPath = homeTrash( QDir::homePath() );
    if ( isTrashAccessible( homeTrashPath ) )
	trashRoots << homeTrashPath;

    MountPoints::reload();

    for ( MountPointIterator it{ false, true }; *it; ++it )
    {
	const QString trashRootPath = trashRoot( it->path() == u'/' ? QString{} : it->path() );

	if ( isValidMainTrash( trashRootPath ) )
	{
	    const QString mainTrashPath = mainTrash( trashRootPath );
	    if ( isTrashAccessible( mainTrashPath ) )
		trashRoots << mainTrashPath;
	}

	const QString userTrashPath = userTrash( trashRootPath );
	if ( isTrashAccessible( userTrashPath ) )
	    trashRoots << userTrashPath;
    }

    return trashRoots;
}


bool Trash::isInTrashDir( const QString & path )
{
    const QStringList trashRootPaths = trashRoots();
    for ( const QString & trashRootPath : trashRootPaths )
    {
	if ( path.startsWith( trashRootPath ) )
	    return true;
    }

    return false;
}


bool Trash::isValidMainTrash( const QString & trashRoot )
{
    struct stat statInfo;
    if ( SysUtil::stat( trashRoot, statInfo ) != 0 )
	return false;

    const mode_t mode = statInfo.st_mode;
    if ( !S_ISDIR( mode ) )
    {
	logWarning() << trashRoot << " is not a directory" << Qt::endl;
	return false;
    }

    if ( !( mode & S_ISVTX ) || !( mode & S_IXOTH ) )
    {
	logWarning() << "Sticky bit not set " << trashRoot << Qt::endl;
	return false;
    }

    return true;
}




namespace
{
    /**
     * Create a directory if it doesn't exist. This throws an exception if
     * the directory cannot be created.
     **/
    void ensureDirExists( const QString & path )
    {
	const QDir dir{ path };
	if ( !dir.exists() )
	{
	    logInfo() << "mkdir " << path << Qt::endl;

	    if ( mkdir( path.toUtf8(), 0700 ) < 0 )
	    {
		const QString what{ "Could not create directory %1: %2" };
		THROW( ( FileException{ path, what.arg( path, formatErrno() ) } ) );
	    }
	}
    }


    /**
     * Create an entry name for 'name', formed by adding the number 'i' to the
     * base name of 'name', followed by any suffix.  Any unique name would be
     * acceptable, but this makes it a little nicer to look at.
     **/
    QString makeEntryName( const QString & name, int i )
    {
	if ( i == 0 )
	    return name;

	// This split happens for every i increment, but i = 0 will be the most common case
	const auto lastDotIndex = name.lastIndexOf( u'.' );
	const QString baseName = lastDotIndex > 0 ? name.left( lastDotIndex ) : name;
	const QString suffix   = lastDotIndex > 0 ? name.mid( lastDotIndex ) : QString{};

	return QString{ "%1_%2%3" }.arg( baseName ).arg( i ).arg( suffix );
    }


    /**
     * Write to fd, an open trashinfo file.  The current date/time is used and
     * the original path 'path'.  The function returns whether the information
     * was successfully written to 'fd'.
     **/
    bool writeTrashInfo( int fd, const QString & path )
    {
	FILE * fp = fdopen( fd, "w" );
	if ( !fp )
	{
	    close( fd );
	    return false;
	}

	QTextStream str{ fp, QIODevice::WriteOnly | QIODevice::Text };
	str << TrashDir::trashInfoTag() << '\n';
	str << TrashDir::trashInfoPathTag() << QUrl::toPercentEncoding( path, "/" ) << '\n';
	str << TrashDir::trashInfoDateTag() << QDateTime::currentDateTime().toString( Qt::ISODate ) << Qt::endl;

//	if ( fprintf( fp, "%s\n%s%s\n%s%s\n",
//	              TrashDir::trashInfoTag().latin1(),
//	              TrashDir::trashInfoPathTag().latin1(),
//	              QUrl::toPercentEncoding( path, "/" ).constData(),
//	              TrashDir::trashInfoDateTag().latin1(),
//	              QDateTime::currentDateTime().toString( Qt::ISODate ).toUtf8().constData() ) < 0 )
//	{
//	    fclose( fp );
//	    return false;
//	}

	if ( fclose( fp ) != 0 )
	    return false;

	return true;
    }


    /**
     * Create a trashinfo file and move 'path' to the corresponding entry name
     * in 'filesDirPath'.  The entry name is constructed to be unique, both in
     * 'filesDirPath' and 'infoDirPath'.  This is done using O_CREAT and O_EXCL
     * to prevent races.
     *
     * One special case is where the terminal component of 'path' is long
     * enough that appending ".trashinfo" and any numerals added to it makes it
     * too long.  The filename is simply truncated until it is short enough,
     * brutal but hopefully very rare.
     *
     * If there is already a trash entry with the chosen name (but obviously
     * there wasn't a trashinfo file for it), that trash entry will be
     * overwritten.
     *
     * Note that some C file I/O functions are used, partly because Qt could
     * not open a QFile with the equivalent of Q_EXCL until 5.11, partly for
     * speed.
     **/
    void moveToTrash( const QString & path, const QString & filesDirPath, const QString & infoDirPath )
    {
	QString pathDir;
	QString name;
	SysUtil::splitPath( path, pathDir, name );

	// Loop until we manage to open a trashinfo file that didn't exist before
	int i = 0;
	while ( true )
	{
	    const QString entryName = makeEntryName( name, i );
	    const QString trashinfoPath{ infoDirPath % '/' % entryName % Trash::trashInfoSuffix() };
	    const QByteArray trashinfoStr = trashinfoPath.toUtf8();

	    const int fd = open( trashinfoStr, O_CREAT | O_EXCL | O_WRONLY, 0600 );
	    if ( fd >= 0 )
	    {
		const auto throwException = [ &path, &trashinfoStr ]( const QString & msg )
		{
		    const QString fullMsg = msg % ": " % formatErrno();
		    unlink( trashinfoStr );
		    THROW( ( FileException{ path, fullMsg } ) );
		};

		const QString targetPath{ filesDirPath % '/' % entryName };

		if ( !writeTrashInfo( fd, path ) )
		    throwException( QString{ "Could not write %1" }.arg( trashinfoPath ) );

		if ( rename( path.toUtf8(), targetPath.toUtf8() ) != 0 )
		    throwException( QString{ "Could not move %1 to %2" }.arg( path, targetPath ) );

		return; // SUCCESS
	    }
	    else if ( errno == EEXIST )
	    {
		// That trashinfo file already exists, try a bigger number
		++i;
	    }
	    else if ( errno == ENAMETOOLONG )
	    {
		// Sanity check, this would be very odd
		if ( name.size() < 2 )
		    return;

		// Just chop one character at a time, slow but this is going to be very rare
		name.chop( 1 );
	    }
	    else
	    {
		const QString msg{ "Could not create trashinfo %1: %2" };
		THROW( ( FileException{ path, msg.arg( trashinfoPath, formatErrno() ) } ) );
	    }
	}
    }

} // namespace


TrashDir::TrashDir( const QString & path ):
    _path{ path }
{
    // Will throw if a directory doesn't exist and cannot be created
    ensureDirExists( path           );
    ensureDirExists( infoDirPath()  );
    ensureDirExists( filesDirPath() );

    // logDebug() << "Created TrashDir " << path << Qt::endl;
}


void TrashDir::trash( const QString & path )
{
    moveToTrash( path, filesDirPath(), infoDirPath() );
}
