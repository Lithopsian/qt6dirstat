/*
 *   File name: Wildcard.h
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Author:    Ian Nartowicz
 */

#ifndef Wildcard_h
#define Wildcard_h

#include <QRegularExpression>


namespace QDirStat
{
    class FileInfo;
    class MimeCategory;

    /* An enum for the backported function wildcardToRegularExpression only added
     * in 5.12.  It also includes NonPathWildcardConversion added in 6.6.  Can be
     * added here unconditionally and only used as required.
     **/
    enum WildcardConversionOption {
	DefaultWildcardConversion = 0x0,
	UnanchoredWildcardConversion = 0x1,
	NonPathWildcardConversion = 0x2,
    };
    Q_DECLARE_FLAGS( WildcardConversionOptions, WildcardConversionOption )


    /**
     * Class for the wildcard-type of string matching used by the categorizer
     * and other filtering.  This is implemented by converting the wildcard pattern
     * string to an anchored non-capturing full-syntax regular expression.
     *
     * A limited set of functions are provided for testing a string and retrieving
     * the original pattern.
     **/

    class Wildcard : public QRegularExpression
    {
    protected:

	/**
	 * Constructor with a pattern.  The wildcard pattern is converted to an
	 * anchored regular expression that doesn't capture any substrings.  The
	 * default is case-sensitive and follows PCRE syntax with no modifiers.
	 * Most pattern options don't make much sense in the case of wildcards,
	 * and CaseInsensitiveOption is specified using the CaseInsensitiveWildcard
	 * sub-class.
	 *
	 * This constructor is protected: access using CaseInsensitiveWildcard or
	 * CaseSensitiveWildcard.
	 **/
	Wildcard( const QString & pattern, PatternOption options ):
	    QRegularExpression{ wildcardRegularExpression( pattern, options ) },
	    _pattern{ pattern }
	{}


    public:

	/**
	 * For the constructor with no pattern and copy constructor, just use the base
	 * constructors.
	 **/
	using QRegularExpression::QRegularExpression;

	/**
	 * Returns the original (unanchored, unconverted) pattern used yo
	 * construct this instance.
	 **/
	const QString & pattern() const { return _pattern; }

	/**
	 * Returns whether this rule is case-sensitive.
	 **/
	bool caseInsensitive() const
	    { return patternOptions() & QRegularExpression::CaseInsensitiveOption; }

	/**
	 * Returns whether the given string matches this regular expression.
	 **/
	bool isMatch( const QString & string ) const { return match( string ).hasMatch(); }

	/**
	 * Convenience function to match the QRegExp syntax.  Matches in this class
	 * are always "exact" anchored matches.
	 **/
	bool exactMatch( const QString & string ) const { return isMatch( string ); }

	/**
	 * Returns whether this wildcard has an empty QRegularExpression
	 **/
	bool isEmpty() const { return pattern().isEmpty(); }

	/**
	 * Returns whether 'pattern' contains characters that would be interpreted as
	 * wildcards.
	 **/
	static bool isWildcard( const QString & pattern )
	    { return pattern.contains( u'*' ) || pattern.contains( u'?' ) || pattern.contains( u'[' ); }

	/**
	 * Helper for the rather long-winded way that a QRegularExpression is made
	 * from a wildcard-style string.
	 **/
	static QRegularExpression wildcardRegularExpression( const QString  & pattern,
	                                                     PatternOptions   options )
	    { return QRegularExpression{ wildcardToRegularExpression( pattern ), options }; }

	/**
	 * This function was added to Qt in 5.12.  It converts any valid regular
	 * expression pattern string into an anchored string.  It doesn't check
	 * whether the pattern is already anchored.
	 **/
#if QT_VERSION < QT_VERSION_CHECK( 5, 12, 0 )
	static QString anchoredPattern( const QString & expression )
	    { return QLatin1String{ "\\A(?:" } + expression + QLatin1String{ ")\\z" }; }
#endif

	/**
	 * This function was added in 5.12, but its behaviour until 6.6 was
	 * to not match slashes as part of any wildcard.  This behaviour is
	 * undesirable for matching paths and not the way that QRegExp behaved,
	 * so the function is backported here.
	 **/
#if QT_VERSION < QT_VERSION_CHECK( 6, 6, 0 )
	static QString wildcardToRegularExpression( const QString & pattern,
	                                            QDirStat::WildcardConversionOptions options = NonPathWildcardConversion);
#endif

	QString _pattern;

    };	// class Wildcard



    /**
     * Convenience class for making a case-sensitive wildcard regular expression.
     **/
    class CaseSensitiveWildcard final : public Wildcard
    {
    public:

	/**
	 * Constructor
	 **/
	CaseSensitiveWildcard( const QString & pattern ):
	    Wildcard{ pattern, NoPatternOption }
	{}

    };	// class CaseSensitiveWildcard



    /**
     * Convenience class for making a case-insensitive wildcard regular expression.
     **/
    class CaseInsensitiveWildcard final : public Wildcard
    {
    public:

	/**
	 * Constructor
	 **/
	CaseInsensitiveWildcard( const QString & pattern ):
	    Wildcard{ pattern, CaseInsensitiveOption }
	{}

    };	// class CaseInsensitiveWildcard



    /**
     * Replacement for QPair so that the members can be accessed by
     * meaningful names rather than just 'first' and 'second'.
     *
     * isEmpty() is a shorthand for wildcard.pattern().isEmpty && category = 0
     * matches() checks that 'item' matches both the wildcard and category
     **/
    struct WildcardCategory
    {
	Wildcard wildcard;
	const MimeCategory * category;
	bool isEmpty() const { return wildcard.isEmpty() && !category; }
	bool matches( const FileInfo * item ) const;
    };

}	// namespace QDirStat

#endif	// Wildcard_h
