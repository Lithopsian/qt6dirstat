/*
 *   File name:	CleanupCollection.h
 *   Summary:	QDirStat classes to reclaim disk space
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */

#ifndef CleanupCollection_h
#define CleanupCollection_h

#include <QList>
#include <QObject>
#include <QPointer>
#include <QStringList>


class QMenu;
class QToolBar;

class OutputWindow;

namespace QDirStat
{
    class Cleanup;
    class FileInfoSet;
    class SelectionModel;
    class Trash;

    typedef QList<Cleanup *>		CleanupList;
    typedef CleanupList::const_iterator CleanupListIterator;

    /**
     * Set of Cleanup actions to be performed for DirTree items, consisting of
     * a number of predefined and a number of user-defined cleanups.
     **/
    class CleanupCollection: public QObject
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	CleanupCollection( QObject	  * parent,
			   SelectionModel * selectionModel,
			   QToolBar	  * toolBar,
			   QMenu	  * menu );

	/**
	 * Destructor
	 **/
	~CleanupCollection() override;

	/**
	 * Add all currently active actions to the given widget.
	 *
	 * This method is used to provide all the active cleanups to dialogs
	 * that may require them (eg. for hotkeys), but is also used internally.
	 **/
	void addActive( QWidget * widget ) const;

	/**
	 * Add all currently enabled actions to the given widget.
	 *
	 * This method is intended for context menus that are created for just
	 * one menu selection and then immediately discarded.  The menu is not
	 * remembered or kept in sync with future changes,  It can also be used
	 * to add Cleanups to other widget, for example to get action hotkeys in
	 * a window.
	 **/
	void addEnabled( QWidget * widget ) const;

	/**
	 * Return the cleanup with the specified index or 0 if the index is out
	 * of range.
	 **/
	const Cleanup * at( int index ) const;

	/**
	 * Return the number of cleanup actions in this collection.
	 **/
	int size() const { return _cleanupList.size(); }

	/**
	 * Return the internal cleanup list.
	 **/
	const CleanupList & cleanupList() const { return _cleanupList; }

	/**
	 * Move the selected items to trash.
	 **/
	void moveToTrash();

	/**
	 * Write configuration for all cleanups.
	 **/
	void writeSettings( const CleanupList & newCleanups );


    signals:

	/**
	 * Emitted when a cleanup is started.
	 **/
	void startingCleanup( const QString & cleanupName );

	/**
	 * Emitted when the last process of a cleanup is finished, but before any refresh.
	 *
	 * 'errorCount' is the total number of errors reported by all processes
	 * that were started.
	 **/
	void cleanupFinished( int errorCount );

	/**
	 * Emitted after a cleanup is completed when the refresh policy
	 * is AssumeDeleted.  There will be no refresh, so this is the only
	 * indication that the tree is now stable.  The childDeleted signal
	 * from DirTree is also emitted when a cache file is read in the middle
	 * of a tree read, so is not a reliable indicator that the tree is
	 * complete and stable.
	 **/
	void assumedDeleted();


    protected slots:

	/**
	 * Update the enabled/disabled state of all cleanup actions depending
	 * on the SelectionModel.
	 **/
	void updateActions();

	/**
	 * Execute a cleanup. This uses sender() to find out which cleanup it
	 * was.
	 **/
	void execute();


    protected:

	/**
	 * Add all actions to the specified menu and keep it updated when the
	 * collection changes.
	 **/
	void addToMenu( QMenu * menu );

	/**
	 * Add all actions that have an icon to the specified tool bar and keep
	 * it updated when the collections changes.
	 **/
	void addToToolBar( QToolBar * toolBar );

	/**
	 * Return 'true' if this collection is empty.
	 **/
	bool isEmpty() const { return _cleanupList.isEmpty(); }

	/**
	 * Remove all cleanups from this collection.
	 **/
	void clear();

	/**
	 * Read configuration for all cleanups.
	 **/
	void readSettings();

	/**
	 * Add a cleanup to this collection. The collection assumes ownerwhip
	 * of this cleanup.
	 **/
	void add( Cleanup * cleanup );

	/**
	 * Return the index of a cleanup or -1 if it is not part of this
	 * collection.
	 **/
	int indexOf( Cleanup * cleanup ) const;

	/**
	 * Add the standard cleanups to this collection.
	 **/
	void addStdCleanups();

	/**
	 * Remove a cleanup from this collection and delete it.
	 **/
//	void remove( Cleanup * cleanup );

	/**
	 * Ask user for confirmation to execute a cleanup action for
	 * 'items'. Returns 'true' if user accepts, 'false' otherwise.
	 **/
	static bool confirmation( Cleanup * cleanup, const FileInfoSet & items );

	/**
	 * Return the URLs for the selected item types in 'items':
	 * directories (including dot entries) or files.
	 **/
	static QStringList filteredUrls( const FileInfoSet & items,
					 bool                dirs,
					 bool                files );

	/**
	 * Update all menus that have the 'keepUpdated' flag set.
	 **/
	void updateMenus();

	/**
	 * Update all tool bars that have the 'keepUpdated' flag set.
	 **/
	void updateToolBars();

	/**
	 * Update all menus that have the 'keepUpdated' flag set.
	 **/
	void updateMenusAndToolBars();

	/**
	 * Create a refresher for the given refresh set.
	 **/
	void createRefresher( OutputWindow * outputWindow, const FileInfoSet & refreshSet );


    private:

	//
	// Data members
	//

	SelectionModel		  * _selectionModel;
	CleanupList		    _cleanupList;
	Trash			  * _trash;
	QList<QPointer<QMenu>>	    _menus;
	QList<QPointer<QToolBar>>   _toolBars;

    };
}	// namespace QDirStat


#endif // ifndef CleanupCollection_h

