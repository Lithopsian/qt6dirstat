/*
 *   File name: MimeCategorizer.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QElapsedTimer>

#include "MimeCategorizer.h"
#include "FileInfo.h"
#include "FormatUtil.h"
#include "Logger.h"
#include "MimeCategory.h"
#include "Settings.h"


using namespace QDirStat;


namespace
{
    /**
     * Add one filename/category combination to a map.
     **/
    void addExactKey( ExactMatches        & keys,
                      QBitArray           & lengths,
                      const QString       & key,
                      const MimeCategory  * category )
    {
	//logDebug() << "adding " << keyList << " to " << category << Qt::endl;
//	if ( keys.contains( key ) )
//	{
//	    logWarning() << "Duplicate key: " << key
//	                  << " for " << keys.value( key )->name() << " and " << category->name()
//	                 << Qt::endl;
//	}
//	else
	{
	    // Add this pattern with no wildcards into a hash map
	    keys.insert( key, category );

	    // Mark the length pf this pattern so we only try to match filenames wih the right length
	    const auto length = key.size();
	    if ( length >= lengths.size() )
		lengths.resize( length + 1 );
	    lengths.setBit( length );
	}
    }


    /**
     * Adds one simple suffix or wildcard suffix to one suffix map.
     **/
    void addSuffixKey( SuffixMatches      & suffixes,
                       const QString      & suffix,
                       const Wildcard     & wildcard,
                       const MimeCategory * category )
    {
	const WildcardCategory pair{ wildcard, category };
//	if ( suffixes.contains( suffix, pair ) )
//	{
//	    logWarning() << "Duplicate pattern: " << ( wildcard.isEmpty() ? suffix : wildcard.pattern() )
//	                 << " for " << suffixes.value( suffix ).category->name() << " and " << category->name()
//	                 << Qt::endl;
//	}
//	else
	{
	    suffixes.insert( suffix, pair );
	}
    }


    /**
     * Write the MimeCategory parameter to the settings.
     **/
    void writeSettings( const MimeCategoryList & categoryList )
    {
	//logDebug() << Qt::endl;

	MimeCategorySettings settings;

	// Remove all leftover cleanup descriptions
	settings.removeListGroups();

	for ( int i=0; i < categoryList.size(); ++i )
	{
	    settings.beginListGroup( i+1 );

	    const MimeCategory * category = categoryList.at(i);
	    //logDebug() << "Adding " << category->name() << Qt::endl;

	    settings.setValue     ( "Name",  category->name()  );
	    settings.setColorValue( "Color", category->color() );

	    QStringList patterns = category->humanReadablePatternList( Qt::CaseInsensitive );

	    if ( patterns.isEmpty() )
		patterns << QString{};

	    settings.setValue( "PatternsCaseInsensitive", patterns );

	    patterns = category->humanReadablePatternList( Qt::CaseSensitive );

	    if ( patterns.isEmpty() )
		patterns << QString{};

	    settings.setValue( "PatternsCaseSensitive", patterns );

	    settings.endListGroup(); // [MimeCategory_01], [MimeCategory_02], ...
	}
    }

} // namespace


MimeCategorizer::~MimeCategorizer()
{
    clear();
}


MimeCategorizer * MimeCategorizer::instance()
{
    static MimeCategorizer _instance;

    return &_instance;
}


void MimeCategorizer::clear()
{
    qDeleteAll( _categories );
    _categories.clear();
}


QString MimeCategorizer::name( const FileInfo * item )
{
    const QReadLocker locker{ &_lock };

    const MimeCategory * matchedCategory = category( item );
    return matchedCategory ? matchedCategory->name() : QString{};
}


QColor MimeCategorizer::color( const FileInfo * item )
{
    const QReadLocker locker{ &_lock };

    const MimeCategory * matchedCategory = category( item );
    return matchedCategory ? matchedCategory->color() : Qt::white;
}


const MimeCategory * MimeCategorizer::category( const FileInfo * item,
                                                QString        & pattern,
                                                bool           & caseInsensitive )
{
    if ( item )
    {
	caseInsensitive = false;

	if ( item->isSymlink() )
	    return _symlinkCategory;

	const QReadLocker locker{ &_lock };

	const MimeCategory * matchedCategory = category( item->name(), &pattern, &caseInsensitive );
	if ( matchedCategory )
	    return matchedCategory;

	if ( ( item->mode() & S_IXUSR ) == S_IXUSR )
	    return _executableCategory;
    }

    return nullptr;
}


const MimeCategory * MimeCategorizer::category( const FileInfo * item ) const
{
    if ( item->isSymlink() )
	return _symlinkCategory;

    if ( item->isFile() )
    {
	const MimeCategory * matchedCategory = category( item->name(), nullptr, nullptr );
	if ( matchedCategory )
	    return matchedCategory;

	if ( ( item->mode() & S_IXUSR ) == S_IXUSR )
	    return _executableCategory;
    }

    return nullptr;
}


const MimeCategory * MimeCategorizer::category( const QString & filename,
                                                QString       * pattern_ret,
                                                bool          * caseInsensitive_ret ) const
{
    if ( filename.isEmpty() )
	return nullptr;

    // Whole filename exact matching will be relatively rare, so quickly check if it is
    // possible to produce any matches before doing the actual case-sensitive and insensitive
    // tests.  There are a finite set of pattern lengths and only filenames of these lengths
    // can match.
    const auto length = filename.size();

    if ( testBit( _caseSensitiveLengths, length ) )
    {
	const MimeCategory * category = _caseSensitiveExact.value( filename, nullptr );
	if ( category )
	{
	    if ( pattern_ret )
		*pattern_ret = filename;

	    return category;
	}
    }

    // A lowercased filename will have been detected already because there is a pattern
    // in the case-sensitive map, so only filenames which are not lowercase are of
    // interest here.
    if ( testBit( _caseInsensitiveLengths, length ) && !isLower( filename ) )
    {
	const MimeCategory * category = _caseInsensitiveExact.value( filename.toLower(), nullptr );
	if ( category )
	{
	    if ( pattern_ret )
		*pattern_ret = filename;

	    return category;
	}
    }

    // Find the longest filename suffix, ignoring any leading dot
    int dotIndex = filename.indexOf( u'.', 1 );

    while ( dotIndex >= 0 )
    {
	const QString suffix = filename.mid( dotIndex + 1 );

        // logVerbose() << "Checking " << suffix << Qt::endl;

	// Try case sensitive first (also includes upper- and lower-cased suffixes
	// from the case-insensitive lists)
	const WildcardCategory * pair = matchWildcardSuffix( _caseSensitiveSuffixes, filename, suffix );
	if ( pair )
	{
	    if ( pattern_ret )
		*pattern_ret = pair->wildcard.isEmpty() ? "*."_L1 % suffix : pair->wildcard.pattern();

	    return pair->category;
	}

	if ( !isLower( suffix ) && !isUpper( suffix ) )
	    pair = matchWildcardSuffix( _caseInsensitiveSuffixes, filename, suffix.toLower() );
	if ( pair )
	{
	    if ( pattern_ret )
	    {
		if ( pair->wildcard.isEmpty() )
		{
		    *pattern_ret = "*."_L1 % suffix;
		}
		else
		{
		    *caseInsensitive_ret = true;
		    *pattern_ret =  pair->wildcard.pattern();
		}
	    }

	    return pair->category;
	}

	// No match so far? Try for a shorter suffix, e.g. after "tar.bz2" try "bz2"
	// Repeated dots within the filename (very rare) will be searched as extra suffixes:
	// eg. "a..b" will lead to searches with suffixes ".b" and "b"
	dotIndex = filename.indexOf( u'.', dotIndex + 1 );
    }

    // Go through all the plain regular expressions one by one
    const WildcardCategory * pair = matchWildcard( filename );
    if ( pair )
    {
	if ( pattern_ret )
	{
	    *caseInsensitive_ret = pair->wildcard.caseInsensitive();
	    *pattern_ret = pair->wildcard.pattern();
	}
	const MimeCategory * category = pair->category;
#if 0
	logVerbose() << "Found " << category << " for " << filename << Qt::endl;
#endif

	return category;
    }

    return nullptr;
}


const WildcardCategory * MimeCategorizer::matchWildcardSuffix( const SuffixMatches & map,
                                                               const QString       & filename,
                                                               const QString       & suffix ) const
{
    const auto rangeIts = map.equal_range( suffix );
    for ( auto it = rangeIts.first; it != rangeIts.second && it.key() == suffix; ++it )
    {
	const WildcardCategory & pair = it.value();
	if ( pair.wildcard.isEmpty() || pair.wildcard.isMatch( filename ) )
	    return &pair;
    }

    return nullptr;
}


const WildcardCategory * MimeCategorizer::matchWildcard( const QString & filename ) const
{
    for ( const WildcardCategory & pair : asConst( _wildcards ) )
    {
	if ( pair.wildcard.isMatch( filename ) )
	    return &pair;
    }

    return nullptr; // No match
}


const MimeCategory * MimeCategorizer::addCategory ( const QString     & name,
                                                    const QColor      & color,
                                                    const QStringList & caseInsensitivePatterns,
                                                    const QStringList & caseSensitivePatterns )
{
    MimeCategory * category = new MimeCategory{ name, color };
    category->addPatterns( caseInsensitivePatterns, Qt::CaseInsensitive );
    category->addPatterns( caseSensitivePatterns,   Qt::CaseSensitive   );

    _categories << category;

    return category;
}


const MimeCategory * MimeCategorizer::addCategory( const QString & name,
                                                   const QColor  & color,
                                                   const QString & caseInsensitivePatterns,
                                                   const QString & caseSensitivePatterns )
{
    const QStringList caseInsensitivePatternList = caseInsensitivePatterns.split( u',' );
    const QStringList caseSensitivePatternList   = caseSensitivePatterns.split  ( u',' );

    return addCategory( name, color, caseInsensitivePatternList, caseSensitivePatternList );
}


void MimeCategorizer::buildMaps()
{
    QElapsedTimer stopwatch;
    stopwatch.start();

    _caseInsensitiveExact.clear();
    _caseSensitiveExact.clear();
    _caseInsensitiveLengths.clear();
    _caseSensitiveLengths.clear();
    _caseInsensitiveSuffixes.clear();
    _caseSensitiveSuffixes.clear();
    _wildcards.clear();

    for ( const MimeCategory * category : asConst( _categories ) )
    {
	addExactKeys( category ); // exact matches with no wildcards
	addSuffixKeys( category ); // simple suffix matches
	addWildcardKeys( category ); // wildcards with a suffix, added last so they are retrieved first
	buildWildcardLists( category ); // regular expressions with no suffix
    }

    logDebug() << "maps built in " << stopwatch.restart() << "ms ("
               << _wildcards.size() << " naked regular expressions)" << Qt::endl;
}


void MimeCategorizer::addExactKeys( const MimeCategory * category )
{
    //logDebug() << "adding " << keyList << " to " << category << Qt::endl;
    for ( const QString & key : category->caseSensitiveExactList() )
	// Add key to the case-sensitive map and record this length
	addExactKey( _caseSensitiveExact, _caseSensitiveLengths, key, category );

    for ( const QString & key : category->caseInsensitiveExactList() )
    {
	// Add key to the case-insensitive map and record this length.  Also add
	// the lowercased name to the case-sensitive map as a common case that
	// will get picked up earlier and prevent the filename string having to
	// be copied when it is converted to lowercase.
	const QString lower = key.toLower();
	addExactKey( _caseInsensitiveExact, _caseInsensitiveLengths, lower, category );
	addExactKey( _caseSensitiveExact, _caseSensitiveLengths, lower, category );
    }
}


void MimeCategorizer::addWildcardKeys( const MimeCategory * category )
{
    // Return the portion of 'pattern' after the "*."
    const auto getSuffix = []( const QString & pattern )
    {
	const int delimeterIndex = pattern.lastIndexOf( "*."_L1 );
	return delimeterIndex < 0 ? pattern : pattern.mid( delimeterIndex + 2 );
    };

    //logDebug() << "adding " << patternList << " to " << category << Qt::endl;

    // Add any case-insensitive regular expression, plus a case-sensitive lowercased version
    for ( const QString & pattern : category->caseInsensitiveWildcardSuffixList() )
    {
	const QString suffix = getSuffix( pattern ).toLower();
	const Wildcard wildcard = CaseInsensitiveWildcard{ pattern };
	addSuffixKey( _caseInsensitiveSuffixes, suffix, wildcard, category);
	addSuffixKey( _caseSensitiveSuffixes, suffix, wildcard, category);
    }

    // Add true case-sensitive regular expressions to the case-sensitive map
    for ( const QString & pattern : category->caseSensitiveWildcardSuffixList() )
    {
	const QString suffix = getSuffix( pattern );
	const Wildcard wildcard = CaseSensitiveWildcard{ pattern };
	addSuffixKey( _caseSensitiveSuffixes, suffix, wildcard, category);
    }
}


void MimeCategorizer::addSuffixKeys( const MimeCategory * category )
{
    //logDebug() << "adding " << keyList << " to " << category << Qt::endl;

    // Add simple suffix matches into case-sensitive and case-insensitive hash maps
    for ( const QString & suffix : category->caseInsensitiveSuffixList() )
    {
	addSuffixKey( _caseInsensitiveSuffixes, suffix, Wildcard{}, category );

	// Add a lowercased and an uppercased version of the suffix into the case-sensitive map
	const QString lowercaseSuffix = suffix.toLower();
	const QString uppercaseSuffix = suffix.toUpper();
	addSuffixKey( _caseSensitiveSuffixes, lowercaseSuffix, Wildcard{}, category );
	if ( lowercaseSuffix != uppercaseSuffix)
	    addSuffixKey( _caseSensitiveSuffixes, uppercaseSuffix, Wildcard{}, category );
    }

    // Add true case-sensitive lookups to the case-sensitive map
    for ( const QString & suffix : category->caseSensitiveSuffixList() )
	addSuffixKey( _caseSensitiveSuffixes, suffix, Wildcard{}, category );
}


void MimeCategorizer::buildWildcardLists( const MimeCategory * category )
{
    //logDebug() << "adding " << keyList << " to " << category << Qt::endl;
    for ( const QString & pattern : category->caseSensitiveWildcardList() )
	_wildcards << WildcardCategory{ CaseSensitiveWildcard{ pattern }, category };

    for ( const QString & pattern : category->caseInsensitiveWildcardList() )
	_wildcards << WildcardCategory{ CaseInsensitiveWildcard{ pattern }, category };
}


void MimeCategorizer::readSettings()
{
    clear();

    MimeCategorySettings settings;

    // Read all settings groups [MimeCategory_xx] that were found
    const QStringList mimeCategoryGroups = settings.findListGroups();
    for ( const QString & groupName : mimeCategoryGroups )
    {
	settings.beginGroup( groupName );
	QString name  = settings.value( "Name", groupName ).toString();
	QColor  color = settings.colorValue( "Color", Qt::white );
	QStringList patternsCaseInsensitive = settings.value( "PatternsCaseInsensitive" ).toStringList();
	QStringList patternsCaseSensitive   = settings.value( "PatternsCaseSensitive"   ).toStringList();
	settings.endGroup(); // [MimeCategory_01], [MimeCategory_02], ...

	//logDebug() << name << Qt::endl;
	addCategory( name, color, patternsCaseInsensitive, patternsCaseSensitive );
    }

    if ( _categories.isEmpty() )
	addDefaultCategories();

    ensureMandatoryCategories();

    buildMaps();
}


void MimeCategorizer::replaceCategories( const MimeCategoryList & categories )
{
   _lock.lockForWrite();
    writeSettings( categories );
    readSettings();
    _lock.unlock();

    // Unlock before the signal to avoid deadlocks
    emit categoriesChanged();
}


void MimeCategorizer::ensureMandatoryCategories()
{
    /**
     * Iterate over all categories to find categories by name.
     **/
    const auto findCategoryByName = [ this ]( const QString & categoryName )
    {
	for ( const MimeCategory * category : asConst( _categories ) )
	{
	    if ( category && category->name() == categoryName )
		return category;
	}

	return static_cast<const MimeCategory *>( nullptr ); // No match
    };

    // Remember this category so we don't have to search for it every time
    _executableCategory = findCategoryByName( executableCategoryName() );
    if ( !_executableCategory )
    {
	// Special catchall category for files that don't match anything else, must exist
	_executableCategory = addCategory( executableCategoryName(),
	                                   Qt::magenta,
	                                   QString{},
	                                   "*.jsa, *.ucode, lft.db, traceproto.db, traceroute.db" );
	writeSettings( _categories );
    }

    // Remember this category so we don't have to search for it every time
    _symlinkCategory = findCategoryByName( symlinkCategoryName() );
    if ( !_symlinkCategory )
    {
	// Special category for symlinks regardless of the filename, must exist
	_symlinkCategory = addCategory( symlinkCategoryName(), Qt::blue, QStringList{}, QStringList{} );
	writeSettings( _categories );
    }
}


void MimeCategorizer::addDefaultCategories()
{
    addCategory( tr( "archive (compressed)" ),
                 Qt::green,
                 "*.7z, *.arj, *.bz2, *.cab, *.cpio.gz, *.gz, *.jmod, " \
                 "*.jsonlz4, *.lz, *.lzo, *.rar, *.tar.bz2, *.tar.gz, " \
                 "*.tar.lz, *.tar.lzo, *.tar.xz, *.tar.zst, *.tbz2, "   \
                 "*.tgz, *.txz, *.tz2, *.tzst, *.xz, *.zip, *.zpaq, "   \
                 "*.zst",
                 "pack-*.pack" );

    addCategory( tr( "archive (uncompressed)" ),
                 "#aaffaa",
                 "*.cpio, *.tar",
                 QString{} );

    addCategory( tr( "configuration file" ),
                 "#77ddff",
                 QString{},
                 "*.alias, *.cfg, *.conf, *.conffiles, *.config, *.dep, "         \
                 "*.desktop, *.ini, *.kmap, *.lang, *.my, *.page, *.properties, " \
                 "*.rc, *.service, *.shlibs, *.symbols, *.templates, *.theme, "   \
                 "*.triggers, *.xcd, *.xsl, .config, .gitignore, Kconfig, "       \
                 "control, gtkrc" );

    addCategory( tr( "database" ),
                 "#2299ff",
                 QString{},
                 "*.alias.bin, *.builtin.bin, *.dat, *.db, *.dep.bin, *.enc, " \
                 "*.hwdb, *.idx, *.lm, *.md5sums, *.odb, *.order, *.sbstore, " \
                 "*.sqlite, *.sqlite-wal, *.symbols.bin, *.tablet, *.vlpset, " \
                 "magic.mgc" );

    addCategory( tr( "disk image" ),
                 "#aaaaaa",
                 "*.fsa, *.iso",
                 "*.BIN, *.img" );

    addCategory( tr( "document" ),
                 "#33bbff",
                 "*.csv, *.doc, *.docbook, *.docx, *.dotx, *.dvi, *.dvi.bz2, "    \
                 "*.epub, *.htm, *.html, *.json, *.latex, *.log, *.ly, *.md, "    \
                 "*.md5, *.pdf, *.pod, *.potx, *.ppsx, *.ppt, *.pptx, *.ps, "     \
                 "*.readme, *.rst, *.sav, *.sdc, *.sdc.gz, *.sdd, *.sdp, *.sdw, " \
                 "*.sla, *.sla.gz, *.slaz, *.sxi, *.tex, *.txt, *.xls, *.xlsx, "  \
                 "*.xlt, *.xml, copyright, readme*",
                 "*.list, *.odc, *.odg, *.odp, *.ods, *.odt, *.otc, *.otp, " \
                 "*.ots, *.ott, *.yaml, *.log.?" );

    addCategory( tr( "font" ),
                 Qt::cyan,
                 QString{},
                 "*.afm, *.bdf, *.cache-7, *.cache-8, *.otf, *.pcf, *.pcf.gz, " \
                 "*.pf1, *.pf2, *.pfa, *.pfb, *.t1, *.ttf" );

    addCategory( tr( "game file" ),
                 "#ff66bb",
                 QString{},
                 "*.MHK, *.bsp, *.mdl, *.pak, *.wad" );

    addCategory( tr( "icon" ),
                 "#aa99ff",
                 "*.icns, *.ico, *.xbm, *.xpm",
                 QString{} );

    addCategory( tr( "image" ),
                 "#dd88ff",
                 "*.gif, *.jpeg, *.jpg, *.jxl, *.mng, *.png, *.tga, *.tif, *.tiff, " \
                 "*.webp, *.xcf.bz2, *.xcf.gz",
                 QString{} );

    addCategory( tr( "image (uncompressed)" ),
                 "#eeaaff",
                 "*.bmp, *.pbm, *.pgm, *.pnm, *.ppm, *.spr, *.svg, *.xcf",
                 QString{} );

    addCategory( tr( "junk" ),
                 Qt::red,
                 "*.bak, *.keep, *.old, *.orig",
                 "core, *.~, *~" );

    addCategory( tr( "music" ),
                 Qt::yellow,
                 "*.aac, *.aif, *.ape, *.caf, *.dff, *.dsf, *.f4a, *.f4b, *.flac, " \
                 "*.m4a, *.m4b, *.mid, *.mka, *.mp3, *.oga, *.ogg, *.opus, *.ra, "  \
                 "*.rax, *.w64, *.wav, *.wma, *.wv, *.wvc",
                 QString{} );

    addCategory( tr( "object file" ),
                 "#ff8811",
                 "lib*.a",
                 "*.Po, *.a.cmd, *.al, *.elc, *.go, *.gresource, *.ko, *.ko.cmd, "   \
                 "*.ko.xz, *.ko.zst, *.la, *.lo, *.mo, *.moc, *.o, *.o.cmd, *.pyc, " \
                 "*.qrc, *.typelib, built-in.a, vmlinux.a" );

    addCategory( tr( "packaged program" ),
                 "#88aa66",
                 "*.rpm, *.xpi",
                 "*.deb, *.ja, *.jar, *.sfi, *.tm" );

    addCategory( tr( "script" ),
                 "#cc6688",
                 QString{},
                 "*.BAT, *.bash, *.bashrc, *.cocci, *.csh, *.css, *.js, *.ksh, *.m4, " \
                 "*.patch, *.pl, *.pm, *.postinst, *.postrm, *.preinst, *.prerm, "     \
                 "*.qml, *.sh, *.tcl, *.tmac, *.xba, *.zsh" );

    addCategory( tr( "shared object" ),
                 "#ff6600",
                 "*.dll, *.dylib, *.so",
                 "*.so.*, *.so.0, *.so.1" );

    addCategory( tr( "source file" ),
                 "#ffb022",
                 QString{},
                 "*.S, *.S_shipped, *.asm, *.c, *.cc, *.cmake, *.cpp, *.cxx, *.dts, "   \
                 "*.dtsi, *.el, *.f, *.fuc3, *.fuc3.h, *.fuc5, *.fuc5.h, *.gir, *.h, "  \
                 "*.h_shipped, *.hpp, *.java, *.msg, *.ph, *.php, *.po, *.pot, *.pro, " \
                 "*.pxd, *.py, *.pyi, *.pyx, *.rb, *.scm, Kbuild, Makefile" );

    addCategory( tr( "source file (generated)" ),
                 "#ffcc22",
                 QString{},
                 "*.f90, *.mod.c, *.ui, moc_*.cpp, qrc_*.cpp, ui_*.h" );

    addCategory( tr( "video" ),
                 "#aa00ff",
                 "*.asf, *.avi, *.divx, *.dv, *.flc, *.fli, *.flv, *.m2ts, *.m4v, *.mk3d, " \
                 "*.mkv, *.mov, *.mp2, *.mp4, *.mpeg, *.mpg, *.mts, *.ogm, *.ogv, *.rm, "   \
                 "*.vdr, *.vob, *.webm, *.wmp, *.wmv",
                 QString{} );

    writeSettings( _categories );
}




bool WildcardCategory::matches( const FileInfo * item ) const
{
    // We only deal with regular files and symlinks
    if ( !item->isFileOrSymlink() )
	return false;

    // If there is a Wildcard pattern, but it doesn't match ...
    const QString & pattern = wildcard.pattern();
    if ( !pattern.isEmpty() && !wildcard.isMatch( item->name() ) )
	return false;

    // Re-categorise any item matching the wildcard so we can compare the actual categoriser match
    QString matchPattern;
    bool matchCaseInsensitive;
    const MimeCategory * matchCategory =
	MimeCategorizer::instance()->category( item, matchPattern, matchCaseInsensitive );

    // Uncategorised files match if we are looking for uncategorised files
    if ( !matchCategory && !category )
	return true;

    // Check that the categoriser result matches the one we're looking for
    const bool caseInsensitive = wildcard.patternOptions() & Wildcard::CaseInsensitiveOption;
    return matchCategory == category && matchCaseInsensitive == caseInsensitive && matchPattern == pattern;
}
