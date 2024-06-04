/*
 *   File name: DirTreeView.cpp
 *   Summary:   Tree view widget for directory tree
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QMenu>
#include <QKeyEvent>
#include <QScrollBar>

#include "DirTreeView.h"
#include "ActionManager.h"
#include "DirTree.h"
#include "DirTreeModel.h"
#include "FileInfo.h"
#include "FormatUtil.h"
#include "HeaderTweaker.h"
#include "PercentBar.h"
#include "Settings.h"
#include "SettingsHelpers.h"
#include "SizeColDelegate.h"
#include "Exception.h"
#include "Logger.h"


using namespace QDirStat;


DirTreeView::DirTreeView( QWidget * parent ):
    QTreeView ( parent ),
    _sizeColDelegate { new SizeColDelegate( this ) },
    _headerTweaker { new HeaderTweaker( header(), this ) }
{
    setItemDelegateForColumn( SizeCol, _sizeColDelegate );

    readSettings();

    connect( verticalScrollBar(), &QScrollBar::valueChanged,
	     this,                &DirTreeView::scrolled );

    connect( this,                &QTreeView::customContextMenuRequested,
	     this,                &DirTreeView::contextMenu );
}


DirTreeView::~DirTreeView()
{
    delete _headerTweaker;
    delete _percentBarDelegate;
    delete _sizeColDelegate;
}


void DirTreeView::readSettings()
{
    Settings settings;

    settings.beginGroup( "DirTreeView" );
    const int barWidth = settings.value( "PercentBarWidth", 150 ).toInt();
    settings.setDefaultValue( "PercentBarWidth", barWidth );
    const QColor barBackground = readColorEntry( settings, "PercentBarBackground", QColor( 160, 160, 160 ) );
    setDefaultValue( settings, "PercentBarBackground", barBackground );
    const ColorList barColors = readColorListEntry( settings, "PercentBarColors", percentBarDefaultColors() );
    setDefaultValue( settings, "PercentBarColors", barColors );
    settings.endGroup();

    // Now we have all the settings for the percent bar delegate
    _percentBarDelegate  = new PercentBarDelegate( this, PercentBarCol, barWidth, barBackground, barColors );
    setItemDelegateForColumn( PercentBarCol, _percentBarDelegate );
}


void DirTreeView::contextMenu( const QPoint & pos )
{
    QModelIndex index = indexAt( pos );
    if ( !index.isValid() )
    {
	//logDebug() << "No item at this position" << Qt::endl;
	return;
    }

    QMenu menu;

    // The first action should not be a destructive one like "move to trash":
    // It's just too easy to select and execute the first action accidentially,
    // especially on a laptop touchpad.
    const QStringList actions1 = { "actionGoUp",
                                   "actionGoToToplevel",
                                 };
    ActionManager::addActions( &menu, actions1 );

    const QStringList actions2 = { "actionStopReading",
                                   "actionRefreshAll",
                                   "---",
                                   "actionRefreshSelected",
                                   "actionReadExcluded",
                                   "actionContinueReading",
                                   "---",
                                   "actionCopyPath",
                                   "actionMoveToTrash",
                                   "---",
                                 };
    ActionManager::addEnabledActions( &menu, actions2 );

    // User-defined cleanups
    ActionManager::addEnabledCleanups( &menu );

    menu.exec( mapToGlobal( pos ) );
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


QModelIndexList DirTreeView::expandedIndexes() const
{
    QModelIndexList expandedList;

    const DirTreeModel * model = dirTreeModel();
    if ( model )
    {
	const auto indexList = model->persistentIndexList();
	for ( const QModelIndex & index : indexList )
	{
	    if ( isExpanded( index ) )
		expandedList << index;
	}
    }

    return expandedList;
}


void DirTreeView::closeAllExcept( const QModelIndex & branch )
{
    QModelIndexList branchesToClose = expandedIndexes();

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
	for ( const QModelIndex & index : branchesToClose )
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


void DirTreeView::setExpanded( FileInfo * item, bool expanded )
{
    const DirTreeModel * model = dirTreeModel();
    if ( model )
    {
	const QModelIndex index = model->modelIndex( item );
	if ( index.isValid() )
	    QTreeView::setExpanded( index, expanded );
    }
}


void DirTreeView::mousePressEvent( QMouseEvent * event )
{
    // By default, the QTreeView parent class uses the back / forward buttons
    // on the mouse to cursor up / cursor down in the tree.
    //
    // This makes sure those events are immediately propagated up to the
    // parent widget, where they can act like the history back / forward
    // buttons in the tool bar.
    if ( event && ( event->button() == Qt::BackButton || event->button() == Qt::ForwardButton ) )
    {
	event->ignore();
	return;
    }

    QTreeView::mousePressEvent( event );
}


void DirTreeView::keyPressEvent( QKeyEvent * event )
{
    // By default, this opens all tree branches which completely
    // kills our performance, negating all our lazy sorting in
    // each branch in the DirTreeModel / DirInfo classes.
    //
    // So let's just ignore this key; we have better alternatives
    // with "Tree" -> "Expand to Level" -> "Level 0" .. "Level 5".
    if ( event && event->key() == Qt::Key_Asterisk )
	return;

    QTreeView::keyPressEvent( event );
}


void DirTreeView::scrolled( int )
{
    // Restrict checking to visible rows
    const int precision = header()->resizeContentsPrecision();
    header()->setResizeContentsPrecision( 0 );

    for ( DataColumn col : { NameCol,
			     PercentNumCol,
			     SizeCol,
			     TotalItemsCol,
			     TotalFilesCol,
			     TotalSubDirsCol,
			     UserCol,
			     GroupCol
			   } )
    {
	// Only check visible columns that are configured to auto-size
	const int section = DataColumns::toViewCol( col );
	if ( !header()->isSectionHidden( section ) &&
	     header()->sectionResizeMode( section ) == QHeaderView::ResizeToContents )
	{
	    // Signal an update if the required width is more than the current width
	    if ( sizeHintForColumn( section ) > header()->sectionSize( section ) )
	    {
		// Pick a row, any row, just to make Qt reassess the columns
		emit _sizeColDelegate->sizeHintChanged( indexAt( { 0, 0 } ) );
		break;
	    }
	}
    }

    // Return the checked rows limit to the default
    header()->setResizeContentsPrecision( precision );
}
