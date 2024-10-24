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


#if VERBOSE_HISTORY
namespace
{
    /**
     * Dump the current history stack to the log.
     * This is meant for debugging.
     **/
    void dump( const QStringList & items, int current )
    {
        if ( items.isEmpty() )
        {
            logDebug() << "Empty history" << Qt::endl;
            return;
        }

        logNewline();

        for ( int i = 0; i < items.size(); ++i )
        {
            logDebug() << ( i == current ? " ---> " : QString{ 6, u' ' } )
                       << "#" << i
                       << ": \"" << items.at( i ) << '"' << Qt::endl;
        }

        logNewline();
    }

} // namespace
#endif

QString History::goBack()
{
    if ( !canGoBack() )
    {
        logWarning() << "Can't go back any more";
        return QString{};
    }

    _current--;

#if VERBOSE_HISTORY
    dump( _items, _current );
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
    dump( _items, _current );
#endif

    return currentItem();
}


QString History::goTo( int index )
{
    if ( !isValidIndex( index ) )
    {
        logWarning() << "Index " << index << " out of range" << Qt::endl;

        return QString{};
    }

    _current = index;
#if VERBOSE_HISTORY
    dump( _items, _current );
#endif

    return currentItem();
}


void History::add( const QString & item )
{
    // Remove all items after the current one
    while ( canGoForward() )
        _items.removeLast();

    // If the history capacity is reached, remove the oldest item
    if ( _items.size() >= capacity() )
        _items.removeFirst();

    // Add the new item to the list
    _items << item;

    // The new current is always the item just added
    _current = _items.size() - 1;

#if VERBOSE_HISTORY
    logDebug() << "After add():" << Qt::endl;
    dump( _items, _current );
#endif
}
