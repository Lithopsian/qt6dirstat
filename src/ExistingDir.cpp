/*
 *   File name: ExistingDir.h
 *   Summary:   QDirStat widget support classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QDir>
#include <QFileSystemModel>

#include "ExistingDir.h"
#include "Logger.h"


using namespace QDirStat;


QValidator::State ExistingDirValidator::validate( QString & input, int & ) const
{
    const bool ok = !input.isEmpty() && QDir( input ).exists();

#if 0
    logDebug() << "Checking \"" << input << "\": "
               << ( ok ? "OK" : "no such directory" )
               << Qt::endl;
#endif

    emit const_cast<ExistingDirValidator *>( this )->isOk( ok );

    return ok ? QValidator::Acceptable : QValidator::Intermediate;
}



ExistingDirCompleter::ExistingDirCompleter( QObject * parent ):
    QCompleter { parent }
{
    QFileSystemModel * model = new QFileSystemModel( this );
    model->setRootPath( "/" );
    model->setFilter( QDir::Dirs );
    model->setReadOnly( true );

    setModel( model );
}
