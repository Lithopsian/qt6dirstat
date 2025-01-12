/*
 *   File name: DiscoverActions.cpp
 *   Summary:   Actions for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QString>

#include "DiscoverActions.h"
#include "BusyPopup.h"
#include "DirInfo.h"
#include "DirTree.h"
#include "FileSearchFilter.h"
#include "FormatUtil.h"
#include "LocateFilesWindow.h"
#include "MainWindow.h"
#include "QDirStatApp.h"
#include "TreeWalker.h"


using namespace QDirStat;


namespace
{
    /**
     * Common function to derive a directory and call the common LocateFilesWindow.
     **/
    void discoverFiles( TreeWalker    * treeWalker,
                        int             sortCol,
                        Qt::SortOrder   sortOrder,
                        const QString & headingText,
                        FileInfo      * fileInfo = nullptr )
    {
        if ( !fileInfo )
            fileInfo = app()->currentDirInfo();

        // Should always be a fileInfo, but if not then do nothing
        if ( fileInfo )
        {
            LocateFilesWindow::populateSharedInstance( treeWalker,
                                                       fileInfo,
                                                       headingText,
                                                       sortCol,
                                                       sortOrder );
        }
    }

} // namespace


void DiscoverActions::discoverLargestFiles()
{
    discoverFiles( new LargestFilesTreeWalker{},
                   LL_SizeCol,
                   Qt::DescendingOrder,
                   QObject::tr( "Largest files in %1" ) );
}


void DiscoverActions::discoverNewestFiles()
{
    discoverFiles( new NewFilesTreeWalker{},
                   LL_MTimeCol,
                   Qt::DescendingOrder,
                   QObject::tr( "Newest files in %1" ) );
}


void DiscoverActions::discoverOldestFiles()
{
    discoverFiles( new OldFilesTreeWalker{},
                   LL_MTimeCol,
                   Qt::AscendingOrder,
                   QObject::tr( "Oldest files in %1" ) );
}


void DiscoverActions::discoverHardLinkedFiles()
{
    discoverFiles( new HardLinkedFilesTreeWalker{},
                   LL_PathCol,
                   Qt::AscendingOrder,
                   QObject::tr( "Files with multiple hard links in %1" ) );
}


void DiscoverActions::discoverBrokenSymlinks()
{
    BusyPopup msg{ QObject::tr( "Checking symlinks..." ), app()->mainWindow() };
    discoverFiles( new BrokenSymlinksTreeWalker{},
                   LL_PathCol,
                   Qt::AscendingOrder,
                   QObject::tr( "Broken symbolic links in %1" ) );
}


void DiscoverActions::discoverSparseFiles()
{
    discoverFiles( new SparseFilesTreeWalker{},
                   LL_SizeCol,
                   Qt::DescendingOrder,
                   QObject::tr( "Sparse files in %1" ) );
}


void DiscoverActions::discoverFilesFromYear( const QString & path, short year )
{
    discoverFiles( new FilesFromYearTreeWalker{ year },
                   LL_MTimeCol,
                   Qt::DescendingOrder,
                   QObject::tr( "Files from %1 in %2" ).arg( year ),
                   app()->dirTree()->locate( path ) );
}


void DiscoverActions::discoverFilesFromMonth( const QString & path, short year, short month )
{
    discoverFiles( new FilesFromMonthTreeWalker{ year, month },
                   LL_MTimeCol,
                   Qt::DescendingOrder,
                   QObject::tr( "Files from %1 %2 in %3" ).arg( monthAbbreviation( month ) ).arg( year ),
                   app()->dirTree()->locate( path ) );
}


void DiscoverActions::findFiles( const FileSearchFilter & filter )
{
    discoverFiles( new FindFilesTreeWalker{ filter },
                   LL_PathCol,
                   Qt::AscendingOrder,
                   QObject::tr( "Search results for '%1' in %2" ).arg( filter.pattern() ),
                   filter.dir() );
}
