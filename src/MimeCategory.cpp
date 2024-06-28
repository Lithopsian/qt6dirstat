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


using namespace QDirStat;


namespace
{
    /**
     * Return 'true' if 'pattern' contains no wildcard characters.
     **/
    bool isWildcard( const QString & pattern )
    {
	return pattern.contains( u'*' ) || pattern.contains( u'?' ) || pattern.contains( u'[' );
    }


    /**
     * Return 'true' if 'pattern' is a simple suffix pattern, i.e. it
     * starts with "*." and does not contain any more wildcard characters.
     **/
    bool isSuffixPattern( const QString & pattern )
    {
	if ( !pattern.startsWith( "*."_L1 ) )
	    return false;

	const QString rest = pattern.mid( 2, -1 ); // Without leading "*."

	return ( !isWildcard( rest ) );
    }


    /**
     * Return 'true' if 'pattern' includes a suffix plus other wildcards,
     * e.g. "lib*.a"
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
    QString pattern = rawPattern.trimmed();

    if ( caseSensitivity == Qt::CaseInsensitive )
	pattern = pattern.toLower();

    // Pick the correct suffix list
    QStringList & patternList =
	caseSensitivity == Qt::CaseSensitive ?_caseSensitiveExactList : _caseInsensitiveExactList;

    // Append pattern if not empty and not already there
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

    if ( caseSensitivity == Qt::CaseInsensitive )
	suffix = suffix.toLower();

    // Pick the correct suffix list
    QStringList & suffixList =
	caseSensitivity == Qt::CaseSensitive ? _caseSensitiveSuffixList : _caseInsensitiveSuffixList;

    // Append suffix if not empty and not already there
    if ( !suffix.isEmpty() && !suffixList.contains( suffix ) )
	suffixList << suffix;
}


void MimeCategory::addWildcardSuffix( const QString       & rawPattern,
				      Qt::CaseSensitivity   caseSensitivity )
{
    QString pattern = rawPattern.trimmed();

    if ( caseSensitivity == Qt::CaseInsensitive )
	pattern = pattern.toLower();

    // Pick the correct suffix list
    QStringList & wildcardSuffixList =
	caseSensitivity == Qt::CaseSensitive ? _caseSensitiveWildcardSuffixList : _caseInsensitiveWildcardSuffixList;

    // Append suffix if not empty and not already there
    if ( !pattern.isEmpty() && !wildcardSuffixList.contains( pattern ) )
	wildcardSuffixList << pattern;
}


void MimeCategory::addWildcard( const QString       & rawPattern,
				Qt::CaseSensitivity   caseSensitivity )
{
    const QString pattern = rawPattern.trimmed();

    // Pick the correct wildcard list
    QStringList & wildcardList =
	caseSensitivity == Qt::CaseSensitive ? _caseSensitiveWildcardList : _caseInsensitiveWildcardList;

    // Append wildcard if not empty and not already there
//    if ( !pattern.isEmpty() && !wildcardList.contains( pattern ) )
	wildcardList << pattern;
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
	QString pattern = rawPattern.trimmed();
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
    QStringList result =
	caseSensitivity == Qt::CaseSensitive ? _caseSensitiveExactList : _caseInsensitiveExactList;
    result.sort( caseSensitivity );

    QStringList list =
	caseSensitivity == Qt::CaseSensitive ? _caseSensitiveWildcardSuffixList : _caseInsensitiveWildcardSuffixList;
    list.sort( caseSensitivity );
    result << list;

    list = caseSensitivity == Qt::CaseSensitive ? _caseSensitiveSuffixList : _caseInsensitiveSuffixList;
    list.sort( caseSensitivity );
    result << humanReadableSuffixList( list );

    list = caseSensitivity == Qt::CaseSensitive ? _caseSensitiveWildcardList : _caseInsensitiveWildcardList;
    list.sort( caseSensitivity );
    result << list;

    return result;
}
