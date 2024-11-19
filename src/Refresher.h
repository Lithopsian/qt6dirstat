/*
 *   File name: Refresher.h
 *   Summary:   Helper class to refresh a number of subtrees
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef Refresher_h
#define Refresher_h

#include <QObject>

#include "FileInfoSet.h"


namespace QDirStat
{
    class DirTree;

    /**
     * Helper class to refresh a number of subtrees:
     *
     * Store a FileInfoSet and when a signal is received (typically
     * OutputWindow::lastProcessFinished()), trigger refreshing all stored
     * subtrees.
     *
     * Do not hold on to pointers to instances of this class since each
     * instance will destroy itself at the end of refresh(). On the other hand,
     * if the signal triggering refresh() never arrives, this object will stay
     * forever, so give it a QObject parent (so it will be destroyed when its
     * parent is destroyed) to avoid a memory leak.
     **/
    class Refresher final : public QObject
    {
	Q_OBJECT

    public:

	/**
	 * Create a Refresher that will refresh all subtrees in 'items' in its
	 * refresh() slot.
	 **/
	Refresher( QObject * parent, const FileInfoSet & items ):
	    QObject{ parent },
	    _items{ items }
	{}


    public slots:

	/**
	 * Refresh all subtrees in the internal FileInfoSet.
	 * After this is done, this object will delete itself.
	 **/
	void refresh();


    private:

	const FileInfoSet _items;

    };	// class Refresher

}	// namespace QDirStat

#endif	// Refresher_h
