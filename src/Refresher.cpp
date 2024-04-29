/*
 *   File name: Refresher.h
 *   Summary:   Helper class to refresh a number of subtrees
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "Refresher.h"
#include "DirTree.h"
#include "DirInfo.h"
#include "Exception.h"
#include "Logger.h"
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
	    // This can throw now, but not a lot we can do about it
	    try
	    {
		tree->refresh( _items );
	    }
	    catch ( const SysCallFailedException & ex )
	    {
		CAUGHT( ex );
		MainWindow * mainWindow = dynamic_cast<MainWindow *>( app()->findMainWindow() );
		if ( mainWindow )
		    mainWindow->showOpenDirErrorPopup( ex );
	    }
	}
    }

    this->deleteLater();
}
