/*
 *   File name: ListEditor.h
 *   Summary:   QDirStat configuration dialog classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QContextMenuEvent>
#include <QMenu>

#include "ListEditor.h"
#include "Exception.h"


using namespace QDirStat;


void ListEditor::setListWidget( QListWidget * listWidget )
{
    _listWidget = listWidget;

    connect( listWidget, &QListWidget::currentItemChanged,
             this,       &ListEditor::currentItemChanged );
}


void ListEditor::setToTopButton( QToolButton * button )
{
    _toTopButton = button;
    button->setDefaultAction( toTopAction() );

    connect( toTopAction(), &QAction::triggered,
             this,          &ListEditor::toTop );
}


void ListEditor::setMoveUpButton( QToolButton * button )
{
    _moveUpButton = button;
    button->setDefaultAction( moveUpAction() );

    connect( moveUpAction(), &QAction::triggered,
             this,           &ListEditor::moveUp );
}


void ListEditor::setAddButton( QToolButton * button )
{
    _addButton = button;
    button->setDefaultAction( addAction() );

    connect( addAction(), &QAction::triggered,
             this,        &ListEditor::add );
}


void ListEditor::setRemoveButton( QToolButton * button )
{
    _removeButton = button;
    button->setDefaultAction( removeAction() );

    connect( removeAction(), &QAction::triggered,
             this,           &ListEditor::remove );
}


void ListEditor::setMoveDownButton( QToolButton * button )
{
    _moveDownButton = button;
    button->setDefaultAction( moveDownAction() );

    connect( moveDownAction(), &QAction::triggered,
             this,             &ListEditor::moveDown );
}


void ListEditor::setToBottomButton( QToolButton * button )
{
    _toBottomButton = button;
    button->setDefaultAction( toBottomAction() );

    connect( toBottomAction(), &QAction::triggered,
             this,             &ListEditor::toBottom );
}


void ListEditor::toTop()
{
    QListWidgetItem * currentItem = _listWidget->currentItem();
    if ( !currentItem )
	return;

    int currentRow = _listWidget->currentRow();
    if ( currentRow > 0 )
    {
	_updatesLocked = true;
	_listWidget->takeItem( currentRow );
	_listWidget->insertItem( 0, currentItem );
	_listWidget->setCurrentItem( currentItem );
	_updatesLocked = false;
    }
}


void ListEditor::moveUp()
{
    QListWidgetItem * currentItem = _listWidget->currentItem();
    if ( !currentItem )
	return;

    const int currentRow = _listWidget->currentRow();
    if ( currentRow > 0 )
    {
	_updatesLocked = true;
	_listWidget->takeItem( currentRow );
	_listWidget->insertItem( currentRow - 1, currentItem );
	_listWidget->setCurrentItem( currentItem );
	_updatesLocked = false;
    }
}


void ListEditor::add()
{
    void * value = createValue();

    ListEditorItem * item = new ListEditorItem( valueText( value ), value );
    _listWidget->addItem( item );
    _listWidget->setCurrentItem( item );
}


void ListEditor::remove()
{
    QListWidgetItem * currentItem = _listWidget->currentItem();
    if ( !currentItem )
	return;

    // Delete current item
    _updatesLocked = true;
    removeValue( this->value( currentItem ) );
    _listWidget->takeItem( _listWidget->currentRow() );
    delete currentItem;
    updateActions();
    _updatesLocked = false;

    load( this->value( _listWidget->currentItem() ) );
}


void ListEditor::moveDown()
{

    QListWidgetItem * currentItem = _listWidget->currentItem();
    if ( !currentItem )
	return;

    const int currentRow = _listWidget->currentRow();
    if ( currentRow < _listWidget->count() - 1 )
    {
	_updatesLocked = true;
	_listWidget->takeItem( currentRow );
	_listWidget->insertItem( currentRow + 1, currentItem );
	_listWidget->setCurrentItem( currentItem );
	_updatesLocked = false;
    }
}


void ListEditor::toBottom()
{
    QListWidgetItem * currentItem = _listWidget->currentItem();
    if ( !currentItem )
	return;

    const int currentRow = _listWidget->currentRow();
    if ( currentRow < _listWidget->count() - 1 )
    {
	_updatesLocked = true;
	_listWidget->takeItem( currentRow );
	_listWidget->addItem( currentItem );
	_listWidget->setCurrentItem( currentItem );
	_updatesLocked = false;
    }
}


void ListEditor::updateActions()
{
    const int currentRow = _listWidget->currentRow();
    const int lastRow    = _listWidget->count() - 1;

    const bool enableMoveUp   = currentRow > 0;
    const bool enableRemove   = lastRow > -1;
    const bool enableMoveDown = currentRow < lastRow;

    _toTopAction.setEnabled   ( enableMoveUp   );
    _moveUpAction.setEnabled  ( enableMoveUp   );
    _removeAction.setEnabled  ( enableRemove   );
    _moveDownAction.setEnabled( enableMoveDown );
    _toBottomAction.setEnabled( enableMoveDown );
}


void ListEditor::currentItemChanged( QListWidgetItem * current,
                                     QListWidgetItem * previous)
{
    save( value( previous ) );
    load( value( current  ) );
    updateActions();
}


void * ListEditor::value( QListWidgetItem * item )
{
    if ( !item )
	return nullptr;

    ListEditorItem * editorItem = dynamic_cast<ListEditorItem *>( item );
    CHECK_DYNAMIC_CAST( editorItem, "ListEditorItem *" );

    return editorItem->value();
}


void ListEditor::contextMenuEvent( QContextMenuEvent * event )
{
    //logDebug() << Qt::endl;

    QMenu menu;
    menu.addAction( toTopAction() );
    menu.addAction( moveUpAction() );
    menu.addSeparator();
    menu.addAction( addAction() );
    menu.addAction( removeAction() );
    menu.addSeparator();
    menu.addAction( moveDownAction() );
    menu.addAction( toBottomAction() );

    menu.exec( event->globalPos() );
}
