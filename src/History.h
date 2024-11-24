/*
 *   File name: History.h
 *   Summary:   Directory navigation history for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef History_h
#define History_h

#include <QStringList>


#define HISTORY_MAX     16


namespace QDirStat
{
    /**
     * Class for managing a string-based navigation history of limited size.
     * This is very like the "back" and "forward" buttons in any web browser.
     *
     * Items are added just like on a stack; you can go back to the previous
     * item, and then you can go forward again (for as many items as you went
     * back). As more and more items are added, from a certain point on, the
     * oldest are removed, so there is a limit how far you can go back.
     **/
    class History final
    {
    public:

        /**
         * Constructor.
         **/
        History() { _items.reserve( capacity() ); }

        /**
         * Add an item to the history stack. If the stack's capacity is
         * reached, the oldest item is removed.
         *
         * All items after the previous current item on are removed; so if you
         * just went back one or more steps, it is no longer possible to go
         * forward again after an item was just added.
         **/
        void add( const QString & item );

        /**
         * Go back one item in the history and return the new current item.
         *
         * Make sure to enable the history "Back" button only if this is
         * possible (check with 'canGoBack()').
         **/
        QString goBack();

        /**
         * Go forward one item in the history and return the new current item.
         *
         * Make sure to enable the history "Forward" button only if this is
         * possible (check with 'canGoBack()').
         **/
        QString goForward();

        /**
         * Go to item number 'index' in the history and return the new current
         * item.
         *
         * Note that an empty string will be return if index is not a valid
         * entry in the history.
         **/
        QString goTo( int index );

        /**
         * Check if it is possible to go one item back in the history.
         * Use this to enable or disable the history "Back" button.
         **/
        bool canGoBack() const { return _current >= 1; }

        /**
         * Check if it is possible to go one item forward in the history.
         * Use this to enable or disable the history "Forward" button.
         **/
        bool canGoForward() const { return _current >= 0 && _current < _items.size() - 1; }

        /**
         * Return whether 'url' is the current item in the history stack.
         *
         * Note that it isn't safe to call this function when _current is
         * not a valid index in the history list (eg. when the list is
         * empty).
         **/
        bool isCurrentItem( const QString & url ) const
            { return isValidIndex( _current ) ? url == _items.at( _current ) : false; }

        /**
         * Return the index (from 0 on) of the current history item or -1 if
         * the history is empty.
         *
         * The current index may change or remain the same when items are
         * added: as long as the history stack is not full, it will change;
         * once it is full, it will remain the same (but the oldest item(s)
         * are discarded).
         **/
        int currentIndex() const { return _current; }

        /**
         * Return 'true' if the history is empty, 'false' otherwise.
         **/
        bool isEmpty() const { return _items.isEmpty(); }

        /**
         * Return the history item with the specified 'index' (from 0 on) or an
         * empty string if there is no item with that index.
         **/
        const QString & item( int index ) const
            { return _items.at( index ); }

        /**
         * The size of the history stack, i.e. the number of items that are
         * currently in it. This is always <= capacity().
         *
         * You can iterate over the history stack from 0 to size() - 1.
         * 0 is the oldest item.
         **/
        int size() const { return _items.size(); }

        /**
         * Return the list of all items on the history stack.
         **/
//        const QStringList & allItems() const { return _items; }

        /**
         * Return begin and end iterators for the internal list.
         **/
        QStringList::const_iterator begin() const { return _items.begin(); }
        QStringList::const_iterator end()   const { return _items.end();   }


    protected:

        /**
         * The capacity of the history stack, i.e. the maximum number of items
         * that it will keep. Once the capacity is reached, each 'add()' first
         * discards the oldest item (i.e. item( 0 )).
         **/
        int capacity() const { return HISTORY_MAX; }

        /**
         * Dump the current history stack to the log.
         * This is meant for debugging.
         **/
        bool isValidIndex( int index ) const { return index >= 0 && index < _items.size(); }

        /**
         * Return the current item in the history stack.
         *
         * Not that it isn't safe to call this function when _current is
         * not a valid index in the history list (eg. when the list is
         * empty).
         **/
        const QString & currentItem() const { return _items.at( _current ); }


    private:

        int         _current{ -1 };
        QStringList _items;

    };  // class History

}       // namespace QDirStat

#endif  // History_h
