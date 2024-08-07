/*
 *   File name: ListEditor.h
 *   Summary:   QDirStat configuration dialog classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QApplication>
#include <QContextMenuEvent>
#include <QMenu>

#include "ListEditor.h"
#include "Exception.h"
#include "Settings.h"
#include "SignalBlocker.h"


using namespace QDirStat;


void ListEditor::createAction( const QString      & actionName,
                               const QString      & icon,
                               const QString      & text,
                               const QKeySequence & keySequence,
                               QToolButton        * button,
                               void( ListEditor::*actee )( void ) )
{
    if ( !button )
	return;

    QAction * action = new QAction { QIcon( icon ), text, this };
    action->setObjectName( actionName );
    action->setShortcut( keySequence );

    Settings settings;
    settings.beginGroup( "ConfigDialog" );
    settings.applyActionHotkey( action );
    settings.endGroup();

    button->setDefaultAction( action );

    connect( action, &QAction::triggered,
             this,   actee );
}


void ListEditor::connectActions()
{
    connect( listWidget(), &QListWidget::currentItemChanged,
             this,         &ListEditor::currentItemChanged );

    createAction( "actionToTop",
                  ":/icons/move-top.png",
                  tr( "Move to &top" ),
                  Qt::ALT | Qt::Key_Home,
                  toTopButton(),
                  &ListEditor::toTop );

    createAction( "actionMoveUp",
                  ":/icons/move-up.png",
                  tr( "Move &up" ),
                  Qt::ALT | Qt::Key_Up,
                  moveUpButton(),
                  &ListEditor::moveUp );

    createAction( "actionAdd",
                  ":/icons/add.png",
                  tr( "&Create a new item" ),
                  Qt::ALT | Qt::Key_Insert,
                  addButton(),
                  &ListEditor::add );

    createAction( "actionRemove",
                  ":/icons/remove.png",
                  tr( "&Remove item" ),
                  Qt::ALT | Qt::Key_Delete,
                  removeButton(),
                  &ListEditor::remove );

    createAction( "actionMoveDown",
                  ":/icons/move-down.png",
                  tr( "Move &down" ),
                  Qt::ALT | Qt::Key_Down,
                  moveDownButton(),
                  &ListEditor::moveDown );

    createAction( "actionToBottom",
                  ":/icons/move-bottom.png",
                  tr( "Move to &bottom" ),
                  Qt::ALT | Qt::Key_End,
                  toBottomButton(),
                  &ListEditor::toBottom );

    fillListWidget();
//    updateActions();
}


void ListEditor::moveCurrentItem( int newRow )
{
    QListWidget * list = listWidget();
    SignalBlocker blocker { list };

    int currentRow = list->currentRow();
    QListWidgetItem * currentItem = list->takeItem( currentRow );
    if ( currentItem )
    {
	list->insertItem( newRow, currentItem );
	list->setCurrentItem( currentItem );
	currentItemChanged( currentItem, currentItem );
    }

}


void ListEditor::toTop()
{
    moveCurrentItem( 0 );
}


void ListEditor::moveUp()
{
    moveCurrentItem( listWidget()->currentRow() - 1 );
}


void ListEditor::add()
{
    void * value = newValue();
    QListWidgetItem * item = createItem( valueText( value ), value );
    listWidget()->setCurrentItem( item );
}


void ListEditor::remove()
{
    const int currentRow = listWidget()->currentRow();
    if ( currentRow < 0 )
	return;

    QListWidgetItem * currentItem = listWidget()->takeItem( currentRow );
    deleteValue( value( currentItem ) );
    delete currentItem;
    updateActions();
}


void ListEditor::moveDown()
{
    moveCurrentItem( listWidget()->currentRow() + 1 );
}


void ListEditor::toBottom()
{
    moveCurrentItem( listWidget()->count() - 1 );
}


void ListEditor::updateActions()
{
    const int currentRow = listWidget()->currentRow();
    const int lastRow    = listWidget()->count() - 1;

    const bool enableMoveUp   = currentRow > 0;
    const bool enableRemove   = lastRow > -1;
    const bool enableMoveDown = currentRow < lastRow;

    actionToTop()   ->setEnabled( enableMoveUp   );
    actionMoveUp()  ->setEnabled( enableMoveUp   );
    actionRemove()  ->setEnabled( enableRemove   );
    actionMoveDown()->setEnabled( enableMoveDown );
    actionToBottom()->setEnabled( enableMoveDown );
}


void ListEditor::currentItemChanged( QListWidgetItem * current,
                                     QListWidgetItem * previous)
{
    save( value( previous ) );
    load( value( current  ) );

    updateActions();
}


QListWidgetItem * ListEditor::createItem( const QString & valueText, void * value )
{
    QListWidgetItem * item = new QListWidgetItem { valueText, listWidget() };
    item->setData( Qt::UserRole, QVariant::fromValue( value ) );

    return item;
}


void * ListEditor::value( QListWidgetItem * item )
{
    if ( item )
	return item->data( Qt::UserRole ).value<void *>();

    return nullptr;
}


void ListEditor::contextMenuEvent( QContextMenuEvent * event )
{
    if ( listWidget()->underMouse() )
    {
	QMenu menu;
	menu.addAction( actionToTop() );
	menu.addAction( actionMoveUp() );
	menu.addSeparator();
	menu.addAction( actionAdd() );
	menu.addAction( actionRemove() );
	menu.addSeparator();
	menu.addAction( actionMoveDown() );
	menu.addAction( actionToBottom() );

	menu.exec( event->globalPos() );
    }
}
