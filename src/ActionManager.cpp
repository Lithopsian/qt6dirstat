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
#include "Logger.h"


using namespace QDirStat;


ActionManager::~ActionManager()
{
    // CleanupCollection is already destroyed by its QObject parent
//    delete _cleanupCollection;
}


ActionManager * ActionManager::instance()
{
    static ActionManager _instance;

    return &_instance;
}


void ActionManager::setActions( QWidget        * parent,
				SelectionModel * selectionModel,
				QToolBar       * toolBar,
				QMenu          * menu )
{
    instance()->addTree( parent );

    CleanupCollection * cleanupCollection = new CleanupCollection( parent, selectionModel, toolBar, menu );
    CHECK_NEW( cleanupCollection );
    instance()->setCleanupCollection( cleanupCollection );
}


void ActionManager::addTree( const QWidget * tree )
{
    CHECK_PTR( tree );

    _widgetTrees << QPointer<const QWidget>( tree );
}


QAction * ActionManager::action( const QString & actionName ) const
{
    for ( const QPointer<const QWidget> & tree : _widgetTrees )
    {
	if ( tree ) // might be destroyed in the meantime
	{
	    QAction * action = tree->findChild<QAction *>( actionName );
	    if ( action )
		return action;
	}
    }

    logError() << "No action with name " << actionName << " found" << Qt::endl;

    return nullptr;
}


bool ActionManager::addActions( QWidget *           widget,
                                const QStringList & actionNames,
                                bool                enabledOnly  )
{
    bool foundAll = true;
    QMenu * menu = qobject_cast<QMenu *>( widget );

    for ( const QString & actionName : actionNames )
    {
	if ( actionName.startsWith( "---" ) )
	{
	    if ( menu )
		menu->addSeparator();
	}
	else
	{
	    QAction * act = action( actionName );
	    if ( act )
	    {
		if ( act->isEnabled() || !enabledOnly )
                    widget->addAction( act );
            }
	    else
	    {
		// ActionManager::action() already logs an error if not found
		foundAll = false;
	    }
	}
    }

    return foundAll;
}


void ActionManager::addActiveCleanups( QWidget * widget )
{
    if ( instance()->_cleanupCollection )
	instance()->_cleanupCollection->addActive( widget );
}


void ActionManager::addEnabledCleanups( QWidget * widget )
{
    if ( instance()->_cleanupCollection )
	instance()->_cleanupCollection->addEnabled( widget );
}


void ActionManager::moveToTrash()
{
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
