/*
 *   File name: HistoryButtons.h
 *   Summary:   History buttons handling for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef HistoryButtons_h
#define HistoryButtons_h

#include <QObject>

#include "History.h"


class QAction;


namespace QDirStat
{
    class FileInfo;

    class HistoryButtons: public QObject
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	HistoryButtons( QAction * actionGoBack,
			QAction * actionGoForward );

	/**
	 * Destructor.
	 **/
	~HistoryButtons() override;

	/**
	 * Clear the complete history.
	 **/
	void clear() { _history->clear(); }

	/**
	 * Access to the history manager.
	 **/
//	History * history() { return _history; }

	/**
	 * Enable or disable the browser-like "Go Back" and "Go Forward"
	 * actions.
	 **/
	void updateActions();

	/**
	 * Locks the history (temporarily) so that changes to the current
	 * item are not recorded in the history.
	 **/
	void lock() { _locked = true; }

	/**
	 * Returns whether the history is locked.
	 **/
//        void locked() const { return _locked; }

	/**
	 * Unlocked the history so that changes to the current item are
	 * recorded in the history.  If the given current item url does not
	 * match the current history position, it is recorded as the most
	 * most recent.
	 **/
	void unlock( const FileInfo * currentItem );

	/**
	 * Remove items from the history list that can no longer be found in
	 * the tree and make sure that there are not duplicate urls in
	 * successive positions.
	 **/
//	 void clean( const FileInfo * currentItem );


    public slots:

	/**
	 * Handle the browser-like "Go Back" button (action):
	 * Move one entry back in the history of visited directories.
	 **/
	void historyGoBack();

	/**
	 * Handle the browser-like "Go Forward" button (action):
	 * Move one entry back in the history of visited directories.
	 **/
	void historyGoForward();

	/**
	 * Add a FileInfo item to the history if it's a directory and its URL
	 * is not the same as the current history item.
	 **/
	void addToHistory( const FileInfo * item );


    signals:

	/**
	 * Emitted when a history item was activated to navigate to the
	 * specified URL.
	 **/
	void navigateToUrl( const QString & url );


    protected slots:

	/**
	 * Clear the old history menu and add all current history items to it.
	 **/
	void updateHistoryMenu();

	/**
	 * The user activated an action from the history menu; fetch the history
	 * item index from that action and navigate to that history item.
	 **/
	void historyMenuAction( QAction * action );


    protected:

	/**
	 * Initialize the history buttons: Change the tool buttons to handle a
	 * menu upon long click.
	 **/
	void initHistoryButtons();


    private:

	//
	// Data members
	//

	History * _history;
	QAction * _actionGoBack;
	QAction * _actionGoForward;
	bool      _locked { false };

    };
}	// namespace QDirStat

#endif	// HistoryButtons_h
