/*
 *   File name: PkgReader.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <sys/stat.h> // fstatat(), struct stat

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


#define DEFAULT_PARALLEL_PROCESSES   10
#define DEFAULT_CACHE_PKG_LIST_SIZE 300


using namespace QDirStat;


namespace
{
    /**
     * Create a suitable display name for a package: packages that are
     * only installed in one version or for one architecture will simply
     * keep their base name; others will have the version and/or the
     * architecture appended so the user can tell them apart.
     **/
    void createDisplayName( const QString & pkgName, const PkgInfoList & pkgList )
    {
	if ( pkgList.size() < 2 )
	    return;

	const QString & version = pkgList.first()->version();
	const QString & arch    = pkgList.first()->arch();

	bool sameVersion = true;
	bool sameArch    = true;

	for ( const PkgInfo * pkg : pkgList )
	{
	    if ( pkg->version() != version )
		sameVersion = false;

	    if ( pkg->arch() != arch )
		sameArch = false;
	}

	//logDebug() << "Found multi version pkg " << pkgName << " same arch: " << sameArch << Qt::endl;

	for ( PkgInfo * pkg : pkgList )
	{
	    QString name = pkgName;

	    if ( !sameVersion )
	    {
		name += '-' % pkg->version();
		pkg->setMultiVersion();
	    }

	    if ( !sameArch )
	    {
		name += ':' % pkg->arch();
		pkg->setMultiArch();
	    }

	    pkg->setName( name );
	}
    }


    /**
     * Handle packages that are installed in multiple versions or for
     * multiple architectures: assign a different display name to each of
     * them.
     **/
    void handleMultiPkg( const PkgInfoList & pkgList )
    {
	// QMultiHash is slightly faster, but uniqueKeys() isn't available before 5.13
	QMultiMap<QString, PkgInfo *> multiPkg;

	for ( PkgInfo * pkg : pkgList )
	    multiPkg.insert( pkg->baseName(), pkg );

	const auto uniqueKeys = multiPkg.uniqueKeys();
	for ( const QString & pkgName : uniqueKeys )
	    createDisplayName( pkgName, multiPkg.values( pkgName ) );
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
     * Add all the packages to the DirTree, including the 'package
     * summary' root.
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
     * appropriate external command. The process is not started yet.
     **/
    QProcess * createReadFileListProcess( PkgInfo * pkg )
    {
	const PkgCommand pkgCommand = pkg->pkgManager()->fileListCommand( pkg );
	if ( pkgCommand.isEmpty() )
	{
	    logError() << "Empty file list command for " << pkg << Qt::endl;
	    return nullptr;
	}

	QProcess * process = SysUtil::commandProcess( pkgCommand.program, pkgCommand.args );
	// Intentionally NOT starting the process yet

	return process;
    }


    /**
     * Create a read job for each package to read its file list from a file
     * list cache and add it to the read job queue.
     **/
    void createCachePkgReadJobs( DirTree           * tree,
                                 const PkgInfoList & pkgList,
                                 const PkgManager  * pkgManager,
                                 bool                verboseMissingPkgFiles )
    {
	// The shared pointer will delete the cache when the last job that uses it is destroyed
	PkgFileListCachePtr fileListCache{ pkgManager->createFileListCache() };
	if ( !fileListCache )
	{
	    logError() << "Creating the file list cache failed" << Qt::endl;
	    tree->sendFinished();
	    return;
	}

	for ( PkgInfo * pkg : pkgList )
	    tree->addJob( new CachePkgReadJob{ tree, pkg, verboseMissingPkgFiles, fileListCache } );
    }


    /**
     * Create a read job for each package with a background process to read
     * its file list and add it as a blocked job to the read job queue.
     **/
    void createAsyncPkgReadJobs( DirTree           * tree,
                                 const PkgInfoList & pkgList,
                                 int                 maxParallelProcesses,
                                 bool                verboseMissingPkgFiles )
    {
	ProcessStarter * processStarter = new ProcessStarter{ maxParallelProcesses, true };

	for ( PkgInfo * pkg : pkgList )
	{
	    QProcess * process = createReadFileListProcess( pkg );
	    if ( process )
	    {
		tree->addBlockedJob( new AsyncPkgReadJob{ tree, pkg, verboseMissingPkgFiles, process } );
		processStarter->add( process );
	    }
	}

	processStarter->start();
    }

} // namespace


void PkgReader::read( DirTree * tree, const PkgFilter & filter )
{
    //logInfo() << "Reading " << filter << Qt::endl;

    readSettings();

    PkgInfoList pkgList = filteredPkgList( filter );
    if ( pkgList.isEmpty() )
    {
	tree->sendFinished();
	return;
    }

    handleMultiPkg( pkgList );
    addToTree( tree, pkgList );

    const PkgManager * primaryPkgManager = PkgQuery::primaryPkgManager();
    if ( primaryPkgManager && pkgList.size() >= _minCachePkgListSize )
	createCachePkgReadJobs( tree, pkgList, primaryPkgManager, _verboseMissingPkgFiles );
    else
	createAsyncPkgReadJobs( tree, pkgList, _maxParallelProcesses, _verboseMissingPkgFiles );
}


void PkgReader::readSettings()
{
    Settings settings;

    settings.beginGroup( "Pkg" );

    _maxParallelProcesses   = settings.value( "MaxParallelProcesses",   DEFAULT_PARALLEL_PROCESSES  ).toInt();
    _minCachePkgListSize    = settings.value( "MinCachePkgListSize",    DEFAULT_CACHE_PKG_LIST_SIZE ).toInt();
    _verboseMissingPkgFiles = settings.value( "VerboseMissingPkgFiles", false ).toBool();

    settings.setDefaultValue( "MaxParallelProcesses",   _maxParallelProcesses   );
    settings.setDefaultValue( "MinCachePkgListSize",    _minCachePkgListSize    );
    settings.setDefaultValue( "VerboseMissingPkgFiles", _verboseMissingPkgFiles );

    settings.endGroup();
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

	if ( !subtree->readError() )
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
	    // Zero the directory's own size fields to prevent them from
	    // distorting the total sums.  Otherwise the directory would be
	    // counted in each package that uses the directory.
	    statInfo.st_size   = 0;
	    statInfo.st_blocks = 0;

	    // mtime is still valid although it may reflect the addition
	    // or deletion of files that are not in this package
//	    statInfo.st_mtime  = 0;
	}

	return true;
    }


    /**
     * Create a DirInfo or FileInfo node from a path and stat call.
     **/
    FileInfo * createItem( const QString & path,
                           const QString & name,
                           DirTree       * tree,
                           DirInfo       * parent )
    {
	// logDebug() << "path: \"" << path << '"' << Qt::endl;

	struct stat statInfo;
	if ( !stat( statInfo, path ) ) // fstatat() failed
	    return nullptr;

	if ( S_ISDIR( statInfo.st_mode ) )	// directory
	    return new DirInfo{ parent, tree, name, statInfo };
	else					// not directory
	    return new FileInfo{ parent, tree, name, statInfo };
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


QStringList PkgReadJob::fileList()
{
    logDebug() << "Using default PkgQuery::fileList() for " << _pkg << Qt::endl;

    return PkgQuery::fileList( _pkg );
}


FileInfo * PkgReadJob::createItem( const QString & path,
                                   const QString & name,
                                   DirInfo       * parent )
{
    FileInfo * newItem = ::createItem( path, name, tree(), parent );
    if ( newItem )
    {
	parent->insertChild( newItem );
	tree()->childAddedNotify( newItem );
	return newItem;
    }

    if ( errno == EACCES )
    {
	// No permissions, unexpected (for a package!) but non-fatal error
	parent->markAsDirty();
	parent->setReadState( DirPermissionDenied );
    }
    else if ( errno != ENOENT )
    {
	// Unexpected error, probably serious
	logError() << _pkg << ": can't stat " << path << Qt::endl;
	parent->markAsDirty();
	parent->setReadState( DirError );
    }
    else if ( _verboseMissingPkgFiles )
    {
	// Packaged file not present, log it
	logWarning() << _pkg << " missing " << path << Qt::endl;
    }

    return nullptr;
}


void PkgReadJob::addFiles( QStringList fileList )
{
    // The tree creation works best with a sorted list
    fileList.sort();

    DirInfo * lastDir = _pkg;
    QString lastDirPath{ u'/' };

    for ( const QString & fileListPath : asConst( fileList ) )
    {
	if ( fileListPath.isEmpty() )
	    continue;

	// logDebug() << "Adding " << fileListPath << " to " << _pkg << Qt::endl;

	// Usually the DirInfo parent will already have been created, and usually just previous
	if ( fileListPath.startsWith( lastDirPath ) )
	{
	    const QString fileName = SysUtil::baseName( fileListPath );

	    // Make sure the last directory is the direct parent of this item
	    if ( fileListPath.size() == lastDirPath.size() + fileName.size() + 1 )
	    {
		// Can just create this item in the existing parent
		createItem( fileListPath, fileName, lastDir );
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
		newParent = createItem( currentPath, currentComponent, parent );

	    if ( newParent && newParent->isDirInfo() )
	    {
		parent = newParent->toDirInfo();
		lastDir = parent;
		lastDirPath = currentPath;
	    }
	    else if ( currentComponent != components.last() )
	    {
		// Failure that used to occur a lot (for dpkg) when symlinks weren't resolved
		logWarning() << newParent << " should be a directory, but is not" << Qt::endl;
	    }
	}
    }
}




AsyncPkgReadJob::AsyncPkgReadJob( DirTree   * tree,
                                  PkgInfo   * pkg,
                                  bool        verboseMissingPkgFiles,
                                  QProcess  * readFileListProcess ):
    PkgReadJob{ tree, pkg, verboseMissingPkgFiles }
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




QStringList CachePkgReadJob::fileList()
{
    if ( !_fileListCache || _fileListCache->pkgManager() != pkg()->pkgManager() )
	return PkgReadJob::fileList();

    const QString pkgName = pkg()->pkgManager()->queryName( pkg() );

    if ( _fileListCache->containsPkg( pkgName ) )
	return _fileListCache->fileList( pkgName );

    if ( _fileListCache->containsPkg( pkg()->name() ) )
	return _fileListCache->fileList( pkg()->name() );

    return QStringList{};
}
