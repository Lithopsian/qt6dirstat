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

#include <memory>

#include <QAction>
#include <QObject>


namespace QDirStat
{
    class FileInfo;
    class History;

    /**
     * Class for managing the actions for navigating the history stack.
     * This class is the owner of the only History object.
     *
     * There are two buttons, each with a menu, two submenus from the
     * main menubar, and their hotkeys, can be used to navigate
     * backwards or forwards in the history, or to a specific url.
     * The actions are enabled and disabled as appropriate for the
     * stats of the DirTree.  The menus are constructed from scratch
     * each time they are opened.
     **/
    class HistoryButtons final : public QObject
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	HistoryButtons( QAction * actionGoBack,
	                QAction * actionGoForward,
	                QWidget * parent );

	/**
	 * Destructor.
	 *
	 * Note that this is the default destructor, but is defined in
	 * the cpp file because of the smart pointer.
	 **/
	~HistoryButtons() override;

	/**
	 * Clear the complete history.
	 **/
	void clear();

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
	 * Unlocked the history so that changes to the current item are
	 * recorded in the history.  If the given current item url does not
	 * match the current history position, it is recorded as the most
	 * most recent.
	 **/
	void unlock( const FileInfo * newCurrentItem );


    signals:

	/**
	 * Emitted when a history item was activated to navigate to the
	 * specified URL.
	 **/
	void navigateToUrl( const QString & url );


    public slots:

	/**
	 * Add a FileInfo item to the history if it's a directory and its URL
	 * is not the same as the current history item.
	 **/
	void addToHistory( const FileInfo * item );


    protected slots:

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
	 * Clear the old history menu and add all current history items to it.
	 **/
	void updateHistoryMenu();

	/**
	 * The user activated an action from the history menu; fetch the history
	 * item index from that action and navigate to that history item.
	 **/
	void historyMenuAction( QAction * action );


    private:

	std::unique_ptr<History> _history;

	QAction * _actionGoBack;
	QAction * _actionGoForward;
	bool      _locked{ false };

    };	// class HistoryButtons

}	// namespace QDirStat

#endif	// HistoryButtons_h
