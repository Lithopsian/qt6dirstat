/*
 *   File name: QDirStatApp.cpp
 *   Summary:   QDirStat application class for key objects
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "QDirStatApp.h"
#include "DirTree.h"
#include "DirTreeModel.h"
#include "MainWindow.h"
#include "SelectionModel.h"
#include "Exception.h"


using namespace QDirStat;


QDirStatApp::QDirStatApp():
    _dirTreeModel { new DirTreeModel() },
    _selectionModel { new SelectionModel( _dirTreeModel ) }
{
    CHECK_NEW( _dirTreeModel );
    CHECK_NEW( _selectionModel );

    // logDebug() << "Creating app" << Qt::endl;

    if ( qApp->styleSheet().isEmpty() )
    {
        const QString cssFile = QString( "%1/%2/%2.css" )
                                .arg( QStandardPaths::writableLocation( QStandardPaths::ConfigLocation ) )
                                .arg( qApp->applicationName() );
        QFile file ( cssFile );
        if ( !file.open( QFile::ReadOnly | QFile::Text) )
            return;

        QTextStream in( &file );
        qApp->setStyleSheet( in.readAll() );
    }
}


QDirStatApp::~QDirStatApp()
{
    // logDebug() << "Destroying app" << Qt::endl;

    delete _selectionModel;
    delete _dirTreeModel;

    // logDebug() << "App destroyed." << Qt::endl;
}


QDirStatApp * QDirStatApp::instance()
{
    static QDirStatApp _instance;

    return &_instance;
}


DirTree * QDirStatApp::dirTree() const
{
    return _dirTreeModel ? _dirTreeModel->tree() : nullptr;
}


QWidget * QDirStatApp::findMainWindow() const
{
    QWidget * mainWin = nullptr;
    const QWidgetList toplevel = QApplication::topLevelWidgets();

    for ( QWidgetList::const_iterator it = toplevel.cbegin(); it != toplevel.cend() && !mainWin; ++it )
        mainWin = qobject_cast<MainWindow *>( *it );

    if ( !mainWin )
        logWarning() << "NULL mainWin for shared instance" << Qt::endl;

    return mainWin;
}


FileInfo * QDirStatApp::root() const
{
    return dirTree() ? dirTree()->firstToplevel() : nullptr;
}


FileInfo * QDirStatApp::currentItem() const
{
    return _selectionModel->currentItem();
}


FileInfo * QDirStatApp::selectedDirInfo() const
{
    const FileInfoSet selectedItems = _selectionModel->selectedItems();
    FileInfo * sel = selectedItems.first();

    return !sel || sel->isDirInfo() ? sel : ( FileInfo * )sel->parent();
}


FileInfo * QDirStatApp::selectedDirInfoOrRoot() const
{
    FileInfo * sel = selectedDirInfo();

    return sel ? sel : root();
}


void QDirStatApp::setWidgetFontSize( QWidget * widget )
{
    if ( _dirTreeModel->dirTreeItemSize() == DTIS_Medium )
    {
        QFont biggerFont = widget->font();
        biggerFont.setPointSizeF( biggerFont.pointSizeF() * 1.1 );
        widget->setFont( biggerFont );
	//setStyleSheet( QString( "QTreeView { font-size: %1pt; }" ).arg( pointSize ) );
    }
}
