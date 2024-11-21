/*
 *   File name: ActionManager.h
 *   Summary:   Common access to QActions defined in a .ui file
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef ActionManager_h
#define ActionManager_h

#include <QAction>
#include <QMenu>
#include <QPointer>
#include <QToolBar>


namespace QDirStat
{
    class CleanupCollection;
    class SelectionModel;

    /**
     * Container class for QActions that are defined in a Qt Designer .ui file, but
     * that are also needed in context menus e.g. in context menus of views.
     *
     * This is a singleton class that is populated by the class that builds the
     * widget tree from the .ui file by simpling passing the toplevel widget of
     * that tree to this class; the ActionManager uses Qt's introspection to find
     * the matching QActions.  Use the static functions for all access.
     **/
    class ActionManager final
    {
	/**
	 * Constructor. Private because this is a singleton class.
	 **/
	ActionManager() = default;

	/**
	 * Return the singleton instance of this class.  Private, use
	 * the static methods for access.
	 **/
	static ActionManager * instance();

	/**
	 * Adds a widget tree and transfers the CleanupCollection to the
	 * ActionManager.  This should generally be called as soon as the
	 * instance is created; eg. instance()->init( ... ).
	 **/
	void init( QWidget        * parent,
	           SelectionModel * selectionModel,
	           QToolBar       * toolBar,
	           QMenu          * cleanupMenu );


    public:

	/**
	 * Adds a widget tree and transfers the CleanupCollection to the
	 * ActionManager.  This should normally be the first call to this class,
	 * and it will create the singleton instance.
	 **/
	static void setActions( QWidget        * parent,
	                        SelectionModel * selectionModel,
	                        QToolBar       * toolBar,
	                        QMenu          * cleanupMenu )
	    { instance()->init( parent, selectionModel, toolBar, cleanupMenu ); }

	/**
	 * Returns the string used to indicate a separator in menus
	 * constructed by this class.
	 **/
	static const QLatin1String separator() { return QLatin1String{ "---" }; }

	/**
	 * Returns the string used to indicate that a list of Cleanups
	 * should be included in menus constructed by this class.
	 **/
	static const QLatin1String cleanups() { return QLatin1String{ "<Cleanups>" }; }

	/**
	 * Add all the actions listed in 'actionNames' to a widget.
	 **/
	static void addActions( QWidget * widget, const QStringList & actionNames )
	    { instance()->addActions( widget, actionNames, false ); }

	/**
	 * Add only the enabled actions in 'actionNames' to a widget.
	 **/
	static void addEnabledActions( QWidget * widget, const QStringList & actionNames )
	    { instance()->addActions( widget, actionNames, true ); }

	/**
	 * Replace one action by another, for example in a toolbar.
	 **/
	static void swapActions( QWidget * widget,
	                         QAction * actionToRemove,
	                         QAction * actionToAdd );

	/**
	 * Create and return a menu containing the given actions.  For now,
	 * this function only supports one list of actions always added (or
	 * if they are active for cleanups), and one list of actions added
	 * only if they are enabled, with the "enabled" list being placed
	 * on the menu after the "active" list.  None of the callers
	 * currently need anything more generic and complex than this.
	 *
	 * The returned QMenu object will automatically delete itself when
	 * it closes.
	 **/
	static QMenu * createMenu( const QStringList & actions,
	                           const QStringList & enabledActions );

	/**
	 * Returns a pointer to the CleanupCollection.
	 *
	 * Note that the CleanupCollection is not created in the constructor,
	 * and 0 will be returned until setActions() has been called.
	 **/
	static CleanupCollection * cleanupCollection()
	    { return instance()->_cleanupCollection; }

	/**
	 * Update the stored toolbar, cleanup menu, and context menu with
	 * the current actions and cleanups.
	 **/
	static void updateActions()
	    { instance()->updateMenu(); }

	/**
	 * Moves the selected items to trash.
	 **/
	static void moveToTrash();

	/**
	 * Read hotkey settings and apply to the existing actions found
	 * within 'tree'.  The ui file hotkeys are used as default values.
	 **/
	static void actionHotkeys( QWidget * parent, const char * group );


    protected:

	/**
	 * Add all the actions in 'actionNames' to a widget.
	 *
	 * If 'enabledOnly' is 'true', only those actions that are currently
	 * enabled are added.
	 *
	 * There are special actionName strings to indicate a separator
	 * (only added for menus) and a list of Cleanups.
	 *
	 * Note that this class logs an error for action names that were not
	 * found.
	 **/
	void addActions( QWidget           * widget,
	                 const QStringList & actionNames,
	                 bool                enabledOnly) const;

	/**
	 * Update any open menu that was created with the createMenu()
	 * fuction. All actions matching the stored enabled actions list are
	 * removed from the end of the menu and then replaced with the
	 * currently-enabled actions from that list.
	 **/
	void updateMenu();


    private:

	CleanupCollection * _cleanupCollection;
	QPointer<QMenu>     _menu;
	QStringList         _menuEnabledActions;

    };	// class ActionManager

}	// namespace QDirStat

#endif	// ActionManager_h
