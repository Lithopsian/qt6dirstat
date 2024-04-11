/*
 *   File name: OpenDirDialog.cpp
 *   Summary:	QDirStat "open directory" dialog
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
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

    _okButton = _ui->buttonBox->button( QDialogButtonBox::Ok );
    CHECK_PTR( _okButton );

    initConnections();
    readSettings();

    _ui->crossFilesystemsCheckBox->setChecked( crossFilesystems );
    _ui->pathComboBox->setFocus();

    QTimer::singleShot( 200, this, &OpenDirDialog::initialSelection );
}


OpenDirDialog::~OpenDirDialog()
{
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
                                 QDir::Drives      );

    _ui->dirTreeView->setModel( _filesystemModel );
    _ui->dirTreeView->hideColumn( 3 );  // Date Modified
    _ui->dirTreeView->hideColumn( 2 );  // Type
    _ui->dirTreeView->hideColumn( 1 );  // Size
//    _ui->dirTreeView->setHeaderHidden( true );
}


void OpenDirDialog::initConnections()
{
    const QItemSelectionModel * selectionModel = _ui->dirTreeView->selectionModel();

    connect( selectionModel,    &QItemSelectionModel::currentChanged,
             this,              &OpenDirDialog::treeSelection );

    connect( _ui->upButton,     &QPushButton::clicked,
             this,              &OpenDirDialog::goUp );

    connect( _validator,        &ExistingDirValidator::isOk,
             _okButton,         &QPushButton::setEnabled );

    connect( _validator,        &ExistingDirValidator::isOk,
             this,              &OpenDirDialog::pathEdited );

    connect( this,              &OpenDirDialog::accepted,
             this,              &OpenDirDialog::writeSettings );

    connect( _ui->pathSelector, &PathSelector::pathSelected,
             this,              &OpenDirDialog::setPathAndExpand );

    connect( _ui->pathSelector, &PathSelector::pathDoubleClicked,
             this,              &OpenDirDialog::setPathAndAccept );

}


void OpenDirDialog::initialSelection()
{
    const QString path = QDir::currentPath();

#if VERBOSE_SELECTION
    logDebug() << "Selecting " << path << Qt::endl;
#endif

    setPath( path );
}


QString OpenDirDialog::selectedPath() const
{
    return _ui->pathComboBox->currentText();
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

    populatePathComboBox( path );

    QLineEdit * lineEdit = _ui->pathComboBox->lineEdit();
    if ( lineEdit )
	lineEdit->setText( path );

//    _ui->pathSelector->selectParentMountPoint( path );
    const QModelIndex index = _filesystemModel->index( path );
    _ui->dirTreeView->setCurrentIndex( index );
    _ui->dirTreeView->scrollTo( index );

    _lastPath = path;
    _settingPath = false;
}


void OpenDirDialog::setPathAndExpand( const QString & path )
{
    setPath( path );

    SignalBlocker sigBlockerPathSelector( _ui->pathSelector );
    const QModelIndex index = _filesystemModel->index( path );
    _ui->dirTreeView->collapseAll();
    _ui->dirTreeView->setExpanded( index, true );
    _ui->dirTreeView->scrollTo( index, QAbstractItemView::PositionAtTop );
}


void OpenDirDialog::setPathAndAccept( const QString & path )
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

    const QString path = _ui->pathComboBox->currentText();
    if ( path != _lastPath )
    {
#if VERBOSE_SELECTION
        logDebug() << "New path:" << path << Qt::endl;
#endif
        setPath( path );
    }
}


void OpenDirDialog::treeSelection( const QModelIndex & newCurrentItem,
                                   const QModelIndex & )
{
    const QString path = _filesystemModel->filePath( newCurrentItem );

    if ( path != _lastPath )
    {
#if VERBOSE_SELECTION
        logDebug() << "Selecting " << path << Qt::endl;
#endif
        setPath( path );
    }
}


void OpenDirDialog::populatePathComboBox( const QString & fullPath )
{
    _ui->pathComboBox->clear();
    _ui->pathComboBox->addItem( "/" );

    QString path;

    const QStringList pathComponents = fullPath.split( "/", Qt::SkipEmptyParts );
    for ( const QString & component : pathComponents )
    {
        path += "/" + component;
        _ui->pathComboBox->addItem( path );
    }
}


void OpenDirDialog::goUp()
{
    QStringList pathComponents = selectedPath().split( "/", Qt::SkipEmptyParts );

    if ( !pathComponents.isEmpty() )
        pathComponents.removeLast();

    const QString path = "/" + pathComponents.join( "/" );
    if ( path != _lastPath )
    {
#if VERBOSE_SELECTION
        logDebug() << "Navigating up to " << path << Qt::endl;
#endif
        setPath( path );
    }
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
    Settings settings;
    // logDebug() << Qt::endl;

    // Do NOT write _crossFilesystems back to the settings here; this is done
    // from the config dialog. The value in this dialog is just temporary for
    // the current program run.

    writeWindowSettings( this, "OpenDirDialog" );

    settings.beginGroup( "OpenDirDialog" );
    settings.setValue( "MainSplitter", _ui->mainSplitter->saveState()  );
    settings.endGroup();
}


QString OpenDirDialog::askOpenDir( QWidget * parent, bool * crossFilesystems )
{
    static bool _crossFilesystems = *crossFilesystems;

    OpenDirDialog dialog( parent, _crossFilesystems );
    dialog.pathSelector()->addHomeDir();
    dialog.pathSelector()->addMountPoints( MountPoints::normalMountPoints() );
    //logDebug() << "Waiting for user selection" << Qt::endl;

    if ( dialog.exec() == QDialog::Rejected )
    {
        //logInfo() << "[Cancel]" << Qt::endl;
	return QString();
    }

    // Remember the flag just for the next time the dialog is opened
    _crossFilesystems = dialog.crossFilesystems();
    if ( crossFilesystems )
	*crossFilesystems = _crossFilesystems;

    const QString path = dialog.selectedPath();
    //logInfo() << "User selected path " << path << Qt::endl;

    return path;
}

