/*
 *   File name: Trash.h
 *   Summary:   Implementation of the XDG Trash spec for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <sys/stat.h> // mkdir(), struct stat, S_ISDIR(), etc.

#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QProcessEnvironment>
#include <QStringBuilder>
#include <QTextStream>
#include <QUrl>

#include "Trash.h"
#include "Exception.h"
#include "SysUtil.h"


using namespace QDirStat;


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
		return Trash::mainTrashPath( trashRoot );
	    }

	    if ( errno != ENOENT )
	    {
		// stat() failed for some other reason (not "no such file or directory")
		logError() << "stat failed for " << trashRoot << ": " << formatErrno() << Qt::endl;
		return QString{};
	    }

	    // No valid $TOPDIR/.Trash: use $TOPDIR/.Trash-$UID
	    return Trash::userTrashPath( trashRoot );
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


    /**
     * Create a name for 'path' that is unique within 'dir', both within the
     * paths directory and the info directory. If the simple name 'path'
     * already exists, then append a number to the filename base to create a
     * unique filename.
     **/
    QString uniqueName( const QString & path, TrashDir * dir )
    {
	const QString filesPath = dir->filesDirPath();
	const QDir filesDir{ filesPath };

	const QString infoPath = dir->infoDirPath();
	const QDir infoDir{ infoPath };

	const QFileInfo file{ path };
	QString name = file.fileName();

	for ( int i = 1; filesDir.exists( name ) || infoDir.exists( name % Trash::trashInfoSuffix() ); ++i )
	{
	    const QString baseName = file.baseName();
	    const QString suffix   = file.completeSuffix();
	    name = QString{ "%1_%2" }.arg( baseName ).arg( i );
	    if ( !suffix.isEmpty() )
		name += '.' % suffix;
	}

	return name;
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


QString Trash::homeTrash( const QString & homePath )
{
    const auto homeTrashParent = [ &homePath ]()
    {
	const QString xdgHome = QProcessEnvironment::systemEnvironment().value( "XDG_DATA_HOME", QString{} );
	return xdgHome.isEmpty() ? homePath % "/.local/share"_L1 : xdgHome;
    };

    return homeTrashParent() % "/Trash"_L1;
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

	const QString targetName = uniqueName( path, dir );
	dir->createTrashInfo( path, targetName );
	dir->move( path, targetName );
    }
    catch ( const FileException & ex )
    {
	CAUGHT( ex );
	return false;
    }

    logInfo() << "Successfully moved to trash: " << path << Qt::endl;

    return true;
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


TrashDir::TrashDir( const QString & path ):
    _path{ path }
{
    // Will throw if a directory doesn't exist and cannot be created
    ensureDirExists( path           );
    ensureDirExists( infoDirPath()  );
    ensureDirExists( filesDirPath() );

    // logDebug() << "Created TrashDir " << path << Qt::endl;
}


void TrashDir::createTrashInfo( const QString & path, const QString & targetName )
{
    QFile trashInfo{ infoPath( targetName ) };

#if QT_VERSION >= QT_VERSION_CHECK( 5, 11, 0 )
    if ( !trashInfo.open( QIODevice::NewOnly | QIODevice::Text ) ) // O_EXCL access
#else
    if ( !trashInfo.open( QIODevice::WriteOnly | QIODevice::Text ) ) // NewOnly not available, so just open
#endif
    {
	const QString msg{ "Can't open %1: %2" };
	THROW( ( FileException{ trashInfo.fileName(), msg.arg( trashInfo.fileName(), formatErrno() ) } ) );
    }

    QTextStream str{ &trashInfo };
    str << trashInfoTag() << Qt::endl;
    str << trashInfoPathTag() << QUrl::toPercentEncoding( path ) << Qt::endl;
    str << trashInfoDateTag() << QDateTime::currentDateTime().toString( Qt::ISODate ) << Qt::endl;
}


void TrashDir::move( const QString & path, const QString & targetName )
{
    const QString targetPath = filesDirPath() % '/' % targetName;

    // QFile::rename will try to move, then try to copy-and-delete, but this will fail for directories
    const bool success = QFile{ path }.rename( targetPath );
    if ( !success )
    {
	// Don't leave trashinfo files lying around with no corresponding trashed file
	QFile{ infoPath( targetName ) }.remove();

	const QString msg{ "Could not move %1 to %2: %3" };
	THROW( ( FileException{ path, msg.arg( path, targetPath, formatErrno() ) } ) );
    }
}
