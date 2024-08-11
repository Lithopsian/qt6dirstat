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
#include "Logger.h"
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
	if ( keys.contains( key ) )
	{
	    logError() << "Duplicate key: " << key << " for "
		       << keys.value( key ) << " and " << category
		       << Qt::endl;
	}
	else
	{
	    // Add this pattern with no wildcards into a hash map
	    keys.insert( key, category );

	    // Mark the length pf this pattern so we only try to match filenames wih the right length
	    const int length = key.size();
	    if ( length >= lengths.size() )
		lengths.resize( length + 1 );
	    lengths.setBit( length );
	}
    }


    /**
     * Adds one simple suffix to one suffix map.
     **/
    void addSuffixKey( SuffixMatches      & suffixes,
                       const QString      & suffix,
                       const MimeCategory * category )
    {
	if ( suffixes.contains( suffix ) )
	{
	    logError() << "Duplicate suffix: " << suffix << " for "
		       << suffixes.value( suffix ).wildcard.pattern() << " and " << category
		       << Qt::endl;
	}
	else
	{
	    suffixes.insert( suffix, { Wildcard{}, category } );
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


const QString & MimeCategorizer::name( const FileInfo * item )
{
    const QReadLocker locker{ &_lock };

    return category( item )->name();
}


const QColor & MimeCategorizer::color( const FileInfo * item )
{
    const QReadLocker locker{ &_lock };

    return category( item )->color();
}


const MimeCategory * MimeCategorizer::category( const FileInfo * item, QString * suffix_ret )
{
    if ( item )
    {
	const QReadLocker locker{ &_lock };

	const MimeCategory * matchedCategory = category( item->name(), suffix_ret );
	if ( matchedCategory )
	    return matchedCategory;

	if ( ( item->mode() & S_IXUSR ) == S_IXUSR )
	    return _executableCategory;
    }

    return nullptr;
}


const MimeCategory * MimeCategorizer::category( const FileInfo * item ) const
{
    if ( item->isSymLink() )
	return _symlinkCategory;

    if ( item->isFile() )
    {
	const MimeCategory * matchedCategory = category( item->name(), nullptr );
	if ( matchedCategory )
	    return matchedCategory;

	if ( ( item->mode() & S_IXUSR ) == S_IXUSR )
	    return _executableCategory;
    }

    return &_emptyCategory;
}


const MimeCategory * MimeCategorizer::category( const QString & filename,
                                                QString       * suffix_ret ) const
{
    if ( suffix_ret )
	*suffix_ret = QString{};

    if ( filename.isEmpty() )
	return nullptr;

    // Whole filename exact matching will be relatively rare, so quickly check if it is
    // possible to produce any matches before doing the actual case-sensitive and insensitive
    // tests.  There are a finite set of pattern lengths and only filenames of these lengths
    // can match.
    const int length = filename.size();

    if ( length < _caseSensitiveLengths.size() && _caseSensitiveLengths.testBit( length ) )
    {
	const MimeCategory * category = _caseSensitiveExact.value( filename, nullptr );
	if ( category )
	    return category;
    }

    if ( length < _caseInsensitiveLengths.size() && _caseInsensitiveLengths.testBit( length ) && !filename.isLower() )
    {
	// A lowercased filename will have been detected already because there is a pattern
	// in the case-sensitive map, so only filenames which are not lowercase are of
	// interest here.
	const MimeCategory * category = _caseInsensitiveExact.value( filename.toLower(), nullptr );
	if ( category )
	    return category;
    }

    // Find the filename suffix, ignoring any leading '.' separator
    // Ignore an initial dot and treat repeated dots as a single separator
    QString suffix = filename.section( u'.', 1, -1, QString::SectionSkipEmpty );

    while ( !suffix.isEmpty() )
    {
        // logVerbose() << "Checking " << suffix << Qt::endl;

	// Try case sensitive first (also ncludes upper- and lower-cased suffixes
	// from the case-insensitive lists)
	const MimeCategory * category = matchWildcardSuffix( _caseSensitiveSuffixes, filename, suffix );

	if ( !category && suffix.size() > 1 && !suffix.isLower() && !suffix.isUpper() )
	    category = matchWildcardSuffix( _caseInsensitiveSuffixes, filename, suffix.toLower() );

	if ( category ) // success
        {
            if ( suffix_ret )
                *suffix_ret = suffix;

	    return category;
        }

	// No match so far? Try the next suffix. Some files might have more
	// than one, e.g., "tar.bz2" - if there is no match for "tar.bz2",
	// there might be one for just "bz2".

	suffix = suffix.section( u'.', 1, -1, QString::SectionSkipEmpty );
    }

    // Go through all the plain regular expressions one by one
    const MimeCategory * category = matchWildcard( filename );

#if 0
    if ( category )
	logVerbose() << "Found " << category << " for " << filename << Qt::endl;
#endif

    return category;
}


const MimeCategory * MimeCategorizer::matchWildcardSuffix( const SuffixMatches & map,
                                                           const QString       & filename,
                                                           const QString       & suffix ) const
{
    const auto rangeIts = map.equal_range( suffix );
    for ( auto it = rangeIts.first; it != rangeIts.second && it.key() == suffix; ++it )
    {
	WildcardPair pair = it.value();
	if ( pair.wildcard.isEmpty() || pair.wildcard.isMatch( filename ) )
	    return pair.category;
    }

    return nullptr;
}


const MimeCategory * MimeCategorizer::matchWildcard( const QString & filename ) const
{
    for ( const WildcardPair & pair : asConst( _wildcards ) )
    {
	if ( pair.wildcard.isMatch( filename ) )
	    return pair.category;
    }

    return nullptr; // No match
}


const MimeCategory * MimeCategorizer::findCategoryByName( const QString & categoryName ) const
{
    for ( const MimeCategory * category : asConst( _categories ) )
    {
	if ( category && category->name() == categoryName )
	    return category;
    }

    return nullptr; // No match
}


MimeCategory * MimeCategorizer::create( const QString & name, const QColor & color )
{
    MimeCategory * category = new MimeCategory{ name, color };
    _categories << category;

    return category;
}


void MimeCategorizer::buildMaps()
{
    QElapsedTimer stopwatch;
    stopwatch.start();

    _caseInsensitiveExact.clear();
    _caseSensitiveExact.clear();
    _caseInsensitiveSuffixes.clear();
    _caseSensitiveSuffixes.clear();
    _wildcards.clear();
    _caseInsensitiveLengths.clear();
    _caseSensitiveLengths.clear();

    for ( const MimeCategory * category : asConst( _categories ) )
    {
	addExactKeys( category );
	addSuffixKeys( category );
	addWildcardKeys( category );
	buildWildcardLists( category );
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
    //logDebug() << "adding " << patternList << " to " << category << Qt::endl;

    // Add any case-insensitive regular expression, plus a case-sensitive lowercased version
    for ( const QString & pattern : category->caseInsensitiveWildcardSuffixList() )
    {
	const QString suffix = pattern.section( "*."_L1, -1 ).toLower();
	const auto pair = WildcardPair{ CaseInsensitiveWildcard( pattern ), category };
	_caseInsensitiveSuffixes.insert( suffix, pair );
	_caseSensitiveSuffixes.insert( suffix, pair );
    }

    // Add any case-sensitive regular expressions last so they are retrieved first
    for ( const QString & pattern : category->caseSensitiveWildcardSuffixList() )
    {
	const QString suffix = pattern.section( "*."_L1, -1 );
	_caseSensitiveSuffixes.insert( suffix, { CaseSensitiveWildcard( pattern ), category } );
    }
}


void MimeCategorizer::addSuffixKeys( const MimeCategory * category )
{
    //logDebug() << "adding " << keyList << " to " << category << Qt::endl;

    // Add simple suffix matches into case-sensitive and case-insensitive hash maps
    for ( const QString & suffix : category->caseInsensitiveSuffixList() )
    {
	addSuffixKey( _caseInsensitiveSuffixes, suffix, category );

	// Add a lowercased and an uppercased version of the suffix into the case-sensitive map
	const QString lowercaseSuffix = suffix.toLower();
	const QString uppercaseSuffix = suffix.toUpper();
	addSuffixKey( _caseSensitiveSuffixes, lowercaseSuffix, category );
	if ( lowercaseSuffix != uppercaseSuffix)
	    addSuffixKey( _caseSensitiveSuffixes, uppercaseSuffix, category );
    }

    // Add any case-sensitive regular expressions last so they are retrieved first
    for ( const QString & suffix : category->caseSensitiveSuffixList() )
	addSuffixKey( _caseSensitiveSuffixes, suffix, category );
}


void MimeCategorizer::buildWildcardLists( const MimeCategory * category )
{
    //logDebug() << "adding " << keyList << " to " << category << Qt::endl;
    for ( const QString & pattern : category->caseSensitiveWildcardList() )
	_wildcards << WildcardPair{ CaseSensitiveWildcard( pattern ), category };

    for ( const QString & pattern : category->caseInsensitiveWildcardList() )
	_wildcards << WildcardPair{ CaseInsensitiveWildcard( pattern ), category };
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
	QColor  color = settings.colorValue( "Color", "#b0b0b0" );
	QStringList patternsCaseInsensitive = settings.value( "PatternsCaseInsensitive" ).toStringList();
	QStringList patternsCaseSensitive   = settings.value( "PatternsCaseSensitive"   ).toStringList();
	settings.endGroup(); // [MimeCategory_01], [MimeCategory_02], ...

	//logDebug() << name << Qt::endl;
	MimeCategory * category = create( name, color );
	category->addPatterns( patternsCaseInsensitive, Qt::CaseInsensitive );
	category->addPatterns( patternsCaseSensitive,   Qt::CaseSensitive   );
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
	_symlinkCategory = create( symlinkCategoryName(), Qt::blue );
	writeSettings( _categories );
    }
}


const MimeCategory * MimeCategorizer::addCategory( const QString & name,
                                                   const QColor  & color,
                                                   const QString & caseInsensitivePatterns,
                                                   const QString & caseSensitivePatterns )
{
    MimeCategory * category = create( name, color );
    category->addPatterns( caseInsensitivePatterns.split( u',' ), Qt::CaseInsensitive );
    category->addPatterns( caseSensitivePatterns.split  ( u',' ), Qt::CaseSensitive   );
    return category;
}

void MimeCategorizer::addDefaultCategories()
{
    addCategory( tr( "archive (compressed)" ),
                 Qt::green,
                 "*.7z, *.arj, *.bz2, *.cab, *.cpio.gz, *.gz, *.jmod, " \
                 "*.jsonlz4, *.lz, *.lzo, *.rar, *.tar.bz2, *.tar.gz, " \
                 "*.tar.lz, *.tar.lzo, *.tar.xz, *.tar.zst, *.tbz2, "   \
                 "*.tgz, *.txz, *.tz2, *.tzst, *.xz, *.zip, *.zpaq, *.zst",
                 "pack-*.pack" );

    addCategory( tr( "archive (uncompressed)" ),
                 "#88ff88",
                 "*.cpio, *.tar",
                 QString{} );

    addCategory( tr( "configuration file" ),
                 "#aabbff",
                 QString{},
                 "*.alias, *.cfg, *.conf, *.conffiles, *.config, *.dep, "         \
                 "*.desktop, *.ini, *.kmap, *.lang, *.my, *.page, *.properties, " \
                 "*.rc, *.service, *.shlibs, *.symbols, *.templates, *.theme, "   \
                 "*.triggers, *.xcd, *.xsl, .config, .gitignore, Kconfig, "       \
                 "control, gtkrc" );

    addCategory( tr( "database" ),
                 "#22aaff",
                 QString{},
                 "*.alias.bin, *.builtin.bin, *.dat, *.db, *.dep.bin, *.enc, " \
                 "*.hwdb, *.idx, *.lm, *.md5sums, *.odb, *.order, *.sbstore, " \
                 "*.sqlite, *.sqlite-wal, *.symbols.bin, *.tablet, *.vlpset, " \
                 "*.yaml, magic.mgc" );

    addCategory( tr( "disk image" ),
                 "#aaaaaa",
                 "*.fsa, *.iso",
                 "*.BIN, *.img" );

    addCategory( tr( "document" ),
                 "#66ccff",
                 "*.css, *.csv, *.doc, *.docbook, *.docx, *.dotx, *.dvi, "         \
                 "*.dvi.bz2, *.epub, *.htm, *.html, *.json, *.latex, *.log, "      \
                 "*.ly, *.md, *.pdf, *.pod, *.potx, *.ppsx, *.ppt, *.pptx, *.ps, " \
                 "*.readme, *.rst, *.sav, *.sdc, *.sdc.gz, *.sdd, *.sdp, *.sdw, "  \
                 "*.sla, *.sla.gz, *.slaz, *.sxi, *.tex, *.txt, *.xls, *.xlsx, "   \
                 "*.xlt, *.xml, copyright, readme*",
                 "*.list, *.log.0, *.log.1, *.odc, *.odg, *.odp, *.ods, *.odt, " \
                 "*.otc, *.otp, *.ots, *.ott" );

    addCategory( tr( "font" ),
                 "#44ddff",
                 QString{},
                 "*.afm, *.bdf, *.cache-7, *.cache-8, *.otf, *.pcf, *.pcf.gz, " \
                 "*.pf1, *.pf2, *.pfa, *.pfb, *.t1, *.ttf" );

    addCategory( tr( "game file" ),
                 "#ff88dd",
                 QString{},
                 "*.MHK, *.bsp, *.mdl, *.pak, *.wad" );

    addCategory( tr( "icon" ),
                 "#00ddff",
                 "*.icns, *.ico, *.xpm",
                 QString{} );

    addCategory( tr( "image" ),
                 Qt::cyan,
                 "*.gif, *.jpeg, *.jpg, *.jxl, *.mng, *.png, *.tga, *.tif, *.tiff, " \
                 "*.webp, *.xcf.bz2, *.xcf.gz",
                 QString{} );

    addCategory( tr( "image (uncompressed)" ),
                 "#88ffff",
                 "*.bmp, *.pbm, *.pgm, *.pnm, *.ppm, *.spr, *.svg, *.xcf",
                 QString{} );

    addCategory( tr( "junk" ),
                 Qt::red,
                 "*.bak, *.keep, *.old, *.orig, *.~",
                 "core" );

    addCategory( tr( "music" ),
                 Qt::yellow,
                 "*.aac, *.aif, *.ape, *.caf, *.dff, *.dsf, *.f4a, *.f4b, *.flac, " \
                 "*.m4a, *.m4b, *.mid, *.mka, *.mp3, *.oga, *.ogg, *.opus, *.ra, "  \
                 "*.rax, *.w64, *.wav, *.wma, *.wv, *.wvc",
                 QString{} );

    addCategory( tr( "object file" ),
                 "#ee8822",
                 "lib*.a",
                 "*.Po, *.a.cmd, *.al, *.elc, *.go, *.gresource, *.ko, *.ko.cmd, "   \
                 "*.ko.xz, *.ko.zst, *.la, *.lo, *.mo, *.moc, *.o, *.o.cmd, *.pyc, " \
                 "*.qrc, *.typelib, built-in.a, vmlinux.a" );

    addCategory( tr( "packaged program" ),
                 "#88aa66",
                 "*.rpm, *.xpi",
                 "*.deb, *.ja, *.jar, *.sfi, *.tm" );

    addCategory( tr( "script" ),
                 "#ff8888",
                 QString{},
                 "*.BAT, *.bash, *.bashrc, *.csh, *.js, *.ksh, *.m4, *.pl, *.pm, " \
                 "*.postinst, *.postrm, *.preinst, *.prerm, *.sh, *.tcl, *.tmac, " \
                 "*.xba, *.zsh" );

    addCategory( tr( "shared object" ),
                 "#ff7722",
                 "*.dll, *.so",
                 "*.so.*, *.so.0, *.so.1" );

    addCategory( tr( "source file" ),
                 "#ffbb44",
                 QString{},
                 "*.S, *.S_shipped, *.asm, *.c, *.cc, *.cmake, *.cpp, *.cxx, *.dts, "   \
                 "*.dtsi, *.el, *.f, *.fuc3, *.fuc3.h, *.fuc5, *.fuc5.h, *.gir, *.h, "  \
                 "*.h_shipped, *.hpp, *.java, *.msg, *.ph, *.php, *.po, *.pot, *.pro, " \
                 "*.pxd, *.py, *.pyi, *.pyx, *.rb, *.scm, Kbuild, Makefile" );

    addCategory( tr( "source file (generated)" ),
                 "#ffaa22",
                 QString{},
                 "*.f90, *.mod.c, *.ui, moc_*.cpp, qrc_*.cpp, ui_*.h" );

    addCategory( tr( "video" ),
                 "#aa44ff",
                 "*.asf, *.avi, *.divx, *.dv, *.flc, *.fli, *.flv, *.m2ts, *.m4v, *.mk3d, " \
                 "*.mkv, *.mov, *.mp2, *.mp4, *.mpeg, *.mpg, *.mts, *.ogm, *.ogv, *.rm, "   \
                 "*.vdr, *.vob, *.webm, *.wmp, *.wmv",
                 QString{} );

    writeSettings( _categories );
}
