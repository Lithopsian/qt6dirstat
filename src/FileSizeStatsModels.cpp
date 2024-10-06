/*
 *   File name: BucketsTableModel.h
 *   Summary:   Data model for buckets table
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QGuiApplication>
#include <QStaticText>

#include "FileSizeStatsModels.h"
#include "FileSizeStats.h"
#include "FormatUtil.h"


using namespace QDirStat;


namespace
{
    /**
     * Return text showing the exact size 'size' in bytes, formatted
     * according to the locale style.
     **/
    QString sizeTooltip( FileSize size )
    {
        if ( size < 1000 )
            return QString{};

        return formatByteSize( size );
    }


    QString formatWidthSize( PercentileBoundary size )
    {
        if ( size >= 2 )
            return formatSize( size );

        const int precision = size < 1 ? 2 : 1;
        return formatByteSize( size, precision );
    }

}


int BucketsTableModel::rowCount( const QModelIndex & parent ) const
{
    return parent.isValid() || !_stats ? 0 : _stats->bucketsCount();
}


QVariant BucketsTableModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    switch ( role )
    {
        case Qt::DisplayRole:
        {
            if ( orientation == Qt::Vertical )
                return QString::number( section + 1 );

            switch ( section )
            {
                case StartCol: return tr( "Start size" );
                case EndCol:   return tr( "End size"   );
                case WidthCol: return tr( "Width"      );
                case ValueCol: return tr( "Files"      );
                default:       return QVariant{}; // should never happen
            }
        }

        case Qt::TextAlignmentRole:
        {
            const auto alignment = orientation == Qt::Horizontal ? Qt::AlignHCenter : Qt::AlignRight;
            return QVariant{ Qt::AlignVCenter | alignment };
        }

        case Qt::ToolTipRole:
        {
            const QString tooltipText = [ section ]()
            {
                switch ( section )
                {
                    case StartCol:
                        return tr( "The size of the smallest file that would be counted in this bucket" );

                    case EndCol:
                        return tr( "The size of the largest file that would be counted in this bucket" );

                    case WidthCol:
                        return tr( "The size range between the smallest and largest files"
                                   "<br/>that would counted in this bucket" );

                    case ValueCol:
                        return tr( "The number of files between 'Start size' and 'End size' inclusive,"
                                   "<br/>represented in the histogram by one bar" );

                    default:
                        return QString{}; // should never happen
                }
            }();
            return whitespacePre( tooltipText );
        }

        default:
            return QVariant{};
    }
}


QVariant BucketsTableModel::data( const QModelIndex & index, int role ) const
{
    if ( !index.isValid() )
        return QVariant{};

    switch ( role )
    {
        case Qt::DisplayRole:
        {
            switch ( index.column() )
            {
                case StartCol:  return formatSize( _stats->bucketStart( index.row() ) );
                case EndCol:    return formatSize( _stats->bucketEnd( index.row() ) );
                case WidthCol:  return formatWidthSize( _stats->bucketWidth( index.row() ) );
                case ValueCol:  return formatCount( _stats->bucketCount( index.row() ) );
                default:        return QVariant{}; // should never happen
            }
        }

        case Qt::TextAlignmentRole:
            return QVariant{ Qt::AlignVCenter | Qt::AlignRight };

        case Qt::ToolTipRole:
        {
            const int row = index.row();

            // Show decimals on the start and end sizes for buckets covering less than 1 byte
            if ( _stats->bucketWidth( row ) <= 1 && _stats->bucketsCount() > 1 )
            {
                switch ( index.column() )
                {
                    case StartCol: return formatByteSize( _stats->rawBucketStart( row ), 2 );
                    case EndCol:   return formatByteSize( _stats->rawBucketEnd( row ), 2 );
                    default:       return QVariant{};
                }
            }

            // Show exact byte size for rounded (eg. 1.4kB) sizes
            switch ( index.column() )
            {
                case StartCol: return sizeTooltip( _stats->bucketStart( row ) );
                case EndCol:   return sizeTooltip( _stats->bucketEnd( row ) );
                case WidthCol: return sizeTooltip( std::round( _stats->bucketWidth( row ) ) );
                default:       return QVariant{};
            }
        }

        default:
            return QVariant{};
    }
}




int PercentileTableModel::rowCount( const QModelIndex & parent ) const
{
    if ( parent.isValid() || !_stats )
        return 0;

    return PercentileStats::maxPercentile() / filterStep() + 1;
}


QVariant PercentileTableModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( role != Qt::ToolTipRole || orientation == Qt::Vertical )
        return QVariant{};

    const QString tooltipText = [ section ]()
    {
        switch ( section )
        {
            case ValueCol:    return tr( "The file size at this percentile" );

            case CountCol:    return tr( "The number of files larger than the previous precentile"
                                         "<br/>shown and up to the size of this percentile" );

            case SumCol:      return tr( "The sum of the sizes of the files larger than the previous"
                                         "<br/>percentile shown and up to the size of this percentile" );

            case CumCountCol: return tr( "The total number of files up to the size of this percentile" );

            case CumSumCol:   return tr( "The sum of the sizes of all files"
                                         "<br/>up to the size of this percentile" );

            default:          return QString{}; // should never happen
        }
    }();
    return whitespacePre( tooltipText );
}


QVariant PercentileTableModel::data( const QModelIndex & index, int role ) const
{
    if ( ! index.isValid() || !_stats )
        return QVariant{};

    switch ( role )
    {
        case Qt::DisplayRole:
        {
            const int row = index.row();
            const int i   = mapRow( row );
            const int col = index.column();

            if ( i == 0 && col != ValueCol )
                return QVariant{}; // no counts or sums for P0

            switch ( col )
            {
                case ValueCol:    return formatSize( _stats->percentileValue( i ) );
                case CountCol:    return formatCount( _stats->percentileCount( mapRow( row - 1 ), i ) );
                case SumCol:      return formatSize( _stats->percentileSum( mapRow( row - 1 ), i ) );
                case CumCountCol: return formatCount( _stats->cumulativeCount( i ) );
                case CumSumCol:   return formatSize( _stats->cumulativeSum( i ) );
                default:          return QVariant{};
            }
        }

        case Qt::FontRole:
        {
            // Show every quartile, including min and max, in bold
            if ( mapRow( index.row() ) % PercentileStats::quartile1() == 0 )
            {
                QFont font;
                font.setWeight( QFont::Bold );
                return font;
            }
            return QVariant{};
        }

        case Qt::TextAlignmentRole:
            return QVariant{ Qt::AlignVCenter | Qt::AlignRight };

        case Qt::BackgroundRole:
        {
            // Shade the background of deciles when all percentiles are being shown
            if ( highlightRow( index ) )
                return QColor::fromHsl( 0, 0, QGuiApplication::palette().highlight().color().lightness() );

            return QVariant{};
        }

        case Qt::ForegroundRole:
        {
            // Highlight text color when the background is shaded
            if ( highlightRow( index ) )
                return QGuiApplication::palette().highlightedText();

            return QVariant{};
        }

        case Qt::ToolTipRole:
        {
            // Show the exact byte size of rounded sizes in the tooltip
            const int col = index.column();
            const int row = index.row();
            const int i   = mapRow( row );

            if ( i == 0 && col != ValueCol )
                return QVariant{};

            switch ( col )
            {
                case ValueCol:
                    return sizeTooltip( _stats->percentileValue( i ) );
                case SumCol:
                    return sizeTooltip( _stats->percentileSum( mapRow( row - 1 ), i ) );
                case CumSumCol:
                    return sizeTooltip( _stats->cumulativeSum( i ) );
                default:
                    return QVariant{};
            }
        }

        default:
            return QVariant{};
    }
}




void PercentileTableHeader::paintSection( QPainter * painter, const QRect & rect, int logicalIndex ) const
{
    // Paint the theme background so we can draw over it
    painter->save();
    QHeaderView::paintSection( painter, rect, logicalIndex );
    painter->restore();

    // Align rich text, which requires setting a width for it to be aligned within
    QStaticText text{ sectionText( logicalIndex ) };
    const QTextOption option{ orientation() == Qt::Horizontal ? Qt::AlignHCenter : Qt::AlignRight };
    text.setTextOption( option );
    const qreal rectWidth = rect.width() - 2 * horizontalMargin();
    text.setTextWidth( qMax( rectWidth, text.size().width() ) );

    // Explicitly place the text to be vertically centred
    const int yCenter = qRound( ( rect.height() - text.size().height() ) / 2 );
    painter->drawStaticText( rect.left() + horizontalMargin(), rect.top() + yCenter, text );
}


QSize PercentileTableHeader::sectionSizeFromContents( int logicalIndex ) const
{
    QStaticText text{ sectionText( logicalIndex ) };
    return text.size().toSize() + 2 * QSize{ horizontalMargin(), verticalMargin() };
}


QString PercentileTableHeader::sectionText( int logicalIndex ) const
{
    const PercentileTableModel * model = tableModel();
    if ( !model )
        return QString{};

    if ( orientation() == Qt::Vertical )
    {
        const int percentile = model->mapRow( logicalIndex );

        switch ( percentile )
        {
            case PercentileStats::minPercentile(): return tr( "<b>Min</b>" );
            case PercentileStats::quartile1():     return tr( "<b>Quartile 1</b>" );
            case PercentileStats::median():        return tr( "<b>Median</b>" );
            case PercentileStats::quartile3():     return tr( "<b>Quartile 3</b>" );
            case PercentileStats::maxPercentile(): return tr( "<b>Max</b>" );
        }

        return tr( "<span style='font-size: large;'>P<sub>%1</sub></span>" ).arg( percentile );
    }

    switch ( logicalIndex )
    {
        case PercentileTableModel::ValueCol:
            return tr( "Value" );
        case PercentileTableModel::CountCol:
            return tr( "Files<sub> P(n-%1)...P(n)</sub>" ).arg( model->filterStep() );
        case PercentileTableModel::SumCol:
            return tr( "Sum<sub> P(n-%1)...P(n)</sub>" ).arg( model->filterStep() );
        case PercentileTableModel::CumCountCol:
            return tr( "Files<sub> P(%1)...P(n)</sub>" ).arg( PercentileStats::minPercentile() );
        case PercentileTableModel::CumSumCol:
            return tr( "Sum<sub> P(%1)...P(n)</sub>" ).arg( PercentileStats::minPercentile() );
    }

    return QString{};
}
