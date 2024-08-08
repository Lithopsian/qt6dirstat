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
	colList << fromViewCol( col );

    return colList;
}


QLatin1String DataColumns::toString( DataColumn col )
{
    switch ( col )
    {
	case NameCol:             return "NameCol"_L1;
	case PercentBarCol:       return "PercentBarCol"_L1;
	case PercentNumCol:       return "PercentNumCol"_L1;
	case SizeCol:             return "SizeCol"_L1;
	case TotalItemsCol:       return "TotalItemsCol"_L1;
	case TotalFilesCol:       return "TotalFilesCol"_L1;
	case TotalSubDirsCol:     return "TotalSubDirsCol"_L1;
	case LatestMTimeCol:      return "LatestMTimeCol"_L1;
        case OldestFileMTimeCol:  return "OldestFileMTimeCol"_L1;
	case UserCol:             return "UserCol"_L1;
	case GroupCol:            return "GroupCol"_L1;
	case PermissionsCol:      return "PermissionsCol"_L1;
	case OctalPermissionsCol: return "OctalPermissionsCol"_L1;
	case UndefinedCol:        return "UndefinedCol"_L1;
	case ReadJobsCol:         return "ReadJobsCol"_L1;

	// Intentionally omitting 'default' so the compiler
	// can catch unhandled enum values
    }

    logError() << "Unknown DataColumn " << toViewCol( col ) << Qt::endl;
    return QLatin1String{};
}


DataColumn DataColumns::fromString( const QString & str )
{
    if ( str == "TotalSizeCol"_L1 ) // Backwards compatibility
        return SizeCol;

    for ( int col = firstCol(); col <= lastCol(); ++col )
    {
	if ( str == toString( fromViewCol( col ) ) )
	    return fromViewCol( col );
    }

    logError() << "Invalid DataColumn \"" << str << '"' << Qt::endl;
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
