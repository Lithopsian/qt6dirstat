/*
 *   File name: ActionManager.h
 *   Summary:   Common access to QActions defined in a .ui file
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QMenu>

#include "ActionManager.h"
#include "Cleanup.h"
#include "CleanupCollection.h"
#include "Exception.h"
#include "Settings.h"


using namespace QDirStat;


namespace
{
    /**
     * Read hotkey settings and apply to the existing actions found
     * within 'tree'.  The ui file hotkeys are used as default values.
     **/
    void readSettings( QWidget * tree )
    {
	Settings settings;

	settings.beginGroup( "Hotkeys" );

	const auto actions = tree->findChildren<QAction *>( nullptr, Qt::FindDirectChildrenOnly );
	for ( QAction * action : actions )
	    settings.applyActionHotkey( action );

	settings.endGroup();
    }


    /**
     * Remove actions in 'actionsNames' from the widget, which will
     * typically be a menu.  Start at the end of the list and remove
     * actions until one is reached that is not in the list.
     *
     * This slightly complicated approach avoids the "active actions"
     * part of the menu flickering even though it can't change while
     * the menu is open.  It also means the active action names list
     * doesn't need to be stored.
     **/
    void removeFromWidget( QWidget * menu, const QStringList & actionNames )
    {
	if ( !menu || actionNames.isEmpty() )
	    return;

	auto pos = actionNames.size() - 1;
	const auto menuActions = menu->actions();
	for ( auto it = menuActions.crbegin(); it != menuActions.crend() && pos >= 0; ++it )
	{
	    QAction * menuAction = *it;

	    // Action name "<Cleanups>", remove actions until one that isn't a Cleanup
	    if ( actionNames[ pos ] == ActionManager::cleanups() )
	    {
		if ( qobject_cast<Cleanup *>( menuAction ) )
		{
		    // Remove a Cleanup, keep looking for more
		    menu->removeAction( menuAction );
		    continue;
		}

		//  Not a Cleanup, move on to the next (previous) action in the list
		--pos;
	    }

	    // Search the list (backwards) for a match
	    while ( menuAction->objectName() != actionNames[ pos ] &&
	            ( !menuAction->isSeparator() || actionNames[ pos ] != ActionManager::separator() ) )
	    {
		// End of the list, can't be any more actions to remove
		if ( --pos < 0 )
		    return;
	    }

	    // Remove a matching action and move to the next (previous) name in the list
	    menu->removeAction( menuAction );
	    --pos;
	}
    }


    /**
     * Search the tree for the first QAction with the Qt object name
     * 'actionName'. Return 0 if there is no such QAction.  All named
     * actions in MainWindow are direct children of the main window
     * object so don't search recursively.
     **/
    QAction * action( const QObject * tree, const QString & actionName )
    {
	QAction * action = tree->findChild<QAction *>( actionName, Qt::FindDirectChildrenOnly );
	if ( action )
	    return action;

	logError() << "No action with name " << actionName << " found" << Qt::endl;

	return nullptr;
    }

} // namespace


ActionManager * ActionManager::instance()
{
    static ActionManager _instance;

    return &_instance;
}


void ActionManager::init( QWidget        * parent,
                          SelectionModel * selectionModel,
                          QToolBar       * toolBar,
                          QMenu          * menu )
{
    CHECK_PTR( parent );
    CHECK_PTR( selectionModel );
    CHECK_PTR( menu );
    CHECK_PTR( toolBar );

    readSettings( parent );

    _cleanupCollection = new CleanupCollection{ parent, selectionModel, toolBar, menu };
}


void ActionManager::addActions( QWidget           * widget,
                                const QStringList & actionNames,
                                bool                enabledOnly  ) const
{
    QMenu * menu = qobject_cast<QMenu *>( widget );

    for ( const QString & actionName : actionNames )
    {
	if ( actionName == separator() )
	{
	    if ( menu )
		menu->addSeparator();
	}
	else if ( actionName == cleanups() )
	{
	    if ( _cleanupCollection )
	    {
		if ( enabledOnly )
		    _cleanupCollection->addEnabled( widget );
		else
		    _cleanupCollection->addActive( widget );
	    }
	}
	else
	{
	    QAction * act = action( _cleanupCollection->parent(), actionName );
	    if ( act )
	    {
		if ( act->isEnabled() || !enabledOnly )
                    widget->addAction( act );
            }
	}
    }
}


void ActionManager::moveToTrash()
{
    if ( instance()->_cleanupCollection )
	cleanupCollection()->moveToTrash();
}


void ActionManager::swapActions( QWidget * widget,
                                 QAction * actionToRemove,
                                 QAction * actionToAdd )
{
    if ( !widget->actions().contains( actionToRemove ) )
	return;

    widget->insertAction( actionToRemove, actionToAdd );
    widget->removeAction( actionToRemove );
}


QMenu * ActionManager::createMenu( const QStringList & actions,
                                   const QStringList & enabledActions )
{
    QMenu * menu = new QMenu{};
    menu->setAttribute( Qt::WA_DeleteOnClose );
    addActions( menu, actions );
    addEnabledActions( menu, enabledActions );

    ActionManager * actionManager = instance();
    actionManager->_menu = menu;
    actionManager->_menuEnabledActions = enabledActions;

    return menu;
}


void ActionManager::updateMenu()
{
    // Make sure the Cleanups are up-to-date
    _cleanupCollection->updateActions();

    if ( !_menu )
    {
	// No context menu, clear out any action names that may have been saved
	_menuEnabledActions.clear();
	return;
    }

    // Remove enabled actions from the menu
    removeFromWidget( _menu, _menuEnabledActions );

    // Add the enabled actions originally provided for the context menu, including any cleanups
    addEnabledActions( _menu, _menuEnabledActions );
}
