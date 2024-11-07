/*
 *   File name: TreeWalker.h
 *   Summary:   QDirStat helper class to walk a FileInfo tree
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef TreeWalker_h
#define TreeWalker_h

#include "FileSearchFilter.h"
#include "Typedefs.h" // FileSize


namespace QDirStat
{
    class FileInfo;

    /**
     * Abstract base class to walk recursively through a FileInfo tree to check
     * for each tree item whether or not it should be used for further
     * processing.
     *
     * This is used in the actions in the "discover" menu to check if items fit
     * into a certain category like
     *   - largest files
     *   - newest files
     *   - oldest files
     *   - files with multiple hard links
     *   - broken symlinks
     *   - sparse files
     **/
    class TreeWalker
    {
    public:

        TreeWalker() = default;

        virtual ~TreeWalker() = default;

        /**
         * Suppress copy and assignment constructors (would slice data members)
         **/
        TreeWalker( const TreeWalker & ) = delete;
        TreeWalker & operator=( const TreeWalker & ) = delete;

        /**
         * General preparations before items are checked.  The base
         * implementation does nothing.
         *
         * Derived classes can reimplement this to collect statistics,
         * calculate thresholds. or initialise variables.
         **/
        virtual void prepare( FileInfo * /* subtree */ ) {}

        /**
         * Check if 'item' fits into the category (largest / newest / oldest
         * file etc.). Return 'true' if it fits, 'false' if not.
         *
         * Derived classes are required to implement this.
         **/
        virtual bool check( const FileInfo * item ) = 0;

        /**
         * Flag: results overflow while walking the tree?  The base class
         * always returns false indicating that there has been no overflow.
         *
         * Derived classes can use this to indicate that the number of results
         * was limited.
         **/
        virtual bool overflow() const { return false; }

    };  // class TreeWalker



    /**
     * TreeWalker to find the largest files.
     **/
    class LargestFilesTreeWalker: public TreeWalker
    {
    public:

        /**
         * Find the threshold for what is considered a "large file".
         *
         * Note that the percentile boundary is rounded down to an
         * integer and the check is for values greater than, but not
         * including, that integer value.  This is consistent with the
         * definition of the percentile boundaries.
         **/
        void prepare( FileInfo * subtree ) override;

        bool check( const FileInfo * item ) override;


    private:

        FileSize _threshold;

    };  // class LargestFilesTreeWalker



    /**
     * TreeWalker to find new files.
     **/
    class NewFilesTreeWalker: public TreeWalker
    {
    public:

        /**
         * Find the threshold for what is considered a "new file".
         *
         * Note that the percentile boundary is rounded down to an
         * integer and the check is for values greater than, but not
         * including, that integer value.  This is consistent with the
         * definition of the percentile boundaries.
         **/
        void prepare( FileInfo * subtree ) override;

        bool check( const FileInfo * item ) override;


    private:

        time_t _threshold;

    };  // class NewFilesTreeWalker



    /**
     * TreeWalker to find old files.
     **/
    class OldFilesTreeWalker: public TreeWalker
    {
    public:

        /**
         * Find the threshold for what is considered an "old file".
         *
         * Note that the percentile boundary is rounded up to an
         * integer and the check is for values less than, or equal to,
         * that integer value.  This is consistent with the definition
         * of the percentile boundaries.
         **/
        void prepare( FileInfo * subtree ) override;

        bool check( const FileInfo * item ) override;


    private:

        time_t _threshold;

    };  // class OldFilesTreeWalker



    /**
     * TreeWalker to find files with multiple hard links.
     **/
    class HardLinkedFilesTreeWalker: public TreeWalker
    {
    public:

        bool check( const FileInfo * item ) override;

    }; // class HardLinkedFilesTreeWalker



    /**
     * TreeWalker to find broken symlinks.
     **/
    class BrokenSymlinksTreeWalker: public TreeWalker
    {
    public:

        bool check( const FileInfo * item ) override;

    };  // class BrokenSymlinksTreeWalker



    /**
     * TreeWalker to find sparse files.
     **/
    class SparseFilesTreeWalker: public TreeWalker
    {
    public:

        bool check( const FileInfo * item ) override;

    };  // class SparseFilesTreeWalker



    /**
     * TreeWalker to find files with the specified modification year.
     **/
    class FilesFromYearTreeWalker: public TreeWalker
    {
    public:

        FilesFromYearTreeWalker( short year ):
            TreeWalker{},
            _year{ year }
        {}

        bool check( const FileInfo * item ) override;


    private:

        short _year;

    };  // class FilesFromYearTreeWalker



    /**
     * TreeWalker to find files with the specified modification year and month.
     **/
    class FilesFromMonthTreeWalker: public TreeWalker
    {
    public:

        FilesFromMonthTreeWalker( short year, short month ):
            TreeWalker{},
            _year{ year },
            _month{ month }
        {}

        bool check( const FileInfo * item ) override;


    private:

        short _year;
        short _month;

    };  // class FilesFromMonthTreeWalker



    /**
     * TreeWalker to find files and/or directories that match a pattern.
     **/
    class FindFilesTreeWalker: public TreeWalker
    {
    public:

        FindFilesTreeWalker( const FileSearchFilter & filter ):
            TreeWalker{},
            _filter{ filter }
        {}

        void prepare( FileInfo * subtree ) override;

        bool check( const FileInfo * item ) override;

        bool overflow() const override { return _overflow; }


    private:

        FileSearchFilter _filter;
        int              _count;
        bool             _overflow;

    };   // class FindFilesTreeWalker

}       // namespace QDirStat

#endif  // TreeWalker_h
