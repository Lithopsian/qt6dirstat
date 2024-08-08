/*
 *   File name: History.cpp
 *   Summary:   Directory navigation history for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "History.h"
#include "Logger.h"


#define VERBOSE_HISTORY 0


using namespace QDirStat;


QString History::goBack()
{
    if ( !canGoBack() )
    {
        logWarning() << "Can't go back any more";
        return QString{};
    }

    _current--;

#if VERBOSE_HISTORY
    dump();
#endif

    return currentItem();
}


QString History::goForward()
{
    if ( !canGoForward() )
    {
        logWarning() << "Can't go forward any more";
        return QString{};
    }

    ++_current;

#if VERBOSE_HISTORY
    dump();
#endif

    return currentItem();
}


bool History::setCurrentIndex( int index )
{
    if ( index >= 0 && index < _items.size() )
    {
        _current = index;
#if VERBOSE_HISTORY
        dump();
#endif

        return true;
    }
    else
    {
        logWarning() << "Index " << index << " out of range" << Qt::endl;
        dump();

        return false;
    }
}


void History::add( const QString & item )
{
    // Remove all items after the current one
    while ( canGoForward() )
        _items.removeLast();

    // If the history capacity is reached, remove the oldest item
    if ( _items.size() >= capacity() )
        _items.removeFirst();

    // Add the new item
    _items << item;

    // The new current is always the item just added
    _current = _items.size() - 1;

#if VERBOSE_HISTORY
    logDebug() << "After add():" << Qt::endl;
    dump();
#endif
}


void History::dump() const
{
    if ( _items.isEmpty() )
    {
        logDebug() << "Empty history" << Qt::endl;
        return;
    }

    logNewline();

    for ( int i = 0; i < _items.size(); ++i )
    {
        logDebug() << ( i == _current ? " ---> " : QString( 6, u' ' ) )
                   << "#" << i
                   << ": \"" << _items.at( i ) << '"' << Qt::endl;
    }

    logNewline();
}
