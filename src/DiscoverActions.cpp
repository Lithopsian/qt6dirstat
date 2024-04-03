/*
 *   File name: DiscoverActions.cpp
 *   Summary:	Actions for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include "DiscoverActions.h"
#include "BusyPopup.h"
#include "DirInfo.h"
#include "DirTree.h"
#include "FileSearchFilter.h"
#include "FormatUtil.h"
#include "LocateFilesWindow.h"
#include "QDirStatApp.h"
#include "TreeWalker.h"
//#include "Logger.h"
//#include "Exception.h"

using namespace QDirStat;


/**
 * Common function to derive a subtree and call the common LocateFilesWindow.
 **/
static void discoverFiles( TreeWalker    * treeWalker,
                           int             sortCol,
                           Qt::SortOrder   sortOrder,
                           const QString & headingText,
                           FileInfo      * subtree = nullptr )
{
    if ( !subtree )
        subtree = app()->selectedDirInfoOrRoot();

    if ( subtree )
    {
	// Should always be a subtree, but if not then do nothing
	LocateFilesWindow::populateSharedInstance( treeWalker,
						   subtree,
						   headingText.arg( subtree->url() ),
						   sortCol,
						   sortOrder );
    }
}



void DiscoverActions::discoverLargestFiles()
{
    discoverFiles( new LargestFilesTreeWalker(),
                   LocateListSizeCol,
                   Qt::DescendingOrder,
                   QObject::tr( "Largest files in %1" ) );
}


void DiscoverActions::discoverNewestFiles()
{
    discoverFiles( new NewFilesTreeWalker(),
                   LocateListMTimeCol,
                   Qt::DescendingOrder,
                   QObject::tr( "Newest files in %1" ) );
}


void DiscoverActions::discoverOldestFiles()
{
    discoverFiles( new OldFilesTreeWalker(),
                   LocateListMTimeCol,
                   Qt::AscendingOrder,
                   QObject::tr( "Oldest files in %1" ) );
}


void DiscoverActions::discoverHardLinkedFiles()
{
    discoverFiles( new HardLinkedFilesTreeWalker(),
                   LocateListPathCol,
                   Qt::AscendingOrder,
                   QObject::tr( "Files with multiple hard links in %1" ) );
}


void DiscoverActions::discoverBrokenSymLinks()
{
    BusyPopup msg( QObject::tr( "Checking symlinks..." ), app()->findMainWindow() );
    discoverFiles( new BrokenSymLinksTreeWalker(),
                   LocateListPathCol,
                   Qt::AscendingOrder,
                   QObject::tr( "Broken symbolic links in %1" ) );
}


void DiscoverActions::discoverSparseFiles()
{
    discoverFiles( new SparseFilesTreeWalker(),
                   LocateListSizeCol,
                   Qt::DescendingOrder,
                   QObject::tr( "Sparse files in %1" ) );
}


void DiscoverActions::discoverFilesFromYear( const QString & path, short year )
{
    discoverFiles( new FilesFromYearTreeWalker( year ),
                   LocateListMTimeCol, Qt::DescendingOrder,
                   QObject::tr( "Files from %1 in %2" ).arg( year ),
                   app()->dirTree()->locate( path, true ) );
}


void DiscoverActions::discoverFilesFromMonth( const QString & path, short year, short month )
{
    discoverFiles( new FilesFromMonthTreeWalker( year, month ),
                   LocateListMTimeCol,
                   Qt::DescendingOrder,
                   QObject::tr( "Files from %1 %2 in %3" ).arg( monthAbbreviation( month ) ).arg( year),
                   app()->dirTree()->locate( path, true ) );
}


void DiscoverActions::findFiles( const FileSearchFilter & filter )
{
    discoverFiles( new FindFilesTreeWalker( filter ),
		   LocateListPathCol,
		   Qt::AscendingOrder,
		   QObject::tr( "Search results for '%1' in %2" ).arg( filter.pattern() ),
		   filter.subtree() );
}
