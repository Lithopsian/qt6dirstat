/*
 *   File name: LocateFileTypeWindow.cpp
 *   Summary:	QDirStat "locate files by type" window
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */

#include <QPointer>

#include "LocateFileTypeWindow.h"
#include "DirTree.h"
#include "DirTreeModel.h"
#include "DotEntry.h"
#include "FormatUtil.h"
#include "HeaderTweaker.h"
#include "QDirStatApp.h"        // SelectionModel, findMainWindow()
#include "SelectionModel.h"
#include "SettingsHelpers.h"
#include "Logger.h"
#include "Exception.h"

using namespace QDirStat;


LocateFileTypeWindow::LocateFileTypeWindow( QWidget * parent ):
    QDialog ( parent ),
    _ui { new Ui::LocateFileTypeWindow }
{
    // logDebug() << "init" << Qt::endl;

    setAttribute( Qt::WA_DeleteOnClose );

    CHECK_NEW( _ui );
    _ui->setupUi( this );

    initWidgets();
    readWindowSettings( this, "LocateFileTypeWindow" );

    connect( _ui->refreshButton, &QPushButton::clicked,
	     this,		 &LocateFileTypeWindow::refresh );

    connect( _ui->treeWidget,	 &QTreeWidget::currentItemChanged,
	     this,		 &LocateFileTypeWindow::selectResult );
}


LocateFileTypeWindow::~LocateFileTypeWindow()
{
    //logDebug() << "destroying" << Qt::endl;
    writeWindowSettings( this, "LocateFileTypeWindow" );
    delete _ui;
}


LocateFileTypeWindow * LocateFileTypeWindow::sharedInstance()
{
    static QPointer<LocateFileTypeWindow> _sharedInstance = nullptr;

    if ( !_sharedInstance )
    {
	_sharedInstance = new LocateFileTypeWindow( app()->findMainWindow() );
	CHECK_NEW( _sharedInstance );
    }

    return _sharedInstance;
}


void LocateFileTypeWindow::clear()
{
    _suffix.clear();
    _ui->treeWidget->clear();
}


void LocateFileTypeWindow::refresh()
{
    populate( _suffix, _subtree() );
    selectFirstItem();
}


void LocateFileTypeWindow::initWidgets()
{
    app()->setWidgetFontSize( _ui->treeWidget );

    _ui->treeWidget->setColumnCount( SSR_ColumnCount );
    _ui->treeWidget->setHeaderLabels( { tr( "Number" ), tr( "Total Size" ), tr( "Directory" ) } );
    _ui->treeWidget->headerItem()->setTextAlignment( SSR_CountCol, Qt::AlignHCenter );
    _ui->treeWidget->headerItem()->setTextAlignment( SSR_TotalSizeCol, Qt::AlignHCenter );

    _ui->treeWidget->setIconSize( app()->dirTreeModel()->dirTreeIconSize() );

    HeaderTweaker::resizeToContents( _ui->treeWidget->header() );
}


void LocateFileTypeWindow::populateSharedInstance( const QString & suffix, FileInfo * subtree )
{
    if ( suffix.isEmpty() || !subtree )
        return;

    // Get the shared instance, creating it if necessary
    LocateFileTypeWindow * instance = sharedInstance();

    instance->populate( suffix, subtree );
    instance->_ui->treeWidget->sortByColumn( SSR_PathCol, Qt::AscendingOrder );
    instance->selectFirstItem();
    instance->show();
    instance->raise();
}


void LocateFileTypeWindow::populate( const QString & suffix, FileInfo * newSubtree )
{
    clear();

    _suffix  = suffix;
    _subtree = newSubtree;

    // For better Performance: Disable sorting while inserting many items
    _ui->treeWidget->setSortingEnabled( false );

    populateRecursive( newSubtree ? newSubtree : _subtree() );

    _ui->treeWidget->setSortingEnabled( true );

    const int count = _ui->treeWidget->topLevelItemCount();
    const QString intro = ( count == 1 ? tr( "1 directory" ) : tr( "%1 directories" ).arg( count ) );
    _ui->heading->setText( intro + tr( " with %2 files below %3" ).arg( displaySuffix() ).arg( _subtree.url() ) );
//    logDebug() << count << " directories" << Qt::endl;
}


void LocateFileTypeWindow::populateRecursive( FileInfo * dir )
{
    if ( !dir )
	return;

    const FileInfoSet matches = matchingFiles( dir );
    if ( !matches.isEmpty() )
    {
	// Create a search result for this path
	FileSize totalSize = 0LL;
	for ( const FileInfo * file : matches )
	    totalSize += file->size();

	SuffixSearchResultItem * searchResultItem =
	    new SuffixSearchResultItem( dir->url(), matches.size(), totalSize );
	CHECK_NEW( searchResultItem );

	_ui->treeWidget->addTopLevelItem( searchResultItem );
    }

    // Recurse through any subdirectories
    for ( FileInfo * child = dir->firstChild(); child; child = child->next() )
    {
	if ( child->isDirInfo() )
	    populateRecursive( child );
    }

    // Notice that unlike in FileTypeStats, there is no need to recurse through
    // any dot entries: They are handled in matchingFiles() already.
}


FileInfoSet LocateFileTypeWindow::matchingFiles( FileInfo * item )
{
    FileInfoSet result;

    if ( !item || !item->isDirInfo() )
	return result;

    const DirInfo * dir = item->toDirInfo();
    if ( dir->dotEntry() )
	dir = dir->dotEntry();

    for ( FileInfo * child = dir->firstChild(); child; child = child->next() )
    {
	if ( child->isFile() && child->name().endsWith( _suffix, Qt::CaseInsensitive ) )
	    result << child;
    }

    return result;
}


void LocateFileTypeWindow::selectResult( QTreeWidgetItem * item )
{
    if ( !item )
	return;

    SuffixSearchResultItem * searchResult = dynamic_cast<SuffixSearchResultItem *>( item );
    CHECK_DYNAMIC_CAST( searchResult, "SuffixSearchResultItem" );

    CHECK_PTR( _subtree.tree() );

    FileInfo * dir = _subtree.tree()->locate( searchResult->path() );

    const FileInfoSet matches = matchingFiles( dir );
    if ( !matches.isEmpty() )
	app()->selectionModel()->setCurrentItem( matches.first(), true );

    app()->selectionModel()->setSelectedItems( matches );

    // logDebug() << "Selecting " << searchResult->path() << " with " << matches.size() << " matches" << Qt::endl;
}






SuffixSearchResultItem::SuffixSearchResultItem( const QString & path,
						int		count,
						FileSize	totalSize ):
    QTreeWidgetItem ( QTreeWidgetItem::UserType ),
    _path { path },
    _count { count },
    _totalSize { totalSize }
{
    set( SSR_CountCol,     QString::number( count ), Qt::AlignRight );
    set( SSR_TotalSizeCol, formatSize( totalSize ),  Qt::AlignRight );
    set( SSR_PathCol,      path,                     Qt::AlignLeft  );

    setIcon( SSR_PathCol,  QIcon( app()->dirTreeModel()->dirIcon() ) );
}


void SuffixSearchResultItem::set( int col, const QString & text, Qt::Alignment alignment )
{
    setText( col, text );
    setTextAlignment( col, alignment | Qt::AlignVCenter );
}


bool SuffixSearchResultItem::operator<( const QTreeWidgetItem & rawOther ) const
{
    // Since this is a reference, the dynamic_cast will throw a std::bad_cast
    // exception if it fails. Not catching this here since this is a genuine
    // error which should not be silently ignored.
    const SuffixSearchResultItem & other = dynamic_cast<const SuffixSearchResultItem &>( rawOther );

    const SuffixSearchResultColumns col = treeWidget() ?
					  (SuffixSearchResultColumns)treeWidget()->sortColumn() :
					  SSR_PathCol;

    switch ( col )
    {
	case SSR_CountCol:     return _count     < other.count();
	case SSR_TotalSizeCol: return _totalSize < other.totalSize();
	case SSR_PathCol:
	case SSR_ColumnCount: break;
    }

    return QTreeWidgetItem::operator<( rawOther );
}

