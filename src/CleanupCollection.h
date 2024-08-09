/*
 *   File name:	CleanupCollection.h
 *   Summary:   QDirStat classes to reclaim disk space
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef CleanupCollection_h
#define CleanupCollection_h

#include <memory>

#include <QObject>


class QMenu;
class QToolBar;


namespace QDirStat
{
    class Cleanup;
    class FileInfoSet;
    class OutputWindow;
    class SelectionModel;
    class Trash;

    typedef QList<Cleanup *> CleanupList;
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
	 * Constructor.  The toolbar and menu are stored and kept in sync
	 * with the set of active Cleanups.  This class does not take
	 * ownership of the selection model, toolbar, or menu.
	 **/
	CleanupCollection( QObject        * parent,
	                   SelectionModel * selectionModel,
	                   QToolBar       * toolBar,
	                   QMenu          * menu );

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
	 * one menu selection and then immediately discarded.
	 **/
	void addEnabled( QWidget * widget ) const;

	/**
	 * Return the cleanup with the specified index or 0 if the index is out
	 * of range.
	 **/
	const Cleanup * at( int index ) const
	    { return index >= 0 && index < _cleanupList.size() ? _cleanupList.at( index ) : nullptr; }

	/**
	 * Return the number of cleanup actions in this collection.
	 **/
	int size() const { return _cleanupList.size(); }

	/**
	 * Move the selected items to trash.
	 **/
	void moveToTrash();

	/**
	 * Return a const iterator for the first Cleanup.
	 **/
	CleanupListIterator begin() const { return cbegin(); }
	CleanupListIterator cbegin() const { return _cleanupList.cbegin(); }

	/**
	 * Return a const iterator for past the last Cleanup.
	 **/
	CleanupListIterator end() const { return cend(); }
	CleanupListIterator cend() const { return _cleanupList.cend(); }

	/**
	 * Write configuration for all cleanups.
	 **/
	void writeSettings( const CleanupList & newCleanups );

	/**
	 * Return whether there is an active Cleanup (that will refresh).
	 **/
	bool isBusy() const { return _activeOutputWindow; }

	/**
	 * Update the enabled/disabled state of all cleanup actions depending
	 * on the SelectionModel.
	 **/
	void updateActions();


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
	 * Execute a cleanup. This uses sender() to find out which cleanup it
	 * was.
	 **/
	void execute();

	/**
	 * Reset _activeOutputWindow to 0 and emit the cleanupFinished signal.
	 **/
	void lastProcessFinished( int totalErrorCount );


    protected:

	/**
	 * Add all Cleanups that have an icon to the specified tool bar and keep
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
	 * Read configuration for all cleanups.  After the Cleanups have been
	 * generated, the stored toolbar and menu are updated.  The previous
	 * actions will have been deleted by Qt, so new ones are created.  They
	 * are always added at the end of the toolbar or menu.
	 **/
	void readSettings();

	/**
	 * Add a cleanup to this collection. The collection assumes ownerwhip
	 * of this cleanup.
	 **/
	void add( Cleanup * cleanup );

	/**
	 * Add the standard cleanups to this collection.
	 **/
	void addStdCleanups();

	/**
	 * Create a refresher for the given refresh set.
	 **/
	void createRefresher( OutputWindow * outputWindow, const FileInfoSet & refreshSet );


    private:

	//
	// Data members
	//

	CleanupList               _cleanupList;
	std::unique_ptr<Trash>    _trash;
	SelectionModel          * _selectionModel;
	QToolBar                * _toolBar;
	QMenu                   * _menu;
	QWidget                 * _activeOutputWindow{ nullptr };

    };	// class CleanupCollection

}	// namespace QDirStat

#endif // ifndef CleanupCollection_h

