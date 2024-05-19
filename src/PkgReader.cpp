/*
 *   File name: PkgReader.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "PkgReader.h"
#include "DirTree.h"
#include "FileInfoIterator.h"
#include "PkgFileListCache.h"
#include "PkgFilter.h"
#include "PkgManager.h"
#include "PkgQuery.h"
#include "ProcessStarter.h"
#include "Settings.h"
#include "Logger.h"
#include "Exception.h"


#define DEFAULT_PARALLEL_PROCESSES	10
#define DEFAULT_CACHE_PKG_LIST_SIZE	300


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

	const QString & version  = pkgList.first()->version();
	const QString & arch     = pkgList.first()->arch();

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
		name += '-' + pkg->version();
		pkg->setMultiVersion( true );
	    }

	    if ( !sameArch )
	    {
		name += ':' + pkg->arch();
		pkg->setMultiArch( true );
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

	for ( const QString & pkgName : multiPkg.uniqueKeys() )
	    createDisplayName( pkgName, multiPkg.values( pkgName ) );
    }


    /**
     * Return a list of packages filtered using the given filter pattern.
     **/
    PkgInfoList filteredPkgList( const PkgFilter & filter )
    {
	const PkgInfoList pkgList = PkgQuery::installedPkg();

	if ( filter.filterMode() == PkgFilter::SelectAll )
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
	CHECK_PTR( tree );
	CHECK_PTR( tree->root() );

	PkgInfo * top = new PkgInfo( tree, tree->root() );
	CHECK_NEW( top );

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
	CHECK_PTR( pkg );
	CHECK_PTR( pkg->pkgManager() );

	const QString command = pkg->pkgManager()->fileListCommand( pkg );

	if ( command.isEmpty() )
	{
	    logError() << "Empty file list command for " << pkg << Qt::endl;
	    return nullptr;
	}

	QStringList args       = command.split( QRegularExpression( "\\s+" ) );
	const QString program = args.takeFirst();

	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	env.insert( "LANG", "C" ); // Prevent output in translated languages

	QProcess * process = new QProcess();
	process->setProgram( program );
	process->setArguments( args );
	process->setProcessEnvironment( env );
	process->setProcessChannelMode( QProcess::MergedChannels ); // combine stdout and stderr

	// Intentionally NOT starting the process yet

	return process;
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

    const PkgManager * pkgManager = PkgQuery::primaryPkgManager();

    if ( pkgManager && pkgManager->supportsFileListCache() && pkgList.size() >= _minCachePkgListSize )
	createCachePkgReadJobs( tree, pkgList );
    else
	createAsyncPkgReadJobs( tree, pkgList );
}


void PkgReader::createCachePkgReadJobs( DirTree * tree, const PkgInfoList & pkgList )
{
    const PkgManager * pkgManager = PkgQuery::primaryPkgManager();
    CHECK_PTR( pkgManager );

    // The shared pointer will take care of deleting the cache when the last
    // job that uses it is destroyed.
    QSharedPointer<PkgFileListCache> fileListCache( pkgManager->createFileListCache( PkgFileListCache::LookupByPkg ) );
    if ( !fileListCache )
    {
	logError() << "Creating the file list cache failed" << Qt::endl;
	tree->sendFinished();
	return;
    }

    for ( PkgInfo * pkg : pkgList )
    {
	CachePkgReadJob * job = new CachePkgReadJob( tree, pkg, _verboseMissingPkgFiles, fileListCache );
	CHECK_NEW( job );
	tree->addJob( job );
    }
}


void PkgReader::createAsyncPkgReadJobs( DirTree * tree, const PkgInfoList & pkgList )
{
    //logDebug() << Qt::endl;

    ProcessStarter * processStarter = new ProcessStarter;
    CHECK_NEW( processStarter );
    processStarter->setAutoDelete( true );
    processStarter->setMaxParallel( _maxParallelProcesses );

    for ( PkgInfo * pkg : pkgList )
    {
	QProcess * process = createReadFileListProcess( pkg );
	if ( process )
	{
	    AsyncPkgReadJob * job = new AsyncPkgReadJob( tree, pkg, _verboseMissingPkgFiles, process );
	    CHECK_NEW( job );
	    tree->addBlockedJob( job );

	    processStarter->add( process );
	}
    }

    processStarter->start();
}


void PkgReader::readSettings()
{
    Settings settings;

    settings.beginGroup( "Pkg" );

    _maxParallelProcesses   = settings.value( "MaxParallelProcesses",   DEFAULT_PARALLEL_PROCESSES  ).toInt();
    _minCachePkgListSize    = settings.value( "MinCachePkgListSize",    DEFAULT_CACHE_PKG_LIST_SIZE ).toInt();
    _verboseMissingPkgFiles = settings.value( "VerboseMissingPkgFiles", false ).toBool();

    settings.setDefaultValue( "MaxParallelProcesses",   DEFAULT_PARALLEL_PROCESSES  );
    settings.setDefaultValue( "MinCachePkgListSize",    DEFAULT_CACHE_PKG_LIST_SIZE );
    settings.setDefaultValue( "VerboseMissingPkgFiles", false );

    settings.endGroup();
}




namespace
{
    /**
     * Recursively finalize all directories in a subtree.
     **/
    void finalizeAll( DirInfo * subtree )
    {
	for ( FileInfo * child = subtree->firstChild(); child; child = child->next() )
	{
	    if ( child->isDirInfo() )
		finalizeAll( child->toDirInfo() );
	}

	if ( !subtree->readError() )
	    subtree->setReadState( DirFinished );

	subtree->finalizeLocal();
    }


    /**
     * Do an lstat() syscall for 'path' or fetch the result from a cache.
     * Return false if lstat() fails.
     **/
    bool lstat( struct stat & statInfo, const QString & path )
    {
	const int result = ::lstat( path.toUtf8(), &statInfo );

	if ( result != 0 )
	    return false;	// lstat() failed

	if ( S_ISDIR( statInfo.st_mode ) )	// directory
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
     * Create a DirInfo or FileInfo node from a path and lstat call.
     **/
    FileInfo * createItem( const QString & path,
			   const QString & name,
			   DirTree       * tree,
			   DirInfo       * parent )
    {
	// logDebug() << "path: \"" << path << "\"" << Qt::endl;

	struct stat statInfo;
	if ( !lstat( statInfo, path ) ) // lstat() failed
	    return nullptr;

	if ( S_ISDIR( statInfo.st_mode ) )	// directory
	{
	    FileInfo * newDir = new DirInfo( parent, tree, name, statInfo );
	    CHECK_NEW( newDir );
	    return newDir;
	}
	else					// not directory
	{
	    FileInfo * newFile = new FileInfo( parent, tree, name, statInfo );
	    CHECK_NEW( newFile );
	    return newFile;
	}
    }


    /**
     * Locate a direct child of a DirInfo by name.
     **/
    FileInfo * locateChild( DirInfo * parent, const QString & pathComponent )
    {
	if ( pathComponent.isEmpty() )
	    return nullptr;

	for ( FileInfoIterator it( parent ); *it; ++it )
	{
	    if ( (*it)->name() == pathComponent )
		return *it;
	}

	return nullptr;
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
    }
    else if ( errno == EACCES )
    {
	// No permissions, expected error
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
	// Packaged file not present, just log it
	logWarning() << _pkg << " missing " << path << Qt::endl;
    }

    return newItem;
}


void PkgReadJob::addFiles( const QStringList & fileList )
{
    DirInfo * lastDir = _pkg;
    QString lastDirPath( '/' );

    for ( const QString & fileListPath : fileList )
    {
	if ( fileListPath.isEmpty() )
	    continue;

	// logDebug() << "Adding " << fileListPath << " to " << _pkg << Qt::endl;

	// usually the DirInfo parent will already have been created, and usually just previous
	if ( fileListPath.startsWith( lastDirPath ) )
	{
	    // Probably just created the directory for this file
	    const QString fileName = fileListPath.section( '/', -1 );
	    if ( fileListPath.size() == lastDirPath.size() + fileName.size() + 1 )
	    {
		// Definitely just created the directory for this file, so it exists
		createItem( fileListPath, fileName, lastDir );
		continue;
	    }
	}

	// Now we have to start from the top level and ensure every directory exists
	QString currentPath;
	DirInfo * parent = _pkg;

	const QStringList components = fileListPath.split( '/', Qt::SkipEmptyParts );
	for ( const QString & currentComponent : components )
	{
	    currentPath += '/' + currentComponent;

	    FileInfo * newParent = locateChild( parent, currentComponent );
	    if ( !newParent )
		newParent = createItem( currentPath, currentComponent, parent );

	    if ( newParent && newParent->isDirInfo() )
	    {
		parent = newParent->toDirInfo();
		lastDir = parent;
		lastDirPath = currentPath;
	    }
	    else if ( currentComponent != components.constLast() )
	    {
		// Failure that used to occur a lot (for dpkg) when symlinks weren't resolved
		logWarning() << newParent << " should be a directory, but is not" << Qt::endl;
		continue;
	    }
	}
    }
}





AsyncPkgReadJob::AsyncPkgReadJob( DirTree   * tree,
				  PkgInfo   * pkg,
				  bool        verboseMissingPkgFiles,
				  QProcess  * readFileListProcess ):
    PkgReadJob ( tree, pkg, verboseMissingPkgFiles ),
    _readFileListProcess { readFileListProcess }
{
    CHECK_PTR( _readFileListProcess );

    connect( _readFileListProcess, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ),
	     this,                 &AsyncPkgReadJob::readFileListFinished );
}


void AsyncPkgReadJob::readFileListFinished( int                  exitCode,
					    QProcess::ExitStatus exitStatus )
{
    // Always get this job out of the blocked queue and clean up the file list process
    tree()->unblock( this );
    _readFileListProcess->deleteLater();

    if ( exitStatus != QProcess::NormalExit )
    {
	logError() << "Get file list command crashed for " << pkg() << Qt::endl;
    }
    else if ( exitCode != 0 )
    {
	logError() << "Get file list command exit status " << exitStatus << " for " << pkg() << Qt::endl;
    }
    else // ok
    {
	const QString output = QString::fromUtf8( _readFileListProcess->readAll() );
	_fileList = pkg()->pkgManager()->parseFileList( output );

	return;
    }

    // There was an error of some sort, logged above
    pkg()->setReadState( DirError );
    tree()->sendReadJobFinished( pkg() );

    finished();
    // Don't add anything after finished() since this deletes this job!
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

    return QStringList();
}
