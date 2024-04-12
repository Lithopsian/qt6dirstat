/*
 *   File name: CleanupConfigPage.h
 *   Summary:	QDirStat configuration dialog classes
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include "CleanupConfigPage.h"
#include "ActionManager.h"
#include "Cleanup.h"
#include "CleanupCollection.h"
#include "ConfigDialog.h"
#include "OutputWindow.h"
#include "Logger.h"
#include "Exception.h"


// This is a mess that became necessary because Qt's moc cannot handle template
// classes. Yes, this is ugly.
#define CLEANUP_CAST(VOID_PTR) (static_cast<Cleanup *>(VOID_PTR))

using namespace QDirStat;


CleanupConfigPage::CleanupConfigPage( ConfigDialog * parent ):
    ListEditor ( parent ),
    _ui { new Ui::CleanupConfigPage },
    _outputWindowDefaultTimeout { OutputWindow::defaultShowTimeout() }
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

    enableEditWidgets( false );
    fillListWidget();
    _ui->toolBox->setCurrentIndex( 0 );
    enableWidgets();
    updateActions();

    connect( _ui->outputWindowPolicyComboBox, qOverload<int>( &QComboBox::currentIndexChanged ),
	     this,		              &CleanupConfigPage::enableWidgets );

    connect( _ui->outputWindowDefaultTimeout, &QCheckBox::stateChanged,
	     this,		              &CleanupConfigPage::enableWidgets );

    connect( _ui->titleLineEdit,              &QLineEdit::textChanged,
	     this,		              &CleanupConfigPage::titleChanged );

    connect( parent,             &ConfigDialog::applyChanges,
	     this,		 &CleanupConfigPage::applyChanges );
}


CleanupConfigPage::~CleanupConfigPage()
{
    // logDebug() << "CleanupConfigPage destructor" << Qt::endl;

    // Delete the working cleanup clones
    for ( int i = 0; i < listWidget()->count(); ++i )
	delete CLEANUP_CAST( value( listWidget()->item( i ) ) );

    delete _ui;
}


void CleanupConfigPage::applyChanges()
{
    // logDebug() << Qt::endl;

    // The values for the current cleanup action might have been modified and not yet saved
    save( value( listWidget()->currentItem() ) );

    // Build a list of the working cleanups to write out to the settings file
    CleanupList cleanups;
    for ( int i = 0; i < listWidget()->count(); ++i )
	cleanups << CLEANUP_CAST( value( listWidget()->item( i ) ) );

    // Check if anything changed before writing, just for fun
    CleanupCollection * collection = ActionManager::cleanupCollection();
    for ( int iOld = 0, iNew = 0; iNew < cleanups.size() || iOld < collection->size(); ++iOld, ++iNew )
    {
	// If we ran past the end of either list, or the cleanups don't match ...
	if ( iNew == cleanups.size() ||
	     iOld == collection->size() ||
	     cleanups.at( iNew ) != collection->at( iOld ) )
	{
	    collection->writeSettings( cleanups );
	    return;
	}
    }
}


void CleanupConfigPage::fillListWidget()
{
    listWidget()->clear();

    for ( const Cleanup * cleanup : ActionManager::cleanupCollection()->cleanupList() )
    {
	// Make a deep copy so the config dialog can work without disturbing the real rules
	Cleanup * newCleanup = new Cleanup( cleanup );

	QListWidgetItem * item = new ListEditorItem( newCleanup->cleanTitle(), newCleanup );
	CHECK_NEW( item );
	listWidget()->addItem( item );
    }

    QListWidgetItem * firstItem = listWidget()->item(0);
    if ( firstItem )
	listWidget()->setCurrentItem( firstItem );
}


void CleanupConfigPage::enableWidgets()
{
    const int  policyIndex      = _ui->outputWindowPolicyComboBox->currentIndex();
    const bool show             = policyIndex != Cleanup::ShowNever;
    const bool showAfterTimeout = policyIndex == Cleanup::ShowAfterTimeout;
    const bool showIfNoError    = show && policyIndex != Cleanup::ShowIfErrorOutput;
    const bool useDefault       = _ui->outputWindowDefaultTimeout->isChecked();

    _ui->outputWindowDefaultTimeout->setEnabled( show );

    _ui->outputWindowDefaultTimeout->setEnabled( showAfterTimeout );
    _ui->outputWindowTimeoutCaption->setEnabled( showAfterTimeout && !useDefault );
    _ui->outputWindowTimeoutSpinBox->setEnabled( showAfterTimeout && !useDefault );
    if ( useDefault )
	_ui->outputWindowTimeoutSpinBox->setValue( _outputWindowDefaultTimeout / 1000.0 );

    _ui->outputWindowAutoClose->setEnabled( showIfNoError );
}


void CleanupConfigPage::titleChanged( const QString & newTitle )
{
    QListWidgetItem * currentItem = listWidget()->currentItem();

    if ( currentItem )
    {
	Cleanup * cleanup = CLEANUP_CAST( value( currentItem ) );
	cleanup->setTitle( newTitle );
	currentItem->setText( cleanup->cleanTitle() );
    }
}


void CleanupConfigPage::save( void * value )
{
    Cleanup * cleanup = CLEANUP_CAST( value );
    // logDebug() << cleanup << Qt::endl;

    if ( !cleanup || updatesLocked() )
	return;

    cleanup->setActive ( _ui->activeGroupBox->isChecked() );
    cleanup->setTitle  ( _ui->titleLineEdit->text()       );
    cleanup->setCommand( _ui->commandLineEdit->text()     );

    if ( _ui->shellComboBox->currentText().startsWith( "$SHELL" ) )
	cleanup->setShell( "" );
    else
	cleanup->setShell( _ui->shellComboBox->currentText() );

    cleanup->setRecurse( _ui->recurseCheckBox->isChecked() );
    cleanup->setAskForConfirmation( _ui->askForConfirmationCheckBox->isChecked() );

    const int refreshPolicy = _ui->refreshPolicyComboBox->currentIndex();
    cleanup->setRefreshPolicy( static_cast<Cleanup::RefreshPolicy>( refreshPolicy ) );

    cleanup->setWorksForDir     ( _ui->worksForDirCheckBox->isChecked()        );
    cleanup->setWorksForFile    ( _ui->worksForFilesCheckBox->isChecked()      );
    cleanup->setWorksForDotEntry( _ui->worksForDotEntriesCheckBox->isChecked() );

    const int outputPolicy = _ui->outputWindowPolicyComboBox->currentIndex();
    cleanup->setOutputWindowPolicy( static_cast<Cleanup::OutputWindowPolicy>( outputPolicy ) );

    const bool useDefaultTimeout = _ui->outputWindowDefaultTimeout->isChecked();
    cleanup->setOutputWindowTimeout( useDefaultTimeout ? 0 : _ui->outputWindowTimeoutSpinBox->value() * 1000 );

    cleanup->setOutputWindowAutoClose( _ui->outputWindowAutoClose->isChecked() );
}


void CleanupConfigPage::enableEditWidgets( bool enable )
{
    _ui->activeGroupBox->setEnabled( enable );
}


void CleanupConfigPage::load( void * value )
{
    if ( updatesLocked() )
	return;

    const Cleanup * cleanup = CLEANUP_CAST( value );
    if ( !cleanup )
    {
	enableEditWidgets( false );

	return;
    }

    enableEditWidgets( true );

    _ui->activeGroupBox->setChecked( cleanup->isActive() );
    _ui->titleLineEdit->setText( cleanup->title() );
    _ui->icon->setPixmap( cleanup->iconName() );
    _ui->commandLineEdit->setText( cleanup->command() );

    if ( cleanup->shell().isEmpty() )
    {
	// Show the default option, the login shell from the environment variable
	_ui->shellComboBox->setCurrentIndex( 0 );
    }
    else
    {
	// Find or insert the configured shell in the combobox list
	const int index = _ui->shellComboBox->findText( cleanup->shell() );
	if ( index == -1 )
	    _ui->shellComboBox->insertItem( 1, cleanup->shell() );
	_ui->shellComboBox->setCurrentIndex( index == -1 ? 1 : index );
    }

    _ui->recurseCheckBox->setChecked           ( cleanup->recurse()            );
    _ui->askForConfirmationCheckBox->setChecked( cleanup->askForConfirmation() );
    _ui->refreshPolicyComboBox->setCurrentIndex( cleanup->refreshPolicy()      );

    _ui->worksForDirCheckBox->setChecked       ( cleanup->worksForDir()        );
    _ui->worksForFilesCheckBox->setChecked     ( cleanup->worksForFile()       );
    _ui->worksForDotEntriesCheckBox->setChecked( cleanup->worksForDotEntry()   );

    _ui->outputWindowPolicyComboBox->setCurrentIndex( cleanup->outputWindowPolicy() );

    const int cleanupTimeout = cleanup->outputWindowTimeout();
    const bool defaultTimeout = cleanupTimeout == 0;
    const int timeout = defaultTimeout ? _outputWindowDefaultTimeout : cleanupTimeout;
    _ui->outputWindowTimeoutSpinBox->setValue  ( timeout / 1000.0                 );
    _ui->outputWindowDefaultTimeout->setChecked( defaultTimeout                   );
    _ui->outputWindowAutoClose->setChecked     ( cleanup->outputWindowAutoClose() );
}


void * CleanupConfigPage::createValue()
{
    Cleanup * cleanup = new Cleanup();
    CHECK_NEW( cleanup );

    return cleanup;
}


void CleanupConfigPage::removeValue( void * value )
{
    Cleanup * cleanup = CLEANUP_CAST( value );
    CHECK_PTR( cleanup );

    delete cleanup;
}


QString CleanupConfigPage::valueText( void * value )
{
    const Cleanup * cleanup = CLEANUP_CAST( value );
    CHECK_PTR( cleanup );

    return cleanup->cleanTitle();
}
