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
	    if ( it->isFile() && it->name().endsWith( suffix, Qt::CaseSensitive ) )
		result << *it;
	}

	return result;
    }


    /**
     * One-time initialization of the tree widget.
     **/
    void initTree( QTreeWidget * tree )
    {
	app()->dirTreeModel()->setTreeIconSize( tree );

	QTreeWidgetItem * headerItem = tree->headerItem();
	headerItem->setText( SSR_CountCol,     QObject::tr( "Files" ) );
	headerItem->setText( SSR_TotalSizeCol, QObject::tr( "Total Size" ) );
	headerItem->setText( SSR_PathCol,      QObject::tr( "Directory" ) );
	headerItem->setTextAlignment( SSR_PathCol, Qt::AlignLeft | Qt::AlignVCenter );

	QHeaderView * header = tree->header();
	header->setDefaultAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
	header->setSectionResizeMode( QHeaderView::ResizeToContents );

	tree->sortByColumn( SSR_PathCol, Qt::AscendingOrder );
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
    instance->show();
    instance->raise();
}


void LocateFileTypeWindow::populate( const QString & suffix, FileInfo * fileInfo )
{
    _suffix  = suffix;
    _subtree = fileInfo;

    _ui->treeWidget->clear();
    populateRecursive( fileInfo ? fileInfo : _subtree() );

    const int count = _ui->treeWidget->topLevelItemCount();
    const QString intro = count == 1 ? tr( "1 directory" ) : tr( "%L1 directories" ).arg( count );
    const QString heading = tr( " with *%1 files below %2" ).arg( suffix, _subtree.url() );

    // Force a redraw of the header from the status tip
    _ui->heading->setStatusTip( intro % heading );
    resizeEvent( nullptr );

    _ui->treeWidget->setCurrentItem( _ui->treeWidget->topLevelItem( 0 ) );
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
}


void LocateFileTypeWindow::selectResults() const
{
    const DirTree * tree = _subtree.tree();
    const QTreeWidgetItem * item = _ui->treeWidget->currentItem();
    if ( !tree || !item )
	return;

    const SuffixSearchResultItem * searchResult = dynamic_cast<const SuffixSearchResultItem *>( item );
    CHECK_DYNAMIC_CAST( searchResult, "SuffixSearchResultItem" );

    FileInfo * dir = tree->locate( searchResult->path() );

    const FileInfoSet matches = matchingFiles( dir, _suffix );
    if ( !matches.isEmpty() )
	app()->selectionModel()->setCurrentItem( matches.first(), false );

    app()->selectionModel()->setSelectedItems( matches );

    //logDebug() << "Selecting " << searchResult->path() << " with " << matches.size() << " matches" << Qt::endl;
}


void LocateFileTypeWindow::selectResult() const
{
    // Select after a delay so the dialog tree appears promptly
    QTimer::singleShot( 100, this, &LocateFileTypeWindow::selectResults );
}


void LocateFileTypeWindow::resizeEvent( QResizeEvent * )
{
    // Calculate the last available pixel from the edge of the dialog less the right-hand layout margin
    const int lastPixel = contentsRect().right() - layout()->contentsMargins().right();
    elideLabel( _ui->heading, _ui->heading->statusTip(), lastPixel );
}




SuffixSearchResultItem::SuffixSearchResultItem( const QString & path,
                                                int             count,
                                                FileSize        totalSize ):
    QTreeWidgetItem{ UserType },
    _path{ path },
    _count{ count },
    _totalSize{ totalSize }
{
    /**
     * Helper function to set the text and text alignment for a column.
     **/
    const auto set = [ this ]( int col, Qt::Alignment alignment, const QString & text )
    {
	setText( col, text );
	setTextAlignment( col, alignment | Qt::AlignVCenter );
    };

    set( SSR_CountCol,     Qt::AlignRight, formatCount( count ) );
    set( SSR_TotalSizeCol, Qt::AlignRight, formatSize( totalSize ) );
    set( SSR_PathCol,      Qt::AlignLeft,  path );

    setIcon( SSR_PathCol,  QIcon( app()->dirTreeModel()->dirIcon() ) );
}


QVariant SuffixSearchResultItem::data( int column, int role ) const
{
    // This is just for the tooltip on columns that are likely to be long and elided
    if ( role != Qt::ToolTipRole || column != SSR_PathCol )
	return QTreeWidgetItem::data( column, role );

    return tooltipForElided( this, SSR_PathCol, 0 );
}


bool SuffixSearchResultItem::operator<( const QTreeWidgetItem & rawOther ) const
{
    if ( !treeWidget() )
	return QTreeWidgetItem::operator<( rawOther );

    // Since this is a reference, the dynamic_cast will throw a std::bad_cast
    // exception if it fails. Not catching this here since this is a genuine
    // error which should not be silently ignored.
    const SuffixSearchResultItem & other = dynamic_cast<const SuffixSearchResultItem &>( rawOther );

    switch ( treeWidget()->sortColumn() )
    {
	case SSR_CountCol:     return _count     < other.count();
	case SSR_TotalSizeCol: return _totalSize < other.totalSize();
	default:               return QTreeWidgetItem::operator<( rawOther );
    }
}

