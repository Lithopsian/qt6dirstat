/*
 *   File name: MimeCategory.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef MimeCategory_h
#define MimeCategory_h

#include <QColor>
#include <QStringList>
#include <QTextStream>


namespace QDirStat
{
    /**
     * Class that represents a category of MIME types like video, music,
     * images, summarizing more detailed MIME types like video/mp4, video/mpeg,
     * video/x-flic etc.
     *
     * The idea is to collect those detailed types in one category to give it
     * common attributes like the QDirStat treemap color so the user can get an
     * impression how much disk space each type consumes.
     **/
    class MimeCategory
    {
    public:

	/**
	 * Create a MimeCategory with the specified name and default color.
	 **/
	MimeCategory():
	    _name{ "" },
	    _color{ Qt::white }
	{}

	/**
	 * Create a MimeCategory with the specified name and default color.
	 **/
	MimeCategory( const QString & name ):
	    _name{ name },
	    _color{ Qt::white }
	{}

	/**
	 * Create a MimeCategory with the specified name and color.
	 **/
	MimeCategory( const QString & name, const QColor  & color ):
	    _name{ name },
	    _color{ color.isValid() ? color : Qt::white }
	{}

	/**
	 * Return the name of this category.
	 **/
	const QString & name() const { return _name; }

	/**
	 * Set the name of this category.
	 **/
	void setName( const QString & newName ) { _name = newName; }

	/**
	 * Return the color for this category.
	 **/
	const QColor & color() const { return _color; }

	/**
	 * Set the color for this category.
	 **/
	void setColor( const QColor & color ) { _color = color; }

	/**
	 * Add a list of patterns. See addPattern() for details.
	 **/
	void addPatterns( const QStringList & patterns, Qt::CaseSensitivity caseSensitivity );

	/**
	 * Clear any suffixes or patterns for this category.
	 **/
	void clear();

	/**
	 * Return the list of case-insensitive exact filename matches.
	 **/
	const QStringList & caseInsensitiveExactList() const
	    { return _caseInsensitiveExactList; }

	/**
	 * Return the list of case-sensitive exact filename matches.
	 **/
	const QStringList & caseSensitiveExactList() const
	    { return _caseSensitiveExactList; }

	/**
	 * Return the list of case-insensitive suffixes for this category.
	 * The suffixes do not contain any leading wildcard or dot,
	 * i.e. it will be "tar.bz2", not ".tar.bz2" or "*.tar.bz2".
	 **/
	const QStringList & caseInsensitiveSuffixList() const
	    { return _caseInsensitiveSuffixList; }

	/**
	 * Return the list of case-sensitive suffixes for this category.
	 * The suffixes do not contain any leading wildcard or dot,
	 * i.e. it will be "tar.bz2", not ".tar.bz2" or "*.tar.bz2".
	 **/
	const QStringList & caseSensitiveSuffixList() const
	    { return _caseSensitiveSuffixList; }

	/**
	 * Return the case-insensitive list of patterns which contain a
	 * suffix plus other wildcards.
	 **/
	const QStringList & caseInsensitiveWildcardSuffixList() const
	    { return _caseInsensitiveWildcardSuffixList; }

	/**
	 * Return the case-sensitive list of patterns which contain a
	 * suffix plus other wildcards.
	 **/
	const QStringList & caseSensitiveWildcardSuffixList() const
	    { return _caseSensitiveWildcardSuffixList; }

	/**
	 * Return the list of case-insensitive patterns for this category
	 * that have wildcards and do not have a trailing suffix.
	 **/
	const QStringList & caseInsensitiveWildcardList() const
	    { return _caseInsensitiveWildcardList; }

	/**
	 * Return the list of case-sensitive patterns for this category
	 * that have wildcards and do not have a trailing suffix.
	 **/
	const QStringList & caseSensitiveWildcardList() const
	    { return _caseSensitiveWildcardList; }

	/**
	 * Return a sorted list of all either case sensitive or case
	 * insensitive suffixes and patterns for this category in human
	 * readable form, i.e. prepend suffixes with "*.":
	 * "tar.bz2" -> "*.tar.bz2".
	 *
	 * The patterns are grouped: exact matches first, then wildcard
	 * suffixes, then suffixes, and lastly any non-suffix wildcard
	 * patterns.
	 *
	 * This is relatively expensive; the individual pattern lists are
	 * not stored sorted and must be sorted by this function.  However,
	 * this will be used relatively infrequently, by the config dialog.
	 **/
	QStringList humanReadablePatternList( Qt::CaseSensitivity caseSensitivity ) const;


    protected:

	/**
	 * Add a pattern that contains no wildcard characters.
	 **/
	void addExactMatch( const QString & suffix, Qt::CaseSensitivity caseSensitivity );

	/**
	 * Add a filename suffix (extension) to this category.
	 * A leading "*." or "*" is cut off.
	 **/
	void addSuffix( const QString & rawSuffix, Qt::CaseSensitivity caseSensitivity );

	/**
	 * Add a filename suffix (extension) to this category.
	 * A leading "*." or "*" is cut off.
	 **/
	void addWildcardSuffix( const QString & rawPattern, Qt::CaseSensitivity caseSensitivity );

	/**
	 * Add a pattern to this category which consists of a suffix plus
	 * other wildcards.
	 **/
	void addWildcard( const QString & rawPattern, Qt::CaseSensitivity caseSensitivity );

	/**
	 * Add a filename pattern to this category. If the pattern contains no wildcard
	 * characters, add it as an exact match.  If it starts with "*." and does not
	 * contain any other wildcard characters, add it as a suffix.  If has a suffix
	 * and contains other wildcard characters before the suffix, add it as a wildcard
	 * suffix.  Otherwise, this will become a regular expression.
	 **/
	void addPattern( const QString & pattern, Qt::CaseSensitivity caseSensitivity );


    private:

	QString _name;
	QColor  _color;

	/**
	 * The raw patterns are categorised into different lists for the convenience of the
	 * categorizer.  The raw patterns themselves are not stored, but are reconstructed
	 * when needed as a single (per case-sensitivity) sorted comma-delimited string by the
	 * humanReadablePatternList() method.
	 **/
	QStringList _caseInsensitiveExactList;
	QStringList _caseSensitiveExactList;
	QStringList _caseInsensitiveSuffixList;
	QStringList _caseSensitiveSuffixList;
	QStringList _caseInsensitiveWildcardSuffixList;
	QStringList _caseSensitiveWildcardSuffixList;
	QStringList _caseInsensitiveWildcardList;
	QStringList _caseSensitiveWildcardList;

    };	// class MimeCategory



    /**
     * Human-readable output of a MimeCategory in a debug stream.
     **/
    inline QTextStream & operator<<( QTextStream & str, MimeCategory * category )
    {
	if ( category )
	    str << "<MimeCategory " << category->name() << ">";
	else
	    str << "<NULL MimeCategory *>";

	return str;
    }

}	// namespace QDirStat

#endif	// MimeCategory_h
