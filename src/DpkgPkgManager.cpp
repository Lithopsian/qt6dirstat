/*
 *   File name: DpkgPkgManager.cpp
 *   Summary:   Dpkg package manager support for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QDir>
#include <QFileInfo>

#include "DpkgPkgManager.h"
#include "Exception.h"
#include "PkgFileListCache.h"
#include "SysUtil.h"


using namespace QDirStat;


#define VERBOSE_DIVERSIONS 0
#define VERBOSE_PACKAGES   0


namespace
{
    /**
     * Return whether a given dpkg query line represents a diversion
     *
     * For dpkg -S, lines may begin "diversion by ... " or "local diversion ..."
     *
     * For dpkg -L, lines may begin "locally diverted to", package diverts others to",
     * or "diverted by".
    */
    bool isDiversion( const QString & line )
	{ return line.startsWith( "diversion by"_L1 ) || line.startsWith( "local diversion"_L1 ); }
    bool isLocalDiversion( const QString & line )
	{ return line.startsWith( "local diversion"_L1 ); }
    bool isDiversionFrom( const QString & line )
	{ return isDiversion( line ) && line.contains( "from: "_L1 ); }
    bool isDiversionTo( const QString & line )
	{ return isDiversion( line ) && line.contains( "to: "_L1 ); }
    bool isDivertedBy( const QString & line )
	{ return line.startsWith( "diverted by"_L1 ) || line.startsWith( "locally diverted"_L1 ); }
    bool isPackageDivert( const QString & line )
	{ return line.startsWith( "package diverts"_L1 ); }


    /**
     * Resolves symlinks in the directory path of a file string.  If the file itself
     * is a symlink, this is kept unresolved.
    */
    QString resolvePath( const QString & pathname )
    {
	const QFileInfo fileInfo{ pathname };

	// a directory that is a symlink in root (eg. /lib) is resolved
	if ( fileInfo.dir().isRoot() && fileInfo.isDir() ) // first test doesn't access the filesystem
	{
	    const QString realpath = fileInfo.canonicalFilePath();

#if VERBOSE_PACKAGES
	    logDebug() << pathname << " " << realpath << Qt::endl;
#endif
	    return realpath.isEmpty() ? pathname : realpath;
	}

	// in all other cases, only symlinks in the parent path are resolved
	const QString filename = fileInfo.fileName();
	const QString pathInfo = fileInfo.path();
	const QString realpath = QFileInfo{ pathInfo }.canonicalFilePath();

#if VERBOSE_PACKAGES
	logDebug() << pathname << " " << realpath << " " << filename << Qt::endl;
#endif

	return ( realpath != pathInfo && !realpath.isEmpty() ) ? realpath % '/' % filename : pathname;
    }

    /**
     * Return the command for the dpkg manager program.
     **/
    QString dpkgCommand()
    {
	return "/usr/bin/dpkg";
    }


    /**
     * Runs dpkg -S against the given path and returns the output.
     *
     * exitCode indicates the success or failure of the command.
    */
    QString runDpkg( const QString & path, int &exitCode, bool logError )
    {
	return SysUtil::runCommand( dpkgCommand(),
	                            { "-S", path },
	                            &exitCode,
	                            1,		// better not to lock the whole program for 15 seconds
	                            false,	// don't log command
	                            false,	// don't log output
	                            logError );
    }


    /**
     * Sub-query to find the original owning package of a renamed diverted file.
     **/
    QString originalOwningPkg( const QString & path )
    {
	// Search with the filename from the (potentially symlinked) path
	// ... looking for exactly three lines matching:
	// diversion by other-package from: path
	// diversion by other-package to: renamed-path
	// package list: path
	// The package list may contain either the original package, or a list including
	// both the original and the diverting package (and possibly others).  The
	// diverting package may include the file explicitly, but often creates a symlink
	// (eg. to /etc/alternatives) on install.

	const QString pathResolved = resolvePath( path );

	int exitCode;
	const QString output = runDpkg( path, exitCode, true ); // don't ignore error codes
	if ( exitCode != 0 )
	    return QString{};

	const QStringList lines = output.trimmed().split( u'\n' );
	for ( auto line = lines.begin(); line != lines.end(); ++line )
	{
	    // Fast-forward to a diversion line
	    while ( !isDiversionFrom( *line ) )
	    {
		++line;
		if ( line == lines.end() ) // should never happen, but who knows
		    return QString{};
	    }

#if VERBOSE_DIVERSIONS
	    logDebug() << *line << Qt::endl;
#endif
	    // The next line should be a diversion to line
	    if ( ++line != lines.end() && isDiversionTo( *line ) )
	    {
		const QString divertingPkg = line->split( u' ' ).at( 2 );

#if VERBOSE_DIVERSIONS
		logDebug() << *line << Qt::endl;
#endif
		if ( ++line != lines.end() )
		{
		    // Might now have the (third) line with the list of packages for the original file
		    const QStringList fields = line->split( ": "_L1 );
		    if ( fields.size() == 2 && resolvePath( fields.last() ) == pathResolved )
		    {
			// Pick any one which isn't the diverting package
			const QStringList packages = fields.first().split( ", "_L1 );

#if VERBOSE_DIVERSIONS
			logDebug() << " diverted file owned by " << packages << Qt::endl;
#endif
			for ( const QString & package : packages )
			    if ( package != divertingPkg )
				return package;
		    }
		}
	    }
	}

	return QString{};
    }


    /**
     * This searches the lines produced by a dpkg -S query.
     **/
    QString searchOwningPkg( const QString & path, const QString & output )
    {
	const QStringList lines = output.trimmed().split( u'\n', Qt::SkipEmptyParts );
	for ( auto line = lines.begin(); line != lines.end(); ++line )
	{
	    // For diversions, the line "diversion by ... from: ..." gives the current owning package
	    // A line "diversion by ... to ..." should immediately follow and indicates a path that is
	    // the divert target.  The file will not exist unless the diverted file has been renamed;
	    // in this case the owning package of this file can only be found by another query against
	    // the file path as shown in the "diversion ... from" line.
	    // Thankfully very rare!
	    if ( isDiversion( *line ) )
	    {
		if ( !isDiversionFrom( *line ) )
		    // something wrong, just skip it and hope
		    continue;

		// Need to remember this first path and package to compare with the third one
		// to see if the file really belongs to that package
		const QStringList fields1 = line->split( ": "_L1 );
		if ( fields1.size() != 2 )
		    continue;

		const QString & path1 = fields1.last();
		const QString divertPkg =
		    isLocalDiversion( *line ) ? QString{} : fields1.first().split( u' ' ).at( 2 );

		// the next line should contain the path where this file now resides
		// (or would reside if it hasn't been diverted yet)
		if ( ++line == lines.end() )
		    return QString{};

		if ( !isDiversionTo( *line ) )
		    continue;

		const QStringList fields2 = line->split( ": "_L1 );
		if ( fields2.size() != 2 )
		    continue;

		const QString path2Resolved = resolvePath( fields2.last() );
		if ( path2Resolved == path )
		    // the renamed file is our file, have to do another query to get the package:
		    // dpkg -S against the pathname from the diversion by ... from line
		    return originalOwningPkg( path1 );

		// if this is a local diversion, give up at this point because there is no owning package
		if ( isLocalDiversion( *line ) )
		    continue;

		if ( ++line == lines.end() )
		    return QString{};

		// and the line after that might give the package and the original file path
		const QStringList fields3 = line->split( ": "_L1 );
		if ( fields3.size() != 2 )
		    continue;

		const QString & packages = fields3.first();

		// if the from/to pair for the renamed file is followed by an unrelated entry ...
		const QString path1Resolved = resolvePath ( path1 );
		const QString path3Resolved = resolvePath( fields3.last() );
		if ( path1Resolved != path3Resolved )
		{
		    // ... then start parsing again normally from the third line
		    --line;
		    continue;
		}

		// if the package from the diversion by ... from line is also in the third line ...
		if ( !divertPkg.isEmpty() && packages.split( ", "_L1 ).contains( divertPkg ) )
		{
		    // ... and the resolved path matches the original file ...
		    if ( path == path1Resolved )
			// ... then return the diverting package from the first line
			return divertPkg;
		}
		// ... or the resolved path matches the renamed file
		else if ( path == path2Resolved )
		{
		    // ... then return the package that owned this file pre-divert
		    return packages.split( ": "_L1 ).first();
		}

		// wrong diversion triplet, carry on, skipping the third line
		continue;
	    }

	    // Just a normal package: path line
	    const QStringList packages = line->split( ": "_L1 );
	    if ( packages.size() != 2 )
		continue;

	    // resolve any symlinks in the package path
	    const QFileInfo pkgFileInfo{ packages.last() };
	    const QString pkgPathInfo = pkgFileInfo.path();
	    const QString pkgRealpath = QFileInfo{ pkgPathInfo }.canonicalFilePath();
	    const QString pkgFilename = pkgFileInfo.fileName();
	    if ( !pkgRealpath.isEmpty() && !pkgFilename.isNull() )
	    {
#if VERBOSE_PACKAGES
		logDebug() << " " << pkgRealpath << " " << pkgFilename << Qt::endl;
#endif

		// Is this the exact file we were looking for?
		if ( pkgRealpath % '/' % pkgFilename == path )
		    return packages.first();
	    }
	}

	return QString{};
    }


    /**
     * Parse a package list as output by "dpkg-query --show --showformat".
     **/
    PkgInfoList parsePkgList( const PkgManager * pkgManager, const QString & output )
    {
	PkgInfoList pkgList;

	const QStringList splitOutput = output.split( u'\n', Qt::SkipEmptyParts );
	for ( const QString & line : splitOutput )
	{
	    const QStringList fields = line.split( " | "_L1 );

	    if ( fields.size() == 4 )
	    {
		const QString & name    = fields.at( 0 );
		const QString & version = fields.at( 1 );
		const QString & arch    = fields.at( 2 );
		const QString & status  = fields.at( 3 );
		if ( status == "install ok installed"_L1 || status == "hold ok installed"_L1 )
		{
		    pkgList << new PkgInfo{ name, version, arch, pkgManager };
		}
#if VERBOSE_PACKAGES
		else
		{
		    logDebug() << "Ignoring " << line << Qt::endl;
		}
#endif
	    }
	    else
	    {
		logError() << "Invalid dpkg-query output: \"" << line << '\n' << Qt::endl;
	    }
	}

	return pkgList;
    }

} // namespace


bool DpkgPkgManager::isPrimaryPkgManager() const
{
    return SysUtil::tryRunCommand( dpkgCommand(), { "-S", dpkgCommand() }, "^dpkg:.*" );
}


bool DpkgPkgManager::isAvailable() const
{
    return SysUtil::haveCommand( dpkgCommand() );
}


QString DpkgPkgManager::owningPkg( const QString & path ) const
{
    // Try first with the full (possibly symlinked) path
    int exitCode;
    const QString fullPathOutput = runDpkg( path, exitCode, false ); // error code likely, ignore
    if ( exitCode == 0 )
    {
	const QString package = searchOwningPkg( path, fullPathOutput );
	if ( !package.isEmpty() )
	    return package;
    }

    // Search again just by filename in case part of the directory path is symlinked
    // (this may produce a lot of rows)
    const QFileInfo fileInfo{ path };
    const QString filenameOutput = runDpkg( fileInfo.fileName(), exitCode, false ); // error code likely, ignore
    if ( exitCode != 0 )
	return QString{};

    return searchOwningPkg( path, filenameOutput );
}


PkgInfoList DpkgPkgManager::installedPkg() const
{
    int exitCode;
    const QString output =
	SysUtil::runCommand( dpkgCommand(),
	                     { "--show", "--showformat=${Package} | ${Version} | ${Architecture} | ${Status}\n" },
	                     &exitCode );

    if ( exitCode == 0 )
	return parsePkgList( this, output );

    return PkgInfoList{};
}


QStringList DpkgPkgManager::parseFileList( const QString & output ) const
{
    QStringList fileList;

    const QStringList lines = output.split( u'\n' );
    for ( const QString & line : lines )
    {
	if ( isDivertedBy( line ) )
	{
	    if ( fileList.isEmpty() ) // should never happen, but avoids a crash
		continue;

	    // previous line referred to a file that has been diverted to a different location
	    fileList.removeLast();

	    // this line contains the new location for the file from this package
	    const QStringList fields = line.split( ": "_L1 );
	    const QString & divertedFile = fields.last();

	    if ( fields.size() == 2 && !divertedFile.isEmpty() )
		fileList << resolvePath( divertedFile );
	}
	else if ( line != "/."_L1 && !isPackageDivert( line ) )
	{
	    fileList << resolvePath( line );
	}
    }

    return fileList;
}


QString DpkgPkgManager::queryName( const PkgInfo * pkg ) const
{
    CHECK_PTR( pkg );

#if 0
    QString name = pkg->baseName();

    if ( pkg->isMultiVersion() )
	name += '_' % pkg->version();

    if ( pkg->isMultiArch() )
	name += ':' % pkg->arch();

    return name;
#else
    return pkg->arch() == "all"_L1 ? pkg->baseName() : pkg->baseName() % ':' % pkg->arch();
#endif
}


PkgFileListCache * DpkgPkgManager::createFileListCache( PkgFileListCache::LookupType lookupType ) const
{
    int exitCode;
    QString output = runDpkg( "*", exitCode, true ); // don't ignore error codes
    if ( exitCode != 0 )
	return nullptr;

    const QStringList lines = output.split( u'\n', Qt::SkipEmptyParts );
#if VERBOSE_PACKAGES
    logDebug() << lines.size() << " output lines" << Qt::endl;
#endif

    PkgFileListCache * cache = new PkgFileListCache{ this, lookupType };

    // Sample output:
    //
    //	   zip: /usr/bin/zip
    //	   zlib1g-dev:amd64: /usr/include/zlib.h
    //	   zlib1g:i386, zlib1g:amd64: /usr/share/doc/zlib1g
    for ( auto line = lines.begin(); line != lines.end(); ++line )
    {
	QString pathname;
	QString packages;

	if ( isDiversion( *line ) )
	{
	    // For diversions, the line "diversion by ... from: ..." gives the current owning package.
	    // Normal lines for files that have been diverted should be ignored because that file will
	    // have been renamed to the path shown in the "diversion by ... to ..." line.
	    // The original file may not exist (see glx-diversions) or may now be owned by a different
	    // package. The new owning package is shown by another query against the file path as shown
	    // in the "diversion ... from" line.
	    // Thankfully very rare!
	    if ( !isDiversionFrom( *line ) )
		// something wrong, just skip it and hope
		continue;

	    // need to take this first path and package to compare with the last one
	    // to see if the file really belongs to that package
	    const QStringList fields1 = line->split( ": "_L1 );
	    const QString & path1 = fields1.last();
	    const QString divertingPkg =
		isLocalDiversion( *line ) ? QString{} : fields1.first().split( u' ' ).at( 2 );

	    // the next line should contain the path where this file now resides
	    ++line;
	    if ( !isDiversionTo( *line ) )
		continue;

	    const QStringList fields2 = line->split( ": "_L1 );
	    if ( fields2.size() != 2 )
		continue;

	    // if the renamed file is the one
	    const QString path2 = resolvePath( fields2.last() );

	    // ... and the one after that might give the package and the original file path
	    ++line;
	    const QStringList fields3 = line->split( ": "_L1 );
	    if ( fields3.size() != 2 )
		continue;

	    packages = fields3.first();
	    // the from/to pair for the renamed file is followed by an unrelated entry
	    const QString & path3 = fields3.last();
	    if ( path1 != path3 )
	    {
		// we start again, could be a normal entry of another diversion line
		--line;
		continue;
	    }
	    else
	    {
		QStringList packagesList = packages.split( ", "_L1 );

		// immediately add the diverting package with this path if it is in the third line
		if ( !divertingPkg.isEmpty() && packagesList.contains( divertingPkg ) )
		{
		    cache->add( divertingPkg, resolvePath( path3 ) );
#if VERBOSE_DIVERSIONS
		    logDebug() << divertingPkg << " diverted " << resolvePath(path3) << Qt::endl;
#endif
		    // remove the diverting package from list, which might now be empty
		    packagesList.removeAt( packagesList.indexOf( divertingPkg ) );
		    packages = packagesList.join( ", "_L1 );
		}

		// associate renamed file only with its original packages
		pathname = resolvePath( path2 );
#if VERBOSE_DIVERSIONS
		if ( !packagesList.isEmpty() )
		    logDebug() << path1 << " from " << packages
		               << " diverted by " << divertingPkg << " to " << pathname << Qt::endl;
#endif
	    }
	}
	else
	{
	    const QStringList fields = line->split( ": "_L1 );
	    if ( fields.size() != 2 )
	    {
		logError() << "Unexpected file list line: \"" << *line << '"' << Qt::endl;
		continue;
	    }

	    pathname = resolvePath( fields.last() );
	    packages = fields.first();
	}

	if ( pathname.isEmpty() || pathname == "/."_L1 )
	    continue;

	const auto splitPackages = packages.split( ", "_L1 );
	for ( const QString & pkgName : splitPackages )
	{
	    if ( !pkgName.isEmpty() )
		cache->add( pkgName, pathname );
	}
    }

    logDebug() << "file list cache finished." << Qt::endl;

    return cache;
}
