/*
 *   File name: FileTypeStats.cpp
 *   Summary:   Statistics classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "FileTypeStats.h"
#include "DirTree.h"
#include "FileInfoIterator.h"
#include "FormatUtil.h"
#include "Logger.h"
#include "MimeCategorizer.h"


#define VERBOSE_STATS 0


using namespace QDirStat;


namespace
{
    /**
     * Check if a suffix is cruft, i.e. a nonstandard suffix that is not
     * useful for classification.
     *
     * The criteria are deliberately extreme, so only really odd
     * extensions are treated as "non-extensions".  Most of the mass
     * of uncommon extensions are hidden by the topX limit.
     **/
    bool isCruft( const QString            & suffix,
                  const QRegularExpression & matchUnusual,
                  const QRegularExpression & matchInvalid )
    {
	// Starting with anything other than a letter or number is odd
//	const ushort ch = suffix.at( 0 ).unicode();
//	if ( !( ch >= 'A' && ch <= 'Z' ) && !( ch >= 'a' && ch <= 'z' ) )
	const QChar ch = suffix.at( 0 );
	if ( !ch.isLetterOrNumber() )
	    return true;

	// Forget suffixes with too many non-word characters
	const int len = suffix.size();
	const int unusual = suffix.count( matchUnusual );
	if ( 3 * unusual > len || unusual > 3 )
	    return true;

	// Any spaces and control characters at all in suffixes are just weird
	if ( suffix.contains( matchInvalid ) )
	    return true;

	return false;
    }


    /**
    * See if a filename has an extension; that is, a section
    * of the name following a '.' character.  Ignore a
    * leading dot which indicates a hidden file, not an
    * extension.
    *
    * The rather complex loop here is an attempt to identify
    * multiple suffixes which might form a single valid
    * extension: eg. .tar.gz.  Starting at the end of the
    * filename, when an entirely-numeric or short suffix is
    * found, a search is made for a previous suffix.  This
    * continues until either there are no more dots or the
    * compound suffix becomes too long, or a suffix is found
    * which is too long to be considered sensible in a
    * compound suffix.
    *
    * This could probably be tuned further, but surprisingly
    * few extra longer extensions are found and they usually
    * get relegated to <other> anyway.
    **/
    QString filenameExtension( const QString            & filename,
                               const QRegularExpression & matchUnusual,
                               const QRegularExpression & matchInvalid )
    {
	// Ignore leading and trailing dots and any suffix longer than 32 characters
	const int lastDot = filename.lastIndexOf( u'.', -2 );
	if ( lastDot <= 0 || filename.size() - lastDot > 32 )
	    return QString{};

	const int suffixIndex = [ &filename, lastDot ]()
	{
	    /**
	    * Return whether to look for another extension section,
	    * based on the suffix between 'startDot' and 'endDot'.
	    * If the suffix has four or fewer characters (excluding
	    * the dots) or contains only numeric digits, then see if
	    * another suffix can be found before it in 'filename'.
	    **/
	    const auto tryAgain = [ &filename ]( const int startDot, const int endDot )
	    {
		if ( endDot - startDot < 6 )
		    return true;

		for ( int i = startDot + 1; i < endDot; ++i )
		{
		    if ( !isdigit( filename.at( i ).unicode() ) )
			return false;
		}

		return true;
	    };

	    // Loop while the suffixes found so far contain only numbers or are very short
	    int thisDot = lastDot;
	    bool tryAnotherSuffix = tryAgain( thisDot, filename.size() );
	    while ( tryAnotherSuffix )
	    {
		const int prevDot = thisDot;
		thisDot = filename.lastIndexOf( u'.', prevDot - 1 );

		// If no new suffix found, or the total extension length or this suffix is too big ...
		// ... then return the extensions without this section
		const int totalSuffixLen = filename.size() - thisDot - 1;
		const int newSectionLen  = prevDot - thisDot - 1;
		if ( thisDot <= 0 || totalSuffixLen > 16 || newSectionLen > 8 )
		    return prevDot;

		tryAnotherSuffix = tryAgain( thisDot, prevDot );
	    }
	    return thisDot;
	}();

	const QString suffix = filename.mid( suffixIndex + 1 );
	if ( isCruft( suffix, matchUnusual, matchInvalid ) )
	    return QString{};

	return suffix;
    }

} // namespace


FileTypeStats::FileTypeStats( FileInfo * subtree ):
    _nonCategory{ new MimeCategory{ QObject::tr( "<uncategorised>" ) } }
{
    if ( subtree && subtree->checkMagicNumber() )
    {
	const QRegularExpression matchUnusual{ "[^\\w]" };
	const QRegularExpression matchInvalid{ "\\p{Z}|\\p{C}" };
	collect( subtree, matchUnusual, matchInvalid );

//	_totalSize = subtree->totalSize();
//	removeCruft( _categories, _suffixes, nonCategory() );

#if VERBOSE_STATS
	sanityCheck( subtree );
#endif
    }

    logDebug() << _suffixes.size() << " suffixes in " << _categories.size() << " categories" << Qt::endl;
}


void FileTypeStats::collect( const FileInfo           * dir,
                             const QRegularExpression & matchUnusual,
                             const QRegularExpression & matchInvalid )
{
    MimeCategorizer * categorizer = MimeCategorizer::instance();

    for ( DotEntryIterator it{ dir }; *it; ++it )
    {
	if ( it->hasChildren() )
	{
	    collect( *it, matchUnusual, matchInvalid );
	}
	else if ( it->isFileOrSymLink() )
	{
	    ++_totalCount;
	    _totalSize += it->size();

	    // First attempt: try the MIME categorizer.
	    QString suffix;
	    const MimeCategory * category = categorizer->category( *it, &suffix );
	    if ( category )
	    {
		// The suffixes the MIME categorizer returns are the exact ones
		// used to match a category.  That could be one of multiple
		// possible suffixies.  When the category is matched using a
		// non-suffix rule, there may be no suffix here even though the
		// file has a suffix.
		addCategoryItem( category, *it );
		addSuffixItem( suffix, category, *it );
	    }
	    else // !category
	    {
		// Use "Uncategorised" with any filename extension as the suffix
		addCategoryItem( nonCategory(), *it );
		const QString suffix = filenameExtension( it->name(), matchUnusual, matchInvalid );
		addSuffixItem( suffix, nonCategory(), *it );
	    }

	    // Disregard block devices and other special files
	}
    }
}


void FileTypeStats::addCategoryItem( const MimeCategory * category, const FileInfo * item )
{
    // Qt will create a value-initialised (ie. all zeroes) entry if it doesn't exist yet
    CountSize & countSize = _categories[ category ];
    ++countSize.count;
    countSize.size += item->size();
}


void FileTypeStats::addSuffixItem( const QString      & suffix,
                                   const MimeCategory * category,
                                   const FileInfo     * item )
{
    // Qt will create a value-initialised (ie. all zeroes) entry if it doesn't exist yet
    CountSize & countSize = _suffixes[ { suffix, category } ];
    ++countSize.count;
    countSize.size += item->size();
}


#if VERBOSE_STATS
void FileTypeStats::sanityCheck( FileInfo * subtree ) const
{
    const FileSize sizeDiff = subtree->totalSize() - _totalSize;
    const float diffPercent = _totalSize ? 100.0f * sizeDiff / _totalSize : 100.0f;

    logDebug() << "Unaccounted in categories: "
               << formatSize( sizeDiff ) << " of " << formatSize( _totalSize )
               << " (" << QString::number( diffPercent, 'f', 2 ) << "%)"
               << Qt::endl;
}
#endif
