/*
 *   File name: SelectionModel.cpp
 *   Summary:   Handling of selected items
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "SelectionModel.h"
#include "DirTreeModel.h"
#include "DirTree.h"
#include "DirInfo.h"
#include "SignalBlocker.h"
#include "Exception.h"
#include "Logger.h"


using namespace QDirStat;


SelectionModel::SelectionModel( DirTreeModel * dirTreeModel, QObject * parent ):
    QItemSelectionModel ( dirTreeModel, parent ),
    _dirTreeModel { dirTreeModel }
{
    connect( this, &SelectionModel::currentChanged,
	     this, &SelectionModel::propagateCurrentChanged );

    connect( this, qOverload<const QItemSelection &, const QItemSelection &>( &QItemSelectionModel::selectionChanged ),
	     this, &SelectionModel::propagateSelectionChanged );

    connect( dirTreeModel->tree(), &DirTree::deletingChild,
	     this,                 &SelectionModel::deletingChildNotify );

    connect( dirTreeModel->tree(), &DirTree::clearing,
	     this,                 &SelectionModel::clear );
}


void SelectionModel::clear()
{
    _currentItem = nullptr;

    _selectedItems.clear();
    _selectedItemsDirty = true;
    clearSelection();
}


FileInfoSet SelectionModel::selectedItems()
{
    if ( _selectedItemsDirty )
    {
	// Build set of selected items from the selected model indexes

	_selectedItems.clear();

	const QModelIndexList indexes = selectedIndexes();
	for ( const QModelIndex index : indexes )
	{
	    if ( index.isValid() )
	    {
		FileInfo * item = static_cast<FileInfo *>( index.internalPointer() );
		CHECK_MAGIC( item );

		// logDebug() << "Adding " << item << " col " << index.column() << " to selected items" << Qt::endl;
		_selectedItems << item;
	    }
	}

	_selectedItemsDirty = false;
    }

    return _selectedItems;
}


void SelectionModel::propagateCurrentChanged( const QModelIndex & newCurrentIndex,
					      const QModelIndex & oldCurrentIndex )
{
    _currentItem = nullptr;

    if ( newCurrentIndex.isValid() )
    {
	_currentItem = static_cast<FileInfo *>( newCurrentIndex.internalPointer() );
	CHECK_MAGIC( _currentItem );
    }

    const FileInfo * oldCurrentItem = nullptr;

    if ( oldCurrentIndex.isValid() )
    {
	oldCurrentItem = static_cast<FileInfo *>( oldCurrentIndex.internalPointer() );
	CHECK_MAGIC( oldCurrentItem );
    }

    emit currentItemChanged( _currentItem, oldCurrentItem );
}


void SelectionModel::propagateSelectionChanged( const QItemSelection &,
						const QItemSelection & )
{
    _selectedItemsDirty = true;
    emit selectionChanged();
    emit selectionChanged( selectedItems() );
}

/*
void SelectionModel::selectItem( FileInfo * item )
{
    extendSelection( item,
		     true ); // clear
}


void SelectionModel::extendSelection( FileInfo * item, bool clear )
{
    if ( item )
    {
	const QModelIndex index = _dirTreeModel->modelIndex( item );
	if ( index.isValid() )
	{
	    logDebug() << "Selecting " << item << Qt::endl;
	    SelectionFlags flags = Select | Rows;

	    if ( clear )
		flags |= Clear;

	    select( index, flags ); // emits selectionChanged()
	}
    }
    else if ( clear )
    {
	clearSelection(); // emits selectionChanged()
    }
}
*/

void SelectionModel::setSelectedItems( const FileInfoSet & selectedItems )
{
    if ( _verbose )
	logDebug() << "Selecting " << selectedItems.size() << " items" << Qt::endl;

    QItemSelection sel;

    for ( FileInfo * item : selectedItems )
    {
	const QModelIndex index = _dirTreeModel->modelIndex( item );
	if ( index.isValid() )
	     sel.merge( QItemSelection( index, index ), Select );
    }

    select( sel, Clear | Select | Rows );
}


void SelectionModel::setCurrentItem( FileInfo * item, bool select )
{
    if ( _verbose )
	logDebug() << item << " select: " << select << Qt::endl;

    if ( select )
	clear();

    _currentItem = item;

    if ( item )
    {
	const QModelIndex index = _dirTreeModel->modelIndex( item );
	if ( index.isValid() )
	{
	    if ( _verbose )
		logDebug() << "Setting current to " << index << Qt::endl;

	    setCurrentIndex( index, select ? ( Current | Select | Rows ) : Current );
	}
	else
	{
	    logError() << "NOT FOUND in dir tree: " << item << Qt::endl;
	}
    }
    else
    {
	clearCurrentIndex();
    }
}


void SelectionModel::setCurrentItem( const QString & path )
{
    FileInfo * item = _dirTreeModel->tree()->locate( path,
						     true ); // findPseudoDirs
    if ( item )
	// Set the current item and select it
	setCurrentItem( item, true );
    else
	logError() << "No item with path " << path << Qt::endl;
}


void SelectionModel::updateCurrentBranch( FileInfo * newItem )
{
    // Grab this before it is overwritten
    const FileInfo * oldItem = _currentItem;

    // This used be be triggered separately from the same signal
    // Order matters, so call it explicitly here
    setCurrentItem ( newItem, true );

    // See if we have actually changed to a new branch
    // Not perfect, but avoids an expensive signal for the common cases
    FileInfo * newBranch = newItem->isDirInfo() ? newItem->toDirInfo() : newItem->parent();
    if ( !oldItem || !oldItem->isInSubtree( newBranch ) )
	emit currentBranchChanged( _dirTreeModel->modelIndex( newItem ) );
}


void SelectionModel::prepareForRefresh( const FileInfoSet & refreshSet )
{
    FileInfo * current = _currentItem ? _currentItem : refreshSet.first();

    if ( current )
    {
	DirInfo * dir = current->isDirInfo() ? current->toDirInfo() : current->parent();

	if ( dir && dir->isPseudoDir() )
	    dir = dir->parent();

	// Go one directory up from the current item as long as there is an
	// ancestor (but not that item itself) in the refreshSet.
	while ( dir && refreshSet.containsAncestorOf( dir ) )
	    dir = dir->parent();

	if ( dir != dir->tree()->root() )
	{
	    if ( _verbose )
		logDebug() << "Selecting " << dir << Qt::endl;

	    updateCurrentBranch( dir );
	}
    }

}


void SelectionModel::deletingChildNotify( FileInfo * deletedChild )
{
    _selectedItemsDirty = true;
    _selectedItems.clear();

    // When the last child is deleted, the current item is lost (by QAbstractItemModel?)
    if ( !_currentItem )
    {
	// The most sensible choice seems to be the parent of the last child being deleted
	if ( deletedChild->parent() != _dirTreeModel->tree()->root() )
	    setCurrentItem( deletedChild->parent() );
    }
    else if ( _currentItem->isInSubtree( deletedChild ) )
    {
	// Not sure this can happen, but this should avoid a dangling pointer
	setCurrentItem( nullptr );
    }
}


void SelectionModel::dumpSelectedItems()
{
    const FileInfoSet items = selectedItems();
    logDebug() << "Current item: " << _currentItem << Qt::endl;
    logDebug() << items.size() << " items selected" << Qt::endl;

    for ( const FileInfo * item : items )
	logDebug() << "	 Selected: " << item << Qt::endl;

    logNewline();
}






SelectionModelProxy::SelectionModelProxy( SelectionModel * master, QObject * parent ):
    QObject ( parent )
{
    connect( master, qOverload<const FileInfoSet &>( &SelectionModel::selectionChanged ),
	     this,   qOverload<const FileInfoSet &>( &SelectionModelProxy::selectionChanged ) );

    connect( master, &SelectionModel::currentItemChanged,
	     this,   &SelectionModelProxy::currentItemChanged );

    // The signals below aren't used at the moment
//    connect( master, qOverload<const QItemSelection &, const QItemSelection &>( &QItemSelectionModel::selectionChanged ),
//	     this,   qOverload<const QItemSelection &, const QItemSelection &>( &SelectionModelProxy::selectionChanged ) );

//    connect( master, &SelectionModel::currentChanged,
//	     this,   &SelectionModelProxy::currentChanged );

//    connect( master, &SelectionModel::currentColumnChanged,
//	     this,   &SelectionModelProxy::currentColumnChanged );

//    connect( master, &SelectionModel::currentRowChanged,
//	     this,   &SelectionModelProxy::currentRowChanged );

//    connect( master, qOverload<>( &SelectionModel::selectionChanged ),
//	     this,   qOverload<>( &SelectionModelProxy::selectionChanged ) );

//    connect( master, &SelectionModel::currentBranchChanged,
//	     this,   &SelectionModelProxy::currentBranchChanged );
}

