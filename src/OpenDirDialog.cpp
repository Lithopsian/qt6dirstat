/*
 *   File name: OpenDirDialog.cpp
 *   Summary:   QDirStat "open directory" dialog
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QPushButton>
#include <QLineEdit>
#include <QDir>
#include <QFileSystemModel>
#include <QTimer>

#include "OpenDirDialog.h"
#include "ExistingDir.h"
#include "MountPoints.h"
#include "Settings.h"
#include "SettingsHelpers.h"
#include "SignalBlocker.h"
#include "Logger.h"
#include "Exception.h"


// The ExistingDirCompleter is useful if there are no other navigation tools
// that compete for widget focus and user attention, but here it's only
// confusing: The dirTreeView, the pathCompleter and the pathComboBox fire off
// a cascade of signals for every tiny change, and that makes the completer
// behave very erratic.
#define USE_COMPLETER           0
#define VERBOSE_SELECTION       0


using namespace QDirStat;


OpenDirDialog::OpenDirDialog( QWidget * parent, bool crossFilesystems ):
    QDialog ( parent ),
    _ui { new Ui::OpenDirDialog },
    _filesystemModel { new QFileSystemModel( this ) }
{
    CHECK_NEW( _ui );
    CHECK_NEW( _filesystemModel );
    _ui->setupUi( this );

    MountPoints::reload();

    initPathComboBox();
    initDirTree();

    initConnections();
    readSettings();

    _ui->crossFilesystemsCheckBox->setChecked( crossFilesystems );
    _ui->pathComboBox->setFocus();

    setPath( QDir::currentPath() );
}


OpenDirDialog::~OpenDirDialog()
{
    // Always save the window geometry, even if cancelled
    writeSettings();
    delete _ui;
}


void OpenDirDialog::initPathComboBox()
{
    QLineEdit * lineEdit = _ui->pathComboBox->lineEdit();
    if ( lineEdit )
        lineEdit->setClearButtonEnabled( true );

#if USE_COMPLETER
    QCompleter * completer = new ExistingDirCompleter( this );
    CHECK_NEW( completer );
    _ui->pathComboBox->setCompleter( completer );
#endif

    _validator = new ExistingDirValidator( this );
    CHECK_NEW( _validator );
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
//    _ui->dirTreeView->setHeaderHidden( true );
}


void OpenDirDialog::initConnections()
{
    const QItemSelectionModel * selectionModel = _ui->dirTreeView->selectionModel();
    QPushButton * okButton = _ui->buttonBox->button( QDialogButtonBox::Ok );
    CHECK_PTR( okButton );

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
    if ( _settingPath || path == _lastPath )
        return;

    // This flag is needed to avoid signal cascades from all the different
    // widgets in the dialog: The pathSelector (the "places"), the
    // pathComboBox, the dirTreeView. If they change their current path, they
    // will also fire off signals to notify all their subscribers which will
    // then change their current path and fire off their signals. We need to
    // break that cycle; that's what this flag is for.
    //
    // Unfortunately just blocking the signals sometimes has bad side effects
    // (see below).
    _settingPath = true;

#if VERBOSE_SELECTION
    logDebug() << "Selecting " << path << Qt::endl;
#endif

    SignalBlocker sigBlockerPathSelector( _ui->pathSelector );
    // Can't block signals of the dirTreeView's selection model:
    // This would mean that the dirTreeView also isn't notified,
    // so any change would not become visible in the tree.

    _ui->dirTreeView->setCurrentIndex( _filesystemModel->index( path ) );

    populatePathComboBox();
    QLineEdit * lineEdit = _ui->pathComboBox->lineEdit();
    if ( lineEdit )
        lineEdit->setText( path );

    // Once the tree is populated and expanded, scroll to the selected row
    QTimer::singleShot( 250, this, &OpenDirDialog::scrollTo );

    _lastPath = path;
    _settingPath = false;
}


void OpenDirDialog::scrollTo()
{
    _ui->dirTreeView->scrollTo( _filesystemModel->index( _lastPath ) );
}


void OpenDirDialog::pathSelected( const QString & path )
{
    setPath( path );

    SignalBlocker sigBlockerPathSelector( _ui->pathSelector );
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
    if ( !ok || _settingPath )
        return;

    const SignalBlocker sigBlockerComboBox ( _ui->pathComboBox );
    const SignalBlocker sigBlockerValidator( _validator );

    setPath( _ui->pathComboBox->currentText() );
}


void OpenDirDialog::treeSelection( const QModelIndex & newCurrentItem,
                                   const QModelIndex & )
{
    setPath( _filesystemModel->filePath( newCurrentItem ) );
}


void OpenDirDialog::populatePathComboBox()
{
    _ui->pathComboBox->clear();

    for ( QModelIndex index = _ui->dirTreeView->currentIndex(); index.isValid(); index =_filesystemModel->parent( index ) )
        _ui->pathComboBox->addItem( _filesystemModel->filePath( index ) );
}


void OpenDirDialog::goUp()
{
    if ( _ui->pathComboBox->count() > 1 )
        _ui->pathComboBox->removeItem( 0 );
}


void OpenDirDialog::readSettings()
{
    // logDebug() << Qt::endl;

    readWindowSettings( this, "OpenDirDialog" );

    Settings settings;
    settings.beginGroup( "OpenDirDialog" );
    const QByteArray mainSplitterState = settings.value( "MainSplitter" , QByteArray() ).toByteArray();
    settings.endGroup();

    if ( !mainSplitterState.isNull() )
        _ui->mainSplitter->restoreState( mainSplitterState );
}


void OpenDirDialog::writeSettings()
{
    // logDebug() << Qt::endl;

    // Do NOT write _crossFilesystems back to the settings here; this is done
    // from the config dialog. The value in this dialog is just temporary for
    // the current program run.

    Settings settings;

    writeWindowSettings( this, "OpenDirDialog" );

    settings.beginGroup( "OpenDirDialog" );
    settings.setValue( "MainSplitter", _ui->mainSplitter->saveState()  );
    settings.endGroup();
}


QString OpenDirDialog::askOpenDir( QWidget * parent, bool & crossFilesystems )
{
    OpenDirDialog dialog( parent, crossFilesystems );
    dialog.pathSelector()->addHomeDir();
    dialog.pathSelector()->addMountPoints( MountPoints::normalMountPoints() );
    //logDebug() << "Waiting for user selection" << Qt::endl;

    if ( dialog.exec() == QDialog::Rejected )
    {
        //logInfo() << "[Cancel]" << Qt::endl;
        return QString();
    }

    // Remember the flag just for the next time the dialog is opened
    crossFilesystems = dialog.crossFilesystems();

    const QString path = dialog.selectedPath();
    //logInfo() << "User selected path " << path << Qt::endl;

    return path;
}

