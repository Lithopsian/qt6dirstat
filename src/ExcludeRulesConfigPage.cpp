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
#include "QDirStatApp.h"
#include "Logger.h"
#include "Exception.h"


// This is a mess that became necessary because Qt's moc cannot handle template
// classes. Yes, this is ugly.
#define EXCLUDE_RULE_CAST(VOID_PTR) (static_cast<ExcludeRule *>(VOID_PTR))


using namespace QDirStat;


ExcludeRulesConfigPage::ExcludeRulesConfigPage( ConfigDialog * parent ):
    ListEditor { parent },
    _ui { new Ui::ExcludeRulesConfigPage }
{
    CHECK_NEW( _ui );
    _ui->setupUi( this );

    setListWidget( _ui->listWidget );

    setMoveUpButton      ( _ui->moveUpButton       );
    setMoveDownButton    ( _ui->moveDownButton     );
    setMoveToTopButton   ( _ui->moveToTopButton    );
    setMoveToBottomButton( _ui->moveToBottomButton );
    setAddButton         ( _ui->addButton          );
    setRemoveButton      ( _ui->removeButton       );

    enableEditRuleWidgets( false );
    fillListWidget();
    updateActions();

    connect( _ui->patternLineEdit, &QLineEdit::textChanged,
	     this,                 &ExcludeRulesConfigPage::patternChanged );

    connect( parent,               &ConfigDialog::applyChanges,
	     this,                 &ExcludeRulesConfigPage::applyChanges );
}


ExcludeRulesConfigPage::~ExcludeRulesConfigPage()
{
    //logDebug() << "ExcludeRulesConfigPage destructor" << Qt::endl;

    // Delete the working rules
    for ( int i = 0; i < listWidget()->count(); ++i )
	delete EXCLUDE_RULE_CAST( value( listWidget()->item( i ) ) );

    delete _ui;
}


void ExcludeRulesConfigPage::applyChanges()
{
    // The values for the current rule might have been modified and not yet saved
    save( value( listWidget()->currentItem() ) );

    // Build a list of the working rules to write out to the settings file
    ExcludeRuleList rules;
    for ( int i = 0; i < listWidget()->count(); ++i )
	rules << EXCLUDE_RULE_CAST( value( listWidget()->item( i ) ) );

    // Check if anything changed before writing, just for fun
    const ExcludeRules * excludeRules = app()->dirTree()->excludeRules();
    ExcludeRuleListIterator it = excludeRules->cbegin();
    for ( int i = 0; i < rules.size() || it != excludeRules->cend(); ++i, ++it )
    {
	// If we ran past the end of either list, or the rules don't match ...
	if ( it == excludeRules->cend() || i == rules.size() || *it != rules.at( i ) )
	{
	    excludeRules->writeSettings( rules );
	    app()->dirTree()->setExcludeRules();
	    return;
	}
    }
}


void ExcludeRulesConfigPage::fillListWidget()
{
    listWidget()->clear();

    const ExcludeRules * excludeRules = app()->dirTree()->excludeRules();
    for ( ExcludeRuleListIterator it = excludeRules->cbegin(); it != excludeRules->cend(); ++it )
    {
	// Make a deep copy so the config dialog can work without disturbing the real rules
	ExcludeRule * rule = new ExcludeRule( *( *it ) );
	CHECK_NEW( rule );

	QListWidgetItem * item = new ListEditorItem( rule->pattern(), rule );
	CHECK_NEW( item );
	listWidget()->addItem( item );
    }

    QListWidgetItem * firstItem = listWidget()->item(0);

    if ( firstItem )
	listWidget()->setCurrentItem( firstItem );
}


void ExcludeRulesConfigPage::patternChanged( const QString & newPattern )
{
    QListWidgetItem * currentItem = listWidget()->currentItem();

    if ( currentItem )
	currentItem->setText( newPattern );
}


void ExcludeRulesConfigPage::enableEditRuleWidgets( bool enable )
{
    _ui->rightColumnWidget->setEnabled( enable );
}


void ExcludeRulesConfigPage::save( void * value )
{
    ExcludeRule * excludeRule = EXCLUDE_RULE_CAST( value );

    if ( ! excludeRule || updatesLocked() )
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

    if ( updatesLocked() )
        return;

    if ( ! excludeRule )
    {
        enableEditRuleWidgets( false );
        _ui->patternLineEdit->setText( "" );

	return;
    }

    enableEditRuleWidgets( true );
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


void * ExcludeRulesConfigPage::createValue()
{
    // "Empty" rule, but set the options that we want to start with:
    // wildcard, case-sensitive, and directory name without path
    ExcludeRule * excludeRule = new ExcludeRule( ExcludeRule::Wildcard, "", true, false, false );
    CHECK_NEW( excludeRule );

    return excludeRule;
}


void ExcludeRulesConfigPage::removeValue( void * value )
{
    ExcludeRule * excludeRule = EXCLUDE_RULE_CAST( value );
    CHECK_PTR( excludeRule );

    delete excludeRule;
}


QString ExcludeRulesConfigPage::valueText( void * value )
{
    const ExcludeRule * excludeRule = EXCLUDE_RULE_CAST( value );
    CHECK_PTR( excludeRule );

    return excludeRule->pattern();
}


void ExcludeRulesConfigPage::add()
{
    ListEditor::add();
    _ui->patternLineEdit->setFocus();
}
