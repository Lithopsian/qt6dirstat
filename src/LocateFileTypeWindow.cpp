/*
 *   File name: LocateFileTypeWindow.cpp
 *   Summary:   QDirStat "locate files by type" window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QPointer>
#include <QResizeEvent>

#include "LocateFileTypeWindow.h"
#include "DirTree.h"
#include "DirTreeModel.h"
#include "Exception.h"
#include "FileInfoIterator.h"
#include "FormatUtil.h"
#include "HeaderTweaker.h"
#include "MainWindow.h"
#include "QDirStatApp.h"        // SelectionModel, mainWindow()
#include "SelectionModel.h"
#include "Settings.h"


using namespace QDirStat;


namespace
{
    /**
     * Return all direct file children matching the given suffix.
     **/
    FileInfoSet matchingFiles( FileInfo * item, const QString & suffix )
    {
	FileInfoSet result;

	if ( !item )
	    return result;

	const DirInfo * dir = item->dotEntry() ? item->dotEntry() : item->toDirInfo();
	for ( FileInfoIterator it{ dir }; *it; ++it )
	{
	    if ( it->isFile() && it->name().endsWith( suffix, Qt::CaseInsensitive ) )
		result << *it;
	}

	return result;
    }


    /**
     * One-time initialization of the tree widget.
     **/
    void initTree( QTreeWidget * tree )
    {
	app()->dirTreeModel()->setTreeWidgetSizes( tree );

//	_ui->treeWidget->setColumnCount( SSR_ColumnCount );
	tree->setHeaderLabels( { QObject::tr( "Number" ),
	                         QObject::tr( "Total Size" ),
	                         QObject::tr( "Directory" ) } );
	tree->header()->setDefaultAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
	tree->headerItem()->setTextAlignment( SSR_PathCol, Qt::AlignLeft | Qt::AlignVCenter );

	HeaderTweaker::resizeToContents( tree->header() );
    }

} // namespace


LocateFileTypeWindow::LocateFileTypeWindow( QWidget * parent ):
    QDialog{ parent },
    _ui{ new Ui::LocateFileTypeWindow }
{
    // logDebug() << "init" << Qt::endl;

    setAttribute( Qt::WA_DeleteOnClose );

    _ui->setupUi( this );

    initTree( _ui->treeWidget );
    Settings::readWindowSettings( this, "LocateFileTypeWindow" );

    connect( _ui->refreshButton, &QPushButton::clicked,
             this,               &LocateFileTypeWindow::refresh );

    connect( _ui->treeWidget,    &QTreeWidget::currentItemChanged,
             this,               &LocateFileTypeWindow::selectResult );
}


LocateFileTypeWindow::~LocateFileTypeWindow()
{
    //logDebug() << "destroying" << Qt::endl;
    Settings::writeWindowSettings( this, "LocateFileTypeWindow" );
}


LocateFileTypeWindow * LocateFileTypeWindow::sharedInstance()
{
    static QPointer<LocateFileTypeWindow> _sharedInstance;

    if ( !_sharedInstance )
	_sharedInstance = new LocateFileTypeWindow{ app()->mainWindow() };

    return _sharedInstance;
}


void LocateFileTypeWindow::populateSharedInstance( const QString & suffix, FileInfo * fileInfo )
{
    if ( suffix.isEmpty() || !fileInfo )
        return;

    // Get the shared instance, creating it if necessary
    LocateFileTypeWindow * instance = sharedInstance();

    instance->populate( suffix, fileInfo );
    instance->_ui->treeWidget->sortByColumn( SSR_PathCol, Qt::AscendingOrder );
    instance->show();
    instance->raise();

    // Select the first row after a delay so it doesn't slow down displaying the list
    QTimer::singleShot( 25, instance, &LocateFileTypeWindow::selectFirstItem );
}


void LocateFileTypeWindow::populate( const QString & suffix, FileInfo * fileInfo )
{
    _suffix  = suffix;
    _subtree = fileInfo;

    // For better Performance: Disable sorting while inserting many items
    _ui->treeWidget->setSortingEnabled( false );
    _ui->treeWidget->clear();

    populateRecursive( fileInfo ? fileInfo : _subtree() );

    _ui->treeWidget->setSortingEnabled( true );

    const int count = _ui->treeWidget->topLevelItemCount();
    const QString intro = count == 1 ? tr( "1 directory" ) : tr( "%1 directories" ).arg( count );
    const QString heading = tr( " with %1 files below %2" ).arg( displaySuffix(), _subtree.url() );
    _ui->heading->setStatusTip( intro % heading );

    // Force a redraw of the header from the status tip
    resizeEvent( nullptr );

//    logDebug() << count << " directories" << Qt::endl;
}


void LocateFileTypeWindow::populateRecursive( FileInfo * dir )
{
    if ( !dir )
	return;

    const FileInfoSet matches = matchingFiles( dir, _suffix );
    if ( !matches.isEmpty() )
    {
	// Create a search result for this path
	FileSize totalSize = 0LL;
	for ( const FileInfo * file : matches )
	    totalSize += file->size();

	_ui->treeWidget->addTopLevelItem( new SuffixSearchResultItem{ dir->url(), matches.size(), totalSize } );
    }

    // Recurse through any subdirectories
    for ( DirInfoIterator it{ dir }; *it; ++it )
	populateRecursive( *it );

    // Unlike in FileTypeStats, there is no need to recurse through
    // any dot entries: They are handled in matchingFiles() already.
}


void LocateFileTypeWindow::refresh()
{
    populate( _suffix, _subtree() );
    selectFirstItem();
}


void LocateFileTypeWindow::selectResult( QTreeWidgetItem * item ) const
{
    if ( !item )
	return;

    SuffixSearchResultItem * searchResult = dynamic_cast<SuffixSearchResultItem *>( item );
    CHECK_DYNAMIC_CAST( searchResult, "SuffixSearchResultItem" );

    DirTree * tree = _subtree.tree();
    if ( tree )
    {
	FileInfo * dir = _subtree.tree()->locate( searchResult->path() );

	const FileInfoSet matches = matchingFiles( dir, _suffix );
	if ( !matches.isEmpty() )
	    app()->selectionModel()->setCurrentItem( matches.first(), true );

	app()->selectionModel()->setSelectedItems( matches );

	// logDebug() << "Selecting " << searchResult->path() << " with " << matches.size() << " matches" << Qt::endl;
    }
}


void LocateFileTypeWindow::resizeEvent( QResizeEvent * )
{
    // Calculate a width from the dialog less margins, less a bit more
    elideLabel( _ui->heading, _ui->heading->statusTip(), size().width() - 24 );
}




SuffixSearchResultItem::SuffixSearchResultItem( const QString & path,
                                                int             count,
                                                FileSize        totalSize ):
    QTreeWidgetItem{ QTreeWidgetItem::UserType },
    _path{ path },
    _count{ count },
    _totalSize{ totalSize }
{
    set( SSR_CountCol,     QString::number( count ), Qt::AlignRight );
    set( SSR_TotalSizeCol, formatSize( totalSize ),  Qt::AlignRight );
    set( SSR_PathCol,      path,                     Qt::AlignLeft  );

    setIcon( SSR_PathCol,  QIcon( app()->dirTreeModel()->dirIcon() ) );
}


bool SuffixSearchResultItem::operator<( const QTreeWidgetItem & rawOther ) const
{
    if ( !treeWidget() )
	return QTreeWidgetItem::operator<( rawOther );

    // Since this is a reference, the dynamic_cast will throw a std::bad_cast
    // exception if it fails. Not catching this here since this is a genuine
    // error which should not be silently ignored.
    const SuffixSearchResultItem & other = dynamic_cast<const SuffixSearchResultItem &>( rawOther );

    switch ( static_cast<SuffixSearchResultColumns>( treeWidget()->sortColumn() ) )
    {
	case SSR_CountCol:     return _count     < other.count();
	case SSR_TotalSizeCol: return _totalSize < other.totalSize();

	case SSR_PathCol:
	case SSR_ColumnCount:
	    break;
    }

    return QTreeWidgetItem::operator<( rawOther );
}

