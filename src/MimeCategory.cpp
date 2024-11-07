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
#include "Typedefs.h" // _L1
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
	const int suffixStartIndex = pattern.lastIndexOf( "*."_L1 );
	if ( suffixStartIndex < 1 || suffixStartIndex >= pattern.size() - 2 )
	    return false;

	return !isWildcard( pattern.mid( suffixStartIndex + 2 ) );
    }


    /**
     * Return a list of suffixes formatted for display.  Each suffix is
     * modified to begin with "*.".
     **/
    QStringList humanReadableSuffixList( const QStringList & suffixList )
    {
	QStringList result;

	for ( const QString & suffix : suffixList )
	    result << ( "*."_L1 + suffix );

	return result;
    }

} // namespace



void MimeCategory::addPattern( QStringList & patternList, const QString & pattern )
{
    // Append pattern if not empty and not already there
    if ( !pattern.isEmpty() && !patternList.contains( pattern ) )
	patternList << pattern;
}

/*
void MimeCategory::addExactMatch( const QString & pattern, Qt::CaseSensitivity caseSensitivity )
{
    // Pick the correct suffix list
    QStringList & patternList =
	caseSensitivity == Qt::CaseSensitive ?_caseSensitiveExactList : _caseInsensitiveExactList;

    // Append pattern if not empty and not already there
    if ( !pattern.isEmpty() && !patternList.contains( pattern ) )
	patternList << pattern;
}


void MimeCategory::addSuffix( const QString & suffix, Qt::CaseSensitivity caseSensitivity )
{
    // Pick the correct suffix list
    QStringList & patternList =
	caseSensitivity == Qt::CaseSensitive ? _caseSensitiveSuffixList : _caseInsensitiveSuffixList;

    // Append suffix if not empty and not already there
    if ( !suffix.isEmpty() && !patternList.contains( suffix ) )
	patternList << suffix;
}


void MimeCategory::addWildcardSuffix( const QString & pattern, Qt::CaseSensitivity caseSensitivity )
{
    // Pick the correct suffix list
    QStringList & patternList =
	caseSensitivity == Qt::CaseSensitive ? _caseSensitiveWildcardSuffixList : _caseInsensitiveWildcardSuffixList;

    // Append suffix if not empty and not already there
    if ( !pattern.isEmpty() && !patternList.contains( pattern ) )
	patternList << pattern;
}


void MimeCategory::addWildcard( const QString & pattern, Qt::CaseSensitivity caseSensitivity )
{
    // Pick the correct wildcard list
    QStringList & patternList =
	caseSensitivity == Qt::CaseSensitive ? _caseSensitiveWildcardList : _caseInsensitiveWildcardList;

    // Append wildcard if not empty and not already there
    if ( !pattern.isEmpty() && !patternList.contains( pattern ) )
	patternList << pattern;
}
*/

void MimeCategory::addPatterns( const QStringList & patterns, Qt::CaseSensitivity caseSensitivity )
{
    for ( const QString & rawPattern : patterns )
    {
	QString pattern = rawPattern.trimmed();

	if ( !isWildcard( pattern ) )
	    addPattern( exactList( caseSensitivity ), pattern );
	else if ( isSuffixPattern( pattern ) )
	    addPattern( suffixList( caseSensitivity ), pattern.remove( 0, 2 ) );
	else if ( isWildcardSuffix( pattern ) )
	    addPattern( wildcardSuffixList( caseSensitivity ), pattern );
	else
	    addPattern( wildcardList( caseSensitivity ), pattern );
    }
}


void MimeCategory::clear()
{
    _caseInsensitiveExactList.clear();
    _caseSensitiveExactList.clear();
    _caseInsensitiveSuffixList.clear();
    _caseSensitiveSuffixList.clear();
    _caseInsensitiveWildcardSuffixList.clear();
    _caseSensitiveWildcardSuffixList.clear();
    _caseInsensitiveWildcardList.clear();
    _caseSensitiveWildcardList.clear();
}


QStringList MimeCategory::humanReadablePatternList( Qt::CaseSensitivity caseSensitivity ) const
{
    QStringList exact = exactList( caseSensitivity );
    exact.sort( caseSensitivity );

    QStringList wildcardSuffixes = wildcardSuffixList( caseSensitivity );
    wildcardSuffixes.sort( caseSensitivity );

    QStringList suffixes = humanReadableSuffixList( suffixList( caseSensitivity ) );
    suffixes.sort( caseSensitivity );

    QStringList wildcards = wildcardList( caseSensitivity );
    wildcards.sort( caseSensitivity );

    return exact + wildcardSuffixes + suffixes + wildcards;
}

/*
bool MimeCategory::contains( const QString & rawPattern, Qt::CaseSensitivity caseSensitivity ) const
{
    const QString pattern = rawPattern.trimmed();

    if ( !isWildcard( pattern ) )
    {
	if ( exactList( Qt::CaseInsensitive ).contains( pattern, Qt::CaseInsensitive ) )
	    return true;

	if ( exactList( Qt::CaseSensitive ).contains( pattern, caseSensitivity ) )
	    return true;
    }
    else if ( isSuffixPattern( pattern ) )
    {
	if ( suffixList( Qt::CaseInsensitive ).contains( pattern.mid( 2 ), Qt::CaseInsensitive ) )
	    return true;

	if ( suffixList( Qt::CaseSensitive ).contains( pattern.mid( 2 ), caseSensitivity ) )
	    return true;
    }
    else if ( isWildcardSuffix( pattern ) )
    {
	if ( wildcardSuffixList( Qt::CaseInsensitive ).contains( pattern, Qt::CaseInsensitive ) )
	    return true;

	if ( wildcardSuffixList( Qt::CaseSensitive ).contains( pattern, caseSensitivity ) )
	    return true;
    }
    else
    {
	if ( wildcardList( Qt::CaseInsensitive ).contains( pattern, Qt::CaseInsensitive ) )
	    return true;

	if ( wildcardList( Qt::CaseSensitive ).contains( pattern, caseSensitivity ) )
	    return true;
    }

    return false;
}
*/
