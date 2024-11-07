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
#include "MimeCategory.h"
#include "QDirStatApp.h" // selectionModel(), mainWindow()
#include "SelectionModel.h"
#include "Settings.h"


using namespace QDirStat;


namespace
{
    /**
     * Elide the header label to fit inside the current dialog width,
     * so that it fills the available width but very long subtree paths
     * don't stretch the dialog.  A little extra room is left for the
     * user to shrink the dialog, which would then force the label to
     * be elided further.
    **/
//    void showElidedLabel( QLabel * label, const QWidget * container )
//    {
//	// Calculate the last available pixel from the edge of the dialog less the right-hand layout margin
//	const int lastPixel =
//	    container->contentsRect().right() - container->layout()->contentsMargins().right();
//	elideLabel( label, label->statusTip(), lastPixel );
//    }


    /**
     * Return all direct file children matching the given WildcardCategory.
     **/
    FileInfoSet matchingFiles( FileInfo * item, const WildcardCategory & wildcardCategory )
    {
	FileInfoSet result;

	if ( !item )
	    return result;

	const DirInfo * dir = item->dotEntry() ? item->dotEntry() : item->toDirInfo();
	for ( FileInfoIterator it{ dir }; *it; ++it )
	{
	    if ( wildcardCategory.matches( *it ) )
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
	headerItem->setText( PSR_CountCol,     QObject::tr( "Files" ) );
	headerItem->setText( PSR_TotalSizeCol, QObject::tr( "Total Size" ) );
	headerItem->setText( PSR_PathCol,      QObject::tr( "Directory" ) );
	headerItem->setTextAlignment( PSR_PathCol, Qt::AlignLeft | Qt::AlignVCenter );

	QHeaderView * header = tree->header();
	header->setDefaultAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
	header->setSectionResizeMode( QHeaderView::ResizeToContents );

	tree->sortByColumn( PSR_PathCol, Qt::AscendingOrder );
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


void LocateFileTypeWindow::populateSharedInstance( const WildcardCategory & wildcardCategory,
                                                   FileInfo               * fileInfo )
{
    if ( !fileInfo )
        return;

    // Get the shared instance, creating it if necessary
    LocateFileTypeWindow * instance = sharedInstance();

    instance->populate( wildcardCategory, fileInfo );
    instance->show();
    instance->raise();
}


void LocateFileTypeWindow::populate( const WildcardCategory & wildcardCategory, FileInfo * fileInfo )
{
    _wildcardCategory = wildcardCategory;
    _subtree = fileInfo;

    _ui->treeWidget->clear();
    populateRecursive( fileInfo ? fileInfo : _subtree() );

    const int count = _ui->treeWidget->topLevelItemCount();
    const QString intro = count == 1 ? tr( "1 directory" ) : tr( "%L1 directories" ).arg( count );
    const QString & pattern = wildcardCategory.wildcard.pattern();
    const QString & name = pattern.isEmpty() ? wildcardCategory.category->name() : pattern;
    const QString heading = tr( " with %1 files below %2" ).arg( name, replaceCrLf( _subtree.url() ) );

    // Force a redraw of the header from the status tip
    _ui->heading->setStatusTip( intro % heading );
    showElidedLabel( _ui->heading, this );

    _ui->treeWidget->setCurrentItem( _ui->treeWidget->topLevelItem( 0 ) );
//    logDebug() << count << " directories" << Qt::endl;
}


void LocateFileTypeWindow::populateRecursive( FileInfo * dir )
{
    if ( !dir )
	return;

    const FileInfoSet matches = matchingFiles( dir, _wildcardCategory );
    if ( !matches.isEmpty() )
    {
	// Create a search result for this path
	FileSize totalSize = 0LL;
	for ( const FileInfo * file : matches )
	    totalSize += file->size();

	_ui->treeWidget->addTopLevelItem( new PatternSearchResultItem{ dir->url(), matches.size(), totalSize } );
    }

    // Recurse through any subdirectories
    for ( DirInfoIterator it{ dir }; *it; ++it )
	populateRecursive( *it );

    // Unlike in FileTypeStats, there is no need to recurse through
    // any dot entries: They are handled in matchingFiles() already.
}


void LocateFileTypeWindow::refresh()
{
    populate( _wildcardCategory, _subtree() );
}


void LocateFileTypeWindow::selectResults() const
{
    const DirTree * tree = _subtree.tree();
    const QTreeWidgetItem * item = _ui->treeWidget->currentItem();
    if ( !tree || !item )
	return;

    const PatternSearchResultItem * searchResult = dynamic_cast<const PatternSearchResultItem *>( item );
    CHECK_DYNAMIC_CAST( searchResult, "PatternSearchResultItem" );

    FileInfo * dir = tree->locate( searchResult->path() );

    const FileInfoSet matches = matchingFiles( dir, _wildcardCategory );
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


bool LocateFileTypeWindow::event( QEvent * event )
{
    if ( event->type() == QEvent::FontChange || event->type() == QEvent::Resize )
	showElidedLabel( _ui->heading, this );

    return QDialog::event( event );
}




PatternSearchResultItem::PatternSearchResultItem( const QString & path,
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

    set( PSR_CountCol,     Qt::AlignRight, formatCount( count ) );
    set( PSR_TotalSizeCol, Qt::AlignRight, formatSize( totalSize ) );
    set( PSR_PathCol,      Qt::AlignLeft,  replaceCrLf( path ) );

    setIcon( PSR_PathCol,  QIcon( app()->dirTreeModel()->dirIcon() ) );
}


QVariant PatternSearchResultItem::data( int column, int role ) const
{
    // This is just for the tooltip on columns that are likely to be long and elided
    if ( role != Qt::ToolTipRole || column != PSR_PathCol )
	return QTreeWidgetItem::data( column, role );

    return hasLineBreak( _path ) ? _path : tooltipForElided( this, PSR_PathCol, 0 );
}


bool PatternSearchResultItem::operator<( const QTreeWidgetItem & rawOther ) const
{
    if ( !treeWidget() )
	return QTreeWidgetItem::operator<( rawOther );

    // Since this is a reference, the dynamic_cast will throw a std::bad_cast
    // exception if it fails. Not catching this here since this is a genuine
    // error which should not be silently ignored.
    const PatternSearchResultItem & other = dynamic_cast<const PatternSearchResultItem &>( rawOther );

    switch ( treeWidget()->sortColumn() )
    {
	case PSR_CountCol:     return _count     < other.count();
	case PSR_TotalSizeCol: return _totalSize < other.totalSize();
	default:               return QTreeWidgetItem::operator<( rawOther );
    }
}

