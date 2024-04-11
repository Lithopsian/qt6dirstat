/*
 *   File name: ExcludeRules.h
 *   Summary:	Support classes for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#ifndef ExcludeRules_h
#define ExcludeRules_h

#include <QString>
#include <QList>
#include <QTextStream>

#include "Wildcard.h"
#include "SettingsHelpers.h"


namespace QDirStat
{
    class DirInfo;

    /**
     * One single exclude rule to check text (file names) against.
     **/
    class ExcludeRule: public QRegularExpression
    {
    public:

	/**
	 * Enum for the three supported types  of pattern.  These match the three
	 * values from QRegExp::PatternSyntax for compatibility with existing
	 * exclude rules configurations.
	 **/
	enum PatternSyntax {
	    RegExp = 0,
	    Wildcard = 1,
	    FixedString = 2
	};

	/**
	 * Constructor from a QString. The string will be used as a regexp.
	 *
	 * 'patternSyntax' is a value from the PatternSyntax enum indicating the
	 * way that 'pattern' should be interpreted for matching.
	 *
	 * 'caseSensitive' indicates whether the matching should be case-
	 * sensitive or case-insensitive.
	 *
	 * 'useFullPath' indicates if this exclude rule uses the full path
	 * ('true') or only the file name without path ('false') for matching.
         *
         * 'checkAnyFileChild' specifies whether the non-directory children of
         * the directory should be used for matching rather than the path or
         * name of the directory itself. That makes it possible to exclude a
         * directory that contains a file ".nobackup".
	 **/
	ExcludeRule( PatternSyntax   patternSyntax,
	             const QString & pattern,
		     bool            caseSensitive,
                     bool            useFullPath,
                     bool            checkAnyFileChild ):
	    QRegularExpression ( formatPattern( patternSyntax, pattern ), makePatternOptions( caseSensitive ) ),
	    _patternSyntax { patternSyntax },
	    _pattern { pattern },
	    _useFullPath { useFullPath },
	    _checkAnyFileChild { checkAnyFileChild }
	{}

	/**
	 * Check a file name with or without its full path against this exclude
	 * rule: If useFullPath() is 'true', the 'fullPath' parameter is used
	 * for matching, if it is 'false', 'fileName' is used.
	 *
	 * Returns 'true' if the string matches, i.e. the file should be
	 * excluded.
	 **/
	bool match( const QString & fullPath, const QString & fileName ) const;

        /**
         * If this exclude rule has the 'checkAnyFileChild' flag set, check if
         * any non-directory direct child of 'dir' (or of its dot entry if it
         * has one) matches the rule.
         *
         * This returns 'false' immediately if 'checkAnyFileChild' is not set.
         **/
        bool matchDirectChildren( DirInfo * dir ) const;

	/**
	 * Return 'true' if this exclude rule uses the full path to match
	 * against, 'false' if it only uses the file name without path.
	 **/
	bool useFullPath() const { return _useFullPath; }

	/**
	 * Set the 'full path' flag.
	 **/
	void setUseFullPath( bool useFullPath ) { _useFullPath = useFullPath; }

        /**
         * Return 'true' if this exclude rule should be used to check against
         * any direct non-directory child of a directory rather than just the
         * directory name or path.
         **/
        bool checkAnyFileChild() const { return _checkAnyFileChild; }

        /**
         * Set the 'check any file child' flag.
         **/
        void setCheckAnyFileChild( bool check ) { _checkAnyFileChild = check; }

        /**
         * Set the matching syntax for this rule.
         **/
        void setPatternSyntax( PatternSyntax patternSyntax );

        /**
         * Returns the matching syntax for this rule.
         **/
        PatternSyntax patternSyntax() const { return _patternSyntax; }

        /**
         * Set the pattern for this rule.
         **/
        void setPattern( const QString & pattern );

        /**
         * Return the pattern used to construct this rule.
         **/
        const QString & pattern() const { return _pattern; }

        /**
         * Set whether this rule is case-sensitive.
         **/
        void setCaseSensitive( bool caseSensitive );

        /**
         * Returns whether this rule is case-sensitive.
         **/
        bool caseSensitive() const { return !( patternOptions() & CaseInsensitiveOption ); }

	/**
	 * Comparison operator for two exclude rules.
	 **/
	bool operator!=( const ExcludeRule * other );


    private:

	/**
	* Returns whether the given string matches this regular expression.
	* Note that RegExp patterns are not automatically anchored and may match
	* just a portion of the string, which differs from QRegExp::exactMatch().
	* FixedString and Wildcard patterns in this class are automatically
	* anchored and so all matches are "exact".
	**/
	bool isMatch( const QString & string ) const
	    { return QRegularExpression::match( string ).hasMatch(); }

	/**
	* Constructs the QRegularExpression::PatternOptions flags, currently just
	* repesenting case-sensitivity.
	**/
	static PatternOptions makePatternOptions( bool caseSensitive )
	    { return caseSensitive ? NoPatternOption : CaseInsensitiveOption; }

	/**
	* Formats a pattern string depending on the specified matching syntax.  Fixed
	* string patterns are escaped and anchored, wildcard patterns are converted
	* to an anchored regular expression, and regular expression patterns are
	* unchnged.
	**/
	static QString formatPattern( PatternSyntax   patternSyntax,
				      const QString & pattern );


	PatternSyntax _patternSyntax;
	QString       _pattern;
	bool          _useFullPath;
        bool          _checkAnyFileChild;
    };


    typedef QList<ExcludeRule *> ExcludeRuleList;
    typedef ExcludeRuleList::const_iterator ExcludeRuleListIterator;


    /**
     * Container for multiple exclude rules. This can be used as a singleton
     * class. Use the static methods or instance() to access the singleton.
     *
     * Normal usage:
     *
     *	   if ( ExcludeRules::instance()->match( filename ) )
     *	   {
     *	       // exclude this file
     *	   }
     **/
    class ExcludeRules
    {

    public:

	/**
	 * Constructor that initialises the rule list from the settings.
	 *
	 * Applications will generally want to use instance() instead to create
	 * and use a global object of this class.  MainWindowUnpkg creates and
	 * uses a separate ExcludeRules object for its own set of exclude rules
	 * each time it is called.
	 **/
	ExcludeRules();

	/**
	 * Constructor that initialises the rules from a given list, with the
	 * given syntax and options.  Thos is used by MainWindowUnpkg to
	 * create its own temporary set of rules.
	 */
	ExcludeRules( const QStringList & paths,
		      ExcludeRule::PatternSyntax patternSyntax,
		      bool caseSensitive,
		      bool useFullPath,
		      bool checkAnyFileChild );

	/**
	 * Destructor.
	 **/
	~ExcludeRules();

	/**
	 * Suppress copy and assignment constructors (wouldn't do a deep copy)
	 **/
	ExcludeRules( const ExcludeRules & ) = delete;
	ExcludeRules & operator=( const ExcludeRules & ) = delete;

	/**
	 * Check a file name against the exclude rules. Each exclude rule
	 * decides individually based on its configuration if it checks against
	 * the full path or against the file name without path, so both have to
	 * be provided here.
	 *
	 * This will return 'true' if the text matches any rule.
	 *
	 * Note that this operation will move current().
	 **/
	bool match( const QString & fullPath, const QString & fileName ) const;

        /**
         * Check the direct non-directory children of 'dir' against any rules
         * that have the 'checkAnyFileChild' flag set.
         *
	 * This will return 'true' if the text matches any rule.
         **/
        bool matchDirectChildren( DirInfo * dir ) const;

	/**
	 * Clear (delete) all exclude rules.
	 **/
	void clear();

	/**
	 * Return 'true' if the exclude rules are empty, i.e. if there are no
	 * exclue rules, 'false' otherwise.
	 **/
	bool isEmpty() const { return _rules.isEmpty(); }

	/**
	 * Return a const iterator for the first exclude rule.
	 **/
	ExcludeRuleListIterator cbegin() const { return _rules.constBegin(); }

	/**
	 * Return a const iterator for past the last exclude rule.
	 **/
	ExcludeRuleListIterator cend() const { return _rules.constEnd(); }

	/**
	 * Write all exclude rules to the settings file.
	 **/
	static void writeSettings( const ExcludeRuleList & rules );


    protected:

	/**
	 * Create an exclude rule and add it to this rule set.
	 **/
	void add( ExcludeRule::PatternSyntax   patternSyntax,
		  const QString              & pattern,
		  bool                         caseSensitive,
                  bool                         useFullPath,
                  bool                         checkAnyFileChild );

	/**
	 * Clear all existing exclude rules and read exclude rules from the
	 * settings file.
	 **/
	void readSettings();

        /**
         * Add some default rules.
         **/
        void addDefaultRules();


    private:

	ExcludeRuleList _rules;
    };


    /**
     * Print the regexp of a FileInfo in a debug stream.
     **/
    inline QTextStream & operator<< ( QTextStream & stream, const ExcludeRule * rule )
    {
	if ( rule )
	    stream << "<ExcludeRule \"" << rule->pattern() << "\""
                   << ( rule->useFullPath() ? " (full path)" : "" )
                   << ">";
	else
	    stream << "<NULL ExcludeRule *>";

	return stream;
    }

}	// namespace QDirStat


#endif // ifndef ExcludeRules_h

