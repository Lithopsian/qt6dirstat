/*
 *   File name: ExistingDir.h
 *   Summary:	QDirStat widget support classes
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#include <QDir>
#include <QFileSystemModel>

#include "ExistingDir.h"
#include "Logger.h"
#include "Exception.h"


using namespace QDirStat;


QValidator::State ExistingDirValidator::validate( QString & input, int &) const
{
    const bool ok = !input.isEmpty() && QDir( input ).exists();

    // This is a complex way to do
    //    emit isOk( ok );
    // from this const method

    const_cast<ExistingDirValidator *>( this )->isOk( ok );

#if 0
    logDebug() << "Checking \"" << input << "\": "
               << ( ok ? "OK" : "no such directory" )
               << Qt::endl;
#endif

    return ok ? QValidator::Acceptable : QValidator::Intermediate;
}



ExistingDirCompleter::ExistingDirCompleter( QObject * parent ):
    QCompleter ( parent )
{
    QFileSystemModel * model = new QFileSystemModel( this );
    CHECK_NEW( model );

    model->setRootPath( "/" );
    model->setFilter( QDir::Dirs );
    model->setReadOnly( true );

    setModel( model );
}
