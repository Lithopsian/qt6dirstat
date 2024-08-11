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
	// See if there is a dot other than a leading dot
	if ( filename.lastIndexOf( u'.' ) > 0 )
	    // Use the shortest extension
	    return filename.section( u'.', -1 );

	return QString{};
    }

    /**
     * Check if a suffix is cruft, i.e. a nonstandard suffix that is not
     * useful for display.
     *
     * The criteria are deliberately extreme, so only really odd
     * extensions are treated as "non-extensions".  Most of the mass
     * of uncommon extensions are hidden by the topX limit.
     **/
    bool isCruft( const QString & suffix, int suffixCount, int categoryCount )
    {
	const int letters = suffix.count( QRegularExpression{ "[a-zA-Z]" } );
	if ( letters == 0 )
	    return true;

	// The most common case: 3-letter suffix
	const auto len = suffix.size();
	if ( len == 3 && letters == 3 )
	    return false;

	// Spaces in suffixes are just weird
	if ( suffix.contains( u' ' ) )
	    return true;

	// Only apply the next two tests to suffixes that are both absolutely and relatively uncommon
	if ( suffixCount == 1 || ( suffixCount < 10 && suffixCount * 1000 < categoryCount ) )
	{
	    // Arbitrary exclusion of very long uncommon suffixes
	    if ( len > 16 )
		return true;

	    // Forget uncommon suffixes with too many non-letters
	    const float lettersPercent = len > 0 ? 100.0f * letters / len : 0.0f;
	    if ( lettersPercent < 75.0f )
		return true;
	}

	return false;
    }

}


FileTypeStats::FileTypeStats( FileInfo * subtree ):
    _otherCategory{ new MimeCategory{ QObject::tr( "Other" ) } }
{
    if ( subtree && subtree->checkMagicNumber() )
    {
        collect( subtree );
        _totalSize = subtree->totalSize();
        removeCruft();
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
	else if ( it->isFile() )
	{
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
		addCategorySum( category, *it );
		addSuffixSum( suffix, category, *it );
	    }
	    else // !category
	    {
		// Use the "Other" category with any filename extension as the suffix
		addCategorySum( otherCategory(), *it );
		addSuffixSum( filenameExtension( it->name() ), otherCategory(), *it );
	    }

	    // Disregard symlinks, block devices and other special files
	}
    }
}


void FileTypeStats::addCategorySum( const MimeCategory * category, const FileInfo * item )
{
    // Qt will create a value-initialised (ie. all zeroes) entry if it doesn't exist yet
    CountSum & countSum = _categories[ category ];
    ++countSum.count;
    countSum.sum += item->size();
}


void FileTypeStats::addSuffixSum( const QString & suffix, const MimeCategory * category, const FileInfo * item )
{
    // Qt will create a value-initialised (ie. all zeroes) entry if it doesn't exist yet
    const SuffixCategory suffixCategory{ suffix, category };
    CountSum & countSum = _suffixes[ suffixCategory ];
    ++countSum.count;
    countSum.sum += item->size();
}


void FileTypeStats::removeCruft()
{
    // Might be nothing at all in "Other"
    if ( !_categories.contains( otherCategory() ) )
	return;

    // Just get this once, isCruft needs it each time
    const int otherCategoryCount = _categories[ otherCategory() ].count;

    // QHash will default-construct (ie. zeroes) the entry if it don't exist yet
    CountSum & noExtension = _suffixes[ { QString{}, otherCategory() } ];

#if VERBOSE_STATS
    FileSize totalMergedSum   = 0LL;
    int      totalMergedCount = 0;
#endif

    QStringList cruftSuffixes;
    for ( auto it = _suffixes.cbegin(); it != _suffixes.cend(); ++it )
    {
	const QString      & suffix   = it.key().suffix;
	const MimeCategory * category = it.key().category;

	// Identify apparent suffixes with no category that are not sensible file extensions
	if ( category == otherCategory() && !suffix.isEmpty() )
	{
	    const int suffixCount = it.value().count;

	    if ( isCruft( suffix, suffixCount, otherCategoryCount ) )
	    {
		// copy the cruft values to the "no extension" entry in the "Other" category
		const FileSize suffixSum = it.value().sum;
		noExtension.count += suffixCount;
		noExtension.sum   += suffixSum;
		cruftSuffixes << suffix;

#if VERBOSE_STATS
		totalMergedCount += suffixCount;
		totalMergedSum   += suffixSum ;
#endif
	    }
	}
    }

    // Remove the merged suffixes
    for ( const QString & cruftSuffix : cruftSuffixes )
	_suffixes.remove( { cruftSuffix, otherCategory() } );

#if VERBOSE_STATS
    if ( cruftSuffixes.size() > 0 )
    {
	logDebug() << "Merged " << cruftSuffixes.size() << " suffixes to <no extension>: "
	           << "*." << cruftSuffixes.join( ", *."_L1 ) << Qt::endl;

	logDebug() << "Merged " << totalMergedCount << " files to <no extension> "
	           << "(" << formatSize( totalMergedSum ) << ")" << Qt::endl;
    }
#endif

    // The price of finding the cruft entry just once is having to delete it ...
    // ... if it was default-constructed and is still empty
    if ( noExtension.count == 0 )
	_suffixes.remove( { QString{}, otherCategory() } );
}


#if VERBOSE_STATS
void FileTypeStats::sanityCheck()
{
    const FileSize diff = std::accumulate( _categories.cbegin(),
                                           _categories.cend(),
                                           _totalSize,
                                           []( FileSize sum, const CountSum & cat )
                                               { return sum - cat.sum; } );

    logDebug() << "Unaccounted in categories: "
               << formatSize( diff ) << " of " << formatSize( _totalSize )
               << " (" << QString::number( percentage( diff ), 'f', 2 ) << "%)"
               << Qt::endl;
}
#endif
