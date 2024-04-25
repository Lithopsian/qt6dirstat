/*
 *   File name: Trash.h
 *   Summary:   Implementation of the XDG Trash spec for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
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
    QString toplevel( const QString & rawPath )
    {
	const dev_t dev = device( rawPath );
	const QFileInfo fileInfo( rawPath );
	QString path = fileInfo.canonicalPath();
	QStringList components = path.split( "/", Qt::SkipEmptyParts );

	// Work up the directory tree
	while ( !components.isEmpty() )
	{
	    // See if we are the top level on this device
	    components.removeLast();
	    const QString nextPath = "/" + components.join( "/" );
	    if ( device( nextPath ) != dev )
		return path;

	    path = nextPath;
	}

	return "/";
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
	    THROW( FileException( trashPath, "stat failed for " + trashPath + ": " + formatErrno() ) );
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
		if ( !S_ISDIR( mode ) )
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
	if ( result >= 0 )
	    return true;

	if ( doThrow )
	{
	    THROW( FileException( path,
				  QString( "Could not create directory %1: %2" )
				  .arg( path ).arg( formatErrno() ) ) );
	}

	return false;
    }

} // namespace


TrashDir::TrashDir( const QString & path, dev_t device ):
    _path { path },
    _device { device }
{
    // logDebug() << "Created TrashDir " << path << Qt::endl;

    ensureDirExists( path,        0700, true );
    ensureDirExists( filesPath(), 0700, true );
    ensureDirExists( infoPath(),  0700, true );
}


QString TrashDir::uniqueName( const QString & path )
{
    const QDir filesDir( filesPath() );

    const QFileInfo file( path );
    QString name = file.fileName();

    for ( int i = 1; filesDir.exists( name ); ++i )
    {
	const QString baseName = file.baseName();
	const QString suffix   = file.completeSuffix();
	name = QString( "%1_%2" ).arg( baseName ).arg( i );
	if ( !suffix.isEmpty() )
	    name += "." + suffix;
    }

    return name;
}


void TrashDir::createTrashInfo( const QString & path,
				const QString & targetName )
{
    QFile trashInfo( infoPath() + "/" + targetName + ".trashinfo" );

    if ( !trashInfo.open( QIODevice::WriteOnly | QIODevice::Text ) )
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
    if ( !success )
	THROW( FileException( path, "Could not move " + path + " to " + targetPath ) );
}
