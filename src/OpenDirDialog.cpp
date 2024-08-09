/*
 *   File name: OpenDirDialog.cpp
 *   Summary:   QDirStat "open directory" dialog
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QDir>
#include <QFileSystemModel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>

#include "OpenDirDialog.h"
#include "ExistingDir.h"
#include "Logger.h"
#include "Settings.h"
#include "SignalBlocker.h"


#define VERBOSE_SELECTION 0


using namespace QDirStat;


OpenDirDialog::OpenDirDialog( QWidget * parent, bool crossFilesystems ):
    QDialog{ parent },
    _ui{ new Ui::OpenDirDialog },
    _filesystemModel{ new QFileSystemModel{ this } }
{
    _ui->setupUi( this );

    _ui->pathSelector->addHomeDir();
    _ui->pathSelector->addNormalMountPoints();
    _ui->crossFilesystemsCheckBox->setChecked( crossFilesystems );

    initPathComboBox();
    initDirTree();
    initConnections();
    readSettings();

    setPath( QDir::currentPath() );
    QTimer::singleShot( 250, this, [ this ]()
    {
        // Have to scroll to this after the tree has actually been instantiated
        _ui->dirTreeView->scrollTo( _filesystemModel->index( _lastPath ) );
    } );
}


OpenDirDialog::~OpenDirDialog()
{
    // Always save the window geometry, even if cancelled
    writeSettings();
}


void OpenDirDialog::initPathComboBox()
{
    QLineEdit * lineEdit = _ui->pathComboBox->lineEdit();
    if ( lineEdit )
        lineEdit->setClearButtonEnabled( true );

    _validator = new ExistingDirValidator{ this };
    _ui->pathComboBox->setValidator( _validator );
}


void OpenDirDialog::initDirTree()
{
    _filesystemModel->setRootPath( "/" );
    _filesystemModel->setFilter( QDir::Dirs       |
                                 QDir::NoDot      |
                                 QDir::NoDotDot   |
                                 QDir::NoSymLinks |
                                 QDir::Drives );

    _ui->dirTreeView->setModel( _filesystemModel );
    _ui->dirTreeView->hideColumn( 3 );  // Date Modified
    _ui->dirTreeView->hideColumn( 2 );  // Type
    _ui->dirTreeView->hideColumn( 1 );  // Size
}


void OpenDirDialog::initConnections()
{
    const QItemSelectionModel * selectionModel = _ui->dirTreeView->selectionModel();
    const QPushButton         * okButton       = _ui->buttonBox->button( QDialogButtonBox::Ok );

    connect( selectionModel,    &QItemSelectionModel::currentChanged,
             this,              &OpenDirDialog::treeSelection );

    connect( _ui->upButton,     &QPushButton::clicked,
             this,              &OpenDirDialog::goUp );

    connect( _validator,        &ExistingDirValidator::isOk,
             okButton,          &QPushButton::setEnabled );

    connect( _validator,        &ExistingDirValidator::isOk,
             this,              &OpenDirDialog::pathEdited );

    connect( _ui->pathSelector, &PathSelector::pathSelected,
             this,              &OpenDirDialog::pathSelected );

    connect( _ui->pathSelector, &PathSelector::pathDoubleClicked,
             this,              &OpenDirDialog::pathDoubleClicked );

}


void OpenDirDialog::setPath( const QString & path )
{
    // Important to stop signal cascades
    if ( path == _lastPath )
        return;

#if VERBOSE_SELECTION
    logDebug() << "Selecting " << path << Qt::endl;
#endif

    // Can't block signals of the dirTreeView's selection model:
    // This would mean that the dirTreeView also isn't notified,
    // so any change would not become visible in the tree.
    const SignalBlocker sigBlockerValidator( _validator );

    const QModelIndex currentIndex = _filesystemModel->index( path );
    populatePathComboBox( currentIndex );
    _ui->dirTreeView->setCurrentIndex( currentIndex );

    _lastPath = path;
}


void OpenDirDialog::pathSelected( const QString & path )
{
#if VERBOSE_SELECTION
    logDebug() << "Selected " << path << Qt::endl;
#endif

    // Can block selection model here as we manually select and scroll in the tree
    const SignalBlocker sigBlockerSelection( _ui->dirTreeView->selectionModel() );

    setPath( path );

    const QModelIndex index = _filesystemModel->index( path );
    _ui->dirTreeView->collapseAll();
    _ui->dirTreeView->setExpanded( index, true );
    _ui->dirTreeView->scrollTo( index, QAbstractItemView::PositionAtTop );
}


void OpenDirDialog::pathDoubleClicked( const QString & path )
{
    setPath( path );
    accept();
}


void OpenDirDialog::pathEdited( bool ok )
{
    if ( !ok )
        return;

#if VERBOSE_SELECTION
    logDebug() << _ui->pathComboBox->currentText() << Qt::endl;
#endif

    setPath( _ui->pathComboBox->currentText() );
}


void OpenDirDialog::treeSelection( const QModelIndex & newCurrentItem )
{
#if VERBOSE_SELECTION
    logDebug() << _ui->pathComboBox->currentText() << Qt::endl;
#endif

    setPath( _filesystemModel->filePath( newCurrentItem ) );
}


void OpenDirDialog::populatePathComboBox( const QModelIndex & currentIndex )
{
    // Keep the contents if the new path is already in the list
    for ( int i=0; i < _ui->pathComboBox->count(); ++i )
    {
        if ( _ui->pathComboBox->itemText( i ) == _filesystemModel->filePath( currentIndex ) )
        {
            _ui->pathComboBox->setCurrentIndex( i );
            return;
        }
    }

    // Build a new list from the current path and all its ancestors
    _ui->pathComboBox->clear();
    for ( QModelIndex index = currentIndex; index.isValid(); index = _filesystemModel->parent( index ) )
        _ui->pathComboBox->addItem( _filesystemModel->filePath( index ) );
}


void OpenDirDialog::goUp()
{
    // Just move one place in the combobox list
    const int nextIndex = _ui->pathComboBox->currentIndex() + 1;
    if ( nextIndex < _ui->pathComboBox->count() )
        _ui->pathComboBox->setCurrentIndex( nextIndex );
}


void OpenDirDialog::readSettings()
{
    Settings::readWindowSettings( this, "OpenDirDialog" );

    Settings settings;

    settings.beginGroup( "OpenDirDialog" );
    const QByteArray mainSplitterState = settings.value( "MainSplitter" , QByteArray() ).toByteArray();
    settings.endGroup();

    if ( !mainSplitterState.isNull() )
        _ui->mainSplitter->restoreState( mainSplitterState );
}


void OpenDirDialog::writeSettings()
{
    // Do NOT write _crossFilesystems back to the settings here; this is done
    // from the config dialog. The value in this dialog is just temporary for
    // the current program run.

    Settings::writeWindowSettings( this, "OpenDirDialog" );

    Settings settings;

    settings.beginGroup( "OpenDirDialog" );
    settings.setValue( "MainSplitter", _ui->mainSplitter->saveState()  );
    settings.endGroup();
}


QString OpenDirDialog::askOpenDir( QWidget * parent, bool & crossFilesystems )
{
    OpenDirDialog dialog( parent, crossFilesystems );
    //logDebug() << "Waiting for user selection" << Qt::endl;

    if ( dialog.exec() == QDialog::Rejected )
    {
        //logInfo() << "[Cancel]" << Qt::endl;
        return QString{};
    }

    // Remember the flag just for the next time the dialog is opened
    crossFilesystems = dialog.crossFilesystems();

    const QString path = dialog.selectedPath();
    //logInfo() << "User selected path " << path << Qt::endl;

    return path;
}

