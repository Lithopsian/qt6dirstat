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
#include "MimeCategory.h"


#define VERBOSE_STATS 0


using namespace QDirStat;


namespace
{
    /**
     * Check if a file extension is cruft, i.e. a nonstandard suffix
     * that is not useful for classification.
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
    * extension and ignore very long extensions.
    **/
    QString filenameExtension( const QString            & filename,
                               const QRegularExpression & matchUnusual,
                               const QRegularExpression & matchInvalid )
    {
	// Ignore leading and trailing dots and any suffix longer than 32 characters
	const int lastDot = filename.lastIndexOf( u'.', -2 );
	if ( lastDot <= 0 || filename.size() - lastDot > 32 )
	    return QString{};

	const QString suffix = filename.mid( lastDot + 1 ); // just return the shortest extension
	if ( isCruft( suffix, matchUnusual, matchInvalid ) )
	    return QString{};

	return "*."_L1 % suffix;
    }

} // namespace


FileTypeStats::FileTypeStats( FileInfo * subtree )
{
    if ( subtree && subtree->checkMagicNumber() )
    {
	const QRegularExpression matchUnusual{ "[^\\w]" };
	const QRegularExpression matchInvalid{ "\\p{Z}|\\p{C}" };
	collect( subtree, matchUnusual, matchInvalid );

#if VERBOSE_STATS
	sanityCheck( subtree );
#endif
    }

    logDebug() << _patterns.size() << " patterns in " << _categories.size() << " categories" << Qt::endl;
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
	else if ( it->isFileOrSymlink() )
	{
	    ++_totalCount;
	    _totalSize += it->size();

	    // First attempt: try the MIME categorizer.
	    QString pattern;
	    bool caseInsensitive;
	    const MimeCategory * category = categorizer->category( *it, pattern, caseInsensitive );
	    if ( category )
	    {
		addCategoryItem( category, *it );
		addPatternItem( pattern, caseInsensitive, category, *it );
	    }
	    else // !category
	    {
		// Use "Uncategorised" with any filename extension as the suffix
		addCategoryItem( nullptr, *it );
		const QString suffix = filenameExtension( it->name(), matchUnusual, matchInvalid );
		addPatternItem( suffix, false, nullptr, *it );
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


void FileTypeStats::addPatternItem( const QString      & pattern,
                                    bool                 caseInsensitive,
                                    const MimeCategory * category,
                                    const FileInfo     * item )
{
    // Qt will create a value-initialised (ie. all zeroes) entry if it doesn't exist yet
    CountSize & countSize = _patterns[ { pattern, caseInsensitive, category } ];
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
