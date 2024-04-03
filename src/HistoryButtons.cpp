/*
 *   File name: HistoryButtons.cpp
 *   Summary:	History buttons handling for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <QMenu>
#include <QAction>
#include <QActionGroup>

#include "HistoryButtons.h"
#include "DirInfo.h"
#include "Logger.h"
#include "Exception.h"

using namespace QDirStat;


HistoryButtons::HistoryButtons( QAction * actionGoBack,
                                QAction * actionGoForward ):
    QObject (),
    _history { new History() },
    _actionGoBack    { actionGoBack },
    _actionGoForward { actionGoForward }
{
    CHECK_NEW( _history );

    initHistoryButtons();
}


HistoryButtons::~HistoryButtons()
{
//    delete _backMenu;
//    delete _forwardMenu;
    delete _history;
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


void HistoryButtons::addToHistory( FileInfo * item )
{
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


void HistoryButtons::initHistoryButtons()
{
    // Two menus - they'll always look the same, but positioning is subtly different
    QMenu * backMenu    = new QMenu();
    QMenu * forwardMenu = new QMenu();


    connect( backMenu, &QMenu::aboutToShow,
             this,     &HistoryButtons::updateHistoryMenu );

    connect( backMenu, &QMenu::triggered,
             this,     &HistoryButtons::historyMenuAction );

    connect( forwardMenu, &QMenu::aboutToShow,
             this,        &HistoryButtons::updateHistoryMenu );

    connect( forwardMenu, &QMenu::triggered,
             this,        &HistoryButtons::historyMenuAction );

    _actionGoBack->setMenu   ( backMenu );
    _actionGoForward->setMenu( forwardMenu );
}


void HistoryButtons::updateHistoryMenu()
{
    QMenu *menu = qobject_cast<QMenu *>( sender() );
    if ( !menu )
        return;

    menu->clear();

    QActionGroup * actionGroup = new QActionGroup( menu );

    const QStringList & items = _history->allItems();
    const int current = _history->currentIndex();

    for ( int i = items.size() - 1; i >= 0; i-- )
    {
        QAction * action = new QAction( items.at( i ), actionGroup );
        action->setCheckable( true );
        action->setChecked( i == current );
        action->setData( i );
        menu->addAction( action );
    }
}


void HistoryButtons::historyMenuAction( QAction * action )
{
    if ( action )
    {
        const int index = action->data().toInt();

        if ( _history->setCurrentIndex( index ) )
            navigateToUrl( _history->currentItem() );
    }
}
