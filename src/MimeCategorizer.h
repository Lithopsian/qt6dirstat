/*
 *   File name: MimeCategorizer.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef MimeCategorizer_h
#define MimeCategorizer_h

#include <QBitArray>
#include <QObject>
#include <QReadWriteLock>

#include "MimeCategory.h"
#include "Wildcard.h"


namespace QDirStat
{
    /* Suffixes matches return a list (possibly with only one entry) of pairs.
     * Each pair contains a regular expression and the category it matches to.
     * The regular expression may be empty, indicating a plain suffix that matches
     * any file with that suffix.  Pairs with an empty regular expression will
     * always be the last pair in a list.
     **/
    typedef QPair<Wildcard, const MimeCategory *> WildcardPair;
    typedef QList<const MimeCategory *>           MimeCategoryList;
    typedef QHash<QString, const MimeCategory *>  ExactMatches;
    typedef QMultiHash<QString, WildcardPair>     SuffixMatches;

    class FileInfo;

    /**
     * Class to determine the MimeCategory of filenames.
     *
     * This class is optimized for performance since the names of all files in
     * QDirStat's DirTree need to be checked (something in the order of 300,000
     * in a typical Linux root filesystem).
     *
     * This is a singleton class. Use instance() to get the instance.
     *
     * Configured patterns are matched against the filaname portion of each file.
     * Patterns are a simplified regular expression and can contain '?', '*', and
     * square bracket pairs.  They are always anchored to match the whole filename.
     *
     * For performance, the most common expected pattern types are processed into
     * hash arrays so that a filename can be matched against every pattern of that
     * type at once.  There are maps for patterns with no wildcard characters and for
     * patterns which match against a filename suffix (eg. .cpp or *.cpp).  There
     * are maps for both case-sensitive and insenitive matches.  Other regular
     * expressions are matched last.
     *
     * Patterns without wildcards (eg. Makefile) are matched first for precedence
     * although they would be expected to be fairly uncommon matches.  Only filenames
     * with the same length as one of the patterns are tested.
     *
     * Patterns with suffixes are matched next, but patterns which include a suffix in
     * addition to other matches (eg. ui_*.h) take precedence.  To do this without
     * looping through multiple regular expressions for every file, such patterns are
     * combined in a list with the plain suffix patterns.  If a match to a suffix is found
     * then all the entries in the list (usually just one) must be examined, any regular
     * expressions tested, and the last entry in the list will be an empty regular
     * expression representing the plain suffix match (assuming there was one).
     *
     * Finally, any file which has not been matched is tested against a list of regular
     * expressions in each category.  This is relatively very slow and hopefully there
     * will be both few regular expressions that don't include suffixes and few file
     * which need to be tested against them.
     **/
    class MimeCategorizer: public QObject
    {
	Q_OBJECT

    private:

	/**
	 * Constructor.
	 * This is a singleton class; use instance() tp get a categorizer object.
	 **/
	MimeCategorizer() { readSettings(); }

	/**
	 * Destructor.
	 **/
	~MimeCategorizer() override { clear(); }

	/**
	 * Suppress copy and assignment constructors (this is a singleton)
	 **/
	MimeCategorizer( const MimeCategorizer & ) = delete;
	MimeCategorizer & operator=( const MimeCategorizer & ) = delete;


    public:

	/**
	 * Get the singleton for this class. The first call to this will create
	 * it.
	 **/
	static MimeCategorizer * instance();

	/**
	 * Return the category name for a FileInfo item or "" if it doesn't fit
	 * into any of the available categories.
	 **/
	const QString & name( const FileInfo * item );

	/**
	 * Return the color for a FileInfo item or white if it doesn't fit
	 * into any of the available categories.
	 **/
	const QColor & color( const FileInfo * item );

	/**
	 * Return the MimeCategory for a filename or 0 if it doesn't fit into
	 * any of the available categories.
	 *
	 * If 'suffix_ret' is non-null, it returns the suffix used if the
	 * category was found by a suffix rule. If the category was not found
	 * or if a wildcard (rather than a suffix rule) matched, this returns an
	 * empty string.
	 **/
	const MimeCategory * category( const FileInfo * item, QString * suffix_ret );

	/**
	 * Return the MimeCategories list.
	 **/
	const MimeCategoryList & categories() const { return _categories; }

	/**
	 * Replace the existing category list wih a new list.  The new categories
	 * will also be written to the settings file.
	 **/
	void replaceCategories( const MimeCategoryList & categories );

	/**
	 * Return the (translated) name of the fixed category for executables.
	 **/
	QString executableCategoryName() const { return tr( "executable" ); }

	/**
	 * Return the (translated) name of the fixed category for symlinks.
	 **/
	QString symlinkCategoryName() const { return tr( "symlink" ); }


    protected:

	/**
	 * Clear all categories.
	 **/
	void clear();

	/**
	 * Read the MimeCategory parameter from the settings.
	 **/
	void readSettings();

	/**
	 * Return the MimeCategory for a FileInfo item or an empty dummy category
	 * if it doesn't fit into any of the available categories.
	 **/
	const MimeCategory * category( const FileInfo * item ) const;

	/**
	 * Return the MimeCategory for a filename or 0 if it doesn't fit into
	 * any of the available categories.
	 *
	 * If 'suffix_ret' is non-null, it returns the suffix used if the
	 * category was found by a suffix rule. If the category was not found
	 * or if a wildcard (rather than a suffix rule) matched, this returns an
	 * empty string.
	 **/
	const MimeCategory * category( const QString & filename, QString * suffix_ret ) const;

	/**
	 * Build the internal maps used for looking up file types.
	 **/
	void buildMaps();

	/**
	 * Add all patterns with no wildcards (exact filename match) to either
	 * the case-sensitive map or the case-insensitive map, and also to a
	 * global set including all the case-sensitive patterns, and all the
	 * case-insensitive patterns, both lowercased and uppercased.
	 * *
	 * This provides an extremely lookup for each filename.
	 **/
	void addExactKeys( const MimeCategory * category );

	/**
	 * Add all suffixes from both suffix lists in the category as keys with a value
	 * that is a pair containing an empty regular expression and a category.  One entry
	 * is created in the case-sensitive map for each case-sensitive suffix, one for an
	 * uppercased copy of each case-insensitive suffixi, and one for a lowercasedd copy
	 * of each case-insensitive suffix.  A lowercased version of each case-insensitive
	 * suffix is also added to the case-insensitive map to catch any strange files with
	 * mixed-case suffixes.
	 * su
	 *
	 * This provides a really fast lookup for each suffix.
	 **/
	void addSuffixKeys( const MimeCategory * category );

	/**
	 * Add regular expressions which include a suffix to the suffix maps.
	 * This allows a more specific wildcard to override a plain suffix match and
	 * reduces the need for matching filenames individually against every regular expression.
	 **/
	void addWildcardKeys( const MimeCategory * category );

	/**
	 * Add regular expression patterns which do not include a suffix pattern to a plain list
	 * of pairs, each containing the regular expression and the corresponding category.
	 **/
	void buildWildcardLists( const MimeCategory * category );

	/**
	 * Iterate over all categories to find categories by name.
	 **/
	const MimeCategory * findCategoryByName( const QString & categoryName ) const;

	/**
	 * Iterate over the pairs of regular expressions and categories that match a
	 * particular suffix.  Return the first category that matches either one of
	 * the regular expressions or has en empty regular expression, indicating a
	 * plain suffix pattern.
	 **/
	const MimeCategory * matchWildcardSuffix( const SuffixMatches & map,
						  const QString & filename,
						  const QString & suffix ) const;

	/**
	 * Iterate over the regular expression list trying each until the first
	 * match. Return the matched category or 0 if none matched.
	 **/
	const MimeCategory * matchWildcard( const QString & filename ) const;

	/**
	 * Make sure that the Executable and Symlink categories exist, in case
	 * they have been manually removed from the configuration file.
	 **/
	void ensureMandatoryCategories();

	/**
	 * Add default categories in case none were read from the settings.
	 **/
	void addDefaultCategory( const QString & name,
				 const QColor & color,
				 const QString & caseSensitivePatterns,
				 const QString & caseInsensitivePatterns );
	void addDefaultCategories();

	/**
	 * Create a new category and add it to the live list held in this class.
	 **/
	MimeCategory * create( const QString & name, const QColor & color );


	signals:

	/**
	 * Emitted when changes are applied from the settings dialogue.
	 **/
	void categoriesChanged();

    private:

	//
	// Data members
	//

	MimeCategoryList _categories;

	const MimeCategory * _executableCategory;
	const MimeCategory * _symlinkCategory;
	const MimeCategory   _emptyCategory;

	ExactMatches        _caseInsensitiveExact;
	ExactMatches        _caseSensitiveExact;
	SuffixMatches       _caseInsensitiveSuffixes;
	SuffixMatches       _caseSensitiveSuffixes;
	QList<WildcardPair> _wildcards;
	QBitArray           _caseInsensitiveLengths;
	QBitArray           _caseSensitiveLengths;

	QReadWriteLock      _lock;

    };	// class MimeCategorizer


}	// namespace QDirStat

#endif	// MimeCategorizer_h
