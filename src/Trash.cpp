/*
 *   File name: Trash.h
 *   Summary:   Implementation of the XDG Trash spec for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <sys/stat.h> // mkdir(), struct stat, S_ISDIR(), etc.
#include <unistd.h> // getuid()

#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QProcessEnvironment>
#include <QStringBuilder>
#include <QTextStream>

#include "Trash.h"
#include "Exception.h"


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

	    const int result = mkdir( path.toUtf8(), 0700 );
	    if ( result < 0 )
	    {
		const QString what{ "Could not create directory %1: %2" };
		THROW( ( FileException{ path, what.arg( path ).arg( formatErrno() ) } ) );
	    }
	}
    }


    /**
     * Return the device of file or directory 'path'.
     **/
    dev_t device( const QString & path )
    {
	struct stat statBuf;
	const int result = stat( path.toUtf8(), &statBuf );
	if ( result < 0 )
	{
	    logError() << "stat( " << path << " ) failed: " << formatErrno() << Qt::endl;

	    return dev_t( -1 );
	}

	return statBuf.st_dev;
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

	return "/";
    }


    TrashDir * createTrashDir( const QString & path, dev_t dev )
    {
	const QString topDir = toplevel( path, dev );

	// Check if there is $TOPDIR/.Trash
	QString trashPath = topDir % "/.Trash"_L1;

	struct stat statBuf;
	const int result = stat( trashPath.toUtf8(), &statBuf );

	if ( result < 0 )
	{
	    if ( errno != ENOENT ) // No such file or directory
	    {
		// stat() failed for some other reason (not "no such file or directory")
		logError() << "stat failed for " << trashPath << ": " << formatErrno() << Qt::endl;
		return nullptr;
	    }

	    // No $TOPDIR/.Trash: Use $TOPDIR/.Trash-$UID
	    logInfo() << "No " << trashPath << Qt::endl;
	    trashPath += '-' % QString::number( getuid() );
	}
	else // stat() was successful
	{
	    const mode_t mode = statBuf.st_mode;

	    if ( !S_ISDIR( mode ) )
	    {
		logWarning() << trashPath << " is not a directory" << Qt::endl;
		return nullptr;
	    }

	    if ( !( mode & S_ISVTX ) )
	    {
		logWarning() << "Sticky bit missing on " << trashPath << Qt::endl;
		return nullptr;
	    }

	    // Use $TOPDIR/.Trash/$UID
	    trashPath += '/' % QString::number( getuid() );
	}

	logInfo() << "Using " << trashPath << Qt::endl;

	TrashDir * newTrashDir;
	try
	{
	    newTrashDir = new TrashDir{ trashPath, dev };
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
    const dev_t homeDevice  = device( homePath );
    const QString homeTrash = [ &homePath ]()
    {
	const QString xdgHome = QProcessEnvironment::systemEnvironment().value( "XDG_DATA_HOME", QString{} );
	return xdgHome.isEmpty() ? homePath % "/.local/share"_L1 : xdgHome;
    }() % "/Trash"_L1;

    // new TrashDir can throw, although very unlikely for the home device
    try
    {
	_homeTrashDir = new TrashDir{ homeTrash, homeDevice };
    }
    catch ( const FileException & ex )
    {
	CAUGHT( ex );
	logWarning() << "Cannot create home trash dir " << homeTrash << Qt::endl;

	_homeTrashDir = nullptr;
    }

    // Store it even if it is null
    _trashDirs[ homeDevice ] = _homeTrashDir;
}


TrashDir * Trash::trashDir( const QString & path )
{
    const dev_t dev = device( path );

    if ( _trashDirs.contains( dev ) )
	return _trashDirs[ dev ];

    TrashDir * newTrashDir = createTrashDir( path, dev );
    if ( newTrashDir )
	return newTrashDir;

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

	const QString targetName = dir->uniqueName( path );
	dir->createTrashInfo( path, targetName );
	dir->move( path, targetName );
    }
    catch ( const FileException & ex )
    {
	CAUGHT( ex );
	logError() << "Move to trash failed for " << path << Qt::endl;

	return false;
    }

    logInfo() << "Successfully moved to trash: " << path << Qt::endl;

    return true;
}

/*
bool Trash::restore( const QString & path )
{
    Q_UNUSED( path )

    // TO DO
    // TO DO
    // TO DO

    return true;
}


void Trash::empty()
{
    // TO DO
    // TO DO
    // TO DO
}
*/



TrashDir::TrashDir( const QString & path, dev_t device ):
    _path{ path },
    _device{ device }
{
    // Will throw if a directory doesn't exist and cannot be created
    ensureDirExists( path        );
    ensureDirExists( filesPath() );
    ensureDirExists( infoPath()  );

    // logDebug() << "Created TrashDir " << path << Qt::endl;
}


QString TrashDir::uniqueName( const QString & path )
{
    const QDir filesDir{ filesPath() };

    const QFileInfo file{ path };
    QString name = file.fileName();

    for ( int i = 1; filesDir.exists( name ); ++i )
    {
	const QString baseName = file.baseName();
	const QString suffix   = file.completeSuffix();
	name = QString{ "%1_%2" }.arg( baseName ).arg( i );
	if ( !suffix.isEmpty() )
	    name += '.' % suffix;
    }

    return name;
}


void TrashDir::createTrashInfo( const QString & path,
				const QString & targetName )
{
    QFile trashInfo( infoPath() % '/' % targetName % ".trashinfo"_L1 );

    if ( !trashInfo.open( QIODevice::WriteOnly | QIODevice::Text ) )
	THROW( ( FileException{ trashInfo.fileName(), "Can't open "_L1 % trashInfo.fileName() } ) );

    QTextStream str( &trashInfo );
    str << "[Trash Info]" << Qt::endl;
    str << "Path=" << path << Qt::endl;
    str << "DeletionDate=" << QDateTime::currentDateTime().toString( Qt::ISODate ) << Qt::endl;
}


void TrashDir::move( const QString & path,
		     const QString & targetName )
{
    QFile file( path );
    const QString targetPath = filesPath() % '/' % targetName;

    // QFile::rename will try to move, then try to copy-and-delete, but this will fail for directories
    const bool success = file.rename( targetPath );
    if ( !success )
	THROW( ( FileException{ path, QString{ "Could not move %1 to %2" }.arg( path, targetPath ) } ) );
}
