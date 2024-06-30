/*
 *   File name: HistoryButtons.cpp
 *   Summary:   History buttons handling for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QAction>
#include <QActionGroup>
#include <QMenu>

#include "HistoryButtons.h"
#include "History.h"
#include "DirInfo.h"
#include "DirTree.h"
#include "Logger.h"


using namespace QDirStat;


HistoryButtons::HistoryButtons( QAction * actionGoBack,
                                QAction * actionGoForward,
                                QWidget * parent ):
    QObject { parent },
    _history { new History() },
    _actionGoBack { actionGoBack },
    _actionGoForward { actionGoForward }
{
    initHistoryButtons();
}


HistoryButtons::~HistoryButtons() = default;


void HistoryButtons::clear()
{
    _history->clear();
}


void HistoryButtons::updateActions()
{
    _actionGoBack->setEnabled   ( _history->canGoBack()    );
    _actionGoForward->setEnabled( _history->canGoForward() );
}


void HistoryButtons::historyGoBack()
{
    emit navigateToUrl( _history->goBack() );
    updateActions();
}


void HistoryButtons::historyGoForward()
{
    emit navigateToUrl( _history->goForward() );
    updateActions();
}


void HistoryButtons::addToHistory( const FileInfo * item )
{
    //logDebug() << item << ( _locked ? " (locked)" : " (not locked)" ) << Qt::endl;

    if ( _locked )
        return;

    if ( item && !item->isDirInfo() && item->parent() )
        item = item->parent();

    if ( item )
    {
        const QString url = item->debugUrl();
        if ( url != _history->currentItem() )
        {
            _history->add( url );
            updateActions();
        }
    }
}


void HistoryButtons::unlock( const FileInfo * currentItem )
{
    _locked = false;

    // Clean the history to remove items that are no longer in the tree
    int currentIndex = _history->currentIndex();
    const QStringList items = _history->allItems();
    _history->clear();

    for ( const QString & item : items )
    {
        // Remove stale items and merge duplicates that are now contiguous
        if ( currentItem->tree()->locate( item, true ) && item != _history->currentItem() )
            _history->add( item );
        else if ( currentIndex >= _history->size() )
            --currentIndex; // adjust the index for items removed before it
    }

    if ( currentIndex >= 0 )
        _history->setCurrentIndex( currentIndex );

    addToHistory( currentItem );
}


void HistoryButtons::initHistoryButtons()
{
    // Two menus - they'll always look the same, but positioning changes slightly as a visual clue
    QMenu * backMenu    = new QMenu();
    QMenu * forwardMenu = new QMenu();

    connect( backMenu,         &QMenu::aboutToShow,
             this,             &HistoryButtons::updateHistoryMenu );

    connect( backMenu,         &QMenu::triggered,
             this,             &HistoryButtons::historyMenuAction );

    connect( forwardMenu,      &QMenu::aboutToShow,
             this,             &HistoryButtons::updateHistoryMenu );

    connect( forwardMenu,      &QMenu::triggered,
             this,             &HistoryButtons::historyMenuAction );

    connect( _actionGoBack,    &QAction::triggered,
             this,             &HistoryButtons::historyGoBack );

    connect( _actionGoForward, &QAction::triggered,
             this,             &HistoryButtons::historyGoForward );

    _actionGoBack->setMenu   ( backMenu    );
    _actionGoForward->setMenu( forwardMenu );
}


void HistoryButtons::updateHistoryMenu()
{
    QMenu * menu = qobject_cast<QMenu *>( sender() );
    if ( !menu )
        return;

    menu->clear();

    QActionGroup * actionGroup = new QActionGroup( menu );

    const QStringList & items = _history->allItems();
    for ( int i = items.size() - 1; i >= 0; i-- )
    {
        QAction * action = new QAction( items.at( i ), actionGroup );
        action->setCheckable( true );
        action->setChecked( i == _history->currentIndex() );
        action->setData( i );
        menu->addAction( action );
    }
}


void HistoryButtons::historyMenuAction( QAction * action )
{
    if ( action && _history->setCurrentIndex( action->data().toInt() ) )
        emit navigateToUrl( _history->currentItem() );
}
