/*
 *   File name: BucketsTableModel.h
 *   Summary:   Data model for buckets table
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include "BucketsTableModel.h"
#include "HistogramView.h"
#include "FormatUtil.h"
#include "Logger.h"
#include "Exception.h"


using namespace QDirStat;


int BucketsTableModel::rowCount( const QModelIndex & parent ) const
{
    return parent.isValid() ? 0 : _histogram->bucketCount();
}


QVariant BucketsTableModel::data( const QModelIndex & index, int role ) const
{
    if ( ! index.isValid() )
        return QVariant();

    switch ( role )
    {
        case Qt::DisplayRole:
            {
                const int row = index.row();
                if ( row < 0 || row >= _histogram->bucketCount() )
                    return QVariant();

                switch ( index.column() )
                {
                    case StartCol:  return formatSize( _histogram->bucketStart( row ) );
                    case EndCol:    return formatSize( _histogram->bucketEnd  ( row ) );
                    case ValueCol:  return QString::number( _histogram->bucket( row ) );
                    default:        return QVariant();
                }
            }

        case Qt::TextAlignmentRole:
            return QVariant( Qt::AlignVCenter | Qt::AlignRight );

        default:
            return QVariant();
    }
}


QVariant BucketsTableModel::headerData( int             section,
                                        Qt::Orientation orientation,
                                        int             role ) const
{
    switch ( role )
    {
        case Qt::DisplayRole:
            if ( orientation == Qt::Horizontal )
            {
                switch ( section )
                {
                    case StartCol: return tr( "Start size" );
                    case EndCol:   return tr( "End size"   );
                    case ValueCol: return tr( "Files"      );
                    default:       return QVariant();
                }
            }

            if ( section < _histogram->bucketCount() )
                return QString::number( section + 1 );

            return QVariant();

        case Qt::TextAlignmentRole:
            if ( orientation == Qt::Horizontal )
                return (QVariant)( Qt::AlignVCenter | Qt::AlignHCenter );
            else
                return (QVariant)( Qt::AlignVCenter | Qt::AlignRight );

        default:
            return QVariant();
    }
}
