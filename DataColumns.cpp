/*
 *   File name: DataColumns.cpp
 *   Summary:   Data column mapping
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "DataColumns.h"
#include "Logger.h"


using namespace QDirStat;


DataColumnList DataColumns::allColumns()
{
    DataColumnList colList;

    for ( int col = firstCol(); col <= lastCol(); ++col  )
	colList << static_cast<DataColumn>( col );

    return colList;
}


QLatin1String DataColumns::toString( DataColumn col )
{
    switch ( col )
    {
	case NameCol:             return QLatin1String( "NameCol" );
	case PercentBarCol:       return QLatin1String( "PercentBarCol" );
	case PercentNumCol:       return QLatin1String( "PercentNumCol" );
	case SizeCol:             return QLatin1String( "SizeCol" );
	case TotalItemsCol:       return QLatin1String( "TotalItemsCol" );
	case TotalFilesCol:       return QLatin1String( "TotalFilesCol" );
	case TotalSubDirsCol:     return QLatin1String( "TotalSubDirsCol" );
	case LatestMTimeCol:      return QLatin1String( "LatestMTimeCol" );
        case OldestFileMTimeCol:  return QLatin1String( "OldestFileMTimeCol" );
	case UserCol:             return QLatin1String( "UserCol" );
	case GroupCol:            return QLatin1String( "GroupCol" );
	case PermissionsCol:      return QLatin1String( "PermissionsCol" );
	case OctalPermissionsCol: return QLatin1String( "OctalPermissionsCol" );
	case UndefinedCol:        return QLatin1String( "UndefinedCol" );
	case ReadJobsCol:         return QLatin1String( "ReadJobsCol" );

	// Intentionally omitting 'default' so the compiler
	// can catch unhandled enum values
    }

    logError() << "Unknown DataColumn " << (int)col << Qt::endl;
    return QLatin1String();
}


DataColumn DataColumns::fromString( const QString & str )
{
    if ( str == QLatin1String( "TotalSizeCol" ) ) // Backwards compatibility for settings
        return SizeCol;

    for ( int col = firstCol(); col <= lastCol(); ++col )
    {
	if ( str == toString( static_cast<DataColumn>( col ) ) )
	    return static_cast<DataColumn>( col );
    }

    logError() << "Invalid DataColumn \"" << str << "\"" << Qt::endl;
    return UndefinedCol;
}


QStringList DataColumns::toStringList( const DataColumnList & colList )
{
    QStringList strList;

    for ( const DataColumn col : colList )
    {
	const QString str = toString( col );
	if ( !str.isEmpty() )
	    strList << str;
    }

    return strList;
}


DataColumnList DataColumns::fromStringList( const QStringList & strList )
{
    DataColumnList colList;

    for ( const QString & str : strList )
    {
	DataColumn col = fromString( str );
	if ( col != UndefinedCol )
	    colList << col;
    }

    return colList;
}


void DataColumns::ensureNameColFirst( DataColumnList & colList )
{
    if ( colList.first() != NameCol )
    {
	// logError() << "NameCol is required to be first!" << Qt::endl;
	colList.removeAll( NameCol );
	colList.prepend( NameCol );
	logError() << "Fixed column list: " << toStringList( colList ) << Qt::endl;
    }
}
