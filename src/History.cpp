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


using namespace QDirStat;

/*
History::History()
{
    _items.reserve( HISTORY_MAX );
}
*/

void History::clear()
{
    //logDebug() << "Clearing history" << Qt::endl;
    _items.clear();
    _current = -1;
}



QString History::goBack()
{
    if ( !canGoBack() )
    {
        logWarning() << "Can't go back any more";
        return "";
    }

    _current--;
    // dump();

    return currentItem();
}


QString History::goForward()
{
    if ( !canGoForward() )
    {
        logWarning() << "Can't go forward any more";
        return "";
    }

    ++_current;
    // dump();

    return currentItem();
}


bool History::setCurrentIndex( int index )
{
    if ( index >= 0 && index < _items.size() )
    {
        _current = index;
        // dump();

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
        _items.removeLast();  // _current remains the same!

    // If the history capacity is reached, remove the oldest item until there is space
    while ( _items.size() >= capacity() )
    {
        _items.removeFirst();
        _current = _items.size() - 1;
    }

    // Add the new item
    _items << item;
    ++_current;

    // logDebug() << "After add():" << Qt::endl;
    // dump();
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
