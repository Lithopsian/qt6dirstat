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
#include <QProcess>
#include <QProcessEnvironment>
#include <QStringBuilder>
#include <QUrl>

#include "Trash.h"
#include "Exception.h"
#include "MainWindow.h"
#include "MountPoints.h"
#include "QDirStatApp.h"
#include "SysUtil.h"


using namespace QDirStat;


namespace
{
    /**
     * Returns all trash root paths, including those that don't exist or are
     * not accessible.
     **/
    QStringList allTrashRoots()
    {
	return Trash::trashRoots( true );
    }


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
     * Return the device number of file or directory 'path'.
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
    QString homeTrash()
    {
	const auto homeTrashParent = []()
	{
	    const QString xdgHome =
		QProcessEnvironment::systemEnvironment().value( "XDG_DATA_HOME", QString{} );
	    return xdgHome.isEmpty() ? QDir::homePath() % "/.local/share"_L1 : xdgHome;
	};

	return homeTrashParent() % "/Trash"_L1;
    }


    /**
     * Return the path of the main trash directory for 'trashRoot'
     * (ie. /.Trash/1000).
     **/
    QString mainTrash( const QString & trashRoot )
    {
	return trashRoot % '/' % QString::number( getuid() );
    }


    /**
     * Return the path of the user trash directory for 'trashRoot'
     * (ie. /.Trash-1000).
     **/
    QString userTrash( const QString & trashRoot )
    {
	return trashRoot % '-' % QString::number( getuid() );
    }


    /**
     * Find the toplevel directory (the mount point) for the device that
     * 'rawPath' (or its symlink target) is on.
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


    /**
     * Return whether 'trashRoot' is a directory (not a symlink) and has
     * the sticky bit (and execute permission) set.
     *
     * Note that if the lstat() call fails, including because the directory
     * does not exist, this function returns false.  errno must be checked
     * to distinguish the reason for the failure.
     **/
    bool isValidMainTrash( const QString & trashRoot )
    {
	struct stat statInfo;
	if ( SysUtil::stat( trashRoot, statInfo ) != 0 )
	    return false;

	errno = 0; // no system error set, but might still return false

	const mode_t mode = statInfo.st_mode;
	if ( !S_ISDIR( mode ) )
	{
	    logWarning() << trashRoot << " is not a directory" << Qt::endl;
	    return false;
	}

	if ( !( mode & S_ISVTX ) )
	{
	    logWarning() << "Sticky bit not set on " << trashRoot << Qt::endl;
	    return false;
	}

	return true;
    }


    /**
     * Attempt to create a (non-home) TrashDir object for 'path'.  'dev' is
     * the device number for 'path'.  The trash directory is located at
     * $TOPDIR/.Trash/$UID or $TOPDIR/.Trash-$UID.  $TOPDIR is the highest
     * level directory still on device 'dev'.
     *
     * The directory is created by attempting to construct a TrashDir object.
     * This throws an exception if the directory cannot be created and this
     * function then returns 0.
     **/
    TrashDir * createToplevelTrashDir( const QString & path, dev_t dev )
    {
	const QString trashPath = [ &path, dev ]()
	{
	    const QString topDir = toplevel( path, dev );

	    // Check if there is $TOPDIR/.Trash
	    const QString trashRoot = Trash::trashRoot( topDir );
	    if ( isValidMainTrash( trashRoot ) )
	    {
		// Use $TOPDIR/.Trash/$UID
		return mainTrash( trashRoot );
	    }

	    // not an error if $TOPDIR/.Trash doesn't exist
	    if ( errno != ENOENT )
	    {
		if ( errno != 0 )
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


Trash::Trash():
    _homeTrashPath{ homeTrash() },
    _homeTrashDev{ device( _homeTrashPath ) }
{
    createHomeTrashDir();
}


void Trash::createHomeTrashDir()
{
    // TrashDir constructor can throw, although very unlikely for the home device
    try
    {
	_trashDirs[ _homeTrashDev ] = new TrashDir{ _homeTrashPath };
    }
    catch ( const FileException & ex )
    {
	CAUGHT( ex );
    }
}


TrashDir * Trash::trashDir( const QString & path )
{
    const dev_t dev = [ this, &path ]()
    {
	// If configured, force using only the home trash directory
	if ( app()->mainWindow()->onlyUseHomeTrashDir() )
	    return _homeTrashDev;

	// Or get st_dev from stat( path )
	return device( path );
    }();

    // Use any previously-used TrashDir for 'dev'
    if ( _trashDirs.contains( dev ) )
	return _trashDirs[ dev ];

    // If a TrashDir for 'dev' hasn't been created yet, do it now (only for non-home devices)
    if ( dev != _homeTrashDev )
    {
	TrashDir * newTrashDir = createToplevelTrashDir( path, dev );
	if ( newTrashDir )
	{
	    _trashDirs[ dev ] = newTrashDir;
	    return newTrashDir;
	}

	// Cannot create a $TOPDIR trash, so try to use the home trash
	logWarning() << "Falling back to home trash dir: " << _homeTrashPath << Qt::endl;
    }

    // The home trash directory should always exist, but in case it doesn't, try again to create it
    if ( !_trashDirs.contains( _homeTrashDev ) )
	createHomeTrashDir();

    return _trashDirs.value( _homeTrashDev, nullptr );
}


bool Trash::trash( const QString & path, QString & msg )
{
    try
    {
	TrashDir * dir = trashDir( path );
	if ( !dir )
	{
	    msg = QObject::tr( "No trash directory for '%1'" ).arg( path );
	    return false;
	}

	dir->trash( path );
    }
    catch ( const FileException & ex )
    {
	CAUGHT( ex );
	msg = ex.what();
	return false;
    }

    logInfo() << "Successfully moved to trash: " << path << Qt::endl;

    return true;
}


QStringList Trash::trashRoots( bool allRoots )
{
    QStringList trashRoots;

    const QString homeTrashPath = homeTrash();
    if ( allRoots || isTrashAccessible( homeTrashPath ) )
	trashRoots << homeTrashPath;

    MountPoints::reload();

    for ( MountPointIterator it{ false, true }; *it; ++it )
    {
	const QString trashRootPath = trashRoot( it->path() == u'/' ? QString{} : it->path() );

	if ( isValidMainTrash( trashRootPath ) )
	{
	    const QString mainTrashPath = mainTrash( trashRootPath );
	    if ( allRoots || isTrashAccessible( mainTrashPath ) )
		trashRoots << mainTrashPath;
	}

	const QString userTrashPath = userTrash( trashRootPath );
	if ( allRoots || isTrashAccessible( userTrashPath ) )
	    trashRoots << userTrashPath;
    }

    return trashRoots;
}


bool Trash::isTrashDir( const QString & path )
{
    const QString comparePath{ path % '/' };

    const QStringList trashRootPaths = allTrashRoots();
    for ( const QString & trashRootPath : trashRootPaths )
    {
	// Is path in the subtree of trashRootPath, or is trashRootPath within the subtree of path
	const QString compareTrashRoot{ trashRootPath % '/' };
	if ( comparePath.startsWith( compareTrashRoot ) || compareTrashRoot.startsWith( comparePath ) )
	    return true;
    }

    return false;
}


bool Trash::move( const QString & path, const QString targetPath, QString & msg, bool copyAndDelete )
{
    // Standard move, only works on single filesystem
    if ( rename( path.toUtf8(), targetPath.toUtf8() ) == 0 )
	return true;

    // Anything other than a cross-device exception with copyAndDelete specified is fatal
    if ( errno != EXDEV || !copyAndDelete )
    {
	msg = QObject::tr( "Failed to move '%1' to '%2': %3" ).arg( path, targetPath, formatErrno() );
	return false;
    }

    // Attempt copy and delete
    if ( QProcess::execute( "cp", { "-a", path, targetPath } ) == 0 )
    {
	// Copy succeeded, try to remove the original subtree
	if ( QProcess::execute( "rm", { "-rf", path } ) == 0 )
	    return true;

	msg = QObject::tr( "Unable to remove '%1' after copying to '%2'" ).arg( path, targetPath );

	// Can't remove (all of) the original subtree, copy back to ensure it is all there
	if ( QProcess::execute( "cp", { "-na", targetPath, path } ) != 0  )
	{
	    msg = QObject::tr( "Unable to replace '%1' after failing to remove - full original is at '%2'" )
	          .arg( path, targetPath );
	    return false;
	}
    }
    else
    {
	msg = QObject::tr( "Unable to copy '%1' to '%2'" ).arg( path, targetPath );
    }

    // Failed to copy or remove original, remove anything left at the target
    QProcess::execute( "rm", { "-rf", targetPath } );

    return false;
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
		const QString what = QObject::tr( "Could not create directory '%1': %2" );
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

	// QTextStream handles EOL mappings (for Windows!) and is simple but still reasonably fast
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
	QString name = SysUtil::baseName( path );

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
		const QString targetPath{ filesDirPath % '/' % entryName };

		if ( !writeTrashInfo( fd, path ) )
		{
		    const QString msg =
			QObject::tr( "Could not write '%1': %2" ).arg( trashinfoPath, formatErrno() );
		    unlink( trashinfoStr );
		    THROW( ( FileException{ path, msg } ) );
		}

		QString msg;
		if ( !Trash::move( path, targetPath, msg, app()->mainWindow()->copyAndDeleteTrash() ) )
		{
		    unlink( trashinfoStr );
		    THROW( ( FileException{ path, msg } ) );
		}

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
		const QString msg = QObject::tr( "Could not create trashinfo '%1': %2" );
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

    // logDebug() << "Created TrashDir " << this << Qt::endl;
}


void TrashDir::trash( const QString & path )
{
    moveToTrash( path, filesDirPath(), infoDirPath() );
}
