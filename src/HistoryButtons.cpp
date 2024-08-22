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
    QObject{ parent },
    _history{ new History },
    _actionGoBack{ actionGoBack },
    _actionGoForward{ actionGoForward }
{
    // Two menus - they'll always look the same, but positioning changes slightly as a visual clue
    QMenu * backMenu = new QMenu;
    _actionGoBack->setMenu( backMenu );

    QMenu * forwardMenu = new QMenu;
    _actionGoForward->setMenu( forwardMenu );

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
}


HistoryButtons::~HistoryButtons() = default;


void HistoryButtons::clear()
{
    _history.reset( new History );
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
        if ( !_history->isCurrentItem( url ) )
        {
            _history->add( url );
            updateActions();
        }
    }
}


void HistoryButtons::unlock( const FileInfo * newCurrentItem )
{
    _locked = false;

    // Create a "cleaned" history list without items that are no longer in the tree
    History * cleanedHistory = new History;
    int currentIndex = _history->currentIndex();

    // No current item, no tree, no history
    if ( newCurrentItem )
    {
        const DirTree * tree = newCurrentItem->tree();

        for ( const QString & item : *_history )
        {
            // Remove stale items and merge duplicates that are now contiguous
            if ( tree->locate( item ) && !cleanedHistory->isCurrentItem( item ) )
                cleanedHistory->add( item );
            else if ( currentIndex >= cleanedHistory->size() )
                --currentIndex; // adjust the index for items not included before it
        }
    }

    // Replace the current history with the cleaned history
    _history.reset( cleanedHistory );

    if ( currentIndex >= 0 )
        _history->goTo( currentIndex );

    // The current item may have changed after a refresh
    addToHistory( newCurrentItem );
}


void HistoryButtons::updateHistoryMenu()
{
    QMenu * menu = qobject_cast<QMenu *>( sender() );
    if ( !menu )
        return;

    menu->clear();

    QActionGroup * actionGroup = new QActionGroup{ menu };

    // Populate the menu, most recent entry first
    for ( int i = _history->size() - 1; i >= 0; i-- )
    {
        QAction * action = new QAction{ _history->item( i ), actionGroup };
        action->setCheckable( true );
        action->setChecked( i == _history->currentIndex() );
        action->setData( i );
        menu->addAction( action );
    }
}


void HistoryButtons::historyMenuAction( QAction * action )
{
    if ( !action )
        return;

    emit navigateToUrl( _history->goTo( action->data().toInt() ) );
}
