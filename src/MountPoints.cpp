/*
 *   File name: MountPoints.cpp
 *   Summary:	Support classes for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <QFile>
#include <QRegularExpression>
#include <QFileInfo>

#include "MountPoints.h"
#include "SysUtil.h"
#include "Logger.h"
#include "Exception.h"

#define LSBLK_TIMEOUT_SEC       10
#define USE_PROC_MOUNTS         1

using namespace QDirStat;


[[gnu::unused]] static void dumpNormalMountPoints()
{
    for ( const MountPoint * mountPoint : MountPoints::normalMountPoints() )
	logDebug() << mountPoint->path() << Qt::endl;
}




MountPoint::~MountPoint()
{
    delete _storageInfo;
}


bool MountPoint::isNetworkMount() const
{
    const QString fsType = _filesystemType.toLower();

    if ( fsType.startsWith( "nfs"  ) ) return true;
    if ( fsType.startsWith( "cifs" ) ) return true;

    return false;
}


bool MountPoint::isSystemMount() const
{
    // All normal block have a path with a slash like "/dev/something" or on some
    // systems maybe "/devices/something". NFS mounts have "hostname:/some/path",
    // Samba mounts have "//hostname/some/path".
    //
    // This check filters out system devices like "cgroup", "tmpfs", "sysfs"
    // and all those other kernel-table devices.

    if ( !_device.contains( "/" ) )	return true;

    if ( _path.startsWith( "/dev"  ) )	return true;
    if ( _path.startsWith( "/proc" ) )	return true;
    if ( _path.startsWith( "/sys"  ) )	return true;

    return false;
}


bool MountPoint::isSnapPackage() const
{
    return _path.startsWith( "/snap" ) && _filesystemType.toLower() == "squashfs";
}


#if HAVE_Q_STORAGE_INFO

QStorageInfo * MountPoint::storageInfo()
{
    if ( !_storageInfo )
    {
        if ( isNetworkMount() )
            logDebug() << "Creating QStorageInfo for " << _path << Qt::endl;

        _storageInfo = new QStorageInfo( _path );
        CHECK_NEW( _storageInfo );
    }

    return _storageInfo;
}


FileSize MountPoint::totalSize()
{
    return storageInfo()->bytesTotal();
}


FileSize MountPoint::usedSize()
{
    return storageInfo()->bytesTotal() - storageInfo()->bytesFree();
}


FileSize MountPoint::reservedSize()
{
    return storageInfo()->bytesFree() - storageInfo()->bytesAvailable();
}


FileSize MountPoint::freeSizeForUser()
{
    return storageInfo()->bytesAvailable();
}


FileSize MountPoint::freeSizeForRoot()
{
    return storageInfo()->bytesFree();
}

#else  // !HAVE_Q_STORAGE_INFO

// Qt before 5.4 does not have QStorageInfo,
// and statfs() is Linux-specific (not POSIX).

FileSize MountPoint::totalSize()	 { return -1; }
FileSize MountPoint::usedSize()		 { return -1; }
FileSize MountPoint::reservedSize()	 { return -1; }
FileSize MountPoint::freeSizeForUser()	 { return -1; }
FileSize MountPoint::freeSizeForRoot()   { return -1; }

#endif // !HAVE_Q_STORAGE_INFO




MountPoints * MountPoints::instance()
{
    static MountPoints _instance;

    return &_instance;
}


MountPoints::~MountPoints()
{
    qDeleteAll( _mountPointList );
    _mountPointList.clear();
    _mountPointMap.clear();
}


MountPoint * MountPoints::findByPath( const QString & path )
{
    instance()->ensurePopulated();

    return instance()->_mountPointMap.value( path, 0 );
}


MountPoint * MountPoints::findNearestMountPoint( const QString & startPath )
{
    const QFileInfo fileInfo( startPath );
    QString path = fileInfo.canonicalFilePath(); // absolute path without symlinks or ..

//    if ( path != startPath )
//	logDebug() << startPath << " canonicalized is " << path << Qt::endl;

    MountPoint * mountPoint = findByPath( path );

    if ( !mountPoint )
    {
	QStringList pathComponents = startPath.split( "/", Qt::SkipEmptyParts );

	while ( !mountPoint && !pathComponents.isEmpty() )
	{
	    // Try one level upwards
	    pathComponents.removeLast();
	    path = QString( "/" ) + pathComponents.join( "/" );

	    mountPoint = instance()->_mountPointMap.value( path, 0 );
	}
    }

    //logDebug() << "Nearest mount point for " << startPath << " is " << mountPoint << Qt::endl;

    return mountPoint;
}


bool MountPoints::isDeviceMounted( const QString & device )
{
    // Do NOT call ensurePopulated() here: This would cause a recursion in the
    // populating process!

    for ( const MountPoint * mountPoint : instance()->_mountPointList )
    {
	if ( mountPoint->device() == device )
	    return true;
    }

    return false;
}


bool MountPoints::hasBtrfs()
{
    instance()->ensurePopulated();

    if ( !instance()->_checkedForBtrfs )
    {
	instance()->_hasBtrfs = instance()->checkForBtrfs();
	instance()->_checkedForBtrfs = true;
    }

    return instance()->_hasBtrfs;
}


void MountPoints::ensurePopulated()
{
    if ( _isPopulated )
	return;

#if USE_PROC_MOUNTS

    read( "/proc/mounts" ) || read( "/etc/mtab" );

    if ( !_isPopulated )
	logError() << "Could not read either /proc/mounts or /etc/mtab" << Qt::endl;

#endif

#if HAVE_Q_STORAGE_INFO

    if ( !_isPopulated )
        readStorageInfo();

#endif

    _isPopulated = true; // don't try more than once
//    dumpNormalMountPoints();
}


bool MountPoints::read( const QString & filename )
{
    QFile file( filename );

    if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
	logWarning() << "Can't open " << filename << Qt::endl;
	return false;
    }

    findNtfsDevices();
    QTextStream in( &file );
    int lineNo = 0;
    int count  = 0;
    QString line = in.readLine();

    while ( !line.isNull() ) // in.atEnd() always returns true for /proc/*
    {
	++lineNo;
	const QStringList fields = line.split( QRegularExpression( "\\s+" ), Qt::SkipEmptyParts );

	if ( fields.isEmpty() ) // allow empty lines
	    continue;

	if ( fields.size() < 4 )
	{
	    logError() << "Bad line " << filename << ":" << lineNo << ": " << line << Qt::endl;
	    continue;
	}

	// File format (/proc/mounts or /etc/mtab):
	//
	//   /dev/sda6 / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0
	//   /dev/sda7 /work ext4 rw,relatime,data=ordered 0 0
	//   nas:/share/work /nas/work nfs rw,local_lock=none 0 0

	const QString & device	 = fields[0];
	QString path		 = fields[1];
	QString fsType		 = fields[2];
	const QString &mountOpts = fields[3];
	// ignoring fsck and dump order (0 0)

        path.replace( "\\040", " " );

        if ( fsType == "fuseblk" && _ntfsDevices.contains( device ) )
            fsType = "ntfs";

	MountPoint * mountPoint = new MountPoint( device, path, fsType, mountOpts );
	CHECK_NEW( mountPoint );

        postProcess( mountPoint );
        add( mountPoint );

	if ( !mountPoint->isDuplicate() )
	    ++count;

	line = in.readLine();
    }

    if ( count < 1 )
    {
//	logWarning() << "Not a single mount point in " << filename << Qt::endl;
	return false;
    }
    else
    {
        // logDebug() << "Read " << _mountPointList.size() << " mount points from " << filename << Qt::endl;
	_isPopulated = true;
	return true;
    }
}


void MountPoints::postProcess( MountPoint * mountPoint )
{
    CHECK_PTR( mountPoint );

//    logDebug() << mountPoint << Qt::endl;

    if ( !mountPoint->isSystemMount() && isDeviceMounted( mountPoint->device() ) )
    {
        mountPoint->setDuplicate();

//        logInfo() << "Found duplicate mount of " << mountPoint->device()
//                  << " at " << mountPoint->path()
//                  << Qt::endl;
    }

    if ( mountPoint->isSnapPackage() )
    {
	const QString pkgName = mountPoint->path().section( "/", 1, 1, QString::SectionSkipEmpty );
        logInfo() << "Found snap package \"" << pkgName << "\" at " << mountPoint->path() << Qt::endl;
    }
}


void MountPoints::add( MountPoint * mountPoint )
{
    CHECK_PTR( mountPoint );

    _mountPointList << mountPoint;
    _mountPointMap[ mountPoint->path() ] = mountPoint;
}


#if HAVE_Q_STORAGE_INFO

bool MountPoints::readStorageInfo()
{
    findNtfsDevices();

    const auto mountedVolumes = QStorageInfo::mountedVolumes();
    for ( const QStorageInfo & mount : mountedVolumes )
    {
        const QString device( QString::fromUtf8( mount.device() ) );
        QString fsType( QString::fromUtf8( mount.fileSystemType() ) );
        QString mountOptions;

        if ( mount.isReadOnly() )
            mountOptions += "ro";

        if ( fsType == "fuseblk" && _ntfsDevices.contains( device ) )
            fsType = "ntfs";

        MountPoint * mountPoint = new MountPoint( device,
                                                  mount.rootPath(),
                                                  fsType,
                                                  mountOptions );
	CHECK_NEW( mountPoint );

        postProcess( mountPoint );
        add( mountPoint );
    }

    if ( _mountPointList.isEmpty() )
    {
	logWarning() << "Not a single mount point found with QStorageInfo" << Qt::endl;
	return false;
    }
    else
    {
        // logDebug() << "Read " << _mountPointList.size() << " mount points from QStorageInfo" << Qt::endl;
	_isPopulated = true;
	return true;
    }
}

#endif // HAVE_Q_STORAGE_INFO


bool MountPoints::checkForBtrfs()
{
    ensurePopulated();

    for ( const MountPoint * mountPoint : _mountPointMap )
    {
	if ( mountPoint && mountPoint->isBtrfs() )
	    return true;
    }

    return false;
}


void MountPoints::findNtfsDevices()
{
    _ntfsDevices.clear();
    QString lsblkCommand = "/bin/lsblk";

    if ( !SysUtil::haveCommand( lsblkCommand ) )
        lsblkCommand = "/usr/bin/lsblk";
    if ( !SysUtil::haveCommand( lsblkCommand ) )
    {
        logInfo() << "No lsblk command available" << Qt::endl;

        return;
    }

    int exitCode;
    const QString output = SysUtil::runCommand( lsblkCommand,
                                                QStringList() << "--noheading" << "--list" << "--output" << "name,fstype",
                                                &exitCode,
                                                LSBLK_TIMEOUT_SEC,
                                                false,        // logCommand
                                                false,        // logOutput
                                                false );      // ignoreErrCode
    if ( exitCode == 0 )
    {
        const QStringList lines = output.split( "\n" )
            .filter( QRegularExpression( "\\s+ntfs", QRegularExpression::CaseInsensitiveOption ) );

        for ( const QString & line : lines )
        {
            const QString device = "/dev/" + line.split( QRegularExpression( "\\s+" ) ).first();
            logDebug() << "NTFS on " << device << Qt::endl;
            _ntfsDevices << device;
        }
    }

//    if ( _ntfsDevices.isEmpty() )
//        logDebug() << "No NTFS devices found" << Qt::endl;
}


QList<MountPoint *> MountPoints::normalMountPoints()
{
    instance()->ensurePopulated();
    QList<MountPoint *> result;

    for ( MountPoint * mountPoint : instance()->_mountPointList )
    {
	if ( !mountPoint->isSystemMount()     &&
             !mountPoint->isDuplicate()       &&
             !mountPoint->isUnmountedAutofs() &&
             !mountPoint->isSnapPackage()        )
        {
	    result << mountPoint;
        }
    }

    return result;
}


void MountPoints::dump()
{
    for ( const MountPoint * mountPoint : instance()->_mountPointList )
	logDebug() << mountPoint->path() << Qt::endl;
}


void MountPoints::reload()
{
    instance()->ensurePopulated();
}


QString MountPoints::device( const QString & url )
{
    const MountPoint * mountPoint = MountPoints::findByPath( url );
    if ( mountPoint )
	return mountPoint->device();

    return QString();
}


bool MountPoints::isDuplicate( const QString & url )
{
    const MountPoint * mountPoint = MountPoints::findByPath( url );
    if ( mountPoint )
	return mountPoint->isDuplicate();

    return false;
}


#if HAVE_Q_STORAGE_INFO
  bool MountPoints::hasSizeInfo() { return true; }
#else
  bool MountPoints::hasSizeInfo() { return false; }
#endif
