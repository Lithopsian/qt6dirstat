/*
 *   File name: DirTreeView.cpp
 *   Summary:   Tree view widget for directory tree
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QMenu>
#include <QMouseEvent>
#include <QScrollBar>

#include "DirTreeView.h"
#include "ActionManager.h"
#include "DirTreeModel.h"
#include "FileInfo.h"
#include "FormatUtil.h"
#include "HeaderTweaker.h"
#include "Logger.h"
#include "PercentBar.h"
#include "Settings.h"
#include "SizeColDelegate.h"


using namespace QDirStat;


namespace
{
    /**
     * Return the list of items that are currently expanded.
     **/
    QModelIndexList expandedIndexes( const DirTreeView * dirTreeView, const DirTreeModel * model )
    {
	QModelIndexList expandedIndexes;

	if ( model )
	{
	    const auto indexList = model->persistentIndexList();
	    for ( const QModelIndex & index : indexList )
	    {
		if ( dirTreeView->isExpanded( index ) )
		    expandedIndexes << index;
	    }
	}

	return expandedIndexes;
    }

}


DirTreeView::DirTreeView( QWidget * parent ):
    QTreeView{ parent },
    _headerTweaker{ new HeaderTweaker{ header(), this } }
{
    readSettings();

    connect( verticalScrollBar(), &QScrollBar::valueChanged,
             this,                &DirTreeView::scrolled );

    connect( this,                &QTreeView::customContextMenuRequested,
             this,                &DirTreeView::contextMenu );
}


DirTreeView::~DirTreeView()
{
    // Must be called here rather than the HeaderTweaker destructor
    // QTreeView virtual methods will no longer be available
    _headerTweaker->writeSettings();
}


void DirTreeView::readSettings()
{
    const auto barDefaultColors = []()
    {
	return ColorList{ QColor{   0,   0, 255 },
	                  QColor{  34,  34, 255 },
	                  QColor{  68,  68, 255 },
	                  QColor{  85,  85, 255 },
	                  QColor{ 102, 102, 255 },
	                  QColor{ 119, 119, 255 },
	                  QColor{ 136, 136, 255 },
	                  QColor{ 153, 153, 255 },
	                  QColor{ 170, 170, 255 },
	                  QColor{ 187, 187, 255 },
	                  QColor{ 204, 204, 255 },
	                };
    };

    Settings settings;

    settings.beginGroup( "DirTreeView" );

    const int barWidth         = settings.value         ( "PercentBarWidth",      150 ).toInt();
    const QColor barBackground = settings.colorValue    ( "PercentBarBackground", QColor{ 160, 160, 160 } );
    const ColorList barColors  = settings.colorListValue( "PercentBarColors",     barDefaultColors() );

    settings.setDefaultValue( "PercentBarWidth",      barWidth );
    settings.setDefaultValue( "PercentBarBackground", barBackground );
    settings.setDefaultValue( "PercentBarColors",     barColors );

    settings.endGroup();

    // Now we have all the settings for the percent bar delegate
    const auto delegate = new PercentBarDelegate{ this, { barWidth, 0 }, barBackground, barColors };
    setItemDelegateForColumn( PercentBarCol, delegate );
    setItemDelegateForColumn( SizeCol, new SizeColDelegate{ this } );
}


void DirTreeView::scrolled( int )
{
    // Reset the precision to just the visible rows, but remember the original setting
    const auto treeHeader = header();
    const int precision = treeHeader->resizeContentsPrecision();
    treeHeader->setResizeContentsPrecision( 0 );

    // Loop through columns which have variable widths
    for ( DataColumn col : { NameCol,
                             PercentNumCol,
                             SizeCol,
                             TotalItemsCol,
                             TotalFilesCol,
                             TotalSubDirsCol,
                             UserCol,
                             GroupCol,
                           } )
    {
	// Only check visible columns that are configured to auto-size
	const int  section = DataColumns::toViewCol( col );
	const auto mode    = treeHeader->sectionResizeMode( section );
	const bool hidden  = treeHeader->isSectionHidden( section );
	if ( !hidden && mode == QHeaderView::ResizeToContents )
	{
	    // Signal an update if the required width is more than the current width
	    if ( sizeHintForColumn( section ) > treeHeader->sectionSize( section ) )
	    {
		// Pick a row, any row, just to make Qt reassess the columns
		emit itemDelegateForColumn( SizeCol )->sizeHintChanged( indexAt( { 0, 0 } ) );
		break;
	    }
	}
    }

    // Return the checked rows limit to the default
    treeHeader->setResizeContentsPrecision( precision );
}


void DirTreeView::contextMenu( const QPoint & pos )
{
    const QModelIndex index = indexAt( pos );
    if ( !index.isValid() )
    {
	//logDebug() << "No item at this position" << Qt::endl;
	return;
    }

    // The first action should not be a destructive one like "move to trash":
    // It's just too easy to select and execute the first action accidentially,
    // especially on a laptop touchpad.
    const QStringList actions{ "actionGoUp",
                               "actionGoToToplevel",
                               ActionManager::separator(),
                               "actionCopyPath",
                               "actionMoveToTrash",
                             };

    const QStringList enabledActions{ ActionManager::separator(),
                                      "actionStopReading",
                                      "actionRefreshAll",
                                      "actionRefreshSelected",
                                      "actionReadExcluded",
                                      "actionContinueReading",
                                      ActionManager::separator(),
                                      ActionManager::cleanups(),
                                    };

    QMenu * menu = ActionManager::createMenu( actions, enabledActions );
    menu->exec( mapToGlobal( pos ) );
}


const DirTreeModel * DirTreeView::dirTreeModel() const
{
    if ( !model() )
	return nullptr;

    const DirTreeModel * dirTreeModel = qobject_cast<const DirTreeModel *>( model() );
    if ( dirTreeModel )
	return dirTreeModel;

    logError() << "Wrong model type to get this information" << Qt::endl;
    return nullptr;
}


void DirTreeView::closeAllExcept( const QModelIndex & branch )
{
    QModelIndexList branchesToClose = expandedIndexes( this, dirTreeModel() );

    // Remove all ancestors of 'branch' from branchesToClose
    for ( QModelIndex index = branch; index.isValid(); index = index.parent() )
    {
	//logDebug() << "Not closing " << index << Qt::endl;
	branchesToClose.removeAll( index );
    }

    // 100 is far too many, but they might all be within a small number of branches
    if ( branchesToClose.size() < 100 )
    {
	// Avoid modifying the list as we iterate through it
	const QModelIndexList branches = branchesToClose;
	for ( const QModelIndex & branchToClose : branches )
	{
	    // Remove any branches that have ancestors that will be closed
	    for ( QModelIndex ancestor = branchToClose; ancestor.isValid(); ancestor = ancestor.parent() )
	    {
		if ( branches.contains( ancestor.parent() ) )
		    branchesToClose.removeAll( ancestor );
	    }
	}
    }

    // Close all items left in branchesToClose
    if ( branchesToClose.size() < 10 )
    {
	// Smoothest transition, but very slow for multiple branches
	for ( const QModelIndex & index : asConst( branchesToClose ) )
	    collapse( index );
    }
    else
    {
	// Collapses too much, then has to open one branch again, not so smooth
	// So only do this for cases that would be too slow one by one
	collapseAll();
    }

    // This positions the item as close as possible to the center of the viewport
    // It re-opens the relevant branch if it has been closed
    scrollToCurrent();
}


void DirTreeView::expandItem( FileInfo * item )
{
    const DirTreeModel * model = dirTreeModel();
    if ( model )
    {
	const QModelIndex index = model->modelIndex( item );
	if ( index.isValid() )
	    expand( index );
    }
}


void DirTreeView::rowsChanged( const QModelIndex & index )
{
    const int uniformHeight = rowHeight( index );
    if ( uniformHeight == 0 || viewportSizeHint().height() / uniformHeight < 1000 )
    {
	// Lay out the whole tree if it isn't too big
	scheduleDelayedItemsLayout();
    }
    else
    {
	// Just a quick and dirty update to the visible row column sizes and data
	updateGeometries();
	viewport()->update();
    }
}


bool DirTreeView::viewportEvent( QEvent * event )
{
    if ( event )
    {
	switch( event->type() )
	{
	    case QEvent::ToolTip:
	    {
		const QHelpEvent * helpEvent = static_cast<QHelpEvent *>( event );
		QModelIndex index = indexAt( helpEvent->pos() );
		if ( index.isValid() )
		{
		    // Show a tooltip when the model provides one or when the column is elided
		    const QRect rect     = visualRect( index );
		    const QSize sizeHint = sizeHintForIndex( index );
		    tooltipForElided( rect, sizeHint, model(), index, helpEvent->globalPos() );

		    return true;
		}

		break;
	    }

	    case QEvent::MouseButtonPress:
	    {
		const QMouseEvent * mouseEvent = static_cast<QMouseEvent *>( event );
		const auto button = mouseEvent->button();
		if ( button == Qt::BackButton || button == Qt::ForwardButton )
		    return false;

		break;
	    }

	    default:
		break;
	}
    }

    return QTreeView::viewportEvent( event );
}
