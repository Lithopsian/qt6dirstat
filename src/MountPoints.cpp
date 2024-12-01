/*
 *   File name: MountPoints.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QFile>
#include <QRegularExpression>
#include <QFileInfo>

#include "MountPoints.h"
#include "Logger.h"
#include "SysUtil.h"


#define LSBLK_TIMEOUT_SEC 10
#define USE_PROC_MOUNTS   1


using namespace QDirStat;


namespace
{
    void dumpMountPoints( bool showAll )
    {
        for ( MountPointIterator it{ showAll }; *it; ++it )
            logDebug() << *it << Qt::endl;
    }

    // Verbose logging, all callers might be commented out
    [[gnu::unused]] void dumpAllMountPoints()
    {
        dumpMountPoints( true );
    }


    [[gnu::unused]] void dumpNormalMountPoints()
    {
        dumpMountPoints( false );
    }


    /**
     * Change filesystem type "fuseblk" to "ntfs" for NTFS devices.
     **/
    void handleFuseblk( QString & fsType, const QStringList & ntfsDevices, const QString & device )
    {
        if ( fsType == "fuseblk"_L1 && ntfsDevices.contains( device ) )
            fsType = "ntfs";
    }


    /**
     * Try to check with the external "lsblk" command (if available) what
     * block devices use NTFS and return a list of them.
     **/
    QStringList findNtfsDevices()
    {
        const QLatin1String lsblkCommand = []()
            {
                 if ( SysUtil::haveCommand( "/bin/lsblk" ) )
                    return "/bin/lsblk"_L1;

                if ( SysUtil::haveCommand( "/usr/bin/lsblk" ) )
                    return "/usr/bin/lsblk"_L1;

                return QLatin1String{};
            }();

        if ( lsblkCommand.isEmpty() )
        {
            logInfo() << "No lsblk command available" << Qt::endl;
            return QStringList{};
        }

        int exitCode;
        const QString output = SysUtil::runCommand( lsblkCommand,
                                                    { "-n", "-l", "-o", "NAME,FSTYPE" },
                                                    &exitCode,
                                                    LSBLK_TIMEOUT_SEC,
                                                    false,        // logCommand
                                                    false,        // logOutput
                                                    true );       // logError
        if ( exitCode != 0 )
            return QStringList{};

        QStringList ntfsDevices;

        const QRegularExpression whitespace{ "\\s+" };
        const QStringList lines = output.split( u'\n' );
        for ( const QString & line : lines )
        {
            const QStringList fields = line.split( whitespace, Qt::SkipEmptyParts );
            if ( fields.size() == 2 )
            {
                const QString & device = fields[ 0 ];
                const QString & fsType = fields[ 1 ];
                if ( fsType.startsWith( "ntfs"_L1, Qt::CaseInsensitive ) )
                {
                    logDebug() << fsType << " on " << device << Qt::endl;
                    ntfsDevices << device;
                }
            }
        }

        return ntfsDevices;
    }


    /**
     * Return 'true' if 'device' is mounted.
     **/
    bool isDeviceMounted( const QString & device, MountPointMap & mountPointMap )
    {
        for ( const MountPoint * mountPoint : mountPointMap )
        {
            if ( mountPoint->device() == device )
            return true;
        }

        return false;
    }


    /**
     * Post-process a mount point: check for duplicate mounts and Snap
     * packages.
     **/
    void postProcess( MountPoint * mountPoint, MountPoints * mountPoints )
    {
        //logDebug() << mountPoint << Qt::endl;

        if ( !mountPoint->isSystemMount() && isDeviceMounted( mountPoint->device(), *mountPoints ) )
        {
            mountPoint->setDuplicate();

            //logInfo() << "Found duplicate mount of " << mountPoint->device()
            //          << " at " << mountPoint->path()
            //          << Qt::endl;
        }

        if ( mountPoint->isSnapPackage() )
        {
            const QString pkgName = mountPoint->path().section( u'/', 1, 1, QString::SectionSkipEmpty );
            logInfo() << "Found snap package \"" << pkgName << "\" at " << mountPoint->path() << Qt::endl;
        }
    }

} // namespace


bool MountPoint::isNetworkMount() const
{
    const QString fsType{ _filesystemType.toLower() };

    if ( fsType.startsWith( "nfs"_L1  ) ) return true;
    if ( fsType.startsWith( "cifs"_L1 ) ) return true;

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

    if ( !_device.contains( u'/' ) ) return true;

    if ( _path.startsWith( "/dev"_L1  ) ) return true;
    if ( _path.startsWith( "/proc"_L1 ) ) return true;
    if ( _path.startsWith( "/sys"_L1  ) ) return true;

    return false;
}


#if HAVE_Q_STORAGE_INFO
const QStorageInfo & MountPoint::storageInfo()
{
    if ( !_storageInfo )
    {
        if ( isNetworkMount() )
            logDebug() << "Creating QStorageInfo for " << _path << Qt::endl;

        _storageInfo.reset( new QStorageInfo{ _path } );
    }

    return *_storageInfo;
}
#endif // !HAVE_Q_STORAGE_INFO




MountPoints * MountPoints::instance()
{
    static MountPoints _instance;

    return &_instance;
}


void MountPoints::init()
{
    clear();

    _hasNtfs = false;

    populate();
}


const MountPoint * MountPoints::findNearestMountPoint( const QString & startPath )
{
    const QFileInfo fileInfo{ startPath };
    QString path = fileInfo.canonicalFilePath(); // absolute path without symlinks or ..

//    if ( path != startPath )
//        logDebug() << startPath << " canonicalized is " << path << Qt::endl;

    const MountPoint * mountPoint = findByPath( path );

    if ( !mountPoint )
    {
        QStringList pathComponents = startPath.split( u'/', Qt::SkipEmptyParts );

        while ( !mountPoint && !pathComponents.isEmpty() )
        {
            // Try one level upwards
            pathComponents.removeLast();
            path = '/' % pathComponents.join( u'/' );

            mountPoint = findByPath( path );
        }
    }

    //logDebug() << "Nearest mount point for " << startPath << " is " << mountPoint << Qt::endl;

    return mountPoint;
}


void MountPoints::populate()
{
    QStringList ntfsDevices = findNtfsDevices();
    _hasNtfs = !ntfsDevices.isEmpty();

#if USE_PROC_MOUNTS
    read( "/proc/mounts", ntfsDevices ) || read( "/etc/mtab", ntfsDevices );
    if ( isEmpty() )
        logError() << "Could not read either /proc/mounts or /etc/mtab" << Qt::endl;
#endif

#if HAVE_Q_STORAGE_INFO
    if ( isEmpty() )
        readStorageInfo( ntfsDevices );
#endif
}


bool MountPoints::read( const QString & filename, const QStringList & ntfsDevices )
{
    QFile file{ filename };

    if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        logWarning() << "Can't open " << filename << Qt::endl;
        return false;
    }

    const QRegularExpression whitespace{ "\\s+" };

    QTextStream in{ &file };
    int lineNo = 0;
    QString line = in.readLine();

    while ( !line.isNull() ) // in.atEnd() always returns true for /proc/*
    {
        ++lineNo;
        QStringList fields = line.split( whitespace, Qt::SkipEmptyParts );

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

        const QString & device = fields[0];

        QString & path = fields[1];
        path.replace( "\\040"_L1, " "_L1 ); // escaped spaces

        QString & fsType = fields[2];
        handleFuseblk( fsType, ntfsDevices, device );

        const QString & mountOpts = fields[3];
        // ignoring fsck and dump order (0 0)

        MountPoint * mountPoint = new MountPoint{ device, path, fsType, mountOpts };
        postProcess( mountPoint, this );
        add( mountPoint );

        line = in.readLine();
    }

    if ( isEmpty() )
    {
        logWarning() << "No mountpoints in " << filename << Qt::endl;
        return false;
    }

    // logDebug() << "Read " << size() << " mount points from " << filename << Qt::endl;
    return true;
}

#if HAVE_Q_STORAGE_INFO
void MountPoints::readStorageInfo( const QStringList & ntfsDevices )
{
    const auto mountedVolumes = QStorageInfo::mountedVolumes();
    for ( const QStorageInfo & mount : mountedVolumes )
    {
        const QString device = QString::fromUtf8( mount.device() );
        const QLatin1String mountOptions = mount.isReadOnly() ? "ro"_L1 : QLatin1String{};

        QString fsType = QString::fromUtf8( mount.fileSystemType() );
        handleFuseblk( fsType, ntfsDevices, device );

        MountPoint * mountPoint = new MountPoint{ device, mount.rootPath(), fsType, mountOptions };
        postProcess( mountPoint, this );
        add( mountPoint );
    }

    if ( isEmpty() )
    {
        logWarning() << "No mountpoints found with QStorageInfo" << Qt::endl;
        return;
    }

    // logDebug() << "Read " << size() << " mount points from QStorageInfo" << Qt::endl;
}
#endif // HAVE_Q_STORAGE_INFO

QString MountPoints::device( const QString & url )
{
    const MountPoint * mountPoint = MountPoints::findByPath( url );
    if ( mountPoint )
        return mountPoint->device();

    return QString{};
}


bool MountPoints::isDuplicate( const QString & url )
{
    const MountPoint * mountPoint = MountPoints::findByPath( url );
    if ( mountPoint )
        return mountPoint->isDuplicate();

    return false;
}
