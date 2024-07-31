/*
 *   File name: CleanupCollection.cpp
 *   Summary:   QDirStat classes to reclaim disk space
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QMenu>
#include <QMessageBox>
#include <QToolBar>

#include "CleanupCollection.h"
#include "Cleanup.h"
#include "DirTree.h"
#include "Exception.h"
#include "FileInfo.h"
#include "FormatUtil.h"
#include "OutputWindow.h"
#include "Refresher.h"
#include "SelectionModel.h"
#include "Settings.h"
#include "StdCleanup.h"
#include "Trash.h"


#define MAX_URLS_IN_CONFIRMATION_POPUP  9
#define MIN_DIALOG_WIDTH               40
#define MAX_SAFE_DIALOG_WIDTH          85


using namespace QDirStat;


namespace
{
    /**
     * Return the URLs for the selected item types in 'items':
     * directories (including dot entries) or files.
     **/
    QStringList filteredUrls( const FileInfoSet & items,
                              bool                dirs,
                              bool                files )
    {
	QStringList urls;

	for ( const FileInfo * item : items )
	{
	    const QString name = elideMiddle( item->debugUrl().toHtmlEscaped(), MAX_SAFE_DIALOG_WIDTH );

	    if ( ( item->isDirInfo() && dirs ) || ( !item->isDirInfo() && files ) )
	    {
		if ( urls.size() >= MAX_URLS_IN_CONFIRMATION_POPUP )
		{
		    urls << "...";
		    return urls;
		}

		urls << name;
	    }
	}

	return urls;
    }


    /**
     * Ask user for confirmation to execute a cleanup action for
     * 'items'. Returns 'true' if user accepts, 'false' otherwise.
     **/
    bool confirmation( Cleanup * cleanup, const FileInfoSet & items )
    {
	// Pad the title to avoid tiny dialog boxes and expand to the width of the longest line
	const QString msg = [ cleanup, &items ]()
	{
	    // QMessageBox wraps most text at a fixed width, but can be forced wider using <h3> and
	    // "white-space: pre", but only up to about 100 characters
	    // So elide ridiculously long names and add enough spaces to the title to avoid wrapping
	    // This is rich text and the title is styled <h3>, so don't try to be too precise and
	    // just rely on the final styled text being wider than we assumed.
	    const QFont font = QGuiApplication::font();
	    const int spaceWidth = textWidth( font, " " );
	    const QString cleanTitle = cleanup->cleanTitle();
	    const int titleWidth = textWidth( font, cleanTitle ) / spaceWidth;

	    if ( items.size() == 1 )
	    {
		const FileInfo * item = items.first();
		const QString name = elideMiddle( item->debugUrl().toHtmlEscaped(), MAX_SAFE_DIALOG_WIDTH );

		// Pad the title to avoid tiny dialog boxes
		const QString itemType =
		    item->isDirInfo() ? QObject::tr( "for directory" ) : QObject::tr( "for file" );
		const QString itemLine = itemType % ": "_L1 % name;
		const int itemSpaces = qMax( textWidth( font, itemLine ) / spaceWidth, MIN_DIALOG_WIDTH );
		const QString title = cleanTitle + QString( itemSpaces - titleWidth, u' ' );

		return QString( "<h3>%1</h3>%2<br/>" ).arg( title, itemLine );
	    }

	    const QStringList dirs  = filteredUrls( items, true, false ); // dirs first, if any
	    const QStringList files = filteredUrls( items, false, true ); // then files

	    QStringList urls;
	    if ( !dirs.isEmpty() )
		urls << QObject::tr( "<u>for directories:</u>" ) << dirs;

	    if ( !files.isEmpty() )
		urls << QObject::tr( "<u>for files:</u>" ) << files;

	    if ( dirs.size() > MAX_URLS_IN_CONFIRMATION_POPUP || files.size() > MAX_URLS_IN_CONFIRMATION_POPUP )
		urls << QObject::tr( "<i>(%1 items total)</i>" ).arg( items.size() );

	    int longestLine = MIN_DIALOG_WIDTH * spaceWidth;
	    for ( const QString & line : asConst( urls ) )
	    {
		const int lineLength = textWidth( font, line );
		if ( lineLength > longestLine )
		    longestLine = lineLength;
	    }
	    const int spaces = longestLine / spaceWidth - titleWidth;
	    const QString title = cleanTitle + QString( spaces, u' ' );

	    return QString( "<h3>%1</h3>%2<br>" ).arg( title, urls.join( "<br>"_L1 ) );
	}();

	const int ret = QMessageBox::question( qApp->activeWindow(),
	                                       QObject::tr( "Please Confirm" ),
	                                       whitespacePre( msg ),
	                                       QMessageBox::Yes | QMessageBox::No );

	return ret == QMessageBox::Yes;
    }


    /**
     * Return a mapping from RefreshPolicy to string.
     **/
    SettingsEnumMapping refreshPolicyMapping()
    {
	return { { Cleanup::NoRefresh,     "NoRefresh"_L1     },
	         { Cleanup::RefreshThis,   "RefreshThis"_L1   },
	         { Cleanup::RefreshParent, "RefreshParent"_L1 },
	         { Cleanup::AssumeDeleted, "AssumeDeleted"_L1 },
	       };
    }


    /**
     * Return a mapping from OutputWindowPolicy to string.
     **/
    SettingsEnumMapping outputWindowPolicyMapping()
    {
	return { { Cleanup::ShowAlways,        "ShowAlways"_L1        },
	         { Cleanup::ShowIfErrorOutput, "ShowIfErrorOutput"_L1 },
	         { Cleanup::ShowAfterTimeout,  "ShowAfterTimeout"_L1  },
	         { Cleanup::ShowNever,         "ShowNever"_L1         },
	       };
    }

} // namespace



CleanupCollection::CleanupCollection( QObject        * parent,
                                      SelectionModel * selectionModel,
                                      QToolBar       * toolBar,
                                      QMenu          * menu ):
    QObject { parent },
    _trash { new Trash() },
    _selectionModel { selectionModel },
    _toolBar { toolBar },
    _menu { menu }
{
    readSettings();
}


CleanupCollection::~CleanupCollection()
{
    clear();
}


void CleanupCollection::add( Cleanup * cleanup )
{
    _cleanupList << cleanup;

    connect( cleanup, &Cleanup::triggered,
             this,    &CleanupCollection::execute );
}


void CleanupCollection::addStdCleanups()
{
    const auto stdCleanups = StdCleanup::stdCleanups( this );
    for ( Cleanup * cleanup : stdCleanups )
	add( cleanup );

    writeSettings( _cleanupList );

    CleanupSettings settings;
    settings.setValue( "StdCleanupsAdded", true );
}


const Cleanup * CleanupCollection::at( int index ) const
{
    if ( index >= 0 && index < _cleanupList.size() )
	return _cleanupList.at( index );

    return nullptr;
}


void CleanupCollection::clear()
{
    qDeleteAll( _cleanupList );
    _cleanupList.clear();
}


void CleanupCollection::updateActions()
{
    const FileInfoSet sel = _selectionModel->selectedItems();

    const bool empty            = sel.isEmpty();
    const bool dirSelected      = sel.containsDir();
    const bool fileSelected     = sel.containsFile();
    const bool pkgSelected      = sel.containsPkg();
    const bool atticSelected    = sel.containsAttic();
    const bool dotEntrySelected = sel.containsDotEntry();
    const bool itemBusy         = sel.containsBusyItem();
    const bool treeBusy         = sel.treeIsBusy();
    const bool canCleanup       = !isBusy() && !atticSelected && !pkgSelected && !itemBusy && !empty;
    const bool pkgView = empty ? false : [ &sel ]()
    {
	const FileInfo * firstToplevel = sel.first()->tree()->firstToplevel();
	return firstToplevel && firstToplevel->isPkgInfo();
    }();

    for ( Cleanup * cleanup : asConst( _cleanupList ) )
    {
	const bool enable = canCleanup && cleanup->isActive() &&
	                    ( !treeBusy         || cleanup->refreshPolicy() == Cleanup::NoRefresh ) &&
	                    ( !pkgView          || !cleanup->requiresRefresh() ) &&
	                    ( !dirSelected      || cleanup->worksForDir() ) &&
	                    ( !dotEntrySelected || cleanup->worksForDotEntry() ) &&
	                    ( !fileSelected     || cleanup->worksForFile() );
	cleanup->setEnabled( enable );
    }
}


void CleanupCollection::execute()
{
    Cleanup * cleanup = qobject_cast<Cleanup *>( sender() );
    if ( !cleanup )
    {
	logError() << "Wrong sender type: " << sender()->metaObject()->className() << Qt::endl;
	return;
    }

    const FileInfoSet selection = _selectionModel->selectedItems();
    if ( selection.isEmpty() )
    {
	logWarning() << "Nothing selected" << Qt::endl;
	return;
    }

    // Remember the active window because it can lose focus if there is a confirmation dialog
    QWidget * activeWindow = qApp->activeWindow();

    if ( cleanup->askForConfirmation() && !confirmation( cleanup, selection ) )
    {
	//logDebug() << "User declined confirmation" << Qt::endl;
	return;
    }

    OutputWindow * outputWindow = new OutputWindow( activeWindow, cleanup->outputWindowAutoClose() );
    if ( cleanup->refreshPolicy() != Cleanup::NoRefresh )
	_activeOutputWindow = outputWindow;
    emit startingCleanup( cleanup->cleanTitle() );

    switch ( cleanup->outputWindowPolicy() )
    {
	case Cleanup::ShowAlways:
	    outputWindow->show();
	    break;

	case Cleanup::ShowAfterTimeout:
	    outputWindow->showAfterTimeout( cleanup->outputWindowTimeout() );
	    break;

	case Cleanup::ShowIfErrorOutput:
	    outputWindow->setShowOnStderr( true );
	    break;

	case Cleanup::ShowNever:
	    break;
    }

    if ( cleanup->refreshPolicy() == Cleanup::RefreshThis )
	createRefresher( outputWindow, selection );
    else if ( cleanup->refreshPolicy() == Cleanup::RefreshParent )
	createRefresher( outputWindow, selection.parents() );

    connect( outputWindow, &OutputWindow::lastProcessFinished,
             this,         &CleanupCollection::lastProcessFinished );

    // Process the raw FileInfoSet to eliminate duplicates.  For Cleanups
    // not containing the %p or %n variables, create a de-duplicated list of
    // directories (including the parents of any file items or pseudo-
    // directories).  Note that the set is not normalised so that the Cleanup
    // can be performed on both an item and one of its ancestors.
    const auto parents = cleanup->deDuplicateParents( selection );
    for ( FileInfo * item : parents )
	cleanup->execute( item, outputWindow );

    if ( cleanup->refreshPolicy() == Cleanup::AssumeDeleted )
    {
        // Use a normalized FileInfoSet to avoid trying to delete an
	// item whose ancestor is, or is going to be, deleted
	const FileInfoSet normalized = selection.normalized();
	normalized.first()->tree()->deleteSubtrees( normalized );
	emit assumedDeleted();
    }

    outputWindow->noMoreProcesses();
}


void CleanupCollection::lastProcessFinished( int totalErrorCount )
{
    _activeOutputWindow = nullptr;
    emit cleanupFinished( totalErrorCount );
}


void CleanupCollection::addActive( QWidget * widget ) const
{
    CHECK_PTR( widget );

    for ( Cleanup * cleanup : asConst( _cleanupList ) )
    {
	if ( cleanup->isActive() )
	    widget->addAction( cleanup );
    }
}


void CleanupCollection::addEnabled( QWidget * widget ) const
{
    CHECK_PTR( widget );

    for ( Cleanup * cleanup : asConst( _cleanupList ) )
    {
	if ( cleanup->isEnabled() )
	    widget->addAction( cleanup );
    }
}


void CleanupCollection::addToToolBar( QToolBar * toolBar )
{
    CHECK_PTR( toolBar );

    for ( Cleanup * cleanup : asConst( _cleanupList ) )
    {
	// Add only cleanups that have an icon to avoid overcrowding the
	// toolbar with actions that only have a text.
	if ( cleanup->isActive() && !cleanup->icon().isNull() )
	    toolBar->addAction( cleanup );
    }
}


void CleanupCollection::readSettings()
{
    clear();

    const SettingsEnumMapping refreshMapping      = refreshPolicyMapping();
    const SettingsEnumMapping outputWindowMapping = outputWindowPolicyMapping();

    CleanupSettings settings;

    // Read all settings groups [Cleanup_xx] that were found
    const auto groups = settings.findListGroups();
    for ( const QString & groupName : groups )
    {
	// Read one cleanup
	settings.beginGroup( groupName );

	const QString title    = settings.value( "Title"   ).toString();
	const QString command  = settings.value( "Command" ).toString();
	const QString iconName = settings.value( "Icon"    ).toString();
	const QString hotkey   = settings.value( "Hotkey"  ).toString();
	const QString shell    = settings.value( "Shell"   ).toString();

	const bool active                = settings.value( "Active",                true  ).toBool();
	const bool worksForDir           = settings.value( "WorksForDir",           true  ).toBool();
	const bool worksForFile          = settings.value( "WorksForFile",          true  ).toBool();
	const bool worksForDotEntry      = settings.value( "WorksForDotEntry",      true  ).toBool();
	const bool recurse               = settings.value( "Recurse",               false ).toBool();
	const bool askForConfirmation    = settings.value( "AskForConfirmation",    false ).toBool();
	const bool outputWindowAutoClose = settings.value( "OutputWindowAutoClose", false ).toBool();
	const int  outputWindowTimeout   = settings.value( "OutputWindowTimeout",   0     ).toInt();

	const int refreshPolicy = settings.enumValue( "RefreshPolicy",
	                                              Cleanup::NoRefresh,
	                                              refreshMapping );

	const int outputWindowPolicy = settings.enumValue( "OutputWindowPolicy",
	                                                   Cleanup::ShowAfterTimeout,
	                                                   outputWindowMapping );

	if ( command.isEmpty() || title.isEmpty() )
	{
	    logError() << "Need at least Command and Title for a cleanup" << Qt::endl;
	}
	else
	{
	    Cleanup * cleanup = new Cleanup( this, active, title, command,
	                                     recurse, askForConfirmation,
	                                     static_cast<Cleanup::RefreshPolicy>( refreshPolicy ),
	                                     worksForDir, worksForFile, worksForDotEntry,
	                                     static_cast<Cleanup::OutputWindowPolicy>( outputWindowPolicy ),
	                                     outputWindowTimeout, outputWindowAutoClose,
	                                     shell );
	    add( cleanup );

	    if ( !iconName.isEmpty() )
		cleanup->setIcon( iconName );

	    if ( !hotkey.isEmpty() )
		cleanup->setShortcut( hotkey );
	}

	settings.endGroup(); // [Cleanup_01], [Cleanup_02], ...
    }

    if ( _cleanupList.isEmpty() && !settings.value( "StdCleanupsAdded", false ).toBool() )
        addStdCleanups();

    updateActions();

    // All Cleanups will be gone (or not yet created) from the toolbar and menu ...
    // ... so add them back
    addToToolBar( _toolBar );
    addActive( _menu );
}


void CleanupCollection::writeSettings( const CleanupList & newCleanups)
{
    CleanupSettings settings;

    // Remove all leftover cleanup descriptions
    settings.removeListGroups();

    const SettingsEnumMapping refreshMapping = refreshPolicyMapping();
    const SettingsEnumMapping outputWindowMapping  = outputWindowPolicyMapping();

    // Using a separate group for each cleanup for better readability in the
    // file.
    //
    // Settings arrays are hard to read and to edit if there are more than,
    // say, 2-3 entries for each array index. Plus, a user editing the file
    // would have to take care of the array count - which is very error prone.
    //
    // We are using [Cleanup_01], [Cleanup_02], ... here just because that's
    // easiest to generate automatically; upon reading, the numbers are
    // irrelevant. It's just important that each group name is
    // unique. readSettings() will happily pick up any group that starts with
    // "Cleanup_".

    for ( int i=0; i < newCleanups.size(); ++i )
    {
	settings.beginListGroup( i+1 );

	const Cleanup * cleanup = newCleanups.at(i);
	if ( !cleanup || cleanup->command().isEmpty() || cleanup->title().isEmpty() )
	    continue;

	settings.setValue( "Command",               cleanup->command()               );
	settings.setValue( "Title",                 cleanup->title()                 );
	settings.setValue( "Active",                cleanup->isActive()              );
	settings.setValue( "WorksForDir",           cleanup->worksForDir()           );
	settings.setValue( "WorksForFile",          cleanup->worksForFile()          );
	settings.setValue( "WorksForDotEntry",      cleanup->worksForDotEntry()      );
	settings.setValue( "Recurse",               cleanup->recurse()               );
	settings.setValue( "AskForConfirmation",    cleanup->askForConfirmation()    );
	settings.setValue( "OutputWindowAutoClose", cleanup->outputWindowAutoClose() );

	// Leave empty to use the OutputWindow default timeout
	if ( cleanup->outputWindowTimeout() > 0 )
	    settings.setValue( "OutputWindowTimeout", cleanup->outputWindowTimeout() );

	settings.setEnumValue( "RefreshPolicy",      cleanup->refreshPolicy(),      refreshMapping );
	settings.setEnumValue( "OutputWindowPolicy", cleanup->outputWindowPolicy(), outputWindowMapping );

	if ( !cleanup->shell().isEmpty() )
	     settings.setValue( "Shell", cleanup->shell() );

	if ( !cleanup->iconName().isEmpty() )
	    settings.setValue( "Icon", cleanup->iconName() );

	if ( !cleanup->shortcut().isEmpty() )
	    settings.setValue( "Hotkey", cleanup->shortcut().toString() );

	settings.endListGroup(); // [Cleanup_01], [Cleanup_02], ...
    }

    // Load the new settings into the real cleanup collection
    readSettings();
}


void CleanupCollection::moveToTrash()
{
    const FileInfoSet selectedItems = _selectionModel->selectedItems();

    // Prepare output window
    OutputWindow * outputWindow = new OutputWindow( qApp->activeWindow(), true );

    // Prepare refresher
    createRefresher( outputWindow, selectedItems.parents() );

    // Don't show window for quick and successful trashes
    outputWindow->showAfterTimeout();

    // Move all selected items to trash
    for ( const FileInfo * item : selectedItems )
    {
	if ( _trash->trash( item->path() ) )
	    outputWindow->addStdout( tr( "Moved to trash: " ) + item->path() );
	else
	    outputWindow->addStderr( tr( "Move to trash failed for " ) + item->path() );

	// Give the output window a chance to display before this loop completes
	QEventLoop eventLoop;
	eventLoop.processEvents( QEventLoop::ExcludeUserInputEvents );
    }

    outputWindow->noMoreProcesses();
}


void CleanupCollection::createRefresher( OutputWindow * outputWindow, const FileInfoSet & refreshSet )
{
    _selectionModel->prepareForRefresh( refreshSet );
    Refresher * refresher = new Refresher( this, refreshSet );

    connect( outputWindow, &OutputWindow::lastProcessFinished,
             refresher,    &Refresher::refresh );
}
