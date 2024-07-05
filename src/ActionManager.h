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

#include <QList>
#include <QPointer>


class QAction;
class QMenu;
class QToolBar;


namespace QDirStat
{
    class CleanupCollection;
    class FileInfo;
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
    class ActionManager
    {

	/**
	 * Constructor. Protected because this is a singleton class.
	 **/
	ActionManager() = default;

	/**
	 * Return the singleton instance of this class.  Private, use
	 * the static methods for access.
	 **/
	static ActionManager * instance();


    public:

	/**
	 * Adds a widget tree and transfers the CleanupCollection to the
	 * ActionManager.  This is most likelely the only actions that will
	 * be needed here and should normally be the first call to this class,
	 * so it will create the singleton instance.
	 **/
	static void setActions( QWidget        * parent,
				SelectionModel * selectionModel,
				QToolBar       * toolBar,
				QMenu          * menu );

	/**
	 * Add all the actions listed in 'actionNames' to a widget.
	 **/
	static bool addActions( QWidget * widget, const QStringList & actionNames )
		{ return instance()->addActions( widget, actionNames, false ); }

	/**
	 * Add only the enabled actions in 'actionNames' to a widget.
	 **/
	static bool addEnabledActions( QWidget * widget, const QStringList & actionNames )
		{ return instance()->addActions( widget, actionNames, true ); }

	/**
	 * Replace one action by another, for example in a toolbar.
	 **/
	static void swapActions( QWidget * widget,
				 QAction * actionToRemove,
				 QAction * actionToAdd );

	/**
	 * Returns a pointer to the CleanupCollection.
	 *
	 * Note that the CleanupCollection is not created in the constructor,
	 * and 0 will be returned until setActions() has been called.
	 **/
	static CleanupCollection * cleanupCollection()
	    { return instance()->_cleanupCollection; }

	/**
	 * Add enabled Cleanups to the given widget.
	 **/
	static void addActiveCleanups( QWidget * widget );

	/**
	 * Add enabled Cleanups to the given widget.
	 **/
	static void addEnabledCleanups( QWidget * widget );

	/**
	 * Moves the selected items to trash.
	 **/
	static void moveToTrash();


    protected:

	/**
	 * Add a widget tree. This does not transfer ownership of that widget
	 * tree. The ActionManager will keep the pointer of this tree (with a
	 * guarded pointer so it doesn't matter if it is destroyed) to search
	 * for QActions when requested.
	 **/
	void addTree( const QWidget * tree );

	/**
	 * Gives a pointer to the (likely only) CleanupCollection.
	 **/
	void setCleanupCollection( CleanupCollection * cleanupCollection )
	    { _cleanupCollection = cleanupCollection; }

	/**
	 * Search the known widget trees for the first QAction with the Qt
	 * object name 'actionName'. Return 0 if there is no such QAction.
	 **/
	QAction * action( const QString & actionName ) const;

	/**
	 * Add all the actions in 'actionNames' to a menu. Return 'true' if
	 * success, 'false' if any of the actions were not found.
	 *
	 * If 'enabledOnly' is 'true', only those actions that are currently
	 * enabled are added.
	 *
	 * If an action name in actionNames starts with "---", a separator is
	 * added to the menu instead of an action.
	 *
	 * Note that this class already logs an error for action names that
	 * were not found.
	 **/
	bool addActions( QWidget           * widget,
			 const QStringList & actionNames,
			 bool                enabledOnly);


    private:

	//
	// Data members
	//

	QList<QPointer<const QWidget>>   _widgetTrees;
	CleanupCollection              * _cleanupCollection { nullptr };

    };	// class ActionManager

}	// namespace QDirStat


#endif	// ActionManager_h
