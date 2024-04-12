/*
 *   File name: FileDetailsView.h
 *   Summary:	Details view for the currently selected file or directory
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */

#include "FileDetailsView.h"
#include "AdaptiveTimer.h"
#include "DirInfo.h"
#include "DirTreeModel.h"
#include "FileInfo.h"
#include "FileInfoSet.h"
#include "MimeCategorizer.h"
#include "MountPoints.h"
#include "PkgQuery.h"
#include "QDirStatApp.h"
#include "SystemFileChecker.h"
#include "SysUtil.h"
#include "FormatUtil.h"
#include "Logger.h"
#include "Exception.h"

#define MAX_SYMLINK_TARGET_LEN 25

using namespace QDirStat;


namespace
{
    /**
     * Set a label with a number and an optional prefix.
     **/
    void setLabel( QLabel        * label,
		  int             number,
		  const QString & prefix = "" )
    {
	CHECK_PTR( label );
	label->setText( prefix + QString( "%L1" ).arg( number ) );
    }


    /**
     * Set a file size label with a file size and an optional prefix.
     **/
    void setLabel( FileSizeLabel * label,
		   FileSize        size,
		   const QString & prefix = "" )
    {
	CHECK_PTR( label );
	label->setValue( size, prefix );
    }

    /**
     * Return the MIME category of a file.
     **/
    const QString & mimeCategory( const FileInfo * fileInfo )
    {
	return MimeCategorizer::instance()->name( fileInfo );
    }

    /**
     * Return a message string describing the status of a DirInfo node.
     **/
    QString subtreeMsg( const DirInfo * dir )
    {
	return FileDetailsView::readStateMsg( dir->isBusy() ? DirReading : dir->readState() );
    }

} // namespace


// The delay stages are constructed to rapidly move to stage 1, which is a short
// delay of half the time taken for the previous query to complete.  In practice,
// this delay will probably not be noticeable.  After that the delay increases only
// with fairly rapid repeated requests so a level which is likely to be visible, but
// will still allow most requests to complete after a moment.  The longest delays
// are only reached with very rapid repeated requests such as scrolling through a list
// of files and then quickly drop to a shorter delay when the repeated requests stop
// or slow down.
FileDetailsView::FileDetailsView( QWidget * parent ):
    QStackedWidget ( parent ),
    _ui { new Ui::FileDetailsView },
    _pkgUpdateTimer { new AdaptiveTimer( this,
	                                 { 0.0, 0.5, 1.0, 2.0, 5.0 }, // delay stages
					 { 3000, 1000, 500, 250, 150 } ) }, // cooldown stages
    _labelLimit { 0 } // Unlimited - TO DO: read _labelLimit from the config file
{
    CHECK_NEW( _ui );
    CHECK_NEW( _pkgUpdateTimer );

    _ui->setupUi( this );

    clear();

    connect( MimeCategorizer::instance(), &MimeCategorizer::categoriesChanged,
             this,                        &FileDetailsView::categoriesChanged );
}


FileDetailsView::~FileDetailsView()
{
    delete _ui;
}


void FileDetailsView::clear()
{
    setCurrentPage( _ui->emptyPage );
}


void FileDetailsView::setCurrentPage( QWidget *page )
{
    // Simply hiding all other widgets is not enough: The QStackedLayout will
    // still reserve screen space for the largest widget. The other pages
    // really need to be removed from the layout. They are still children of
    // the QStackedWidget, but no longer in the layout.

    while ( count() > 0 )
	removeWidget( widget( 0 ) );

    addWidget( page );
    setCurrentWidget( page );
}

/*
void FileDetailsView::showDetails( const FileInfoSet & selectedItems )
{
    const FileInfoSet sel = selectedItems.normalized();

    if ( sel.isEmpty() )
	clear();
    else
	showSelectionSummary( sel );
}
*/

void FileDetailsView::showDetails( FileInfo * file )
{
    if ( !file )
    {
	clear();
	return;
    }

    if ( file->isPkgInfo() )
    {
	showDetails( file->toPkgInfo() );
	return;
    }

    if ( file->isDirInfo() )
    {
	showDetails( file->toDirInfo() );
	return;
    }

    // logDebug() << "Showing file details about " << file << Qt::endl;

    showFilePkgInfo( file );
    showFileInfo( file );
    setCurrentPage( _ui->fileDetailsPage );
}


void FileDetailsView::showFileInfo( FileInfo * file )
{
    CHECK_PTR( file );

    const bool isSpecial = file->isSpecial();
    const bool isSymlink = file->isSymLink();

    setLabelLimited(_ui->fileNameLabel, file->baseName() );
    _ui->fileTypeLabel->setText( formatFilesystemObjectType( file ) );

    _ui->symlinkIcon->setVisible( file->isSymLink() );
    _ui->fileIcon->setVisible( file->isFile() );
    _ui->blockIcon->setVisible( file->isBlockDevice() );
    _ui->charIcon->setVisible( file->isCharDevice() );
    _ui->specialIcon->setVisible( file->isFifo() || file->isSocket() );

    _ui->fileMimeCaption->setVisible( !isSymlink );
    _ui->fileMimeLabel->setVisible( !isSymlink );
    _ui->fileLinkCaption->setVisible( isSymlink );
    _ui->fileLinkLabel->setVisible( isSymlink );

    if ( isSymlink )
    {
	const QString fullTarget  = file->symLinkTarget();
	QString shortTarget = fullTarget;
	if ( fullTarget.length() >= MAX_SYMLINK_TARGET_LEN && fullTarget.contains( '/' ) )
	    shortTarget = ".../" + SysUtil::baseName( fullTarget );
	_ui->fileLinkLabel->setText( shortTarget );

	if ( file->isBrokenSymLink() )
	{
	    _ui->fileLinkLabel->setStyleSheet( errorStyleSheet() );
	    _ui->fileLinkLabel->setToolTip( fullTarget + tr( " (broken)" ) );
	}
	else if ( shortTarget != fullTarget )
	{
	    _ui->fileLinkLabel->setStyleSheet( QString() );
	    _ui->fileLinkLabel->setToolTip( fullTarget );
	}
	else
	{
	    _ui->fileLinkLabel->setToolTip( QString() );
	}
    }
    else if ( isSpecial )
    {
	_ui->fileMimeCaption->setEnabled( false );
	_ui->fileMimeLabel->setEnabled( false );
	_ui->fileMimeLabel->clear();
	_ui->fileSizeLabel->clear();
	_ui->fileAllocatedLabel->clear();
    }
    else // ! isSymLink
    {
	setMimeCategory( file );
    }

    _ui->fileSizeCaption->setEnabled( !isSpecial );
    _ui->fileAllocatedCaption->setEnabled( !isSpecial );
    if ( !isSpecial )
    {
	_ui->fileSizeLabel->setSize( file );
	_ui->fileAllocatedLabel->setAllocated( file );
    }

    _ui->fileUserLabel->setText( file->userName() );
    _ui->fileGroupLabel->setText( file->groupName() );
    _ui->filePermissionsLabel->setText( formatPermissions( file->mode() ) );
    _ui->fileMTimeLabel->setText( formatTime( file->mtime() ) );

//    if ( !file->isSparseFile() )
//	_ui->fileSizeLabel->suppressIfSameContent( _ui->fileAllocatedLabel, _ui->fileAllocatedCaption );
}


QString FileDetailsView::formatFilesystemObjectType( const FileInfo * file )
{
    if      ( file->isFile() )		return tr( "file"             );
    else if ( file->isSymLink() )	return tr( "symbolic link"    );
    else if ( file->isBlockDevice() )	return tr( "block device"     );
    else if ( file->isCharDevice() )	return tr( "character device" );
    else if ( file->isFifo() )		return tr( "named pipe"       );
    else if ( file->isSocket() )	return tr( "socket"           );

    logWarning() << " unexpected mode: " << file->mode() << Qt::endl;
    return QString();
}


void FileDetailsView::showFilePkgInfo( const FileInfo * file )
{
    CHECK_PTR( file );

    // If this is in a package view, then we know it is a packaged file
    const PkgInfo * pkg = file->pkgInfoParent();

    // Packaged files are always system files
    const bool isSystemFile = pkg || SystemFileChecker::isSystemFile( file );
    setSystemFileWarningVisibility( isSystemFile );

    if ( PkgQuery::foundSupportedPkgManager() )
    {
	setFilePkgBlockVisibility( isSystemFile );

	if ( pkg )
	{
	    // We already know the package ...
	    _ui->filePackageCaption->setEnabled( true );
	    _ui->filePackageLabel->setText( pkg->name() );
	}
	else if ( isSystemFile )
	{
	    // Submit a timed query to find the owning package, if any
	    const QString delayHint = QString( _pkgUpdateTimer->delayStage(), '.' ).replace( ".", ". " );
	    _ui->filePackageLabel->setText( delayHint );

	    // Caspture url by value because the FileInfo may be gone by the time the timer expires
	    const QString url = file->url();
	    _pkgUpdateTimer->request( [ this, url ]() { updatePkgInfo( url ); } );

	    // Leave the caption unchanged for now as the most likely state is the same as the previous selection
	}
    }
    else // No supported package manager found
    {
	setFilePkgBlockVisibility( false );
    }
}


void FileDetailsView::updatePkgInfo( const QString & path )
{
    // logDebug() << "Updating pkg info for " << path << Qt::endl;

    const QString pkg = PkgQuery::owningPkg( path );
    _ui->filePackageLabel->setText( pkg );
    _ui->filePackageCaption->setEnabled( !pkg.isEmpty() );
}


void FileDetailsView::setSystemFileWarningVisibility( bool visible )
{
    _ui->fileSystemFileWarning->setVisible( visible );
    _ui->fileSystemFileWarningSpacer->setVisible( visible );
}

void FileDetailsView::setFilePkgBlockVisibility( bool visible )
{
    _ui->filePackageCaption->setVisible( visible );
    _ui->filePackageLabel->setVisible( visible );
}


void FileDetailsView::showDetails( DirInfo * dir )
{
    // logDebug() << "Showing dir details about " << dir << Qt::endl;

    if ( !dir )
    {
	clear();
	return;
    }

    const QString name = dir->isPseudoDir() ? dir->name() : ( dir->baseName() + "/" );
    setLabelLimited(_ui->dirNameLabel, name );

    const bool isMountPoint = dir->isMountPoint() && !dir->readError();
    _ui->dirUnreadableIcon->setVisible( dir->readError() );
    _ui->mountPointIcon->setVisible( isMountPoint );
    _ui->dotEntryIcon->setVisible( dir->isDotEntry() && !dir->readError() );
    _ui->dirIcon->setVisible( !dir->isMountPoint() && !dir->isDotEntry() && !dir->readError() );

    _ui->dirTypeLabel->setText( dir->isMountPoint() ? tr( "mount point" ) :
                                dir->isPseudoDir() ?  tr( "pseudo directory" ) :
		                                      tr( "directory" ) );
    _ui->dirTypeLabel->setStyleSheet( dir->isPseudoDir() ? QString() : "QToolTip { max-width: 0px }" );

//    _ui->dirLockedIcon->setVisible   ( dir->readError() );
    _ui->dirFromCacheIcon->setVisible( dir->isFromCache() );
    _ui->dirDuplicateIcon->setVisible( isMountPoint && MountPoints::isDuplicate( dir->url() ) );

    // Subtree information
    setDirBlockVisibility( !dir->isPseudoDir() );
    showSubtreeInfo( dir );
    if ( !dir->isPseudoDir() )
	showDirNodeInfo( dir );

    // Set the row visibilities before showing the page to avoid briefly showing the ...
    // ... wrong rows, then hiding them and shuffling the mtime about
    setCurrentPage( _ui->dirDetailsPage );
}


QString FileDetailsView::readStateMsg( int readState )
{
    switch ( readState )
    {
	case DirQueued:
	case DirReading:		return tr( "[reading]" );
	case DirPermissionDenied:	return tr( "[permission denied]" );
	case DirError:			return tr( "[read error]" );
	case DirOnRequestOnly:		return tr( "[not read]" );
	case DirAborted:		return tr( "[aborted]" );
//	case DirFinished:
//	case DirCached:
//	default:
    }

    return QString();
}


void FileDetailsView::showSubtreeInfo( DirInfo * dir )
{
    CHECK_PTR( dir );

    const QString msg = subtreeMsg( dir );
    if ( msg.isEmpty() )
    {
	// No special msg -> show summary fields

	const QString prefix = dir->sizePrefix();
	setLabel( _ui->dirTotalSizeLabel,   dir->totalSize(),          prefix );
	setLabel( _ui->dirAllocatedLabel,   dir->totalAllocatedSize(), prefix );
	setLabel( _ui->dirItemCountLabel,   dir->totalItems(),         prefix );
	setLabel( _ui->dirFileCountLabel,   dir->totalFiles(),         prefix );
	setLabel( _ui->dirSubDirCountLabel, dir->totalSubDirs(),       prefix );
	_ui->dirLatestMTimeLabel->setText( formatTime( dir->latestMtime() ) );

//	_ui->dirTotalSizeLabel->suppressIfSameContent( _ui->dirAllocatedLabel, _ui->dirAllocatedCaption );
	_ui->dirAllocatedLabel->setBold( dir->totalUsedPercent() < ALLOCATED_FAT_PERCENT );
    }
    else  // Special msg -> show it and clear all summary fields
    {
	_ui->dirTotalSizeLabel->setText( msg );
	_ui->dirAllocatedLabel->clear();
	_ui->dirItemCountLabel->clear();
	_ui->dirFileCountLabel->clear();
	_ui->dirSubDirCountLabel->clear();
	_ui->dirLatestMTimeLabel->clear();
    }
}


QString FileDetailsView::errorStyleSheet() const
{
    return QString( "QLabel { color:" + app()->dirTreeModel()->dirReadErrColor().name() + " }" );
}


QString FileDetailsView::dirColorStyle( const DirInfo * dir ) const
{
    return dir->readState() == DirPermissionDenied ? errorStyleSheet() : "";
}


void FileDetailsView::showDirNodeInfo( const DirInfo * dir )
{
    CHECK_PTR( dir );

    _ui->dirOwnSizeCaption->setVisible( dir->size() > 0 );
    _ui->dirOwnSizeLabel->setVisible  ( dir->size() > 0 );
    setLabel( _ui->dirOwnSizeLabel, dir->size() );

    _ui->dirUserLabel->setText( dir->userName() );
    _ui->dirGroupLabel->setText( dir->groupName() );
    _ui->dirPermissionsLabel->setText( formatPermissions( dir->mode() ) );

    _ui->dirMTimeCaption->setVisible( dir->mtime() > 0 );
    _ui->dirMTimeLabel->setVisible( dir->mtime() > 0);
    _ui->dirMTimeLabel->setText( formatTime( dir->mtime() ) );

    // Show permissions in red if there was a "permission denied" error while reading this directory
    // Using (and removing) a stylesheet better respects theme changes
    _ui->dirPermissionsLabel->setStyleSheet( dirColorStyle( dir ) );
}


void FileDetailsView::setDirBlockVisibility( bool visible )
{
    _ui->dirDirectoryHeading->setVisible( visible );

    _ui->dirOwnSizeCaption->setVisible( visible );
    _ui->dirUserCaption->setVisible( visible );
    _ui->dirGroupCaption->setVisible( visible );
    _ui->dirPermissionsCaption->setVisible( visible );
    _ui->dirMTimeCaption->setVisible( visible );

    _ui->dirOwnSizeLabel->setVisible( visible );
    _ui->dirUserLabel->setVisible( visible );
    _ui->dirGroupLabel->setVisible( visible );
    _ui->dirPermissionsLabel->setVisible( visible );
    _ui->dirMTimeLabel->setVisible( visible );

    // A dot entry cannot have directory children
    _ui->dirSubDirCountCaption->setVisible( visible );
    _ui->dirSubDirCountLabel->setVisible( visible );
}


void FileDetailsView::showDetails( PkgInfo * pkg )
{
    // logDebug() << "Showing pkg details about " << pkg << Qt::endl;

    if ( !pkg )
    {
	clear();
	return;
    }

    if ( pkg->url() == PkgInfo::pkgSummaryUrl() )
    {
	showPkgSummary( pkg );
	return;
    }

    setLabelLimited( _ui->pkgNameLabel, pkg->name() );
    _ui->pkgVersionLabel->setText( pkg->version() );
    _ui->pkgArchLabel->setText( pkg->arch() );

    const QString msg = subtreeMsg( pkg );
    if ( msg.isEmpty() )
    {
	// No special msg -> show summary fields
	const QString prefix = pkg->sizePrefix();
	setLabel( _ui->pkgTotalSizeLabel,   pkg->totalSize(),          prefix );
	setLabel( _ui->pkgAllocatedLabel,   pkg->totalAllocatedSize(), prefix );
	setLabel( _ui->pkgItemCountLabel,   pkg->totalItems(),         prefix );
	setLabel( _ui->pkgFileCountLabel,   pkg->totalFiles(),         prefix );
	setLabel( _ui->pkgSubDirCountLabel, pkg->totalSubDirs(),       prefix );

//	_ui->pkgTotalSizeLabel->suppressIfSameContent( _ui->pkgAllocatedLabel, _ui->pkgAllocatedCaption );
    }
    else
    {
	// Special msg -> show it and clear all summary fields
	_ui->pkgTotalSizeLabel->setText( msg );
	_ui->pkgAllocatedLabel->clear();
	_ui->pkgItemCountLabel->clear();
	_ui->pkgFileCountLabel->clear();
	_ui->pkgSubDirCountLabel->clear();
    }

    _ui->pkgLatestMTimeLabel->setText( formatTime( pkg->latestMtime() ) );

    setCurrentPage( _ui->pkgDetailsPage );
}


void FileDetailsView::showPkgSummary( PkgInfo * pkg )
{
    // logDebug() << "Showing pkg details about " << pkg << Qt::endl;

    if ( !pkg )
    {
	clear();
	return;
    }

    setLabel( _ui->pkgSummaryPkgCountLabel, pkg->directChildrenCount() );

    const QString msg = subtreeMsg( pkg );
    if ( msg.isEmpty() )
    {
	const QString prefix = pkg->sizePrefix();
	setLabel( _ui->pkgSummaryTotalSizeLabel,   pkg->totalSize(),          prefix );
	setLabel( _ui->pkgSummaryAllocatedLabel,   pkg->totalAllocatedSize(), prefix );
	setLabel( _ui->pkgSummaryItemCountLabel,   pkg->totalItems(),         prefix );
	setLabel( _ui->pkgSummaryFileCountLabel,   pkg->totalFiles(),         prefix );
	setLabel( _ui->pkgSummarySubDirCountLabel, pkg->totalSubDirs(),       prefix );

//	_ui->pkgSummaryTotalSizeLabel->suppressIfSameContent( _ui->pkgSummaryAllocatedLabel,
//							      _ui->pkgSummaryAllocatedCaption );
    }
    else
    {
	_ui->pkgSummaryTotalSizeLabel->setText( msg );
	_ui->pkgSummaryAllocatedLabel->clear();
	_ui->pkgSummaryItemCountLabel->clear();
	_ui->pkgSummaryFileCountLabel->clear();
	_ui->pkgSummarySubDirCountLabel->clear();
    }

    _ui->pkgSummaryLatestMTimeLabel->setText( formatTime( pkg->latestMtime() ) );

    setCurrentPage( _ui->pkgSummaryPage );
}


void FileDetailsView::showDetails( const FileInfoSet & selectedItems )
{
    // logDebug() << "Showing selection summary" << Qt::endl;

    int fileCount	 = 0;
    int dirCount	 = 0;
    int subtreeFileCount = 0;

    const FileInfoSet sel = selectedItems.normalized();
    for ( FileInfo * item : sel )
    {
	if ( item->isDirInfo() )
	{
	    ++dirCount;
	    subtreeFileCount += item->totalFiles();
	}
	else
	{
	    ++fileCount;
	}
    }

    _ui->selFileCountCaption->setEnabled( fileCount > 0 );
    _ui->selFileCountLabel->setEnabled( fileCount > 0 );

    _ui->selDirCountCaption->setEnabled( dirCount > 0 );
    _ui->selDirCountLabel->setEnabled( dirCount > 0 );

    _ui->selSubtreeFileCountCaption->setEnabled( subtreeFileCount > 0 );
    _ui->selSubtreeFileCountLabel->setEnabled( subtreeFileCount > 0 );

    const QString itemCount =
	sel.count() == 1 ? tr( "1 Selected Item" ) : tr( "%1 Selected Items" ).arg( sel.count() );
    _ui->selHeading->setText( itemCount );

    setLabel( _ui->selTotalSizeLabel,	     sel.totalSize()	      );
    setLabel( _ui->selAllocatedLabel,	     sel.totalAllocatedSize() );
    setLabel( _ui->selFileCountLabel,	     fileCount		      );
    setLabel( _ui->selDirCountLabel,	     dirCount		      );
    setLabel( _ui->selSubtreeFileCountLabel, subtreeFileCount	      );

//    _ui->selTotalSizeLabel->suppressIfSameContent( _ui->selAllocatedLabel, _ui->selAllocatedCaption );

    setCurrentPage( _ui->selectionSummaryPage );
}


void FileDetailsView::setLabelLimited( QLabel * label, const QString & text )
{
    CHECK_PTR( label );
    const QString limitedText = elideMiddle( text, _labelLimit );
    label->setText( limitedText );
}


void FileDetailsView::setMimeCategory( const FileInfo * fileInfo )
{
    const QString & categoryName = mimeCategory( fileInfo );
    _ui->fileMimeCaption->setEnabled( !categoryName.isEmpty() );
    _ui->fileMimeLabel->setEnabled( !categoryName.isEmpty() );
    _ui->fileMimeLabel->setText( categoryName );
}


void FileDetailsView::categoriesChanged()
{
    if ( currentWidget() != _ui->fileDetailsPage )
	return;

    const FileInfo * fileInfo = app()->currentItem();
    if ( !fileInfo || fileInfo->isSymLink() || fileInfo->baseName() != _ui->fileNameLabel->text() )
	return;

    setMimeCategory( fileInfo );
}
