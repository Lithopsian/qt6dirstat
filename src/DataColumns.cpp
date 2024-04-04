/*
 *   File name: DataColumns.cpp
 *   Summary:	Data column mapping
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
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


QString DataColumns::toString( DataColumn col )
{
    switch ( col )
    {
	case NameCol:			return "NameCol";
	case PercentBarCol:		return "PercentBarCol";
	case PercentNumCol:		return "PercentNumCol";
	case SizeCol:		        return "SizeCol";
	case TotalItemsCol:		return "TotalItemsCol";
	case TotalFilesCol:		return "TotalFilesCol";
	case TotalSubDirsCol:		return "TotalSubDirsCol";
	case LatestMTimeCol:		return "LatestMTimeCol";
        case OldestFileMTimeCol:        return "OldestFileMTimeCol";
	case UserCol:			return "UserCol";
	case GroupCol:			return "GroupCol";
	case PermissionsCol:		return "PermissionsCol";
	case OctalPermissionsCol:	return "OctalPermissionsCol";
	case UndefinedCol:		return "UndefinedCol";
	case ReadJobsCol:		return "ReadJobsCol";

	    // Intentionally omitting 'default' so the compiler
	    // can catch unhandled enum values
    }

    logError() << "Unknown DataColumn " << (int)col << Qt::endl;
    return QString();
}


DataColumn DataColumns::fromString( const QString & str )
{
    if ( str == "TotalSizeCol" ) // Backwards compatibility for settings
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
