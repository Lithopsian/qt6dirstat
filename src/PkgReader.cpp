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
    // logInfo() << Qt::endl;
    readSettings();
}


PkgReader::~PkgReader()
{
    writeSettings();

    // Intentionally NOT deleting the PkgInfo * items of _pkgList:
    // They are now owned by the DirTree.
}


void PkgReader::read( const PkgFilter & filter )
{
    //logInfo() << "Reading " << filter << Qt::endl;

    _pkgList = PkgQuery::installedPkg();
    filterPkgList( filter );

    if ( _pkgList.isEmpty() )
    {
	_tree->sendFinished();
	return;
    }

    handleMultiPkg();
    addPkgToTree();

    const PkgManager * pkgManager = PkgQuery::primaryPkgManager();

    if ( pkgManager && pkgManager->supportsFileListCache() && _pkgList.size() >= _minCachePkgListSize )
	createCachePkgReadJobs();
    else
	createAsyncPkgReadJobs();

    // Ownership of the PkgInfo * items in _pkgList was transferred to the
    // tree, so intentionally NOT calling qDeleteItems( _pkgList ) !

    _pkgList.clear();
    _multiPkg.clear();
}


void PkgReader::filterPkgList( const PkgFilter & filter )
{
    if ( filter.filterMode() == PkgFilter::SelectAll )
	return;

    PkgInfoList matches;

    for ( PkgInfo * pkg : _pkgList )
    {
	if ( filter.matches( pkg->baseName() ) )
	{
	    // logDebug() << "Selecting pkg " << pkg << Qt::endl;
	    matches << pkg;
	}
    }

    _pkgList = matches;
}


void PkgReader::handleMultiPkg()
{
    _multiPkg.clear();

    for ( PkgInfo * pkg : _pkgList )
	_multiPkg.insert( pkg->baseName(), pkg );

    for ( const QString & pkgName : _multiPkg.uniqueKeys() )
	createDisplayName( pkgName );
}


void PkgReader::createDisplayName( const QString & pkgName )
{
    const PkgInfoList pkgList = _multiPkg.values( pkgName );

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


void PkgReader::addPkgToTree()
{
    CHECK_PTR( _tree );
    CHECK_PTR( _tree->root() );

    PkgInfo * top = new PkgInfo( _tree, _tree->root() );
    CHECK_NEW( top );
    _tree->root()->insertChild( top );

    for ( PkgInfo * pkg : _pkgList )
    {
	pkg->setTree( _tree );
	top->insertChild( pkg );
    }

    top->setReadState( DirFinished );
    top->finalizeLocal();
}


void PkgReader::createCachePkgReadJobs()
{
    const PkgManager * pkgManager = PkgQuery::primaryPkgManager();
    CHECK_PTR( pkgManager );

    QSharedPointer<PkgFileListCache> fileListCache( pkgManager->createFileListCache( PkgFileListCache::LookupByPkg ) );
    // The shared pointer will take care of deleting the cache when the last
    // job that uses it is destroyed.

    if ( !fileListCache )
    {
	logError() << "Creating the file list cache failed" << Qt::endl;
	return;
    }

    for ( PkgInfo * pkg : _pkgList )
    {
	CachePkgReadJob * job = new CachePkgReadJob( this, _tree, pkg, fileListCache );
	CHECK_NEW( job );
	_tree->addJob( job );
    }
}


void PkgReader::createAsyncPkgReadJobs()
{
    //logDebug() << Qt::endl;

    ProcessStarter * processStarter = new ProcessStarter;
    CHECK_NEW( processStarter );
    processStarter->setAutoDelete( true );
    processStarter->setMaxParallel( _maxParallelProcesses );

    for ( PkgInfo * pkg : _pkgList )
    {
	QProcess * process = createReadFileListProcess( pkg );

	if ( process )
	{
	    AsyncPkgReadJob * job = new AsyncPkgReadJob( this, _tree, pkg, process );
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






QMap<QString, struct stat> PkgReadJob::_statCache;
int PkgReadJob::_activeJobs = 0;
int PkgReadJob::_cacheHits  = 0;
int PkgReadJob::_lstatCalls = 0;


PkgReadJob::PkgReadJob( PkgReader * reader,
		        DirTree   * tree,
			PkgInfo   * pkg  ):
    QObject (),
    DirReadJob ( tree, pkg ),
    _reader { reader },
    _pkg { pkg }
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
    // logInfo() << "Reading " << _pkg << Qt::endl;

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

    QStringList remaining = fileListPath.split( "/", Qt::SkipEmptyParts );
    QStringList currentPath;
    DirInfo *	parent = _pkg;

    while ( !remaining.isEmpty() )
    {
	const QString currentName = remaining.takeFirst();
	currentPath << currentName;

	FileInfo * newParent = _pkg->locate( parent, QStringList() << currentName );

	if ( !newParent )
	{
	    newParent = createItem( currentPath, _tree, parent );
	    if ( !newParent )
	    {
		//parent->setReadState( DirError );
		if ( _reader->verboseMissingPkgFiles() )
		    logWarning() << _pkg << ": missing: " << fileListPath << Qt::endl;

		return;
	    }

	    _tree->childAddedNotify( newParent );
	    // logDebug() << "Created " << newParent << Qt::endl;
	}

	if ( !remaining.isEmpty() )
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
    FileInfo * child = subtree->firstChild();

    while ( child )
    {
	if ( child->isDirInfo() )
	    finalizeAll( child->toDirInfo() );

	child = child->next();
    }

    if ( !subtree->readError() )
	subtree->setReadState( DirFinished );

    subtree->finalizeLocal();
}


FileInfo * PkgReadJob::createItem( const QStringList & pathComponents,
				   DirTree	     * tree,
				   DirInfo	     * parent )
{
    struct stat * statInfo;
    const QString path = QString( "/" ) + pathComponents.join( "/" );

    // logDebug() << "path: \"" << path << "\"" << Qt::endl;

    statInfo = this->lstat( path );

    if ( !statInfo ) // lstat() failed
	return nullptr;

    const QString name = pathComponents.last();

    if ( S_ISDIR( statInfo->st_mode ) )		// directory?
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


struct stat * PkgReadJob::lstat( const QString & path )
{
    static struct stat statInfo;

    if ( _statCache.contains( path ) )
    {
        ++_cacheHits;
        // logDebug() << "stat cache hit for " << path << Qt::endl;
        statInfo = _statCache.value( path );
    }
    else
    {
        const int result = ::lstat( path.toUtf8(), &statInfo );
        ++_lstatCalls;

        if ( result != 0 )
            return nullptr;	// lstat() failed

        _statCache.insert( path, statInfo );
    }

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

    return &statInfo;
}






AsyncPkgReadJob::AsyncPkgReadJob( PkgReader * reader,
				  DirTree   * tree,
				  PkgInfo   * pkg,
				  QProcess  * readFileListProcess ):
    PkgReadJob ( reader, tree, pkg ),
    _readFileListProcess { readFileListProcess }
{
    if ( _readFileListProcess )
    {
	connect( _readFileListProcess, qOverload<int, QProcess::ExitStatus>( &QProcess::finished ),
		 this,		       &AsyncPkgReadJob::readFileListFinished );
    }
}


void AsyncPkgReadJob::readFileListFinished( int			 exitCode,
					    QProcess::ExitStatus exitStatus )
{
    CHECK_PTR( _readFileListProcess );
    CHECK_PTR( _pkg );
    CHECK_PTR( _pkg->pkgManager() );

    if ( exitStatus != QProcess::NormalExit )
    {
	logError() << "Get file list command crashed for " << _pkg << Qt::endl;
    }
    else if ( exitCode != 0 )
    {
	logError() << "Get file list command exit status " << exitStatus << " for " << _pkg << Qt::endl;
    }
    else // ok
    {
	const QString output = QString::fromUtf8( _readFileListProcess->readAll() );
	_fileList      = _pkg->pkgManager()->parseFileList( output );
	_tree->unblock( this ); // schedule this job
	_readFileListProcess->deleteLater();

	return;
    }

    // There was an error of some sort, logged above
    _pkg->setReadState( DirError );
    _tree->sendReadJobFinished( _pkg );

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
    if ( _fileListCache && _fileListCache->pkgManager() == _pkg->pkgManager() )
    {
	const QString pkgName = _pkg->pkgManager()->queryName( _pkg );
        QStringList fileList;

	if ( _fileListCache->containsPkg( pkgName ) )
	{
            fileList = _fileListCache->fileList( pkgName );
            _fileListCache->remove( pkgName );
	}
	else if ( _fileListCache->containsPkg( _pkg->name() ) )
	{
	    fileList = _fileListCache->fileList( _pkg->name() );
            _fileListCache->remove( _pkg->name() );
	}

        return fileList;
    }

    logDebug() << "Falling back to the simple PkgQuery::fileList() for " << _pkg << Qt::endl;

    return PkgQuery::fileList( _pkg );
}
