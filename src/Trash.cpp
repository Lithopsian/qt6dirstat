/*
 *   File name: Trash.h
 *   Summary:	Implementation of the XDG Trash spec for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QProcessEnvironment>
#include <QTextStream>

#include "Trash.h"
#include "Logger.h"
#include "Exception.h"


namespace
{
    /**
     * Return the device of file or directory 'path'.
     **/
    dev_t device( const QString & path )
    {
	dev_t dev = 0;
	struct stat statBuf;
	const int result = stat( path.toUtf8(), &statBuf );
	dev = statBuf.st_dev;

	if ( result < 0 )
	{
	    logError() << "stat( " << path << " ) failed: "
		       << formatErrno() << Qt::endl;

	    dev = dev_t( -1 );
	}

	return dev;
    }


    /**
     * Find the toplevel directory (the mount point) for the device that 'path'
     * is on.
     **/
    QString toplevel( const QString & rawPath )
    {
	const dev_t dev = device( rawPath );
	const QFileInfo fileInfo( rawPath );
	QString path = fileInfo.canonicalPath();
	QStringList components = path.split( "/", Qt::SkipEmptyParts );
	QString lastPath;

	// Go one directory level up as long as we are on the same device

	while ( ! components.isEmpty() && device( path ) == dev )
	{
	    lastPath = path;
	    components.removeLast();
	    path = "/" + components.join( "/" );
	}

	if ( components.isEmpty() && device( "/" ) == dev )
	    lastPath = "/";

	return lastPath;
    }

} // namespace

using namespace QDirStat;


Trash::Trash()
{
    const dev_t homeDevice = device( QDir::homePath() );

    const QString xdg_data_home = QProcessEnvironment::systemEnvironment().value( "XDG_DATA_HOME", QString() );

    const QString homeTrash =
	xdg_data_home.isEmpty() ? QDir::homePath() + "/.local/share" : xdg_data_home;

    _homeTrashDir = new TrashDir( homeTrash + "/Trash", homeDevice );
    CHECK_NEW( _homeTrashDir );

    _trashDirs[ homeDevice ] = _homeTrashDir;
}


TrashDir * Trash::trashDir( const QString & path )
{
    const dev_t dev = device( path );

    if ( _trashDirs.contains( dev ) )
	return _trashDirs[ dev ];

    const QString topDir = toplevel( path );

    try
    {
	// Check if there is $TOPDIR/.Trash
	QString trashPath = topDir + "/.Trash";

	struct stat statBuf;
	const int result = stat( trashPath.toUtf8(), &statBuf );

	if ( result < 0 && errno == ENOENT ) // No such file or directory
	{
	    // No $TOPDIR/.Trash: Use $TOPDIR/.Trash-$UID
	    logInfo() << "No " << trashPath << Qt::endl;
	    trashPath = topDir + QString( "/.Trash-%1" ).arg( getuid() );
	    logInfo() << "Using " << trashPath << Qt::endl;
	}
	else if ( result < 0 )
	{
	    // stat() failed for some other reason (not "no such file or directory")
	    THROW( FileException( trashPath, "stat() failed for " + trashPath
				  + ": " + formatErrno() ) );
	}
	else // stat() was successful
	{
	    const mode_t mode = statBuf.st_mode;

	    if ( S_ISDIR( mode ) && ( mode & S_ISVTX	) ) // Check sticky bit
	    {
		// Use $TOPDIR/.Trash/$UID
		trashPath += QString( "/%1" ).arg( getuid() );
		logInfo() << "Using " << trashPath << Qt::endl;
	    }
	    else // Not a directory or sticky bit not set
	    {
		if ( ! S_ISDIR( mode ) )
		    THROW( FileException( trashPath, trashPath + " is not a directory" ) );
		else
		    THROW( FileException( trashPath, "Sticky bit required on " + trashPath ) );
	    }
	}

	TrashDir * trashDir = new TrashDir( trashPath, dev );
	CHECK_NEW( trashDir );
	_trashDirs[ dev ] = trashDir;

	return trashDir;
    }
    catch ( const FileException &ex )
    {
	CAUGHT( ex );
	logWarning() << "Falling back to home trash dir: " << _homeTrashDir->path() << Qt::endl;

	return _homeTrashDir;
    }
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
//	logError() << "Move to trash failed for " << path << Qt::endl;

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


namespace
{
    /**
     * Create a directory if it doesn't exist. This throws an exception if
     * 'doThrow' is 'true'.
     *
     * Return 'true' if success, 'false' if error (and doThrow is 'false').
     **/
    bool ensureDirExists( const QString & path, mode_t mode, bool doThrow )
    {
	const QDir dir( path );

	if ( dir.exists() )
	    return true;

	logInfo() << "mkdir " << path << Qt::endl;
	const int result = mkdir( path.toUtf8(), mode );

	if ( result < 0 && doThrow )
	{
	    THROW( FileException( path,
				  QString( "Could not create directory %1: %2" )
				  .arg( path ).arg( formatErrno() ) ) );
	}

	return result >= 0;
    }

} // namespace


TrashDir::TrashDir( const QString & path, dev_t device ):
    _path { path },
    _device { device }
{
    // logDebug() << "Created TrashDir " << path << Qt::endl;

    ensureDirExists( path,	  0700, true );
    ensureDirExists( filesPath(), 0700, true );
    ensureDirExists( infoPath(),  0700, true );
}


QString TrashDir::uniqueName( const QString & path )
{
    const QFileInfo file( path );
    const QDir filesDir( filesPath() );

    const QString baseName  = file.baseName();
    const QString extension = file.completeSuffix();
    int	    count     = 0;
    QString name      = baseName;

    if ( ! extension.isEmpty() )
	name += "." + extension;

    while ( filesDir.exists( name ) )
    {
	name = QString( "%1_%2" ).arg( baseName ).arg( ++count );

	if ( ! extension.isEmpty() )
	    name += "." + extension;
    }

    // We don't care if a .trashinfo file with that name already exists in the
    // Trash/info directory: Without a corresponding file or directory in the
    // Trash/files directory, that .trashinfo file is worthless anyway and can
    // safely be overwritten.

    return name;
}


void TrashDir::createTrashInfo( const QString & path,
				const QString & targetName )
{
    QFile trashInfo( infoPath() + "/" + targetName + ".trashinfo" );

    if ( ! trashInfo.open( QIODevice::WriteOnly | QIODevice::Text ) )
	THROW( FileException( trashInfo.fileName(), "Can't open " + trashInfo.fileName() ) );

    QTextStream str( &trashInfo );
    str << "[Trash Info]" << Qt::endl;
    str << "Path=" << path << Qt::endl;
    str << "DeletionDate=" << QDateTime::currentDateTime().toString( Qt::ISODate ) << Qt::endl;
}


void TrashDir::move( const QString & path,
		     const QString & targetName )
{
    QFile file( path );
    const QString targetPath = filesPath() + "/" + targetName;

    // QFile::rename will try to move, then try to copy-and-delete, but this will fail for directories
    const bool success = file.rename( targetPath );
    if ( ! success )
	THROW( FileException( path, "Could not move " + path + " to " + targetPath ) );
}
