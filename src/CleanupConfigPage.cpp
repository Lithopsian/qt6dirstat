/*
 *   File name: CleanupConfigPage.h
 *   Summary:   QDirStat configuration dialog classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QMouseEvent>

#include "CleanupConfigPage.h"
#include "ActionManager.h"
#include "Cleanup.h"
#include "CleanupCollection.h"
#include "ConfigDialog.h"
#include "OutputWindow.h"
#include "Typedefs.h" // _L1


#define CLEANUP_CAST(VOID_PTR) (static_cast<Cleanup *>(VOID_PTR))


using namespace QDirStat;


CleanupConfigPage::CleanupConfigPage( ConfigDialog * parent ):
    ListEditor{ parent },
    _ui{ new Ui::CleanupConfigPage },
    _outputWindowDefaultTimeout{ OutputWindow::defaultShowTimeout() }
{
    _ui->setupUi( this );

    enableEditWidgets( false );
    connectActions();
    enableWindowPolicyWidgets();

#if QT_VERSION >= QT_VERSION_CHECK( 6, 4, 0 )
    _ui->keySequenceEdit->setClearButtonEnabled( true );
#endif

    connect( _ui->outputWindowPolicyComboBox, QOverload<int>::of( &QComboBox::currentIndexChanged ),
             this,                            &CleanupConfigPage::enableWindowPolicyWidgets );

    connect( _ui->outputWindowDefaultTimeout, &QCheckBox::toggled,
             this,                            &CleanupConfigPage::enableWindowPolicyWidgets );

    connect( _ui->titleLineEdit,              &QLineEdit::textChanged,
             this,                            &CleanupConfigPage::titleChanged );

    connect( parent,                          &ConfigDialog::applyChanges,
             this,                            &CleanupConfigPage::applyChanges );
}


CleanupConfigPage::~CleanupConfigPage()
{
    // logDebug() << "CleanupConfigPage destructor" << Qt::endl;

    // Delete the working cleanup clones
    for ( int i = 0; i < _ui->listWidget->count(); ++i )
	delete CLEANUP_CAST( value( _ui->listWidget->item( i ) ) );
}


void CleanupConfigPage::applyChanges()
{
    // The values for the current cleanup action might have been modified and not yet saved
    save( value( _ui->listWidget->currentItem() ) );

    // Build a list of the working cleanups to write out to the settings file
    CleanupList cleanups;
    for ( int i = 0; i < _ui->listWidget->count(); ++i )
	cleanups << CLEANUP_CAST( value( _ui->listWidget->item( i ) ) );

    // Check if anything changed before writing, just for fun
    CleanupCollection * collection = ActionManager::cleanupCollection();
    for ( auto itOld = collection->cbegin(), itNew = cleanups.cbegin();
          itOld != collection->cend() || itNew != cleanups.cend();
          ++itOld, ++itNew )
    {
	// If we ran past the end of either list, or the cleanups don't match ...
	if ( itNew == cleanups.cend() || itOld == collection->cend() || **itOld != **itNew )
	{
	    collection->writeSettings( cleanups );
	    return;
	}
    }
}


void CleanupConfigPage::fillListWidget()
{
    _ui->listWidget->clear();

    for ( const Cleanup * cleanup : *ActionManager::cleanupCollection() )
    {
	// Make a deep copy so the config dialog can work without disturbing the real rules
	Cleanup * newCleanup = new Cleanup{ cleanup };
	createItem( newCleanup->cleanTitle(), newCleanup );
    }

    _ui->listWidget->setCurrentRow( 0 );
}


void CleanupConfigPage::enableWindowPolicyWidgets()
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
    QListWidgetItem * currentItem = _ui->listWidget->currentItem();

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
    if ( !cleanup )
	return;

    cleanup->setActive ( _ui->activeGroupBox->isChecked() );
    cleanup->setTitle  ( _ui->titleLineEdit->text()       );
    cleanup->setCommand( _ui->commandLineEdit->text()     );

    cleanup->setShortcut( _ui->keySequenceEdit->keySequence().toString() );

    if ( _ui->shellComboBox->currentText().startsWith( "$SHELL"_L1 ) )
	cleanup->setShell( QString{} );
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


void CleanupConfigPage::load( void * value )
{
    const Cleanup * cleanup = CLEANUP_CAST( value );
    if ( !cleanup )
    {
	enableEditWidgets( false );
	_ui->titleLineEdit->clear();
	_ui->commandLineEdit->clear();
	_ui->keySequenceEdit->clear();
	_ui->icon->clear();

	return;
    }

    enableEditWidgets( true );

    _ui->activeGroupBox->setChecked( cleanup->isActive() );
    _ui->titleLineEdit->setText( cleanup->title() );
    _ui->commandLineEdit->setText( cleanup->command() );
    _ui->keySequenceEdit->setKeySequence( cleanup->shortcut().toString() );

    QIcon icon = cleanup->icon();
    _ui->icon->setPixmap( icon.pixmap( icon.actualSize( QSize{ 24, 24 } ) ) );

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


void * CleanupConfigPage::newValue()
{
    return new Cleanup{};
}


void CleanupConfigPage::deleteValue( void * value )
{
    delete CLEANUP_CAST( value );
}


QString CleanupConfigPage::valueText( void * value )
{
    const Cleanup * cleanup = CLEANUP_CAST( value );

    return cleanup->cleanTitle();
}


void CleanupConfigPage::add()
{
    ListEditor::add();
    _ui->titleLineEdit->setFocus();
}
