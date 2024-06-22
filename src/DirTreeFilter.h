/*
 *   File name: DirTreeFilter.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef DirTreeFilter_h
#define DirTreeFilter_h

#include <memory>

#include "Wildcard.h"


namespace QDirStat
{
    class PkgManager;
    class PkgFileListCache;

    /**
     * Abstract base class to check if a filesystem object should be ignored
     * during directory reading.  The pure virtual function ignore() must
     * be implemented by derived classes.
     **/
    class DirTreeFilter
    {
    public:

	/**
	 * Constructor.
	 **/
	DirTreeFilter() = default;

	/**
	 * Destructor.  Declared explicitly only so it can be virtual.
	 **/
	virtual ~DirTreeFilter() = default;

	/**
	 * Suppress copy and assignment constructors (this is not a QObject)
	 **/
	DirTreeFilter( const DirTreeFilter & ) = delete;
	DirTreeFilter & operator=( const DirTreeFilter & ) = delete;

	/**
	 * Return 'true' if the filesystem object specified by 'path' should
	 * be ignored, 'false' if not.
	 *
	 * Derived classes are required to implement this.
	 **/
	virtual bool ignore( const QString & path ) const = 0;

    };	// class DirTreeFilter


    /**
     * Dir tree filter that checks a wildcard match against a path.
     * This uses QRegularExpression in wildcard mode through the
     * Wildcard wrapper class.
     **/
    class DirTreePatternFilter: public DirTreeFilter
    {
    protected:

	/**
	 * Constructor. If 'pattern' contains a slash ("/"), it is matched
	 * against the complete path. Otherwise, it is matched only against the
	 * filename.  Used by the create method to generate a filter.
	 **/
	DirTreePatternFilter( const QString & pattern ):
	    DirTreeFilter (),
	    _wildcard { CaseSensitiveWildcard( pattern.contains( '/' ) ? pattern : "*/" + pattern ) }
	{}


    public:

	/**
	 * Factory method to create a filter from the specified pattern.
	 * If the pattern is simple, it might be a DirTreeSuffixFilter.
	 * In most other cases, it will be a DirTreePatternFilter.
	 * If 'pattern' is empty, this returns 0.
	 *
	 * Ownership of the created object is transferred to the caller.
	 **/
	static const DirTreeFilter * create( const QString & pattern );

	/**
	 * Return 'true' if the filesystem object specified by 'path' should
	 * be ignored, 'false' if not.
	 *
	 * Implemented from DirTreeFilter.
	 **/
	bool ignore( const QString & path ) const override;


    private:

        Wildcard _wildcard;

    };	// class DirTreePatternFilter


    /**
     * Simpler, but much more common pattern filter:
     *
     * This checks for a filename suffix (extension), i.e. a pattern
     * "*.suffix". This is more efficient than the full-fledged wildcard match
     * that DirTreePatternFilter supports.
     **/
    class DirTreeSuffixFilter: public DirTreeFilter
    {
    public:

	/**
	 * Constructor. 'suffix' should start with a dot (".").
	 **/
	DirTreeSuffixFilter( const QString & suffix ):
	    DirTreeFilter (),
	    _suffix { suffix }
	{}

	/**
	 * Return 'true' if the filesystem object specified by 'path' should
	 * be ignored, 'false' if not.
	 *
	 * Implemented from DirTreeFilter.
	 **/
	bool ignore( const QString & path ) const override;


    private:

	QString _suffix;

    };	// class DirTreeSuffixFilter


    /**
     * Concrete DirTreeFilter class to ignore files that belong to any
     * installed package during directory reading.
     **/
    class DirTreePkgFilter: public DirTreeFilter
    {
    public:

	/**
	 * Constructor.
	 **/
	DirTreePkgFilter( const PkgManager * pkgManager );

	/**
	 * Return 'true' if the filesystem object specified by 'path' should
	 * be ignored, 'false' if not.
	 *
	 * Implemented from DirTreeFilter.
	 **/
	bool ignore( const QString & path ) const override;


    private:

	std::unique_ptr<PkgFileListCache> _fileListCache;

    };	// class DirTreeFilter

}	// namespace QDirStat

#endif	// DirTreeFilter_h
