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
#include "CleanupCollection.h"
#include "Exception.h"


using namespace QDirStat;


namespace
{
    /**
     * Remove all actions from the given widget, which would normally
     * be a menu or toolbar.
     **/
    void removeAllFromWidget( QWidget * widget )
    {
	if ( !widget )
	    return;

	const auto actions = widget->actions();
	for ( QAction * action : actions )
	    widget->removeAction( action );
    }


    /**
     * Search the tree for the first QAction with the Qt object name
     * 'actionName'. Return 0 if there is no such QAction.
     **/
    QAction * action( const QObject * tree, const QString & actionName )
    {
	QAction * action = tree->findChild<QAction *>( actionName );
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

    _cleanupCollection = new CleanupCollection( parent, selectionModel, toolBar, menu );
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


QMenu * ActionManager::createMenu( const QStringList & actions, const QStringList & enabledActions )
{
    QMenu * menu = new QMenu {};
    menu->setAttribute(Qt::WA_DeleteOnClose);
    instance()->_menu = menu;

    addActions( menu, actions );
    addEnabledActions( menu, enabledActions );
    instance()->_menuActiveActions = actions;
    instance()->_menuEnabledActions = enabledActions;

    return menu;
}


void ActionManager::updateMenu()
{
    // Make sure the Cleanups are up-to-date
    _cleanupCollection->updateActions();

    if ( !_menu )
    {
	// No context menu, clear out any action names that may have been saved
	_menuActiveActions.clear();
	_menuEnabledActions.clear();
	return;
    }

    // Remove all actions from this menu
    removeAllFromWidget( _menu );

    // Add the actions provided for the context menu, including any cleanups
    addActions( _menu, _menuActiveActions );
    addEnabledActions( _menu, _menuEnabledActions );
}
