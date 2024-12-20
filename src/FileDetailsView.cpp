/*
 *   File name: FileDetailsView.h
 *   Summary:   Details view for the currently selected file or directory
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QtMath>

#include "FileDetailsView.h"
#include "AdaptiveTimer.h"
#include "DirInfo.h"
#include "DirTreeModel.h"
#include "FileInfo.h"
#include "FileInfoSet.h"
#include "FormatUtil.h"
#include "Logger.h"
#include "MimeCategorizer.h"
#include "MountPoints.h"
#include "PkgInfo.h"
#include "PkgQuery.h"
#include "QDirStatApp.h" // DirTreeModel, SelectionModel
#include "SelectionModel.h"
#include "SystemFileChecker.h"
#include "SysUtil.h"


#define MAX_SYMLINK_TARGET_LEN 25
#define ALLOCATED_FAT_PERCENT 33


using namespace QDirStat;


namespace
{
    /**
     * Return the last component of the path for 'item'.  In most
     * cases this will simply be the item name, but for the root item
     * is the full absolute pathname for 'item'.
     *
     * Note that for "/" (and any other path ending in "/"), this
     * function will return an empty string so that "/" can be
     * appended to any directory name for display, including root.
     **/
    QString baseName( const FileInfo * item )
    {
	return SysUtil::baseName( item->name() );
    }


    /**
     * Clear the visible text and tooltip from 'label'.
     **/
    void clearLabel( QLabel * label )
    {
	label->setToolTip( QString{} );
	label->clear();
    }


    /**
     * Format the mode (the permissions bits) returned from the stat() system
     * call in the commonly used formats, both symbolic and octal, e.g.
     *           drwxr-xr-x  0755
     **/
    QString formatPermissions( const FileInfo * item )
    {
	if ( !item->hasPerm() )
	    return QString{};

	return symbolicMode( item->mode() ) % "  "_L1 % formatOctal( ALLPERMS & item->mode() );
    }


    /**
     * Set the font bold property for 'label' to 'bold'.
     **/
    void setBold( QLabel * label, bool bold )
    {
	QFont textFont = label->font();
	textFont.setBold( bold );
	label->setFont( textFont );
    }


    /**
     * Set the tooltip for 'label' to a value.  The value will be formatted as the
     * exact number of bytes with the unit "bytes".  For values below 1000 bytes
     * (will appear as 1.0kB), no tooltip will be shown since the exact number of
     * bytes are already visible.  The tooltip may have a prefix (eg. ">") or it may
     * have hard links, but it should never have both.
     **/
    void setToolTip( QLabel * label, FileSize size, QLatin1String prefix, nlink_t numLinks )
    {
	if ( size < 1000 ) // not useful below (rounded) 1 kB
	{
	    label->setToolTip( QString{} );
	    return;
	}

	const QString tooltipText{ prefix % formatByteSize( size ) % formatLinksRichText( numLinks ) };
	label->setToolTip( whitespacePre( tooltipText ) );
    }


    /**
     * Set a label with 'text'.
     *
     * If 'lastPixel' is less than zero, the whole text is displayed and
     * the panel may have a horizontal scrollbar.  Otherwise, 'lastPixel'
     * gives the x-coordinate of the right-hand edge of the contents
     * portion of the details panel and 'text' is elided to fit in 'label'
     * without requiring a scrollbar.  Also, carriage returns and
     * linefeeds are replaced by spaces to prevent breaking the panel
     * layout.
     *
     * If the label is elided or contains a line-breaking character, then
     * a tooltip is set containing the original full text.
     **/
    void setLabelLimited( QLabel * label, QString text, int lastPixel )
    {
	const bool lineBreak = hasLineBreak( text );
	const QString cleanedText = replaceCrLf( text );
	if ( lastPixel < 0 )
	    label->setText( cleanedText );
	else
	    elideLabel( label, cleanedText, lastPixel );

	label->setToolTip( lineBreak || label->text() != text ? pathTooltip( text ) : QString{} );
    }


    /**
     * Set the text and tooltip for 'label'.  The label string is formatted in a
     * human-readable format, including the number of hard links (only when there
     * is more than one hard link).
     **/
    void setValueWithLinks( QLabel * label, FileSize size, nlink_t numLinks )
    {
	label->setText( formatSize( size ) % formatLinksInline( numLinks ) );
	setToolTip( label, size, QLatin1String{}, numLinks );
    }


    /**
     * Set the text for 'label' to a file size, including special handling for
     * sparse files and files with multiple hard links.
     **/
    void setSize( QLabel * label, const FileInfo * file )
    {
	setValueWithLinks( label, file->rawByteSize(), file->links() );
    }


    /**
     * Set the text for 'label' to an allocated size, including special handling
     * for sparse files and files with multiple hard links.
     *
     * Note that this is only useful for plain files, not for directories,
     * packages, or multiple selected files.
     **/
    void setAllocated( QLabel * label, const FileInfo * file )
    {
	const auto size = file->rawAllocatedSize();
	setValueWithLinks( label, size, file->links() );

	const bool bold =
	    file->isSparseFile() || ( size > 4096 && file->usedPercent() < ALLOCATED_FAT_PERCENT );
	setBold( label, bold );
    }


    /**
     * Set the text for 'label' to a formatted timestamp 'time'.
     **/
    void setTimeLabel( QLabel * label, time_t time )
    {
	label->setText( formatTime( time ) );
    }


    /**
     * Set the text for 'label' to a number with an optional prefix.
     **/
    void setCountLabel( QLabel * label, FileCount number, QLatin1String prefix = QLatin1String{} )
    {
	label->setText( prefix % QString{ "%L1" }.arg( number ) );
    }


    /**
     * Set the text and tooltip for 'label'. This will format the value and
     * display it in human-readable format, i.e. something like "123.4 MB".
     * Values such as zero or -1 will be formatted as an empty string.
     *
     * 'prefix' is an optional text prefix like "> " to indicate that the
     * exact value is unknown (e.g. because of insuficcient permissions in
     * a directory tree).
     *
     * If the value is more than 1024, the label is given a tooltip containing
     * the exact value in bytes.
     **/
    void setSizeLabel( QLabel * label, FileSize size, QLatin1String prefix = QLatin1String{} )
    {
	label->setText( prefix % formatSize( size ) );
	setToolTip( label, size, prefix, 0 );
    }


    /**
     * Return a message string describing the status of a DirInfo node.
     **/
    QString subtreeMsg( const DirInfo * dir )
    {
	return FileDetailsView::readStateMsg( dir->isBusy() ? DirReading : dir->readState() );
    }


    /**
     * The ratio of totalSize() / totalAllocatedSize() in percent for a directory.
     **/
    int totalUsedPercent( DirInfo * dir )
    {
	if ( dir->totalAllocatedSize() <= 0 || dir->totalSize() <= 0 )
	    return 100;

	return qRound( 100.0 * dir->totalSize() / dir->totalAllocatedSize() );
    }


    /**
     * Return a string describing the type of a FileInfo object.
     **/
    QString formatFileInfoType( const FileInfo * file )
    {
	if ( file->isFile()        ) return QObject::tr( "file"             );
	if ( file->isSymlink()     ) return QObject::tr( "symbolic link"    );
	if ( file->isBlockDevice() ) return QObject::tr( "block device"     );
	if ( file->isCharDevice()  ) return QObject::tr( "character device" );
	if ( file->isFifo()        ) return QObject::tr( "named pipe"       );
	if ( file->isSocket()      ) return QObject::tr( "socket"           );

	logWarning() << " unexpected mode: " << file->mode() << Qt::endl;

	return QString{};
    }


    /**
     * Return a string describing the type of a DirInfo object.
     **/
    QString formatDirInfoType( const DirInfo * dir )
    {
	if ( dir->readError()    ) return QObject::tr( "unknown"          );
	if ( dir->isMountPoint() ) return QObject::tr( "mount point"      );
	if ( dir->isPseudoDir()  ) return QObject::tr( "pseudo-directory" );
	return QObject::tr( "directory" );
    }


    /**
     * Return a string summarizing multiple selected items.
     **/
    QString formatSelectionSummary( int selectionCount )
    {
	if ( selectionCount == 1 ) return QObject::tr( "1 Selected Item" );

	return QObject::tr( "%L1 Selected Items" ).arg( selectionCount );
    }


    /**
     * Return a stylesheet string to set a label text to the configured
     * directory read error colour.
     **/
    QString dirColorStyle( const DirInfo * dir )
    {
	if ( dir->readState() == DirPermissionDenied )
	    return app()->dirTreeModel()->errorStyleSheet();

	return QString{};
    }


    /**
     * Set the owning package details for a file.  This happens
     * asynchronously, triggered by AdaptiveTimer, although the external
     * process itself runs synchronously after a variable delay.
     **/
    void updatePkgInfo( const Ui::FileDetailsView * ui, const QString & path, int lastPixel )
    {
	// logDebug() << "Updating pkg info for " << path << Qt::endl;

	const QString * pkg = PkgQuery::owningPkg( path );
	ui->filePackageCaption->setEnabled( !pkg->isEmpty() );
	setLabelLimited( ui->filePackageLabel, *pkg, lastPixel );
    }


    /**
     * Set the mime category field for a file.
     **/
    void setMimeCategory( const Ui::FileDetailsView * ui, const FileInfo * fileInfo )
    {
	const QString categoryName = MimeCategorizer::instance()->name( fileInfo );
	ui->fileMimeLabel->setText( categoryName );
    }


    /**
     * Show the visibility of the system file labels.
     **/
    void setSystemFileWarningVisibility( const Ui::FileDetailsView * ui, bool visible )
    {
	ui->fileSystemFileWarning->setVisible( visible );
	ui->fileSystemFileWarningSpacer->setVisible( visible );
    }


    /**
     * Show the visibility of the file package block of labels.
     **/
    void setFilePkgBlockVisibility( const Ui::FileDetailsView * ui, bool visible )
    {
	ui->filePackageCaption->setVisible( visible );
	ui->filePackageLabel->setVisible( visible );
    }


    /**
     * Show the visibility of the directory block of labels.
     **/
    void setDirBlockVisibility( const Ui::FileDetailsView * ui, bool visible )
    {
	ui->dirDirectoryHeading->setVisible( visible );

	ui->dirOwnSizeCaption->setVisible( visible );
	ui->dirUserCaption->setVisible( visible );
	ui->dirGroupCaption->setVisible( visible );
	ui->dirPermissionsCaption->setVisible( visible );
	ui->dirMTimeCaption->setVisible( visible );

	ui->dirOwnSizeLabel->setVisible( visible );
	ui->dirUserLabel->setVisible( visible );
	ui->dirGroupLabel->setVisible( visible );
	ui->dirPermissionsLabel->setVisible( visible );
	ui->dirMTimeLabel->setVisible( visible );

	// A dot entry cannot have directory children
	ui->dirSubDirCountCaption->setVisible( visible );
	ui->dirSubDirCountLabel->setVisible( visible );
    }


    /**
     * Show the directory section details for a DirInfo item.  The own row size
     * is completely removed for directories in package views since it is
     * somewhat meaningless and always zero.  When the uid, gid, and
     * permissions are marked as missing, usually from an old-version cache
     * read, the captions are disabled.  If there was an error accessing details
     * about 'dir', then the fields are left empty.
     **/
    void showDirNodeInfo( const Ui::FileDetailsView * ui, const DirInfo * dir )
    {
	if ( app()->isPkgView() )
	{
	    ui->dirOwnSizeCaption->hide();
	    ui->dirOwnSizeLabel->hide();
	}

	ui->dirUserCaption->setEnabled( dir->hasUid() );
	ui->dirGroupCaption->setEnabled( dir->hasGid() );
	ui->dirPermissionsCaption->setEnabled( dir->hasPerm() );

	if ( dir->readError() )
	{
	    clearLabel( ui->dirOwnSizeLabel );
	    clearLabel( ui->dirUserLabel );
	    clearLabel( ui->dirGroupLabel );
	    clearLabel( ui->dirPermissionsLabel );
	    clearLabel( ui->dirMTimeLabel );
	}
	else
	{
	    setSizeLabel( ui->dirOwnSizeLabel, dir->size() );
	    ui->dirUserLabel->setText( dir->userName() );
	    ui->dirGroupLabel->setText( dir->groupName() );
	    ui->dirPermissionsLabel->setText( formatPermissions( dir ) );
	    setTimeLabel( ui->dirMTimeLabel, dir->mtime() );
	}

	// Show permissions in "red" if there was a permission denied error reading this directory
	// Using (and removing) a stylesheet better respects theme changes
	ui->dirPermissionsLabel->setStyleSheet( dirColorStyle( dir ) );
    }


    /**
     * Show the subtree section details for a DirInfo item, size and count
     * totals for all the items below this directory.
     **/
    void showSubtreeInfo( const Ui::FileDetailsView * ui, DirInfo * dir )
    {
	const QString msg = subtreeMsg( dir );
	if ( msg.isEmpty() )
	{
	    // No special msg -> show summary fields
	    const QLatin1String prefix = dir->sizePrefix();
	    setSizeLabel ( ui->dirTotalSizeLabel,   dir->totalSize(),          prefix );
	    setSizeLabel ( ui->dirAllocatedLabel,   dir->totalAllocatedSize(), prefix );
	    setCountLabel( ui->dirItemCountLabel,   dir->totalItems(),         prefix );
	    setCountLabel( ui->dirFileCountLabel,   dir->totalFiles(),         prefix );
	    setCountLabel( ui->dirSubDirCountLabel, dir->totalSubDirs(),       prefix );
	    setTimeLabel ( ui->dirLatestMTimeLabel, dir->latestMTime() );

	    setBold( ui->dirAllocatedLabel, totalUsedPercent( dir ) < ALLOCATED_FAT_PERCENT );
	}
	else
	{
	    // Special msg -> show it and clear all summary fields
	    ui->dirTotalSizeLabel->setText( msg );
	    clearLabel( ui->dirAllocatedLabel );
	    clearLabel( ui->dirItemCountLabel );
	    clearLabel( ui->dirFileCountLabel );
	    clearLabel( ui->dirSubDirCountLabel );
	    clearLabel( ui->dirLatestMTimeLabel );
	}
    }


    /**
     * Show the file info section details for a FileInfo
     * item.
     **/
    void showFileInfo( const Ui::FileDetailsView * ui, const FileInfo * file, int lastPixel )
    {
	const bool isSpecial = file->isSpecial();
	const bool isSymlink = file->isSymlink();

	setLabelLimited(ui->fileNameLabel, baseName( file ), lastPixel );
	ui->fileTypeLabel->setText( formatFileInfoType( file ) );

	// Set an indicator icon for the type of file
	ui->symlinkIcon->setVisible( isSymlink );
	ui->fileIcon->setVisible( file->isFile() );
	ui->blockIcon->setVisible( file->isBlockDevice() );
	ui->charIcon->setVisible( file->isCharDevice() );
	ui->specialIcon->setVisible( file->isFifo() || file->isSocket() );

	// Mime category for regular files, or target for symlinks
	ui->fileMimeCaption->setVisible( !isSymlink );
	ui->fileMimeLabel->setVisible( !isSymlink );
	ui->fileLinkCaption->setVisible( isSymlink );
	ui->fileLinkLabel->setVisible( isSymlink );

	if ( isSymlink )
	{
	    // Shorten long targets that include a path component to the base name
	    QString fullTarget  = file->symlinkTarget();
	    const bool shorten = fullTarget.length() > MAX_SYMLINK_TARGET_LEN && fullTarget.contains( u'/' );
	    const QString shortTarget{ shorten ? "…/" % SysUtil::baseName( fullTarget ) : fullTarget };
	    setLabelLimited( ui->fileLinkLabel, shortTarget, lastPixel ); // don't set tooltip yet

	    if ( file->isBrokenSymlink() )
	    {
		ui->fileLinkLabel->setStyleSheet( app()->dirTreeModel()->errorStyleSheet() );
		ui->fileLinkLabel->setToolTip( QObject::tr( "Broken symlink:\n" ) % pathTooltip( fullTarget ) );
	    }
	    else
	    {
		ui->fileLinkLabel->setStyleSheet( QString{} );
		if ( shortTarget != fullTarget ) // setLabelLimited won't have detected this case
		    ui->fileLinkLabel->setToolTip( pathTooltip( fullTarget ) );
	    }
	}
	else if ( isSpecial )
	{
	    ui->fileMimeCaption->setEnabled( false );
	    clearLabel( ui->fileMimeLabel );
	    clearLabel( ui->fileSizeLabel );
	    clearLabel( ui->fileAllocatedLabel );
	}
	else // regular file
	{
	    ui->fileMimeCaption->setEnabled( true );
	    setMimeCategory( ui, file );
	}

	ui->fileSizeCaption->setEnabled( !isSpecial );
	ui->fileAllocatedCaption->setEnabled( !isSpecial );
	if ( !isSpecial )
	{
	    // Show size  for regular file or symlink
	    setSize( ui->fileSizeLabel, file );
	    setAllocated( ui->fileAllocatedLabel, file );
	}

	ui->fileUserCaption->setEnabled( file->hasUid() );
	ui->fileUserLabel->setText( file->userName() );
	ui->fileGroupCaption->setEnabled( file->hasGid() );
	ui->fileGroupLabel->setText( file->groupName() );
	ui->filePermissionsCaption->setEnabled( file->hasPerm() );
	ui->filePermissionsLabel->setText( formatPermissions( file ) );
	setTimeLabel( ui->fileMTimeLabel, file->mtime() );
    }


    /**
     * Show the package info section details for a FileInfo
     * item.
     **/
    void showFilePkgInfo( const Ui::FileDetailsView * ui,
                          AdaptiveTimer             * pkgUpdateTimer,
                          const FileInfo            * file,
                          int                         lastPixel )
    {
	// If this is in a package view, then we know it is a packaged file
	const PkgInfo * pkg = file->pkgInfoParent();

	// Packaged files are always system files
	const bool isSystemFile = pkg || SystemFileChecker::isSystemFile( file );
	setSystemFileWarningVisibility( ui, isSystemFile );

	if ( PkgQuery::foundSupportedPkgManager() )
	{
	    setFilePkgBlockVisibility( ui, isSystemFile );

	    if ( pkg )
	    {
		// We already know the package ...
		ui->filePackageCaption->setEnabled( true );
		ui->filePackageLabel->setText( pkg->name() );
	    }
	    else if ( isSystemFile )
	    {
		// Submit a timed query to find the owning package, if any
		QString delayHint{ pkgUpdateTimer->delayStage(), u'.' };
		ui->filePackageLabel->setText( delayHint.replace( u'.', ". "_L1 ) );

		// Capture url by value because the FileInfo may be gone by the time the timer expires
		const QString url = file->url();
		const auto payload = [ ui, url, lastPixel ]() { updatePkgInfo( ui, url, lastPixel ); };
		pkgUpdateTimer->request( payload );

		// Leave the caption unchanged for now as the most likely state is the same as the previous selection
	    }
	}
	else // No supported package manager found
	{
	    setFilePkgBlockVisibility( ui, false );
	}
    }


    /**
     * Show details about a directory.
     **/
    void showDirInfo( const Ui::FileDetailsView * ui, DirInfo * dir, int lastPixel )
    {
	// logDebug() << "Showing dir details about " << dir << Qt::endl;

	const bool isPseudoDir = dir->isPseudoDir();
	const QString name = isPseudoDir ? dir->name() : ( baseName( dir ) % '/' );
	setLabelLimited( ui->dirNameLabel, name, lastPixel );

	const bool readError = dir->subtreeReadError();
	const bool isMountPoint = dir->isMountPoint() && !readError;
	ui->dirUnreadableIcon->setVisible( readError );
	ui->mountPointIcon->setVisible( isMountPoint );
	ui->dotEntryIcon->setVisible( dir->isDotEntry() && !readError );
	ui->dirIcon->setVisible( !dir->isMountPoint() && !dir->isDotEntry() && !readError );

	ui->dirTypeLabel->setText( formatDirInfoType( dir ) );
	ui->dirTypeLabel->setStyleSheet( isPseudoDir ? QString{} : "QToolTip { max-width: 0px }" );

	ui->dirFromCacheIcon->setVisible( dir->isFromCache() );
	ui->dirDuplicateIcon->setVisible( isMountPoint && MountPoints::isDuplicate( dir->url() ) );

	showSubtreeInfo( ui, dir );

	const bool showDirBlock = !isPseudoDir && !dir->readError();
	setDirBlockVisibility( ui, showDirBlock );
	if ( showDirBlock )
	    showDirNodeInfo( ui, dir );
    }


    /**
     * Show details about a package.
     **/
    void showPkgInfo( const Ui::FileDetailsView * ui, PkgInfo * pkg, int lastPixel )
    {
	// logDebug() << "Showing pkg details about " << pkg << Qt::endl;

	setLabelLimited( ui->pkgNameLabel, pkg->name(), lastPixel );
	ui->pkgVersionLabel->setText( pkg->version() );
	ui->pkgArchLabel->setText( pkg->arch() );

	const QString msg = subtreeMsg( pkg );
	if ( msg.isEmpty() )
	{
	    // No special msg -> show summary fields
	    const QLatin1String prefix = pkg->sizePrefix();
	    setSizeLabel ( ui->pkgTotalSizeLabel,   pkg->totalSize(),          prefix );
	    setSizeLabel ( ui->pkgAllocatedLabel,   pkg->totalAllocatedSize(), prefix );
	    setCountLabel( ui->pkgItemCountLabel,   pkg->totalItems(),         prefix );
	    setCountLabel( ui->pkgFileCountLabel,   pkg->totalFiles(),         prefix );
	    setCountLabel( ui->pkgSubDirCountLabel, pkg->totalSubDirs(),       prefix );
	}
	else
	{
	    // Special msg -> show it and clear all summary fields
	    ui->pkgTotalSizeLabel->setText( msg );
	    clearLabel( ui->pkgAllocatedLabel );
	    clearLabel( ui->pkgItemCountLabel );
	    clearLabel( ui->pkgFileCountLabel );
	    clearLabel( ui->pkgSubDirCountLabel );
	}

	setTimeLabel( ui->pkgLatestMTimeLabel, pkg->latestMTime() );
    }


    /**
     * Show the packages summary (Pkg:/).
     **/
    void showPkgSummary( const Ui::FileDetailsView * ui, PkgInfo * pkg )
    {
	// logDebug() << "Showing pkg details about " << pkg << Qt::endl;

	setCountLabel( ui->pkgSummaryPkgCountLabel, pkg->childCount() );

	const QString msg = subtreeMsg( pkg );
	if ( msg.isEmpty() )
	{
	    const QLatin1String prefix = pkg->sizePrefix();
	    setSizeLabel ( ui->pkgSummaryTotalSizeLabel,   pkg->totalSize(),          prefix );
	    setSizeLabel ( ui->pkgSummaryAllocatedLabel,   pkg->totalAllocatedSize(), prefix );
	    setCountLabel( ui->pkgSummaryItemCountLabel,   pkg->totalItems(),         prefix );
	    setCountLabel( ui->pkgSummaryFileCountLabel,   pkg->totalFiles(),         prefix );
	    setCountLabel( ui->pkgSummarySubDirCountLabel, pkg->totalSubDirs(),       prefix );
	}
	else
	{
	    ui->pkgSummaryTotalSizeLabel->setText( msg );
	    clearLabel( ui->pkgSummaryAllocatedLabel );
	    clearLabel( ui->pkgSummaryItemCountLabel );
	    clearLabel( ui->pkgSummaryFileCountLabel );
	    clearLabel( ui->pkgSummarySubDirCountLabel );
	}

	setTimeLabel( ui->pkgSummaryLatestMTimeLabel, pkg->latestMTime() );
    }


    /**
     * Show details for multiple selected items.
     **/
    void showFileInfoSet( const Ui::FileDetailsView * ui, const FileInfoSet & sel )
    {
	FileCount fileCount        = 0;
	FileCount dirCount         = 0;
	FileCount subtreeFileCount = 0;

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

	ui->selFileCountCaption->setEnabled( fileCount > 0 );
	ui->selFileCountLabel->setEnabled( fileCount > 0 );

	ui->selDirCountCaption->setEnabled( dirCount > 0 );
	ui->selDirCountLabel->setEnabled( dirCount > 0 );

	ui->selSubtreeFileCountCaption->setEnabled( subtreeFileCount > 0 );
	ui->selSubtreeFileCountLabel->setEnabled( subtreeFileCount > 0 );

	ui->selHeading->setText( formatSelectionSummary( sel.count() ) );

	setSizeLabel ( ui->selTotalSizeLabel,        sel.totalSize()          );
	setSizeLabel ( ui->selAllocatedLabel,        sel.totalAllocatedSize() );
	setCountLabel( ui->selFileCountLabel,        fileCount                );
	setCountLabel( ui->selDirCountLabel,         dirCount                 );
	setCountLabel( ui->selSubtreeFileCountLabel, subtreeFileCount         );
    }

} // namespace


// The delay stages are constructed to rapidly move to stage 1, which is a short
// delay of half the time taken for the previous query to complete.  In practice,
// this delay will probably not be noticeable.  After that the delay increases only
// with fairly rapid repeated requests to a level which is likely to be visible, but
// will still allow most requests to complete after a moment.  The longest delays
// are only reached with very rapid repeated requests such as scrolling through a
// list of files and then quickly drop to a shorter delay when the repeated requests
// stop or slow down.
FileDetailsView::FileDetailsView( QWidget * parent ):
    QStackedWidget{ parent },
    _ui{ new Ui::FileDetailsView },
    _pkgUpdateTimer{ new AdaptiveTimer{ this,
                                        { 0.0f, 0.5f, 1.0f, 2.0f, 5.0f }, // delay stages, x payload time
                                        { 3000, 1000, 500, 250, 150 },  // cooldown stages, ms
                                      } }
{
    _ui->setupUi( this );

    clear();

    connect( MimeCategorizer::instance(), &MimeCategorizer::categoriesChanged,
             this,                        &FileDetailsView::categoriesChanged );
}


void FileDetailsView::showDetails()
{
    if ( !isVisible() )
	return;

    const FileInfoSet sel = app()->selectionModel()->selectedItems();
    if ( sel.count() > 1 )
    {
	// logDebug() << "Showing selection summary" << Qt::endl;
	showFileInfoSet( ui(), sel.normalized() );
	setCurrentPage( _ui->selectionSummaryPage );
    }
    else if ( !sel.isEmpty() )
	showDetails( sel.first() );
    else
	showDetails( app()->selectionModel()->currentItem() );
}


void FileDetailsView::showDetails( FileInfo * file )
{
    if ( !file )
    {
	clear();
    }
    else if ( file->isPkgInfo() )
    {
	PkgInfo * pkgInfo = file->toPkgInfo();
	if ( pkgInfo == app()->firstToplevel() )
	{
	    showPkgSummary( ui(), pkgInfo );
	    setCurrentPage( _ui->pkgSummaryPage );
	}
	else
	{
	    showPkgInfo( ui(), pkgInfo, _lastPixel );
	    setCurrentPage( _ui->pkgDetailsPage );
	}
    }
    else if ( file->isDirInfo() )
    {
	showDirInfo( ui(), file->toDirInfo(), _lastPixel );
	setCurrentPage( _ui->dirDetailsPage );
    }
    else
    {
	// logDebug() << "Showing file details about " << file << Qt::endl;
	showFilePkgInfo( ui(), _pkgUpdateTimer, file, _lastPixel );
	showFileInfo( ui(), file, _lastPixel );
	setCurrentPage( _ui->fileDetailsPage );
    }
}


QString FileDetailsView::readStateMsg( int readState )
{
    switch ( readState )
    {
	case DirQueued:
	case DirReading:       return tr( "[reading]" );
	case DirPermissionDenied:
	case DirNoAccess:      return tr( "[permission denied]" );
	case DirMissing:       return tr( "[missing]" );
	case DirError:         return tr( "[read error]" );
	case DirOnRequestOnly: return tr( "[not read]" );
	case DirAborted:       return tr( "[aborted]" );
//	case DirFinished:
//	case DirCached:
	default: break;
    }

    return QString{};
}


void FileDetailsView::categoriesChanged()
{
    // Categories are only shown on the file details page
    if ( currentWidget() != _ui->fileDetailsPage )
	return;

    // Only regular files show a category
    const FileInfo * fileInfo = app()->selectionModel()->currentItem();
    if ( !fileInfo || !fileInfo->isFile() )
	return;

    setMimeCategory( ui(), fileInfo );
}


void FileDetailsView::setCurrentPage( QWidget * page )
{
    // Simply hiding all other widgets is not enough: The QStackedLayout will
    // still reserve screen space for the largest widget. The other pages
    // need to be removed from the layout. They are still children of the
    // QStackedWidget, but no longer in the layout.
    while ( count() > 0 )
	removeWidget( widget( 0 ) );

    addWidget( page );
    setCurrentWidget( page );
}


void FileDetailsView::changeEvent( QEvent * event )
{
    const auto type = event->type();
    if ( type == QEvent::PaletteChange || type == QEvent::FontChange )
	showDetails();

    QStackedWidget::changeEvent( event );
}


void FileDetailsView::resizeEvent( QResizeEvent * )
{
    // Stay away if not in elide mode, triggering a scrollbar may cause an infinite loop
    if ( _lastPixel < 0 || !currentWidget() )
	return;

    // Recalculate the last pixel
    const QLayout * layout = currentWidget()->layout();
    if ( layout )
	_lastPixel = contentsRect().right() - layout->contentsMargins().right() - layout->spacing();

    // Grab any package name because showDetails() may blank it and wait for a process to update it
    const QString tooltipText = _ui->filePackageLabel->toolTip();
    const QString fullText = tooltipText.isEmpty() ? _ui->filePackageLabel->text() : tooltipText;

    // Refresh the whole panel and put the package name back before anyone notices
    showDetails();
    setLabelLimited( _ui->filePackageLabel, fullText, _lastPixel );
}

