/*
 *   File name: Subtree.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef Subtree_h
#define Subtree_h

#include <QString>


namespace QDirStat
{
    class DirInfo;
    class DirTree;
    class FileInfo;

    /**
     * Class to store information about a subtree of a DirTree.
     *
     * This is basically a wrapper around a FileInfo pointer that takes the
     * very limited life time of such a pointer into account: Whenever a part
     * of the DirTree is refreshed (e.g. after cleanup actions), all pointers
     * in that subtree become invalid. While the DirTree does send signals when
     * that happens, in many cases it is overkill to connect to those signals
     * and monitor all the time for the off-chance that the one FileInfo
     * pointer we keep is affected.
     *
     * This class simply stores the URL of the subtree and locates the FileInfo
     * item in the tree when needed. In addition to that, it can also fall back
     * to the tree's root if that URL no longer exists in the tree.
     *
     * Not using Qt's signals and slots has the added benefit of not needing to
     * inherit QObject which means that instances of this class do not need to
     * be created on the heap with 'new', and they can be aggregated in other
     * classes.
     **/
    class Subtree
    {
    public:
        /**
         * Default constructor, has no tree and no url.  Explicitly declared,
         * otherwise it is suppressed because of the deleted copy constructors.
         **/
        Subtree() = default;

        /**
         * Copy constructor.
         *
         * clone() doesn't work properly, and probably isn't necessary given
         * the existing data members.  Luckily, no-one is using the
         * copy constructors.  Explicitly delete them to draw attention if
         * any code accidentally or deliberately tries to use them.
         **/
        Subtree( const Subtree & other ) = delete;
//            { clone( other ); }

        /**
         * Normal copy assignment operator.
         **/
        Subtree & operator=( const Subtree & other ) = delete;
//            { clone( other ); return *this; }

        /**
         * Assignment Operator for a FileInfo pointer. This is an alias for
         * set().
         **/
        Subtree & operator=( FileInfo * subtree )
            { set( subtree ); return *this; }

        /**
         * Return the DirTree.
         **/
        DirTree * tree() const { return _tree; }

        /**
         * Return the URL.
         **/
        const QString & url() const;

        /**
         * Return 'true' if the tree's root item should be used as a fallback
         * if no URL is set or if no item with that URL can be located. The
         * default is 'true'.
         **/
//        bool useRootFallback() const { return _useRootFallback; }

        /**
         * Enable or disable using the tree's root as a fallback.
         **/
        void setUseRootFallback( bool val ) { _useRootFallback = val; }

        /**
         * Return 'true if the item's parent should be used as a fallback if no
         * item with that URL can be located. The default is 'false'.
         **/
//        bool useParentFallback() const { return _useParentFallback; }

        /**
         * Enable or disable using the item's parent URL as a fallback.
         **/
        void setUseParentFallback( bool val ) { _useParentFallback = val; }

        /**
         * Get the corresponding subtree item from the DirTree via the URL.
         *
         * If a URL was set before (typically by setting the subtree), this
         * traverses the DirTree to find the item with that URL. This is an
         * expensive operation.
         *
         * If no URL was set or if no item with that URL could be found and the
         * 'useRootFallback' flag is set, the root item of the DirTree is used.
         *
         * This might return 0 if no tree was set (also typically by setting
         * the subtree) or if using the root as a fallback is disabled and the
         * URL could not be found in the DirTree.
         **/
        FileInfo * subtree() const;

        /**
         * Get the corresponding DirInfo from the DirTree via the URL.
         * This is very much like 'subtree()', but if the result is not a
         * DirInfo, it traverses up the tree to get the parent.
         *
         * Remember that this may also return a DotEntry, a PkgInfo or an Attic
         * because they are all subclasses of DirInfo.
         **/
        const DirInfo * dir() const;

        /**
         * Dereference operator. This is an alias for subtree(): Get the
         * subtree via the URL.
         **/
        FileInfo * operator()() const { return subtree(); }

        /**
         * Set the subtree. This also sets the tree and the URL which both can
         * be obtained from 'subtree'. This pointer is not stored internally,
         * just the URL and the tree.
         *
         * Setting the subtree to 0 clears the URL, but not the tree. That
         * means if using the root as a fallback is enabled the next call to
         * subtree() will return the tree's root.
         **/
        void set( FileInfo * subtree );

        /**
         * Clear the subtree (but keep the tree).
         **/
        void clear() { set( nullptr ); }

        /**
         * Return 'true' if this subtree is empty, i.e. if it was cleared or if
         * no FileInfo and no URL was ever set.
         **/
        bool isEmpty() const { return _url.isEmpty(); }

        /**
         * Set the DirTree.  Used by, for example, MainWindow::_futureSelection,
         * which sets the tree once and then sets the url as required.
         **/
        void setTree( DirTree * tree ) { _tree = tree; }

        /**
         * Set the URL.  Used when the url is known, but there is not (yet)
         * a FileInfo object for it.
         **/
        void setUrl( const QString & newUrl ) { _url = newUrl; }


    protected:

        /**
         * Locate the FileInfo item with the stored URL in the stored tree.
         **/
        FileInfo * locate() const;

        /**
         * Clone subtree 'other' to this one. This is what both the assignment
         * operator (for Subtrees) and the copy constructor do internally.
         *
         * All data members have their own copy constructors, so there isn't a
         * need for a separate clone/copy function.  It has been fixed to
         * reference all data members, but not tested as there are no uses of
         * it in qdirstat.
         **/
//        void clone( const Subtree & other );


    private:

        //
        // Data members
        //

        DirTree * _tree;
        QString   _url;
        QString   _parentUrl;

        bool      _useRootFallback      { true };
        bool      _useParentFallback    { false };

    };	// class Subtree

}	// namespace QDirStat


#endif // ifndef Subtree_h
