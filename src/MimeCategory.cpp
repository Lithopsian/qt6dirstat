/*
 *   File name: MimeCategory.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "MimeCategory.h"
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
	if ( !pattern.startsWith( "*."_L1 ) )
	    return false;

	const QString rest = pattern.mid( 2 ); // Without leading "*."

	return ( !isWildcard( rest ) );
    }


    /**
     * Return 'true' if 'pattern' includes a suffix with other characters,
     * e.g. "lib*.a".
     **/
    bool isWildcardWithSuffix( const QString & pattern )
    {
	const QStringList list = pattern.split( "*."_L1, Qt::SkipEmptyParts );

	return list.size() > 1 && !isWildcard( list.last() );
    }


    QStringList humanReadableSuffixList( const QStringList & suffixList )
    {
	QStringList result;

	for ( const QString & suffix : suffixList )
	    result << ( "*."_L1 + suffix );

	return result;
    }

} // namespace



void MimeCategory::addExactMatch( const QString       & rawPattern,
                                  Qt::CaseSensitivity   caseSensitivity )
{
    // Pick the correct suffix list
    QStringList & patternList =
	caseSensitivity == Qt::CaseSensitive ?_caseSensitiveExactList : _caseInsensitiveExactList;

    // Append pattern if not empty and not already there
    QString pattern = rawPattern.trimmed();
    if ( !pattern.isEmpty() && !patternList.contains( pattern ) )
	patternList << pattern;
}


void MimeCategory::addSuffix( const QString       & rawSuffix,
                              Qt::CaseSensitivity   caseSensitivity )
{
    // Normalize suffix: Remove leading "*." or "."
    QString suffix = rawSuffix.trimmed();
    if ( suffix.startsWith( "*."_L1 ) )
	suffix.remove( 0, 2 );
    else if ( suffix.startsWith( u'.' ) )
	suffix.remove( 0, 1 );

    // Pick the correct suffix list
    QStringList & patternList =
	caseSensitivity == Qt::CaseSensitive ? _caseSensitiveSuffixList : _caseInsensitiveSuffixList;

    // Append suffix if not empty and not already there
    if ( !suffix.isEmpty() && !patternList.contains( suffix ) )
	patternList << suffix;
}


void MimeCategory::addWildcardSuffix( const QString       & rawPattern,
                                      Qt::CaseSensitivity   caseSensitivity )
{
    // Pick the correct suffix list
    QStringList & patternList =
	caseSensitivity == Qt::CaseSensitive ? _caseSensitiveWildcardSuffixList : _caseInsensitiveWildcardSuffixList;

    // Append suffix if not empty and not already there
    QString pattern = rawPattern.trimmed();
    if ( !pattern.isEmpty() && !patternList.contains( pattern ) )
	patternList << pattern;
}


void MimeCategory::addWildcard( const QString       & rawPattern,
                                Qt::CaseSensitivity   caseSensitivity )
{
    // Pick the correct wildcard list
    QStringList & patternList =
	caseSensitivity == Qt::CaseSensitive ? _caseSensitiveWildcardList : _caseInsensitiveWildcardList;

    // Append wildcard if not empty and not already there
    const QString pattern = rawPattern.trimmed();
    if ( !pattern.isEmpty() && !patternList.contains( pattern ) )
	patternList << pattern;
}


void MimeCategory::addPattern( const QString       & rawPattern,
                               Qt::CaseSensitivity   caseSensitivity )
{
    const QString pattern = rawPattern.trimmed();

    if ( !isWildcard( pattern ) )
	addExactMatch( pattern, caseSensitivity );
    else if ( isSuffixPattern( pattern ) )
	addSuffix( pattern, caseSensitivity );
    else if ( isWildcardWithSuffix( pattern ) )
	addWildcardSuffix( pattern, caseSensitivity );
    else
	addWildcard( pattern, caseSensitivity );
}


void MimeCategory::addPatterns( const QStringList   & patterns,
                                Qt::CaseSensitivity   caseSensitivity )
{
    for ( const QString & rawPattern : patterns )
    {
	const QString pattern = rawPattern.trimmed();
	if ( !pattern.isEmpty() )
	    addPattern( pattern, caseSensitivity );
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
    QStringList exactList =
	caseSensitivity == Qt::CaseSensitive ? _caseSensitiveExactList : _caseInsensitiveExactList;
    exactList.sort( caseSensitivity );

    QStringList wildcardSuffixList =
	caseSensitivity == Qt::CaseSensitive ? _caseSensitiveWildcardSuffixList : _caseInsensitiveWildcardSuffixList;
    wildcardSuffixList.sort( caseSensitivity );

    QStringList suffixList =
	caseSensitivity == Qt::CaseSensitive ? _caseSensitiveSuffixList : _caseInsensitiveSuffixList;
    suffixList.sort( caseSensitivity );

    QStringList wildcardList =
	caseSensitivity == Qt::CaseSensitive ? _caseSensitiveWildcardList : _caseInsensitiveWildcardList;
    wildcardList.sort( caseSensitivity );

    return exactList + wildcardSuffixList + humanReadableSuffixList( suffixList ) + wildcardList;
}
