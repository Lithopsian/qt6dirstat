/*
 *   File name: Refresher.h
 *   Summary:   Helper class to refresh a number of subtrees
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "Refresher.h"
#include "DirInfo.h"
#include "DirTree.h"
#include "Exception.h"
#include "MainWindow.h"
#include "QDirStatApp.h"

using namespace QDirStat;


void Refresher::refresh()
{
    if ( _items.isEmpty() )
    {
	logWarning() << "No items to refresh" << Qt::endl;
    }
    else
    {
	//logDebug() << "Refreshing " << _items.size() << " items" << Qt::endl;

	DirTree * tree = _items.first()->tree();
	if ( tree )
	{
	    // This can throw when refreshing the root if it is no longer accessible
	    try
	    {
		tree->refresh( _items );
	    }
	    catch ( const SysCallFailedException & ex )
	    {
		CAUGHT( ex );
		app()->mainWindow()->showOpenDirErrorPopup( ex );
	    }
	}
    }

    this->deleteLater();
}
