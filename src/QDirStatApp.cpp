/*
 *   File name: QDirStatApp.cpp
 *   Summary:   QDirStat application class for key objects
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QStandardPaths>

#include "QDirStatApp.h"
#include "DirInfo.h"
#include "DirTree.h"
#include "DirTreeModel.h"
#include "Exception.h"
#include "SelectionModel.h"


using namespace QDirStat;


QDirStatApp::QDirStatApp( int &argc, char **argv ):
    QApplication ( argc, argv )
{
    // logDebug() << "Creating app" << Qt::endl;

    if ( styleSheet().isEmpty() )
    {
        const QString cssFile = QString( "%1/%2/%2.css" )
                                .arg( QStandardPaths::writableLocation( QStandardPaths::ConfigLocation ) )
                                .arg( applicationName() );
        QFile file ( cssFile );
        if ( !file.open( QFile::ReadOnly | QFile::Text ) )
            return;

        QTextStream in( &file );
        setStyleSheet( in.readAll() );
    }
}


void QDirStatApp::setModels( MainWindow     * mainWindow,
                             DirTreeModel   * dirTreeModel,
                             SelectionModel * selectionModel )
{
    CHECK_PTR( mainWindow );
    instance()->_mainWindow = mainWindow;

    CHECK_PTR( dirTreeModel );
    instance()->_dirTreeModel = dirTreeModel;

    CHECK_PTR( selectionModel );
    instance()->_selectionModel = selectionModel;
}


void QDirStatApp::resetModels()
{
    instance()->_mainWindow = nullptr;
    instance()->_dirTreeModel = nullptr;
    instance()->_selectionModel = nullptr;
}


DirTree * QDirStatApp::dirTree() const
{
    return _dirTreeModel ? _dirTreeModel->tree() : nullptr;
}


FileInfo * QDirStatApp::firstToplevel() const
{
    DirTree * tree = dirTree();
    return tree ? tree->firstToplevel() : nullptr;
}


FileInfo * QDirStatApp::currentDirInfo() const
{
    FileInfo * sel = _selectionModel->currentItem();
    return !sel || sel->isDirInfo() ? sel : sel->parent();
}
