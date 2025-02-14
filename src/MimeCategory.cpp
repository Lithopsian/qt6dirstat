/*
 *   File name: MimeCategory.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "MimeCategory.h"
#include "Logger.h"
#include "Wildcard.h"


using namespace QDirStat;


namespace
{
    /**
     * Return 'true' if 'pattern' contains wildcard characters.
     **/
    bool isWildcard( const QString & pattern )
    {
	return Wildcard::isWildcard( pattern );
    }


    /**
     * Return 'true' if 'pattern' is a simple suffix pattern, i.e. it
     * starts with "*." and does not contain any more wildcard characters.
     **/
    bool isSuffixPattern( const QString & pattern )
    {
	if ( pattern.size() < 3 || !pattern.startsWith( "*."_L1 ) )
	    return false;

	return ( !isWildcard( pattern.mid( 2 ) ) );
    }


    /**
     * Return 'true' if 'pattern' includes a suffix with other characters,
     * e.g. "lib*.a".
     **/
    bool isWildcardSuffix( const QString & pattern )
    {
	const auto suffixStartIndex = pattern.lastIndexOf( "*."_L1 );
	if ( suffixStartIndex < 1 || suffixStartIndex >= pattern.size() - 2 )
	    return false;

	return !isWildcard( pattern.mid( suffixStartIndex + 2 ) );
    }


    /**
     * Add 'pattern' to 'patternList'.
     **/
    void addPattern( QStringList & patternList, const QString & pattern )
    {
	// Append a pattern if not empty and not already there
	if ( !pattern.isEmpty() && !patternList.contains( pattern ) )
	    patternList << pattern;
    }


    /**
     * Set patterns for this category.  Replace any existing
     * patterns with 'patterns', into the given lists, sorted
     * according to 'caseSensitivity'.
     **/
    void replacePatterns( const QStringList & patterns,
                          QStringList       & exact,
                          QStringList       & suffixes,
                          QStringList       & wildcardSuffixes,
                          QStringList       & wildcards,
                          Qt::CaseSensitivity caseSensitivity )
    {
	exact.clear();
	suffixes.clear();
	wildcardSuffixes.clear();
	wildcards.clear();

	for ( const QString & pattern : patterns )
	{
//	    QString pattern = rawPattern.trimmed();

	    if ( !isWildcard( pattern ) )
		addPattern( exact, pattern );
	    else if ( isSuffixPattern( pattern ) )
		addPattern( suffixes, pattern );
	    else if ( isWildcardSuffix( pattern ) )
		addPattern( wildcardSuffixes, pattern );
	    else
		addPattern( wildcards, pattern );
	}

	exact.sort( caseSensitivity );
	suffixes.sort( caseSensitivity );
	wildcardSuffixes.sort( caseSensitivity );
	wildcards.sort( caseSensitivity );
    }

} // namespace


void MimeCategory::setPatterns( const QStringList & caseInsensitivePatterns,
                                const QStringList & caseSensitivePatterns )
{
    replacePatterns( caseInsensitivePatterns,
                     _caseInsensitiveExactList,
                     _caseInsensitiveSuffixList,
                     _caseInsensitiveWildcardSuffixList,
                     _caseInsensitiveWildcardList,
                     Qt::CaseInsensitive );

    replacePatterns( caseSensitivePatterns,
                     _caseSensitiveExactList,
                     _caseSensitiveSuffixList,
                     _caseSensitiveWildcardSuffixList,
                     _caseSensitiveWildcardList,
                     Qt::CaseSensitive );
}


QStringList MimeCategory::patterns( Qt::CaseSensitivity caseSensitivity ) const
{
    const bool caseSensitive = caseSensitivity == Qt::CaseSensitive;
    const QStringList & exact =
	caseSensitive ? _caseSensitiveExactList : _caseInsensitiveExactList;
    const QStringList & wildcardSuffixes =
	caseSensitive ? _caseSensitiveWildcardSuffixList : _caseInsensitiveWildcardSuffixList;
    const QStringList & suffixes =
	caseSensitive ? _caseSensitiveSuffixList : _caseInsensitiveSuffixList;
    const QStringList & wildcards =
	caseSensitive ? _caseSensitiveWildcardList : _caseInsensitiveWildcardList;

    return exact + wildcardSuffixes + suffixes + wildcards;
}
