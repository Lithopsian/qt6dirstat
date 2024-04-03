/*
 *   File name: ListEditor.h
 *   Summary:	QDirStat configuration dialog classes
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <QPushButton>

#include "ListEditor.h"
#include "Logger.h"
#include "Exception.h"

using namespace QDirStat;


void ListEditor::setListWidget( QListWidget * listWidget )
{
    _listWidget = listWidget;

    connect( _listWidget, &QListWidget::currentItemChanged,
	     this,	  &ListEditor::currentItemChanged );
}


void ListEditor::setMoveUpButton( QAbstractButton * button )
{
    _moveUpButton = button;
    connect( button, &QPushButton::clicked, this, &ListEditor::moveUp );
}


void ListEditor::setMoveDownButton( QAbstractButton * button )
{
    _moveDownButton = button;
    connect( button, &QPushButton::clicked, this, &ListEditor::moveDown );
}


void ListEditor::setMoveToTopButton( QAbstractButton * button )
{
    _moveToTopButton = button;
    connect( button, &QPushButton::clicked, this, &ListEditor::moveToTop );
}


void ListEditor::setMoveToBottomButton( QAbstractButton * button )
{
    _moveToBottomButton = button;
    connect( button, &QPushButton::clicked, this, &ListEditor::moveToBottom );
}


void ListEditor::setAddButton( QAbstractButton * button )
{
    _addButton = button;
    connect( button, &QPushButton::clicked, this, &ListEditor::add );
}


void ListEditor::setRemoveButton( QAbstractButton * button )
{
    _removeButton = button;
    connect( button, &QPushButton::clicked, this, &ListEditor::remove );
}


void ListEditor::moveUp()
{
    QListWidgetItem * currentItem = _listWidget->currentItem();
    const int currentRow	  = _listWidget->currentRow();

    if ( ! currentItem )
	return;

    if ( currentRow > 0 )
    {
	_updatesLocked = true;
	_listWidget->takeItem( currentRow );
	_listWidget->insertItem( currentRow - 1, currentItem );
	_listWidget->setCurrentItem( currentItem );
	_updatesLocked = false;
    }
}


void ListEditor::moveDown()
{
    QListWidgetItem * currentItem = _listWidget->currentItem();
    const int currentRow	  = _listWidget->currentRow();

    if ( ! currentItem )
	return;

    if ( currentRow < _listWidget->count() - 1 )
    {
	_updatesLocked = true;
	_listWidget->takeItem( currentRow );
	_listWidget->insertItem( currentRow + 1, currentItem );
	_listWidget->setCurrentItem( currentItem );
	_updatesLocked = false;
    }
}


void ListEditor::moveToTop()
{
    QListWidgetItem * currentItem = _listWidget->currentItem();
    int currentRow		  = _listWidget->currentRow();

    if ( ! currentItem )
	return;

    if ( currentRow > 0 )
    {
	_updatesLocked = true;
	_listWidget->takeItem( currentRow );
	_listWidget->insertItem( 0, currentItem );
	_listWidget->setCurrentItem( currentItem );
	_updatesLocked = false;
    }
}


void ListEditor::moveToBottom()
{
    QListWidgetItem * currentItem = _listWidget->currentItem();
    const int currentRow	  = _listWidget->currentRow();

    if ( ! currentItem )
	return;

    if ( currentRow < _listWidget->count() - 1 )
    {
	_updatesLocked = true;
	_listWidget->takeItem( currentRow );
	_listWidget->addItem( currentItem );
	_listWidget->setCurrentItem( currentItem );
	_updatesLocked = false;
    }
}


void ListEditor::add()
{
    void * value = createValue();
    CHECK_NEW( value );

    ListEditorItem * item = new ListEditorItem( valueText( value ), value );
    CHECK_NEW( item );

    _listWidget->addItem( item );
    _listWidget->setCurrentItem( item );
}


void ListEditor::remove()
{
    QListWidgetItem * currentItem = _listWidget->currentItem();
    const int currentRow	  = _listWidget->currentRow();

    if ( ! currentItem )
	return;

    void * value = this->value( currentItem );

    // Delete current item
    _updatesLocked = true;
    _listWidget->takeItem( currentRow );
    delete currentItem;
    removeValue( value );
    updateActions();
    _updatesLocked = false;

    load( this->value( _listWidget->currentItem() ) );
}


void ListEditor::updateActions()
{
    if ( _listWidget->count() == 0 )
    {
        enableButton( _removeButton,       false );
        enableButton( _moveToTopButton,    false );
        enableButton( _moveUpButton,       false );
        enableButton( _moveDownButton,     false );
        enableButton( _moveToBottomButton, false );
    }
    else
    {
        int currentRow = _listWidget->currentRow();
        int lastRow    = _listWidget->count() - 1;

        enableButton( _removeButton,       true );
        enableButton( _moveToTopButton,    currentRow > _firstRow );
        enableButton( _moveUpButton,       currentRow > _firstRow );
        enableButton( _moveDownButton,     currentRow < lastRow   );
        enableButton( _moveToBottomButton, currentRow < lastRow   );
    }
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
    if ( ! item )
	return nullptr;

    ListEditorItem * editorItem = dynamic_cast<ListEditorItem *>( item );
    CHECK_DYNAMIC_CAST( editorItem, "ListEditorItem *" );

    return editorItem->value();
}


void ListEditor::enableButton( QAbstractButton * button, bool enabled )
{
    if ( button )
	button->setEnabled( enabled );
}
