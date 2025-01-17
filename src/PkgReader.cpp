/*
 *   File name: PkgReader.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <sys/stat.h> // struct stat

#include <QThread>

#include "PkgReader.h"
#include "DirTree.h"
#include "Exception.h"
#include "FileInfoIterator.h"
#include "PkgFileListCache.h"
#include "PkgFilter.h"
#include "PkgManager.h"
#include "PkgQuery.h"
#include "ProcessStarter.h"
#include "Settings.h"
#include "SysUtil.h"


using namespace QDirStat;


namespace
{
    /**
     * Read parameters from the settings file.
     **/
    void readSettings( int & maxParallelProcesses, int & minCachePkgListSize )
    {
	Settings settings;

	settings.beginGroup( "Pkg" );

	const int defaultParallelProcesses = QThread::idealThreadCount();
	const int defaultCachePkgListSize  = defaultParallelProcesses * 40;
	maxParallelProcesses   = settings.value( "MaxParallelProcesses",   defaultParallelProcesses ).toInt();
	minCachePkgListSize    = settings.value( "MinCachePkgListSize",    defaultCachePkgListSize  ).toInt();

	settings.setDefaultValue( "MaxParallelProcesses",   maxParallelProcesses   );
	settings.setDefaultValue( "MinCachePkgListSize",    minCachePkgListSize    );

	settings.endGroup();
    }


    /**
     * Create a suitable display name for a package: packages that are
     * only installed in one version or for one architecture will simply
     * keep their base name; others will have the version and/or the
     * architecture appended so the user can tell them apart.
     *
     * Note that there is no specific logic for disambiguating packages
     * with the same name in different pakage managers, but these will
     * hopefully have either a different architecture or different version.
     **/
    void createDisplayName( const PkgInfoList & pkgList )
    {
	if ( pkgList.size() < 2 )
	    return;

	const QString & arch = pkgList.first()->arch();
	for ( auto it = pkgList.cbegin() + 1; it != pkgList.cend(); ++it )
	{
	    if ( (*it)->arch() != arch )
	    {
		const QString & pkgName = (*it)->name();
		//logDebug() << "Found multi-arch package " << pkgName << Qt::endl;

		for ( PkgInfo * pkg : pkgList )
		    pkg->setName( pkgName % ':' % pkg->arch() );

		break;
	    }
	}

	const QString & version = pkgList.first()->version();
	for ( auto it = pkgList.cbegin() + 1; it != pkgList.cend(); ++it )
	{
	    if ( (*it)->version() != version )
	    {
		const QString & pkgName = (*it)->name();
		//logDebug() << "Found multi-version package " << pkgName << Qt::endl;

		for ( PkgInfo * pkg : pkgList )
		    pkg->setName( pkgName % '=' % pkg->version() );

		break;
	    }
	}
    }


    /**
     * Handle packages that are installed in multiple versions or for
     * multiple architectures: assign a different display name to each of
     * them.
     **/
    void handleMultiPkg( const PkgInfoList & pkgList )
    {
	QMultiHash<QString, PkgInfo *> multiPkg;

	for ( PkgInfo * pkg : pkgList )
	    multiPkg.insert( pkg->baseName(), pkg );

	const auto uniqueKeys = multiPkg.uniqueKeys();
	for ( const QString & pkgName : uniqueKeys )
	    createDisplayName( multiPkg.values( pkgName ) );
    }


    /**
     * Return a list of packages filtered using the given filter pattern.
     **/
    PkgInfoList filteredPkgList( const PkgFilter & filter )
    {
	const PkgInfoList pkgList = PkgQuery::installedPkg();

	if ( filter.filterMode() == SearchFilter::SelectAll )
	    return pkgList;

	PkgInfoList filteredList;

	for ( PkgInfo * pkg : pkgList )
	{
	    if ( filter.matches( pkg->baseName() ) )
		filteredList << pkg;
	}

	return filteredList;
    }


    /**
     * Add all the packages to the DirTree, as children of a top-level package
     * summary.
     **/
    void addToTree( DirTree * tree, const PkgInfoList & pkgList )
    {
	PkgInfo * top = new PkgInfo{ tree, tree->root() };
	tree->root()->insertChild( top );

	for ( PkgInfo * pkg : pkgList )
	{
	    pkg->setTree( tree );
	    top->insertChild( pkg );
	}

	top->setReadState( DirFinished );
	top->finalizeLocal();
    }


    /**
     * Create a process for reading the file list for 'pkg' with the
     * appropriate external command. The processes will be started by the
     * ProcessStarter.
     **/
    QProcess * createReadFileListProcess( PkgInfo * pkg )
    {
	const PkgCommand pkgCommand = pkg->pkgManager()->fileListCommand( pkg );
	if ( pkgCommand.isEmpty() )
	{
	    logError() << "Empty file list command for " << pkg << Qt::endl;
	    return nullptr;
	}

	// Intentionally NOT starting the process yet
	QProcess * process = SysUtil::commandProcess( pkgCommand.program, pkgCommand.args );

	return process;
    }


    /**
     * Create a read job for each package to read its file list from a file
     * list cache, and add it to the read job queue.  This requires a primary
     * package manager, one that should own the majority of packages and
     * support generating a file list cache.  Packages added as a
     * CachePkgReadJob will be removed from 'pkgList' so that the remainder
     * can be processed using external process read jobs.
     **/
    void createCachePkgReadJobs( DirTree * tree, PkgInfoList & pkgList )
    {
	const PkgManager * primaryPkgManager = PkgQuery::primaryPkgManager();
	if ( !primaryPkgManager )
	    return;

	// The shared pointer will delete the cache when the last job that uses it is destroyed
	PkgFileListCachePtr fileListCache{ primaryPkgManager->createFileListCache() };
	if ( !fileListCache )
	{
	    logWarning() << "Creating file list cache failed - fall back to AsyncPkgReadJob" << Qt::endl;
	    return;
	}

	// Keep a list of packages not in the file list cache
	PkgInfoList nonCachePkgList;

	for ( PkgInfo * pkg : asConst( pkgList ) )
	{
	    if ( pkg->pkgManager() == primaryPkgManager )
		tree->addJob( new CachePkgReadJob{ tree, pkg, fileListCache } );
	    else
		nonCachePkgList << pkg;
	}

	logInfo() << "File list cache created with "
	          << pkgList.size() - nonCachePkgList.size() << " packages and "
	          << fileListCache->size() << " pathnames" << Qt::endl;

	// Return any remaining packages to be processed asynchronously
	pkgList.swap( nonCachePkgList );
    }


    /**
     * Create a read job for each package with a background process to read
     * its file list and add it as a blocked job to the read job queue.
     **/
    void createAsyncPkgReadJobs( DirTree           * tree,
                                 const PkgInfoList & pkgList,
                                 int                 maxParallelProcesses )
    {
	ProcessStarter * processStarter = new ProcessStarter{ maxParallelProcesses };

	for ( PkgInfo * pkg : pkgList )
	{
	    QProcess * process = createReadFileListProcess( pkg );
	    if ( process )
	    {
		tree->addBlockedJob( new AsyncPkgReadJob{ tree, pkg, process } );
		processStarter->add( process );
	    }
	}

	// Tell the ProcessStarter it is allowed to die now
	processStarter->noMoreProcesses();
    }

} // namespace


void PkgReader::read( DirTree * tree, const PkgFilter & filter )
{
    //logInfo() << "Reading " << filter << Qt::endl;

    PkgInfoList pkgList = filteredPkgList( filter );
    if ( pkgList.isEmpty() )
    {
	tree->sendFinished();
	return;
    }

    handleMultiPkg( pkgList );
    addToTree( tree, pkgList );

    int maxParallelProcesses;
    int minCachePkgListSize;
    readSettings( maxParallelProcesses, minCachePkgListSize );

    // Use a cache for the primary package manager if there are enough packages to make it worthwhile
    if ( pkgList.size() >= minCachePkgListSize )
	createCachePkgReadJobs( tree, pkgList );

    // Otherwise, or for non-primary packages, use external process reads
    if ( !pkgList.isEmpty() )
	createAsyncPkgReadJobs( tree, pkgList, maxParallelProcesses );
}




namespace
{
    /**
     * Recursively finalize all directories in a subtree.
     **/
    void finalizeAll( DirInfo * subtree )
    {
	for ( DirInfoIterator it{ subtree }; *it; ++it )
	    finalizeAll( *it );

	if ( !subtree->subtreeReadError() )
	    subtree->setReadState( DirFinished );

	subtree->finalizeLocal();
    }


    /**
     * Do an fstatat() syscall for 'path' or fetch the result from a cache.
     * Return false if fstatat() fails.
     **/
    bool stat( struct stat & statInfo, const QString & path )
    {
	const int result = SysUtil::stat( path, statInfo );

	if ( result != 0 )
	    return false; // fstatat() failed

	if ( S_ISDIR( statInfo.st_mode ) ) // directory
	{
	    // Zero the directory's own size fields.  These may represent, in
	    // part, files from other packages and, when they occur in multiple
	    // packages they will be counted multiple times in the total.
	    statInfo.st_size   = 0;
	    statInfo.st_blocks = 0;

	    // mtime is still valid although it may reflect the addition
	    // or deletion of files that are not in this package
//	    statInfo.st_mtime  = 0;
	}

	return true;
    }


    /**
     * Locate a direct child of a DirInfo by name.  Ignore dot-entries
     * since they will never match a real pathname.
     **/
    FileInfo * locateChild( DirInfo * parent, const QString & pathComponent )
    {
	if ( pathComponent.isEmpty() )
	    return nullptr;

	const auto compare = [ & pathComponent ]( FileInfo * item ) { return item->name() == pathComponent; };
	auto it = std::find_if( begin( parent ), end( parent ), compare );
	return *it;
    }

} // namespace


void PkgReadJob::startReading()
{
    //logInfo() << "Reading " << _pkg << Qt::endl;

    CHECK_PTR( _pkg );

    _pkg->setReadState( DirReading );
    addFiles( fileList() );
    finalizeAll( _pkg );
    tree()->sendReadJobFinished( _pkg );

    finished();
    // Don't add anything after finished() since this deletes this job!
}


FileInfo * PkgReadJob::addItem( const QString & path, const QString & name, DirInfo * parent )
{
    // logDebug() << "path: \"" << path << '"' << Qt::endl;

    FileInfo * newItem = [ this, &path, &name, parent ]() -> FileInfo *
    {
	struct stat statInfo;
	if ( stat( statInfo, path ) )
	{
	    if ( S_ISDIR( statInfo.st_mode ) )		// directory
		return new DirInfo{ parent, tree(), name, statInfo };
	    else					// not directory
		return new FileInfo{ parent, tree(), name, statInfo };
	}

	// Create something, anything, if fstatat() failed
	DirInfo * dir = new DirInfo{ parent, tree(), name };
	switch (errno )
	{
	    case EACCES:
		// No permissions at all, can't even be sure the file is there, uncommon but non-fatal
		dir->setReadError( DirNoAccess );
		// logDebug() << _pkg << ": permission denied for " << path << Qt::endl;
		break;

	    case ENOENT:
		// File definitely not on disk, should be very rare
		dir->setReadError( DirMissing );
		// logDebug() << _pkg << ": missing " << path << Qt::endl;
		break;

	    default:
		// Unexpected error, probably serious
		dir->setReadError( DirError );
		logWarning() << _pkg << ": couldn't stat " << path << Qt::endl;
		break;
	}
	return dir;
    }();

    if ( newItem )
    {
	parent->insertChild( newItem );
	tree()->childAddedNotify( newItem );
    }

    return newItem;
}


void PkgReadJob::addFiles( QStringList fileList )
{
    // The parent-finding and duplicate-checking works best with a sorted list
    fileList.sort();

    DirInfo * lastDir = _pkg;
    QString lastDirPath{ u'/' };
    QString prevFileListPath;

    for ( const QString & fileListPath : asConst( fileList ) )
    {
	if ( fileListPath.isEmpty() || fileListPath == prevFileListPath )
	    continue;

	prevFileListPath = fileListPath;

	// logDebug() << "Adding " << fileListPath << " to " << _pkg << Qt::endl;

	// Usually the DirInfo parent will already have been created, and usually just previous
	if ( fileListPath.startsWith( lastDirPath ) )
	{
	    const QString fileName = SysUtil::baseName( fileListPath );

	    // Make sure the last directory is the direct parent of this item
	    if ( fileListPath.size() == lastDirPath.size() + fileName.size() + 1 )
	    {
		// Can just create this item in the existing parent
		addItem( fileListPath, fileName, lastDir );
		continue;
	    }
	}

	// Now we have to start from the top level and ensure every directory exists
	QString currentPath;
	DirInfo * parent = _pkg;

	const QStringList components = fileListPath.split( u'/', Qt::SkipEmptyParts );
	for ( const QString & currentComponent : components )
	{
	    currentPath += '/' % currentComponent;

	    FileInfo * newParent = locateChild( parent, currentComponent );
	    if ( !newParent )
		newParent = addItem( currentPath, currentComponent, parent );

	    if ( newParent && newParent->isDirInfo() )
	    {
		parent = newParent->toDirInfo();
		lastDir = parent;
		lastDirPath = currentPath;
	    }
	}
    }
}




AsyncPkgReadJob::AsyncPkgReadJob( DirTree   * tree,
                                  PkgInfo   * pkg,
                                  QProcess  * readFileListProcess ):
    PkgReadJob{ tree, pkg }
{
    CHECK_PTR( readFileListProcess );

    connect( readFileListProcess, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ),
             this,                &AsyncPkgReadJob::readFileListFinished );
}


void AsyncPkgReadJob::readFileListFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
    // Always get this job out of the blocked queue and clean up the file list process
    tree()->unblock( this );
    QProcess * senderProcess = qobject_cast<QProcess *>( sender() );
    if ( !senderProcess)
	return;

    senderProcess->deleteLater();

    if ( exitStatus == QProcess::CrashExit )
    {
	logError() << "Get file list command crashed for " << pkg() << Qt::endl;
    }
    else if ( exitCode != 0 )
    {
	logError() << "Get file list command exit code " << exitCode << " for " << pkg() << Qt::endl;
    }
    else // ok
    {
	const QString output = QString::fromUtf8( senderProcess->readAll() );
	_fileList = pkg()->pkgManager()->parseFileList( output );

	return;
    }

    // There was an error of some sort, logged above
    pkg()->setReadState( DirError );
    tree()->sendReadJobFinished( pkg() );

    // Dequeue and delete this job
    finished();
    // Don't add anything after finished()!
}




QStringList CachePkgReadJob::fileList() const
{
    if ( _fileListCache )
    {
	const QString pkgName = pkg()->pkgManager()->queryName( pkg() );
	if ( _fileListCache->containsPkg( pkgName ) )
	    return _fileListCache->fileList( pkgName );

	if ( _fileListCache->containsPkg( pkg()->name() ) )
	    return _fileListCache->fileList( pkg()->name() );
    }

    return QStringList{};
}
