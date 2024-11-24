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
#include <QHelpEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>

#include "OpenDirDialog.h"
#include "DirTreeModel.h"
#include "ExistingDirValidator.h"
#include "FormatUtil.h"
#include "Logger.h"
#include "QDirStatApp.h" // DirTreeModel
#include "Settings.h"
#include "SignalBlocker.h"


#define VERBOSE_SELECTION 0


using namespace QDirStat;


namespace
{
    /**
     * Read settings from the config file
     **/
    void readSettings( QDialog * dialog, QSplitter * mainSplitter )
    {
        Settings::readWindowSettings( dialog, "OpenDirDialog" );

        Settings settings;

        settings.beginGroup( "OpenDirDialog" );
        const QByteArray mainSplitterState = settings.value( "MainSplitter" , QByteArray() ).toByteArray();
        settings.endGroup();

        if ( !mainSplitterState.isNull() )
            mainSplitter->restoreState( mainSplitterState );
    }


    /**
     * Initialise the QFileSystemModel and DirTreeView.
     **/
    void initDirTree( QTreeView * dirTreeView, QFileSystemModel * filesystemModel )
    {
        app()->dirTreeModel()->setTreeIconSize( dirTreeView );

        const auto filter = QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Drives;
        filesystemModel->setFilter( filter );
        filesystemModel->setRootPath( "/" );

        dirTreeView->setModel( filesystemModel );
        dirTreeView->hideColumn( 3 );  // Date Modified
        dirTreeView->hideColumn( 2 );  // Type
        dirTreeView->hideColumn( 1 );  // Size

        dirTreeView->setItemDelegateForColumn( 0, new OpenDirDelegate{ dirTreeView } );
    }


    /**
     * Populate the path combo-box with a new path.  If the path
     * is already in the list, then set it as the current item,
     * otherwise build a new list with the path and all its
     * ancestors.
     **/
    void populatePathComboBox( QComboBox         * pathComboBox,
                               QFileSystemModel  * filesystemModel,
                               const QModelIndex & currentIndex )
    {
        // Keep the contents if the new path is already in the list
        for ( int i=0; i < pathComboBox->count(); ++i )
        {
            if ( pathComboBox->itemText( i ) == filesystemModel->filePath( currentIndex ) )
            {
                pathComboBox->setCurrentIndex( i );
                return;
            }
        }

        // Build a new list from the current path and all its ancestors
        pathComboBox->clear();
        for ( QModelIndex index = currentIndex; index.isValid(); index = filesystemModel->parent( index ) )
            pathComboBox->addItem( filesystemModel->filePath( index ) );
    }


    /**
     * Create and apply an ExistingDirValidator, enable the clear
     * button, and set the current combo-box text in the line edit.
     **/
    ExistingDirValidator * initPathComboBox( QComboBox * pathComboBox )
    {
        QLineEdit * lineEdit = pathComboBox->lineEdit();
        if ( lineEdit )
            lineEdit->setClearButtonEnabled( true );

        ExistingDirValidator * validator = new ExistingDirValidator{ pathComboBox };
        pathComboBox->setValidator( validator );
        return validator;
    }

}


OpenDirDialog::OpenDirDialog( QWidget * parent, bool crossFilesystems ):
    QDialog{ parent },
    _ui{ new Ui::OpenDirDialog },
    _filesystemModel{ new QFileSystemModel{ this } }
{
    _ui->setupUi( this );

    _ui->pathSelector->addHomeDir();
    _ui->pathSelector->addNormalMountPoints();
    _ui->crossFilesystemsCheckBox->setChecked( crossFilesystems );

    _validator = initPathComboBox( _ui->pathComboBox );
    initDirTree( _ui->dirTreeView, _filesystemModel );
    initConnections();
    readSettings( this, _ui->mainSplitter );

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
    Settings::writeWindowSettings( this, "OpenDirDialog" );

    Settings settings;

    settings.beginGroup( "OpenDirDialog" );
    settings.setValue( "MainSplitter", _ui->mainSplitter->saveState()  );
    settings.endGroup();

    // Do NOT write _crossFilesystems back to the settings here; this is done
    // from the config dialog. The value in this dialog is just temporary for
    // the current program run.
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
    const SignalBlocker sigBlockerValidator{ _validator };

    const QModelIndex currentIndex = _filesystemModel->index( path );
    populatePathComboBox( _ui->pathComboBox, _filesystemModel, currentIndex );
    _ui->dirTreeView->setCurrentIndex( currentIndex );

    _lastPath = path;
}


void OpenDirDialog::pathSelected( const QString & path )
{
#if VERBOSE_SELECTION
    logDebug() << "Selected " << path << Qt::endl;
#endif

    // Can block selection model here as we manually select and scroll in the tree
    const SignalBlocker sigBlockerSelection{ _ui->dirTreeView->selectionModel() };

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


void OpenDirDialog::goUp()
{
    // Just move one place in the combobox list
    const int nextIndex = _ui->pathComboBox->currentIndex() + 1;
    if ( nextIndex < _ui->pathComboBox->count() )
        _ui->pathComboBox->setCurrentIndex( nextIndex );
}


QString OpenDirDialog::askOpenDir( QWidget * parent, bool & crossFilesystems )
{
    OpenDirDialog dialog{ parent, crossFilesystems };
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




bool OpenDirDelegate::helpEvent( QHelpEvent                 * event,
                                 QAbstractItemView          * view,
                                 const QStyleOptionViewItem & option,
                                 const QModelIndex          & index )
{
    if ( event && event->type() == QEvent::ToolTip && view && index.isValid() )
    {
        tooltipForElided( option.rect, sizeHint( option, index ), view->model(), index, event->globalPos() );

        return true;
    }

    return QStyledItemDelegate::helpEvent( event, view, option, index );
}
