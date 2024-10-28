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
    * See if a filename has an extension; that is, a section
    * of the name following a '.' character.  Ignore a
    * leading dot which indicates a hidden file, not an
    * extension.
    *
    * Use the last (i.e. the shortest) suffix if the MIME
    * categorizer didn't know it: use section -1 (the last
    * one, ignoring any trailing '.' separator).
    *
    * The downside is that this would not find a ".tar.bz",
    * but just the ".bz" for a compressed tarball. But it's
    * much better than getting a ".eab7d88df-git.deb" rather
    * than a ".deb".
    **/
    QString filenameExtension( const QString & filename )
    {
	// See if there is a dot other than a leading or trailing dot
	const int lastDot = filename.lastIndexOf( u'.', -2 );
	if ( lastDot > 0 )
	    return filename.mid( lastDot + 1 );

	return QString{};
    }

    /**
     * Check if a suffix is cruft, i.e. a nonstandard suffix that is not
     * useful for classification.
     *
     * The criteria are deliberately extreme, so only really odd
     * extensions are treated as "non-extensions".  Most of the mass
     * of uncommon extensions are hidden by the topX limit.
     **/
    bool isCruft( const QString            & suffix,
                  int                        suffixCount,
                  int                        categoryCount,
                  const QRegularExpression & matchLetters,
                  const QRegularExpression & matchSpaces )
    {
	// Exclude extreme suffixes for any number of reasons
	const auto len = suffix.size();
	if ( len == 0 || len > 32 )
	    return true;

	// Just treat standard Latin letters as normal for suffixes
	const int letters = suffix.count( matchLetters );
	if ( letters == 0 )
	    return true;

	// Common cases: all letters (and numbers) and a sensible length
	if ( len == letters && len >= 2 && len <= 6 )
	    return false;

	// Only apply the next two tests to suffixes that are both absolutely and relatively uncommon
	if ( suffixCount == 1 || ( suffixCount < 10 && suffixCount * 1000 < categoryCount ) )
	{
	    // Arbitrary exclusion of long uncommon suffixes
	    if ( len > 16 )
		return true;

	    // Forget uncommon suffixes with too many non-letters
	    if ( 4 * ( len - letters ) > len )
		return true;
	}

	// Spaces and control characters in suffixes are just weird
	if ( suffix.contains( matchSpaces ) )
	    return true;

	return false;
    }


    /**
     * Move entries that have a '.' in the name but do not have
     * meaningful extensions into the "no extension" category map
     * for the "Other" category". This includes extremely long
     * extensions, those with no letters, uncommon extensions with
     * a low proportion of letters, and anything at all with a
     * space in it.
     **/
    void removeCruft( CategoryMap & categories, SuffixMap & suffixes, const MimeCategory * otherCategory)
    {
	// Might be nothing at all in "Other"
	if ( !categories.contains( otherCategory ) )
	    return;

	// Just get this once, isCruft needs it each time
	const int otherCategoryCount = categories[ otherCategory ].count;

	// QHash will default-construct (ie. zeroes) the entry if it don't exist yet
	CountSize & noExtension = suffixes[ { QString{}, otherCategory } ];

#if VERBOSE_STATS
	FileSize totalMergedSize  = 0LL;
	int      totalMergedCount = 0;
#endif

	// Create these here so we only have to compile them once
	const QRegularExpression matchLetters{ "[a-zA-Z]" };
	const QRegularExpression matchSpaces{ "\\p{Z}|\\p{C}" };

	QStringList cruftSuffixes;
	for ( auto it = suffixes.cbegin(); it != suffixes.cend(); ++it )
	{
	    const QString      & suffix   = it.key().suffix;
	    const MimeCategory * category = it.key().category;

	    // Identify apparent suffixes with no category that are not sensible file extensions
	    if ( category == otherCategory && !suffix.isEmpty() )
	    {
		const int suffixCount = it.value().count;

		if ( isCruft( suffix, suffixCount, otherCategoryCount, matchLetters, matchSpaces ) )
		{
		    // copy the cruft values to the "no extension" entry in the "Other" category
		    const FileSize suffixSize = it.value().size;
		    noExtension.count += suffixCount;
		    noExtension.size  += suffixSize;
		    cruftSuffixes << suffix;

#if VERBOSE_STATS
		    totalMergedCount += suffixCount;
		    totalMergedSize  += suffixSize;
#endif
		}
	    }
	}

	// Remove the merged suffixes
	for ( const QString & cruftSuffix : asConst( cruftSuffixes ) )
	    suffixes.remove( { cruftSuffix, otherCategory } );

#if VERBOSE_STATS
	if ( cruftSuffixes.size() > 0 )
	{
	    logDebug() << "Merged " << cruftSuffixes.size() << " suffixes to <no extension>: "
	               << "*." << cruftSuffixes.join( ", *."_L1 ) << Qt::endl;

	    logDebug() << "Merged " << totalMergedCount << " files to <no extension> "
	               << "(" << formatSize( totalMergedSize ) << ")" << Qt::endl;
	}
#endif

	// The price of finding the cruft entry just once is having to delete it ...
	// ... if it was default-constructed and is still empty
	if ( noExtension.count == 0 )
	    suffixes.remove( { QString{}, otherCategory } );
    }

} // namespace


FileTypeStats::FileTypeStats( FileInfo * subtree ):
    _otherCategory{ new MimeCategory{ QObject::tr( "Other" ) } }
{
    if ( subtree && subtree->checkMagicNumber() )
    {
	collect( subtree );
	_totalSize = subtree->totalSize();
	removeCruft( _categories, _suffixes, otherCategory() );
#if VERBOSE_STATS
	sanityCheck();
#endif
    }

    logDebug() << _suffixes.size() << " suffixes in " << _categories.size() << " categories" << Qt::endl;
}


void FileTypeStats::collect( const FileInfo * dir )
{
    MimeCategorizer * categorizer = MimeCategorizer::instance();

    for ( DotEntryIterator it{ dir }; *it; ++it )
    {
	if ( it->hasChildren() )
	{
	    collect( *it );
	}
	else if ( it->isFileOrSymLink() )
	{
	    ++_totalCount;

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
		// Use the "Other" category with any filename extension as the suffix
		addCategoryItem( otherCategory(), *it );
		addSuffixItem( filenameExtension( it->name() ), otherCategory(), *it );
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


void FileTypeStats::addSuffixItem( const QString & suffix,
                                   const MimeCategory * category,
                                   const FileInfo * item )
{
    // Qt will create a value-initialised (ie. all zeroes) entry if it doesn't exist yet
    CountSize & countSize = _suffixes[ { suffix, category } ];
    ++countSize.count;
    countSize.size += item->size();
}


#if VERBOSE_STATS
void FileTypeStats::sanityCheck() const
{
    const FileSize sizeDiff =
	std::accumulate( _categories.cbegin(), _categories.cend(), _totalSize,
                        []( FileSize size, const CountSize & cat ) { return size - cat.size; } );
    const float diffPercent = _totalSize ? 100.0f * sizeDiff / _totalSize : 100.0f;

    logDebug() << "Unaccounted in categories: "
               << formatSize( sizeDiff ) << " of " << formatSize( _totalSize )
               << " (" << QString::number( diffPercent, 'f', 2 ) << "%)"
               << Qt::endl;
}
#endif
