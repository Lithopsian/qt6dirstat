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
	 * Default constructor: create a MimeCategory with an empty
	 * name and default color.
	 **/
	MimeCategory() = default;

	/**
	 * Constructor with name and colour: create a MimeCategory
	 * with the specified name and color.
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
	 * Set all the patterns for this category.  Replace any
	 * existing patterns with 'caseInsensitivePatterns' and
	 * 'caseSensitivePatterns'.
	 **/
	void setPatterns( const QStringList & caseInsensitivePatterns,
	                  const QStringList & caseSensitivePatterns );

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
	 **/
	const QStringList & caseInsensitiveSuffixList() const
	    { return _caseInsensitiveSuffixList; }

	/**
	 * Return the list of case-sensitive suffixes for this category.
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
	 * Return a sorted list of all either case-sensitive or case-
	 * insensitive suffixes and patterns for this category.
	 *
	 * The patterns are grouped: exact matches first, then wildcard
	 * suffixes, then suffixes, and lastly any non-suffix wildcard
	 * patterns.  Within each group they are sorted alphabetically.
	 **/
	QStringList patterns( Qt::CaseSensitivity caseSensitivity ) const;


    private:

	// The category name and treemap color
	QString _name;
	QColor  _color;

	// The raw patterns categorised into lists according to format and case-sensitivity
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
    inline QTextStream & operator<<( QTextStream & str, const MimeCategory * category )
    {
	if ( category )
	    str << "<MimeCategory " << category->name() << ">";
	else
	    str << "<NULL MimeCategory *>";

	return str;
    }

}	// namespace QDirStat

#endif	// MimeCategory_h
