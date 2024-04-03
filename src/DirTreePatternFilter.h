/*
 *   File name: DirTreePatternFilter.h
 *   Summary:	Support classes for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#ifndef DirTreePatternFilter_h
#define DirTreePatternFilter_h

#include "DirTreeFilter.h"
#include "Wildcard.h"


namespace QDirStat
{
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
	 * filename.  Used the the create method to generate a filter.
	 **/
	DirTreePatternFilter( const QString & pattern ):
	    _wildcard { CaseSensitiveWildcard( pattern.contains( "/" ) ? pattern : QString( "*/" ) + pattern ) }
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
	static DirTreeFilter * create( const QString & pattern );

	/**
	 * Destructor.
	 **/
//	virtual ~DirTreePatternFilter() {}

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
	    _suffix { suffix }
	{}

	/**
	 * Destructor.
	 **/
//	virtual ~DirTreeSuffixFilter() {}

	/**
	 * Return 'true' if the filesystem object specified by 'path' should
	 * be ignored, 'false' if not.
	 *
	 * Implemented from DirTreeFilter.
	 **/
	bool ignore( const QString & path ) const override;

	/**
	 * Return the suffix.
	 **/
	QString suffix() const { return _suffix; }


    private:

	QString _suffix;

    };	// class DirTreeSuffixFilter

}	// namespace QDirStat

#endif	// DirTreePatternFilter_h
