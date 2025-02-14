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
#include <QVector>

#include "Wildcard.h"


namespace QDirStat
{
    class FileInfo;
    class MimeCategory;

    /**
     * Suffix matches return a list (possibly with only one entry) of pairs.
     * Each pair contains a regular expression and the category it matches to.
     * The regular expression may be empty, indicating a plain suffix that
     * matches any file with that suffix.  Pairs with an empty regular
     * expression will always be the last pair in a list.
     **/
    typedef QHash<QString, const MimeCategory *>  ExactMatches;
#if QT_VERSION < QT_VERSION_CHECK( 5, 7, 0 )
    typedef QMultiMap<QString, WildcardCategory> SuffixMatches;
#else
    typedef QMultiHash<QString, WildcardCategory> SuffixMatches;
#endif
    typedef QVector<const MimeCategory *>         MimeCategoryList;
    typedef MimeCategoryList::const_iterator      MimeCategoryIterator;
    typedef QVector<WildcardCategory>             WildcardList;


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
     * expression representing the plain suffix match (assuming there was one).  The loop
     * is optimized for the most common cases of a single suffix or no suffix.
     *
     * Finally, any file which has not been matched is tested against a list of regular
     * expressions in each category.  This is relatively very slow and hopefully there
     * will be both few regular expressions that don't include suffixes and few file
     * which need to be tested against them.
     **/
    class MimeCategorizer final : public QObject
    {
	Q_OBJECT

	/**
	 * Constructor.
	 *
	 * This is a singleton class; use instance() to get the categorizer
	 * object.
	 **/
	MimeCategorizer() { readSettings(); }

	/**
	 * Destructor.
	 **/
	~MimeCategorizer() override;


    public:

	/**
	 * Get the singleton for this class. The first call to this function
	 * will create the singleton.
	 **/
	static MimeCategorizer * instance();

	/**
	 * Return the category name for a FileInfo item or an empty string if
	 * it doesn't fit into any of the available categories.
	 *
	 * This function is mutex-protected against modification of the
	 * pattern maps because it will be called from inside treemap render
	 * threads.
	 **/
	QString name( const FileInfo * item );

	/**
	 * Return the color for a FileInfo item or white if it doesn't fit
	 * into any of the available categories.
	 *
	 * This function is mutex-protected although it is only currently
	 * called from the main thread, the same thread that makes any
	 * modifications,
	 **/
	QColor color( const FileInfo * item );

	/**
	 * Return the MimeCategory for a filename or 0 if it doesn't fit into
	 * any of the available categories.
	 *
	 * The exact raw pattern used to categorise the file is returned in
	 * 'pattern' and 'caseInsensitive' is set to indicate whether pattern
	 * was matched case-insensitively or not.
	 *
	 * Extra checks are made for symlinks and executable files.  These
	 * always return an empty string for the suffix even if the file has
	 * an extension.
	 *
	 * This function is mutex-protected although it is only currently
	 * called from the main thread, the same thread that makes any
	 * modifications,
	 **/
	const MimeCategory * category( const FileInfo * item,
	                               QString        & pattern,
	                               bool           & caseInsensitive );

	/**
	 * Return a const iterator for the first category.
	 **/
	MimeCategoryIterator begin() const { return cbegin(); }
	MimeCategoryIterator cbegin() const { return _categories.cbegin(); }

	/**
	 * Return a const iterator for past the last category.
	 **/
	MimeCategoryIterator end() const { return cend(); }
	MimeCategoryIterator cend() const { return _categories.cend(); }

	/**
	 * Replace the existing category list wih a new list.  The new
	 * categories will also be written to the settings file.
	 **/
	void replaceCategories( const MimeCategoryList & categories );

	/**
	 * Return the (translated) name of the fixed category for executables.
	 **/
	static QString executableCategoryName() { return tr( "executable" ); }

	/**
	 * Return the (translated) name of the fixed category for symlinks.
	 **/
	static QString symlinkCategoryName() { return tr( "symlink" ); }


    signals:

	/**
	 * Emitted when changes are applied from the settings dialogue.
	 **/
	void categoriesChanged();


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
	 * Return the MimeCategory for a FileInfo item or an empty dummy
	 * category if it doesn't fit into any of the available categories.
	 *
	 * Extra checks are made for symlinks and executable files.
	 **/
	const MimeCategory * category( const FileInfo * item ) const;

	/**
	 * Return the MimeCategory for a filename or 0 if it doesn't fit into
	 * any of the available categories.
	 *
	 * If 'pattern_ret' is non-null, it returns the suffix used if the
	 * category was found by a suffix rule. If the category was not found
	 * or if a wildcard (rather than a suffix rule) matched, the suffix
	 * is not set; the caller is responsible for initialising the suffix
	 * to a suitable default value.
	 **/
	const MimeCategory * category( const QString & filename,
	                               QString       * pattern_ret,
	                               bool          * caseInsensitive_ret ) const;

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
	 * that is a pair containing an empty regular expression and a category.  One
	 * entry is created in the case-sensitive map for each case-sensitive suffix,
	 * one for an uppercased copy of each case-insensitive suffix, and one for a
	 * lowercased copy of each case-insensitive suffix.  A lowercased version of
	 * each case-insensitive suffix is also added to the case-insensitive map to
	 * catch any strange files with mixed-case suffixes.
	 *
	 * This provides a really fast lookup for each suffix.
	 **/
	void addSuffixKeys( const MimeCategory * category );

	/**
	 * Add regular expressions which include a suffix to the suffix maps.
	 * This allows a more specific wildcard to override a plain suffix match and
	 * reduces the need for matching filenames individually against every regular
	 * expression.
	 **/
	void addWildcardSuffixKeys( const MimeCategory * category );

	/**
	 * Add regular expression patterns which do not include a suffix pattern to a
	 * plain list of pairs, each containing the regular expression and the
	 * corresponding category.
	 **/
	void buildWildcardLists( const MimeCategory * category );

	/**
	 * Iterate over the pairs of regular expressions and categories that match a
	 * particular suffix.  Return the first category that matches either one of
	 * the regular expressions or has en empty regular expression, indicating a
	 * plain suffix pattern.
	 **/
	const WildcardCategory * matchWildcardSuffix( const SuffixMatches & map,
	                                              const QString       & filename,
	                                              const QString       & suffix ) const;

	/**
	 * Iterate over the regular expression list trying each until the first
	 * match. Return the matched category or 0 if none matched.
	 **/
	const WildcardCategory * matchWildcard( const QString & filename ) const;

	/**
	 * Make sure that the Executable and Symlink categories exist, in case
	 * they have been manually removed from the configuration file.
	 **/
	void ensureMandatoryCategories();

	/**
	 * Create a new category and add it to the live list held in this class.
	 * The patterns are passed as QStringLists.
	 **/
	const MimeCategory * addCategory( const QString     & name,
	                                  const QColor      & color,
	                                  const QStringList & caseInsensitivePatterns,
	                                  const QStringList & caseSensitivePatterns );

	/**
	 * Add a category from a name, colour, and comma-delimited patterns
	 * strings.
	 **/
	const MimeCategory * addCategory( const QString & name,
	                                  const QColor  & color,
	                                  const QString & caseInsensitivePatterns,
	                                  const QString & caseSensitivePatterns );

	/**
	 * Add default categories in case none were read from the settings.
	 **/
	void addDefaultCategories();

	/**
	 * Return whether 'length' has the bit 'lengths[length]' set to true.
	 **/
	bool testBit( const QBitArray & lengths, QString::size_type length ) const
	    { return length < lengths.size() && lengths.testBit( length ); }


    private:

	MimeCategoryList     _categories;

	const MimeCategory * _executableCategory;
	const MimeCategory * _symlinkCategory;

	ExactMatches        _caseInsensitiveExact;
	ExactMatches        _caseSensitiveExact;
	SuffixMatches       _caseInsensitiveSuffixes;
	SuffixMatches       _caseSensitiveSuffixes;
	WildcardList        _wildcards;
	QBitArray           _caseInsensitiveLengths;
	QBitArray           _caseSensitiveLengths;

	QReadWriteLock      _lock;

    };	// class MimeCategorizer

}	// namespace QDirStat

#endif	// MimeCategorizer_h
