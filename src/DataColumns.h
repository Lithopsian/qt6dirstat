/*
 *   File name: DataColumns.h
 *   Summary:   Data column mapping
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef DataColumns_h
#define DataColumns_h

#include <QList>
#include <QStringList>
#include <QTextStream>


// For use in loops
#define DataColumnBegin NameCol
#define DataColumnEnd   UndefinedCol


namespace QDirStat
{
    /**
     * Data columns for data model, view, and sorting.  Most of the work
     * is now done in HeaderTweaker.  There are just a few convenience functions
     * and the enum.  The mappings are kept although they are now just casts
     * to the enum type.
     **/
    enum DataColumn
    {
	NameCol = 0,		// File / directory name
	PercentBarCol,		// Graphical percentage bar
	PercentNumCol,		// Numeric percentage Value
	SizeCol,		// size (subtree or own size for files)
	TotalItemsCol,		// Total number of items    in subtree
	TotalFilesCol,		// Total number of files    in subtree
	TotalSubDirsCol,	// Total number of subdirs  in subtree
	LatestMTimeCol,		// Latest modification time in subtree
	OldestFileMTimeCol,	// mtime of the oldest file in subtree
	UserCol,		// User (owner)
	GroupCol,		// Group
	PermissionsCol,		// Permissions (symbolic; -rwxrxxrwx)
	OctalPermissionsCol,	// Permissions (octal; 0644)
	UndefinedCol,
	ReadJobsCol,		// Dummy column only for sorting by pending read jobs
    };

    typedef QList<DataColumn> DataColumnList;

    namespace DataColumns
    {
	/**
	 * Map a view column to the corresponding model column
	 **/
	inline DataColumn fromViewCol( int viewCol ) { return static_cast<DataColumn>( viewCol ); }

	/**
	 * Map a model column to the corresponding view column
	 **/
	inline int toViewCol( DataColumn modelCol ) { return static_cast<int>( modelCol ); }

        /**
         * Return all model columns in default order
         **/
	DataColumnList allColumns();

	/**
	 * Return the default model columns
	 **/
	inline DataColumnList defaultColumns() { return allColumns(); }

	/**
	 * Return the number of (real, not read jobs) columns
	 **/
	inline int colCount() { return UndefinedCol; }

	/**
	 * Return the first column
	 **/
	inline int firstCol() { return 0; }

	/**
	 * Return the last column
	 **/
	inline int lastCol() { return colCount() - 1; }

	/**
	 * Return whether the given model column (int) represents a valid
	 * header column enum, nopt including ReadJobsCol and UndefinedCol
	 **/
	inline bool isValidCol( int modelCol ) { return modelCol >= firstCol() && modelCol <= lastCol(); }

	/**
	 * Convert a column to string
	 **/
	QLatin1String toString( DataColumn col );

	/**
	 * Convert string to column
	 **/
	DataColumn fromString( const QString & str );

	/**
	 * Convert a list of columns to a string list
	 **/
	QStringList toStringList( const DataColumnList & colList );

	/**
	 * convert a string list to a list of columns
	 **/
	DataColumnList fromStringList( const QStringList & strList );

	/**
	 * Ensure that NameCol at the first position of colList
	 **/
	void ensureNameColFirst( DataColumnList & colList );

    };	// DataColumns namespace


    /**
     * Print a DataColumn in text form to a debug stream
     **/
    inline QTextStream & operator<< ( QTextStream & stream, DataColumn col )
    {
	stream << DataColumns::toString( col );

	return stream;
    }


    /**
     * Print a DataColumnList in text form to a debug stream
     **/
    inline QTextStream & operator<< ( QTextStream &          stream,
                                      const DataColumnList & colList )
    {
	stream << "[ "
               << DataColumns::toStringList( colList ).join( QLatin1String( ", " ) )
               << " ]";

	return stream;
    }


}	// namespace QDirStat

#endif	// DataColumns_h
