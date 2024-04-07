/*
 *   File name: PkgReader.cpp
 *   Summary:	Support classes for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include "PkgReader.h"
#include "DirTree.h"
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


PkgReader::PkgReader( DirTree * tree ):
    _tree { tree }
{
    readSettings();
}


PkgReader::~PkgReader()
{
    writeSettings();
}


/**
 * Filter the package list: Remove those package that don't match the
 * filter.
 **/
static void filterPkgList( PkgInfoList & pkgList, const PkgFilter & filter )
{
    if ( filter.filterMode() == PkgFilter::SelectAll )
	return;

    for ( PkgInfo * pkg : pkgList )
    {
	if ( filter.matches( pkg->baseName() ) )
	    pkgList.removeAll( pkg );
    }
}


/**
 * Create a suitable display names for a package: Packages that are
 * only installed in one version or for one architecture will simply
 * keep their base name; others will have the version and/or the
 * architecture appended so the user can tell them apart.
 **/
static void createDisplayName( const QString & pkgName, const PkgInfoList & pkgList )
{
    if ( pkgList.size() < 2 )
	return;

    const QString & version  = pkgList.first()->version();
    const QString & arch     = pkgList.first()->arch();

    bool sameVersion = true;
    bool sameArch    = true;

    for ( int i = 1; i < pkgList.size(); ++i )
    {
	if ( pkgList.at( i )->version() != version )
	    sameVersion = false;

	if ( pkgList.at( i )->arch() != arch )
	    sameArch = false;
    }

    if ( !sameVersion )
    {
	logDebug() << "Found multi version pkg " << pkgName
		   << " same arch: " << sameArch
		   << Qt::endl;
    }

    for ( PkgInfo * pkg : pkgList )
    {
	QString name = pkgName;

	if ( !sameVersion )
	{
	    name += "-" + pkg->version();
	    pkg->setMultiVersion( true );
	}

	if ( !sameArch )
	{
	    name += ":" + pkg->arch();
	    pkg->setMultiArch( true );
	}

	// logDebug() << " Setting name " << name << Qt::endl;
	pkg->setName( name );
    }
}


/**
 * Handle packages that are installed in multiple versions or for
 * multiple architectures: Assign a different display name to each of
 * them.
 **/
static void handleMultiPkg( const PkgInfoList & pkgList, MultiPkgInfo & multiPkg )
{
    for ( PkgInfo * pkg : pkgList )
	multiPkg.insert( pkg->baseName(), pkg );

    for ( const QString & pkgName : multiPkg.uniqueKeys() )
	createDisplayName( pkgName, multiPkg.values( pkgName ) );
}


void PkgReader::read( const PkgFilter & filter )
{
    //logInfo() << "Reading " << filter << Qt::endl;

    PkgInfoList pkgList = PkgQuery::installedPkg();
    filterPkgList( pkgList, filter );

    if ( pkgList.isEmpty() )
    {
	_tree->sendFinished();
	return;
    }

    MultiPkgInfo multiPkg;
    handleMultiPkg( pkgList, multiPkg );
    addPkgToTree( pkgList );

    const PkgManager * pkgManager = PkgQuery::primaryPkgManager();

    if ( pkgManager && pkgManager->supportsFileListCache() && pkgList.size() >= _minCachePkgListSize )
	createCachePkgReadJobs( pkgList );
    else
	createAsyncPkgReadJobs( pkgList );
}


void PkgReader::addPkgToTree( const PkgInfoList & pkgList )
{
    CHECK_PTR( _tree );
    CHECK_PTR( _tree->root() );

    PkgInfo * top = new PkgInfo( _tree, _tree->root() );
    CHECK_NEW( top );
    _tree->root()->insertChild( top );

    for ( PkgInfo * pkg : pkgList )
    {
	pkg->setTree( _tree );
	top->insertChild( pkg );
    }

    top->setReadState( DirFinished );
    top->finalizeLocal();
}


void PkgReader::createCachePkgReadJobs( const PkgInfoList & pkgList )
{
    const PkgManager * pkgManager = PkgQuery::primaryPkgManager();
    CHECK_PTR( pkgManager );

    // The shared pointer will take care of deleting the cache when the last
    // job that uses it is destroyed.
    QSharedPointer<PkgFileListCache> fileListCache( pkgManager->createFileListCache( PkgFileListCache::LookupByPkg ) );
    if ( !fileListCache )
    {
	logError() << "Creating the file list cache failed" << Qt::endl;
	return;
    }

    for ( PkgInfo * pkg : pkgList )
    {
	CachePkgReadJob * job = new CachePkgReadJob( _tree, pkg, _verboseMissingPkgFiles, fileListCache );
	CHECK_NEW( job );
	_tree->addJob( job );
    }
}


void PkgReader::createAsyncPkgReadJobs( const PkgInfoList & pkgList )
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
	    AsyncPkgReadJob * job = new AsyncPkgReadJob( _tree, pkg, _verboseMissingPkgFiles, process );
	    CHECK_NEW( job );
	    _tree->addBlockedJob( job );
	    processStarter->add( process );
	}
    }

    processStarter->start();
}


QProcess * PkgReader::createReadFileListProcess( PkgInfo * pkg )
{
    CHECK_PTR( pkg );
    CHECK_PTR( pkg->pkgManager() );

    const QString command = pkg->pkgManager()->fileListCommand( pkg );

    if ( command.isEmpty() )
    {
	logError() << "Empty file list command for " << pkg << Qt::endl;
	return nullptr;
    }

    QStringList args	  = command.split( QRegularExpression( "\\s+" ) );
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


void PkgReader::readSettings()
{
    Settings settings;
    settings.beginGroup( "Pkg" );

    _maxParallelProcesses   = settings.value( "MaxParallelProcesses"  , DEFAULT_PARALLEL_PROCESSES  ).toInt();
    _minCachePkgListSize    = settings.value( "MinCachePkgListSize"   , DEFAULT_CACHE_PKG_LIST_SIZE ).toInt();
    _verboseMissingPkgFiles = settings.value( "VerboseMissingPkgFiles", false ).toBool();

    settings.endGroup();
}


void PkgReader::writeSettings()
{
    Settings settings;
    settings.beginGroup( "Pkg" );

    settings.setValue( "MaxParallelProcesses"  , _maxParallelProcesses   );
    settings.setValue( "MinCachePkgListSize"   , _minCachePkgListSize    );
    settings.setValue( "VerboseMissingPkgFiles", _verboseMissingPkgFiles );

    settings.endGroup();
}






QHash<QString, struct stat> PkgReadJob::_statCache;
int PkgReadJob::_activeJobs = 0;
int PkgReadJob::_cacheHits  = 0;
int PkgReadJob::_lstatCalls = 0;


PkgReadJob::PkgReadJob( DirTree   * tree,
			PkgInfo   * pkg,
			bool        verboseMissingPkgFiles ):
    QObject (),
    DirReadJob ( tree, pkg ),
    _pkg { pkg },
    _verboseMissingPkgFiles { verboseMissingPkgFiles }
{
    ++_activeJobs;
}


PkgReadJob::~PkgReadJob()
{
    if ( --_activeJobs < 1 )
    {
        // logDebug() << "The last PkgReadJob is done; clearing the stat cache." << Qt::endl;
        reportCacheStats();
        clearStatCache();
    }
}


void PkgReadJob::clearStatCache()
{
    _statCache.clear();
    _activeJobs = 0;
    _cacheHits  = 0;
    _lstatCalls = 0;
}


void PkgReadJob::reportCacheStats()
{
    const float hitPercent = _lstatCalls > 0 ? ( 100.0 * _cacheHits ) /_lstatCalls : 0.0;

    logDebug() << _lstatCalls << " lstat() calls" << Qt::endl;
    logDebug() << _cacheHits << " stat cache hits (" << qRound( hitPercent ) << "%)" << Qt::endl;
}


void PkgReadJob::startReading()
{
    //logInfo() << "Reading " << _pkg << Qt::endl;

    CHECK_PTR( _pkg );

    _pkg->setReadState( DirReading );

    for ( const QString & path : fileList() )
	addFile( path );

    finalizeAll( _pkg );
    _tree->sendReadJobFinished( _pkg );
    finished();
    // Don't add anything after finished() since this deletes this job!
}


QStringList PkgReadJob::fileList()
{
    logDebug() << "Using default PkgQuery::fileList() for " << _pkg << Qt::endl;

    return PkgQuery::fileList( _pkg );
}


void PkgReadJob::addFile( const QString & fileListPath )
{
    if ( fileListPath.isEmpty() )
	return;

    // logDebug() << "Adding " << fileListPath << " to " << _pkg << Qt::endl;

    QStringList currentPath;
    DirInfo * parent = _pkg;

    const QStringList components = fileListPath.split( "/", Qt::SkipEmptyParts );
    for ( const QString & currentComponent : components )
    {
	currentPath << currentComponent;

	FileInfo * newParent = parent->locateChild( currentComponent );
	if ( !newParent )
	{
	    newParent = createItem( currentPath, _tree, parent );
	    if ( !newParent )
	    {
		//Packaged file not actually on disk, just log it
		//parent->setReadState( DirError );
		if ( _verboseMissingPkgFiles )
		    logWarning() << _pkg << ": missing: " << fileListPath << Qt::endl;

		return;
	    }

	    _tree->childAddedNotify( newParent );
	    // logDebug() << "Created " << newParent << Qt::endl;
	}

	if ( currentComponent != components.constLast() )
	{
	    parent = newParent->toDirInfo();
	    if ( !parent )
	    {
		logWarning() << newParent << " should be a directory, but is not" << Qt::endl;
		return;
	    }
	}
    }
}


void PkgReadJob::finalizeAll( DirInfo * subtree )
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


FileInfo * PkgReadJob::createItem( const QStringList & pathComponents,
				   DirTree	     * tree,
				   DirInfo	     * parent )
{
    const QString path = QString( "/" ) + pathComponents.join( "/" );

    // logDebug() << "path: \"" << path << "\"" << Qt::endl;

    struct stat statInfo;

    if ( !PkgReadJob::lstat( statInfo, path ) ) // lstat() failed
	return nullptr;

    const QString & name = pathComponents.last();

    if ( S_ISDIR( statInfo.st_mode ) )		// directory?
    {
	DirInfo * dir = new DirInfo( parent, tree, name, statInfo );
	CHECK_NEW( dir );

	if ( parent )
	    parent->insertChild( dir );

	return dir;
    }
    else					// no directory
    {
	FileInfo * file = new FileInfo( parent, tree, name, statInfo );
	CHECK_NEW( file );

	if ( parent )
	    parent->insertChild( file );

	return file;
    }
}


bool PkgReadJob::lstat( struct stat & statInfo, const QString & path )
{
    if ( _statCache.contains( path ) )
    {
        ++_cacheHits;
        //logDebug() << "stat cache hit for " << path << Qt::endl;
        statInfo = _statCache.value( path );
	return true;
    }

    const int result = ::lstat( path.toUtf8(), &statInfo );
    ++_lstatCalls;

    if ( result != 0 )
	return false;	// lstat() failed

    _statCache.insert( path, statInfo );
    if ( S_ISDIR( statInfo.st_mode ) )	// directory?
    {
	// Zero the directory's own size fields to prevent them from
	// distorting the total sums: Otherwise the directory would be
	// counted in each package that uses the directory, and a directory
	// with a large own size and only a tiny file that belongs to that
	// package would completely dwarf the package file in the treemap.
	statInfo.st_size   = 0;
	statInfo.st_blocks = 0;
	statInfo.st_mtime  = 0;
    }

    return true;
}






AsyncPkgReadJob::AsyncPkgReadJob( DirTree   * tree,
				  PkgInfo   * pkg,
				  bool        verboseMissingPkgFiles,
				  QProcess  * readFileListProcess ):
    PkgReadJob ( tree, pkg, verboseMissingPkgFiles ),
    _readFileListProcess { readFileListProcess }
{
    if ( _readFileListProcess )
    {
	connect( _readFileListProcess, qOverload<int, QProcess::ExitStatus>( &QProcess::finished ),
		 this,                 &AsyncPkgReadJob::readFileListFinished );
    }
}


void AsyncPkgReadJob::readFileListFinished( int			 exitCode,
					    QProcess::ExitStatus exitStatus )
{
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
	_tree->unblock( this ); // schedule this job
	_readFileListProcess->deleteLater();

	return;
    }

    // There was an error of some sort, logged above
    pkg()->setReadState( DirError );
    _tree->sendReadJobFinished( pkg() );

    delete _readFileListProcess;
    _readFileListProcess = nullptr;

    finished();
    // Don't add anything after finished() since this deletes this job!
}


QStringList AsyncPkgReadJob::fileList()
{
    return _fileList;
}





QStringList CachePkgReadJob::fileList()
{
    if ( !_fileListCache || _fileListCache->pkgManager() != pkg()->pkgManager() )
    {
	logDebug() << "Falling back to the simple PkgQuery::fileList() for " << pkg() << Qt::endl;

	return PkgQuery::fileList( pkg() );
    }

    const QString pkgName = pkg()->pkgManager()->queryName( pkg() );

    if ( _fileListCache->containsPkg( pkgName ) )
	return _fileListCache->fileList( pkgName );
//            _fileListCache->remove( pkgName ); // making a QHash smaller is no help

    if ( _fileListCache->containsPkg( pkg()->name() ) )
	return _fileListCache->fileList( pkg()->name() );
//            _fileListCache->remove( pkg()->name() );

    return QStringList();
}
