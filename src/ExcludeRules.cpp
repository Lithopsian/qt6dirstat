/*
 *   File name: ExcludeRules.cpp
 *   Summary:   Support classes for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "ExcludeRules.h"
#include "FileInfoIterator.h"
#include "Logger.h"
#include "Settings.h"


#define VERBOSE_EXCLUDE_MATCHES 0


using namespace QDirStat;


QString ExcludeRule::formatPattern( PatternSyntax   patternSyntax,
				    const QString & pattern )
{
    // Anchor and escape all special characters so a regexp match behaves like a simple comparison
    if ( patternSyntax == FixedString )
	return Wildcard::anchoredPattern( Wildcard::escape( pattern ) );

    // Convert the *, ?, and [] wildcards to regexp equivalents and anchor the pattern
    if ( patternSyntax == Wildcard )
	return Wildcard::wildcardToRegularExpression( pattern );

    // Note: unanchored for regexp!
    return pattern;
}


void ExcludeRule::setPatternSyntax( PatternSyntax patternSyntax )
{
    _patternSyntax = patternSyntax;
    setPattern( formatPattern( patternSyntax, _pattern ) );
}


void ExcludeRule::setPattern( const QString & pattern )
{
    _pattern = pattern;
    QRegularExpression::setPattern( formatPattern( _patternSyntax, pattern ) );
}


bool ExcludeRule::match( const QString & fullPath, const QString & fileName ) const
{
    if ( _checkAnyFileChild )  // use matchDirectChildren() for those rules
        return false;

    const QString & matchText = _useFullPath ? fullPath : fileName;
    if ( matchText.isEmpty() || _pattern.isEmpty() )
	return false;

    return isMatch( matchText );
}


bool ExcludeRule::matchDirectChildren( const DirInfo * dir ) const
{
    if ( !_checkAnyFileChild || !dir )
        return false;

    if ( _pattern.isEmpty() )
        return false;

    // Search through FileInfo children to see if any match
    const DirInfo * parent = dir->dotEntry() ? dir->dotEntry() : dir;
    auto match = [ this ]( FileInfo * item ) { return !item->isDir() && isMatch( item->name() ); };
    return std::any_of( begin( parent ), end( parent ), match );
}


bool ExcludeRule::operator!=( const ExcludeRule * other ) const
{
    if ( !other )
        return true;

    if ( other == this )
        return false;

    if ( other->patternSyntax()     != patternSyntax() ||
	 other->pattern()           != pattern()       ||
	 other->caseSensitive()     != caseSensitive() ||
	 other->useFullPath()       != useFullPath()   ||
         other->checkAnyFileChild() != checkAnyFileChild() )
    {
        return true;
    }

    return false;
}


//
//---------------------------------------------------------------------------
//

namespace
{
    [[gnu::unused]] void dumpExcludeRules( const ExcludeRules * excludeRules )
    {
	if ( excludeRules->isEmpty() )
	    logDebug() << "No exclude rules defined" << Qt::endl;

	for ( const ExcludeRule * excludeRule : *excludeRules )
	    logDebug() << excludeRule << Qt::endl;
    }

} // namespace


ExcludeRules::ExcludeRules()
{
    readSettings();
#if VERBOSE_EXCLUDE_MATCHES
    dumpExcludeRules( this );
#endif
}


ExcludeRules::ExcludeRules( const QStringList & paths,
			    ExcludeRule::PatternSyntax patternSyntax,
			    bool caseSensitive,
			    bool useFullPath,
			    bool checkAnyFileChild )
{
    for ( const QString & path : paths )
        add( patternSyntax, path, caseSensitive, useFullPath, checkAnyFileChild );
}


void ExcludeRules::clear()
{
    qDeleteAll( _rules );
    _rules.clear();
}


void ExcludeRules::add( ExcludeRule::PatternSyntax   patternSyntax,
			const QString              & pattern,
			bool                         caseSensitive,
                        bool                         useFullPath,
                        bool                         checkAnyFileChild )
{
    ExcludeRule * rule = new ExcludeRule( patternSyntax, pattern, caseSensitive, useFullPath, checkAnyFileChild );
    _rules << rule;

    logInfo() << "Added " << rule << Qt::endl;
}


bool ExcludeRules::match( const QString & fullPath, const QString & fileName ) const
{
    if ( fullPath.isEmpty() || fileName.isEmpty() )
	return false;

    for ( const ExcludeRule * rule : asConst( _rules ) )
    {
	if ( rule->match( fullPath, fileName ) )
	{
#if VERBOSE_EXCLUDE_MATCHES
	    logDebug() << fullPath << " matches " << rule << Qt::endl;
#endif
	    return true;
	}
    }

    return false;
}


bool ExcludeRules::matchDirectChildren( const DirInfo * dir ) const
{
    if ( !dir )
	return false;

    for ( const ExcludeRule * rule : asConst( _rules ) )
    {
	if ( rule->matchDirectChildren( dir ) )
	{
#if VERBOSE_EXCLUDE_MATCHES
	    logDebug() << dir << " matches " << rule << Qt::endl;
#endif
	    return true;
	}
    }

    return false;
}


void ExcludeRules::addDefaultRules()
{
    //logInfo() << "Adding default exclude rules" << Qt::endl;

    add( ExcludeRule::FixedString, "/timeshift", true, true, false );

    ExcludeRuleSettings settings;
    settings.setValue( "DefaultExcludeRulesAdded", true );

    writeSettings( _rules );
}


namespace
{
    /**
     * Return the enum mapping for the pattern syntax enum PatternSyntax
     **/
    SettingsEnumMapping patternSyntaxMapping()
    {
	return { { ExcludeRule::RegExp,      "RegExp"_L1      },
		 { ExcludeRule::Wildcard,    "Wildcard"_L1    },
		 { ExcludeRule::FixedString, "FixedString"_L1 },
	       };
    }

} // namespace


void ExcludeRules::readSettings()
{
//    clear(); // a new object is always used

    const SettingsEnumMapping mapping = patternSyntaxMapping();

    ExcludeRuleSettings settings;

    // Read all settings groups [ExcludeRule_xx] that were found
    const auto groups = settings.findListGroups();
    for ( const QString & groupName : groups )
    {
	settings.beginGroup( groupName );

	// Read one exclude rule
	const QString pattern        = settings.value( "Pattern"                  ).toString();
	const bool caseSensitive     = settings.value( "CaseSensitive",     true  ).toBool();
	const bool useFullPath       = settings.value( "UseFullPath",       false ).toBool();
	const bool checkAnyFileChild = settings.value( "CheckAnyFileChild", false ).toBool();

	const int syntax = settings.enumValue( "Syntax", ExcludeRule::RegExp, mapping );

	ExcludeRule * rule = new ExcludeRule( ( ExcludeRule::PatternSyntax )syntax,
					      pattern,
					      caseSensitive,
					      useFullPath,
					      checkAnyFileChild );

	if ( !pattern.isEmpty() && rule->isValid() )
	{
	    _rules << rule;
	}
	else
	{
	    logError() << "Invalid regexp: \"" << rule->pattern() << "\": " << rule->errorString() << Qt::endl;
	    delete rule;
	}

	settings.endGroup(); // [ExcludeRule_01], [ExcludeRule_02], ...
    }

    if ( isEmpty() && !settings.value( "DefaultExcludeRulesAdded", false ).toBool() )
	addDefaultRules();
}


void ExcludeRules::writeSettings( const ExcludeRuleList & newRules )
{
    ExcludeRuleSettings settings;

    // Remove all leftover exclude rule descriptions
    settings.removeListGroups();

    SettingsEnumMapping mapping = patternSyntaxMapping();

    // Similar to CleanupCollection::writeSettings(), using a separate group
    // for each exclude rule for better readability in the settings file.
    for ( int i=0; i < newRules.size(); ++i )
    {
	const ExcludeRule * rule = newRules.at(i);
	if ( rule && !rule->pattern().isEmpty() )
	{
	    settings.beginListGroup( i+1 );

	    settings.setValue( "Pattern",           rule->pattern()           );
	    settings.setValue( "CaseSensitive",     rule->caseSensitive()     );
	    settings.setValue( "UseFullPath",       rule->useFullPath()       );
	    settings.setValue( "CheckAnyFileChild", rule->checkAnyFileChild() );

	    settings.setEnumValue( "Syntax", rule->patternSyntax(), mapping );

	    settings.endListGroup(); // [ExcludeRule_01], [ExcludeRule_02], ...
	}
    }
}
