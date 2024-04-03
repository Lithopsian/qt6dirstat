/*
 *   File name: CleanupCollection.cpp
 *   Summary:	QDirStat classes to reclaim disk space
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <QMenu>
#include <QToolBar>
#include <QMessageBox>

#include "CleanupCollection.h"
#include "StdCleanup.h"
#include "Cleanup.h"
#include "DirTree.h"
#include "FormatUtil.h"
#include "OutputWindow.h"
#include "Refresher.h"
#include "SelectionModel.h"
#include "Settings.h"
#include "SettingsHelpers.h"
#include "Trash.h"
#include "Logger.h"
#include "Exception.h"

#define MAX_URLS_IN_CONFIRMATION_POPUP 7

using namespace QDirStat;


static void removeAllFromWidget( QWidget * widget )
{
    if ( !widget )
	return;

    for ( QAction * action : widget->actions() )
    {
	Cleanup * cleanup = dynamic_cast<Cleanup *>( action );

	if ( cleanup )
	    widget->removeAction( cleanup );
    }
}



CleanupCollection::CleanupCollection( QObject	     * parent,
				      SelectionModel * selectionModel,
				      QToolBar	     * toolBar,
				      QMenu	     * menu ):
    QObject ( parent ),
    _selectionModel { selectionModel },
    _trash { new Trash() }
{
    CHECK_NEW( _trash );

    readSettings();

    // Add to the toolbar and menu, and keep these in sync with this object
    addToToolBar( toolBar );
    addToMenu( menu );

    // Initialize to show the current status in the log; the map will be cached
//    (void)Cleanup::desktopSpecificApps();

    // Available Cleanups depend on the currently-selected items
    connect( selectionModel, qOverload<>( &SelectionModel::selectionChanged ),
	     this,	     &CleanupCollection::updateActions );
}


CleanupCollection::~CleanupCollection()
{
//    writeSettings( _cleanupList );
    clear();
}


void CleanupCollection::add( Cleanup * cleanup )
{
    _cleanupList << cleanup;

    connect( cleanup, &Cleanup::triggered,
	     this,    &CleanupCollection::execute );

    updateMenusAndToolBars();
}

/*
void CleanupCollection::remove( Cleanup * cleanup )
{
    const int index = indexOf( cleanup );

    if ( index == -1 )
    {
	logError() << "No such cleanup: " << cleanup << Qt::endl;
	return;
    }

    _cleanupList.removeAt( index );
    delete cleanup;

    // No need for updateMenusAndToolBars() since QObject/QWidget will take care of
    // deleted actions all by itself.
}
*/

void CleanupCollection::addStdCleanups()
{
    for ( Cleanup * cleanup : StdCleanup::stdCleanups( this ) )
    {
	CHECK_PTR( cleanup );
	add( cleanup );
    }

    writeSettings( _cleanupList );

    CleanupSettings settings;
    settings.setValue( "StdCleanupsAdded", true );
}


int CleanupCollection::indexOf( Cleanup * cleanup ) const
{
    const int index = _cleanupList.indexOf( cleanup );
    if ( index == -1 )
	logError() << "Cleanup " << cleanup << " is not in this collection" << Qt::endl;

    return index;
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

    // No need for updateMenusAndToolBars() since QObject/QWidget will take
    // care of deleted actions all by itself.
}


void CleanupCollection::updateMenusAndToolBars()
{
    updateMenus();
    updateToolBars();
}


void CleanupCollection::updateActions()
{
    const FileInfoSet sel = _selectionModel->selectedItems();

    const bool empty		= sel.isEmpty();
    const bool dirSelected	= sel.containsDir();
    const bool fileSelected	= sel.containsFile();
    const bool pkgSelected      = sel.containsPkg();
    const bool dotEntrySelected = sel.containsDotEntry();
    const bool busy		= sel.containsBusyItem();
    const bool treeBusy		= sel.treeIsBusy();
    const bool canCleanup	= !pkgSelected && !busy && !empty;

    for ( Cleanup * cleanup : _cleanupList )
    {
	const bool enable = canCleanup && cleanup->isActive() &&
			    ( !treeBusy         || cleanup->refreshPolicy() == Cleanup::NoRefresh ) &&
			    ( !dirSelected      || cleanup->worksForDir() ) &&
			    ( !dotEntrySelected || cleanup->worksForDotEntry() ) &&
			    ( !fileSelected     || cleanup->worksForFile() );
	cleanup->setEnabled( enable );
    }
}


void CleanupCollection::updateMenus()
{
    _menus.removeAll( nullptr ); // Remove QPointers that have become invalid

    for ( QMenu * menu : _menus )
    {
	if ( menu )
	{
	    // Remove all Cleanups from this menu
	    removeAllFromWidget( menu );

	    // Add the current cleanups in the current order
	    addToMenu( menu );
	}
    }
}


void CleanupCollection::updateToolBars()
{
    _toolBars.removeAll( nullptr ); // Remove QPointers that have become invalid

    for ( QToolBar * toolBar : _toolBars )
    {
	if ( toolBar )
	{
	    // Remove all Cleanups from this tool bar
	    removeAllFromWidget( toolBar );

	    // Add the current cleanups in the current order
	    addToToolBar( toolBar );
	}
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

    if ( cleanup->askForConfirmation() && !confirmation( cleanup, selection ) )
    {
	logDebug() << "User declined confirmation" << Qt::endl;
	return;
    }

    emit startingCleanup( cleanup->cleanTitle() );

    OutputWindow * outputWindow = new OutputWindow( qApp->activeWindow(), cleanup->outputWindowAutoClose() );
    CHECK_NEW( outputWindow );

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

    if ( cleanup->refreshPolicy() == Cleanup::RefreshThis ||
	 cleanup->refreshPolicy() == Cleanup::RefreshParent )
    {
	createRefresher( outputWindow,
			 cleanup->refreshPolicy() == Cleanup::RefreshParent ? selection.parents() : selection );
    }

    connect( outputWindow, &OutputWindow::lastProcessFinished,
	     this,	   &CleanupCollection::cleanupFinished );


    // Process the raw FileInfoSet to eliminate duplicates.  For Cleanups
    // bot containing the %p or %n variables, create a de-duplicated list of
    // directories (including the parents of any file items or pseudo-
    // directories).  Note that the set is not normalised so that the Cleanup
    // can be performed on both an item and one of its ancestors.
    for ( FileInfo * item : cleanup->deDuplicateParents( selection ) )
    {
	if ( cleanup->worksFor( item ) )
	    cleanup->execute( item, outputWindow );
	else
	    logWarning() << "Cleanup " << cleanup << " does not work for " << item << Qt::endl;
    }

    if ( cleanup->refreshPolicy() == Cleanup::AssumeDeleted )
    {
	connect( outputWindow,	&OutputWindow::lastProcessFinished,
		 this,		&CleanupCollection::assumedDeleted );

        // It is important to use the normalized FileInfoSet here to avoid a
        // segfault because we are iterating over items whose ancestors we just
        // deleted (thus invalidating pointers to it). Normalizing removes
        // items from the set that also have any ancestors in the set.
        for ( FileInfo * item : selection.invalidRemoved().normalized() )
        {
            DirTree * tree = item->tree();
            if ( tree->isBusy() )
                logWarning() << "Ignoring AssumeDeleted: DirTree is being read" << Qt::endl;
            else
                tree->deleteSubtree( item );
        }
    }

    outputWindow->noMoreProcesses();
}


bool CleanupCollection::confirmation( Cleanup * cleanup, const FileInfoSet & items )
{
    // Pad the title to avoid tiny dialog boxes
    const QString title = pad( cleanup->cleanTitle(), 40 );

    QString msg = "<html>";
    if ( items.size() == 1 ) // The most common case
    {
	const FileInfo * item = items.first();
	const QString name = item->debugUrl().toHtmlEscaped();

	if ( item->isDir() || item->isPseudoDir() )
	    msg += tr( "<h3>%1</h3>for <b>directory</b> %2" ).arg( title ).arg( name );
	else
	    msg += tr( "<h3>%1</h3>for file %2" ).arg( title ).arg( name );

	msg += "<br>";
    }
    else // Multiple items selected
    {
	QStringList urls;
	if ( items.containsDir() && items.containsFile() )
	{
	    QStringList dirs	= filteredUrls( items, true, false );  // dirs
	    QStringList nonDirs = filteredUrls( items, false, true  ); // nonDirs

	    dirs    = dirs.mid	 ( 0, MAX_URLS_IN_CONFIRMATION_POPUP );
	    nonDirs = nonDirs.mid( 0, MAX_URLS_IN_CONFIRMATION_POPUP );

	    urls << dirs << "" << nonDirs;
	}
	else // !isMixed
	{
	    // Build a list of the first few selected items (7 max)
	    urls = filteredUrls( items, true, true ); // dirs + nonDirs
	    urls = urls.mid( 0, MAX_URLS_IN_CONFIRMATION_POPUP );
	}

	if ( urls.size() < items.size() ) // Only displaying part of the items?
	{
	    urls << "...";
	    urls << tr( "<i>(%1 items total)</i>" ).arg( items.size() );
	}

	msg += tr( "<h3>%1</h3> for:<br>\n%2<br>" ).arg( title ).arg( urls.join( "<br>" ) );
    }

    const int ret = QMessageBox::question( qApp->activeWindow(),
					   tr( "Please Confirm" ), // title
					   msg,		     // text
					   QMessageBox::Yes | QMessageBox::No );
    return ret == QMessageBox::Yes;
}


QStringList CleanupCollection::filteredUrls( const FileInfoSet & items,
					     bool		 dirs,
					     bool		 nonDirs ) const
{
    QStringList urls;

    for ( const auto item : items )
    {
	const QString name = item->debugUrl().toHtmlEscaped();

	if ( item->isDirInfo() && dirs )
	    urls << tr( "<b>directory</b> %1" ).arg( name );

	if ( !item->isDirInfo() && nonDirs )
	    urls << name;
    }

    return urls;
}


void CleanupCollection::addToMenu( QMenu * menu )
{
    CHECK_PTR( menu );

    addActive( menu );

    if ( !_menus.contains( menu ) )
	_menus << menu;
}


void CleanupCollection::addActive( QWidget * widget ) const
{
    CHECK_PTR( widget );

    for ( Cleanup * cleanup : _cleanupList )
    {
	if ( cleanup->isActive() )
	    widget->addAction( cleanup );
    }
}


void CleanupCollection::addEnabled( QWidget * widget ) const
{
    CHECK_PTR( widget );

    for ( Cleanup * cleanup : _cleanupList )
    {
	if ( cleanup->isEnabled() )
	    widget->addAction( cleanup );
    }
}


void CleanupCollection::addToToolBar( QToolBar * toolBar )
{
    CHECK_PTR( toolBar );

    for ( Cleanup * cleanup : _cleanupList )
    {
	// Add only cleanups that have an icon to avoid overcrowding the
	// toolbar with actions that only have a text.
	if ( cleanup->isActive() && !cleanup->icon().isNull() )
	    toolBar->addAction( cleanup );
    }

    if ( !_toolBars.contains( toolBar ) )
	_toolBars << toolBar;
}


void CleanupCollection::readSettings()
{
    clear();

    const SettingsEnumMapping refreshPolicyMapping      = Cleanup::refreshPolicyMapping();
    const SettingsEnumMapping outputWindowPolicyMapping = Cleanup::outputWindowPolicyMapping();

    CleanupSettings settings;

    // Read all settings groups [Cleanup_xx] that were found
    for ( const QString & groupName : settings.findGroups( settings.groupPrefix() ) )
    {
	// Read one cleanup
	settings.beginGroup( groupName );

	const QString title    = settings.value( "Title"	).toString();
	const QString command  = settings.value( "Command"	).toString();
	const QString iconName = settings.value( "Icon"		).toString();
	const QString hotkey   = settings.value( "Hotkey"	).toString();
	const QString shell    = settings.value( "Shell"	).toString();

	const bool active		 = settings.value( "Active"			, true	).toBool();
	const bool worksForDir		 = settings.value( "WorksForDir"		, true	).toBool();
	const bool worksForFile		 = settings.value( "WorksForFile"		, true	).toBool();
	const bool worksForDotEntry      = settings.value( "WorksForDotEntry"		, true	).toBool();
	const bool recurse		 = settings.value( "Recurse"			, false ).toBool();
	const bool askForConfirmation    = settings.value( "AskForConfirmation"		, false ).toBool();
	const bool outputWindowAutoClose = settings.value( "OutputWindowAutoClose"	, false ).toBool();
	const int  outputWindowTimeout   = settings.value( "OutputWindowTimeout"	, 0	).toInt();

	const int refreshPolicy	 = readEnumEntry( settings, "RefreshPolicy",
						  Cleanup::NoRefresh,
						  refreshPolicyMapping );

	const int outputWindowPolicy = readEnumEntry( settings, "OutputWindowPolicy",
						      Cleanup::ShowAfterTimeout,
						      outputWindowPolicyMapping );

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
	    CHECK_NEW( cleanup );
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
}


void CleanupCollection::writeSettings( const CleanupList & newCleanups)
{
    CleanupSettings settings;

    // Remove all leftover cleanup descriptions
    settings.removeGroups( settings.groupPrefix() );

    const SettingsEnumMapping refreshPolicyMapping = Cleanup::refreshPolicyMapping();
    const SettingsEnumMapping windowPolicyMapping  = Cleanup::outputWindowPolicyMapping();

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
	settings.beginGroup( "Cleanup", i+1 );

	const Cleanup * cleanup = newCleanups.at(i);
	if ( !cleanup || cleanup->command().isEmpty() || cleanup->title().isEmpty() )
	    continue;

	settings.setValue( "Command"		  , cleanup->command()		     );
	settings.setValue( "Title"		  , cleanup->title()		     );
	settings.setValue( "Active"		  , cleanup->isActive()		     );
	settings.setValue( "WorksForDir"	  , cleanup->worksForDir()	     );
	settings.setValue( "WorksForFile"	  , cleanup->worksForFile()	     );
	settings.setValue( "WorksForDotEntry"	  , cleanup->worksForDotEntry()	     );
	settings.setValue( "Recurse"		  , cleanup->recurse()		     );
	settings.setValue( "AskForConfirmation"	  , cleanup->askForConfirmation()    );
	settings.setValue( "OutputWindowAutoClose", cleanup->outputWindowAutoClose() );

	// Leave empty to use the OutputWindow default timeout
	if ( cleanup->outputWindowTimeout() > 0 )
	    settings.setValue( "OutputWindowTimeout", cleanup->outputWindowTimeout() );

	writeEnumEntry( settings, "RefreshPolicy",      cleanup->refreshPolicy(),      refreshPolicyMapping );
	writeEnumEntry( settings, "OutputWindowPolicy", cleanup->outputWindowPolicy(), windowPolicyMapping );

	if ( !cleanup->shell().isEmpty() )
	     settings.setValue( "Shell", cleanup->shell() );

	if ( !cleanup->iconName().isEmpty() )
	    settings.setValue( "Icon", cleanup->iconName() );

	if ( !cleanup->shortcut().isEmpty() )
	    settings.setValue( "Hotkey", cleanup->shortcut().toString() );

	settings.endGroup(); // [Cleanup_01], [Cleanup_02], ...
    }

    // Load the new settings into the real cleanup collection
    readSettings();
}


void CleanupCollection::moveToTrash()
{
    const FileInfoSet selectedItems = _selectionModel->selectedItems();

    // Prepare output window
    OutputWindow * outputWindow = new OutputWindow( qApp->activeWindow(), true );
    CHECK_NEW( outputWindow );

    // Prepare refresher
    createRefresher( outputWindow, selectedItems.parents() );

    // Window will never show for quick and successful trashes
    outputWindow->showAfterTimeout();

    // Move all selected items to trash
    for ( const FileInfo * item : selectedItems )
    {
	QCoreApplication::processEvents(); // give the output window a chance
	if ( _trash->trash( item->path() ) )
	    outputWindow->addStdout( tr( "Moved to trash: " ) + item->path() );
	else
	    outputWindow->addStderr( tr( "Move to trash failed for " ) + item->path() );
    }

    outputWindow->noMoreProcesses();
}


void CleanupCollection::createRefresher( OutputWindow * outputWindow, const FileInfoSet & refreshSet )
{
    _selectionModel->prepareRefresh( refreshSet );
    Refresher * refresher  = new Refresher( this, refreshSet );
    CHECK_NEW( refresher );

    connect( outputWindow, &OutputWindow::lastProcessFinished,
	     refresher,	   &Refresher::refresh );
}
