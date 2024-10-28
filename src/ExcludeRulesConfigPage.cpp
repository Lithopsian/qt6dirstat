/*
 *   File name: ExcludeRulesConfigPage.h
 *   Summary:   QDirStat configuration dialog classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "ExcludeRulesConfigPage.h"
#include "ConfigDialog.h"
#include "DirTree.h"
#include "ExcludeRules.h"
#include "FormatUtil.h"
#include "QDirStatApp.h"


#define EXCLUDE_RULE_CAST(VOID_PTR) (static_cast<ExcludeRule *>(VOID_PTR))


using namespace QDirStat;


ExcludeRulesConfigPage::ExcludeRulesConfigPage( ConfigDialog * parent ):
    ListEditor{ parent },
    _ui{ new Ui::ExcludeRulesConfigPage }
{
    _ui->setupUi( this );
    _ui->patternLineEdit->setValidator( new QRegularExpressionValidator{ hasNoControlCharacters(), this } );

    enableEditWidgets( false );
    initListWidget();

    connect( _ui->patternLineEdit, &QLineEdit::textChanged,
             this,                 &ExcludeRulesConfigPage::patternChanged );

    connect( parent,               &ConfigDialog::applyChanges,
             this,                 &ExcludeRulesConfigPage::applyChanges );
}


ExcludeRulesConfigPage::~ExcludeRulesConfigPage()
{
    //logDebug() << "ExcludeRulesConfigPage destructor" << Qt::endl;

    // Delete the working rules
    for ( int i = 0; i < _ui->listWidget->count(); ++i )
	delete EXCLUDE_RULE_CAST( value( _ui->listWidget->item( i ) ) );
}


void ExcludeRulesConfigPage::applyChanges()
{
    // The values for the current rule might have been modified and not yet saved
    save( value( _ui->listWidget->currentItem() ) );

    // Build a list of the working rules to write out to the settings file
    ExcludeRuleList rules;
    for ( int i = 0; i < _ui->listWidget->count(); ++i )
	rules << EXCLUDE_RULE_CAST( value( _ui->listWidget->item( i ) ) );

    // Check if anything changed before writing, just for fun
    DirTree * tree = app()->dirTree();
    const auto excludeRulesEnd = tree->excludeRules()->cend();
    for ( auto itOld = tree->excludeRules()->cbegin(), itNew = rules.cbegin();
          itNew != rules.cend() || itOld != excludeRulesEnd;
          ++itNew, ++itOld )
    {
	// If we ran past the end of either list, or the rules don't match ...
	if ( itNew == rules.cend() || itOld == excludeRulesEnd || **itOld != **itNew )
	{
	    ExcludeRules::writeSettings( rules );
	    tree->setExcludeRules();
	    return;
	}
    }
}


void ExcludeRulesConfigPage::fillListWidget()
{
    for ( const ExcludeRule * excludeRule : *( app()->dirTree()->excludeRules() ) )
    {
	// Make a deep copy so the config dialog can work without disturbing the real rules
	ExcludeRule * rule = new ExcludeRule{ *excludeRule };
	createItem( rule->pattern(), rule );
    }
}


void ExcludeRulesConfigPage::patternChanged( const QString & newPattern )
{
    QListWidgetItem * currentItem = _ui->listWidget->currentItem();

    if ( currentItem )
	currentItem->setText( newPattern );
}


void ExcludeRulesConfigPage::save( void * value )
{
    ExcludeRule * excludeRule = EXCLUDE_RULE_CAST( value );
    if ( !excludeRule )
	return;

    if ( _ui->regexpRadioButton->isChecked() )
	excludeRule->setPatternSyntax( ExcludeRule::RegExp );
    else if ( _ui->wildcardsRadioButton->isChecked() )
	excludeRule->setPatternSyntax( ExcludeRule::Wildcard );
    else if ( _ui->fixedStringRadioButton->isChecked() )
	excludeRule->setPatternSyntax( ExcludeRule::FixedString );

    excludeRule->setCaseSensitive( _ui->caseSensitiveCheckBox->isChecked() );

    excludeRule->setPattern( _ui->patternLineEdit->text() );

    excludeRule->setUseFullPath( _ui->fullPathRadioButton->isChecked() );
    excludeRule->setCheckAnyFileChild( _ui->checkAnyFileChildRadioButton->isChecked() );
}


void ExcludeRulesConfigPage::load( void * value )
{
    const ExcludeRule * excludeRule = EXCLUDE_RULE_CAST( value );
    if ( !excludeRule )
    {
	enableEditWidgets( false );
	_ui->patternLineEdit->clear();

	return;
    }

    enableEditWidgets( true );
    _ui->patternLineEdit->setText( excludeRule->pattern() );

    _ui->caseSensitiveCheckBox->setChecked( excludeRule->caseSensitive() );

    switch ( excludeRule->patternSyntax() )
    {
	case ExcludeRule::RegExp:
	    _ui->regexpRadioButton->setChecked( true );
	    break;

	case ExcludeRule::Wildcard:
	    _ui->wildcardsRadioButton->setChecked( true );
	    break;

	case ExcludeRule::FixedString:
	    _ui->fixedStringRadioButton->setChecked( true );
	    break;

	default:
	    break;
    }

    if ( excludeRule->useFullPath() )
	_ui->fullPathRadioButton->setChecked( true );
    else if ( excludeRule->checkAnyFileChild() )
        _ui->checkAnyFileChildRadioButton->setChecked( true );
    else
	_ui->dirNameWithoutPathRadioButton->setChecked( true );
}


void * ExcludeRulesConfigPage::newValue()
{
    // "Empty" rule, but set the options that we want to start with:
    // wildcard, case-sensitive, and directory name without path
    return new ExcludeRule{ ExcludeRule::Wildcard, QString{}, true, false, false };
}


void ExcludeRulesConfigPage::deleteValue( void * value )
{
    delete EXCLUDE_RULE_CAST( value );
}


QString ExcludeRulesConfigPage::valueText( void * value )
{
    const ExcludeRule * excludeRule = EXCLUDE_RULE_CAST( value );

    return excludeRule->pattern();
}


void ExcludeRulesConfigPage::add()
{
    ListEditor::add();
    _ui->patternLineEdit->setFocus();
}
