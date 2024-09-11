/*
 *   File name: BucketsTableModel.h
 *   Summary:   Data model for buckets table
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef BucketsTableModel_h
#define BucketsTableModel_h

#include <QAbstractTableModel>
#include <QHeaderView>
#include <QTableView>


namespace QDirStat
{
    class FileSizeStats;

    /**
     * Data model for the buckets table in the file size statistics window.
     * This displays the data of that window's histogram view in a table.
     **/
    class BucketsTableModel: public QAbstractTableModel
    {
        Q_OBJECT

	enum Column
	{
	    StartCol,
	    EndCol,
	    ValueCol,
	    ColCount
	};


    public:

	/**
	 * Constructor.
	 **/
	BucketsTableModel( QObject * parent ):
	    QAbstractTableModel{ parent }
	{}

	/**
	 * Provide a pointer to the statistics to be used by the model.
	 *
	 * Note that this call is not wrapped in begin/endResetModel().
	 * It must either be called within begin/endReset() or must be
	 * followed by calls to begin/endReset() before the event loop
	 * spins.
	 **/
	void setStats( const FileSizeStats * stats )
	    { _stats = stats; }

	/**
	 * Wrappers around the protected beginResetModel() and
	 * endResetModel() functions for when the buckets contents are
	 * being replaced.
	 **/
	void beginReset() { beginResetModel(); }
	void endReset() { endResetModel(); }


    protected:

	/**
	 * Return the number of rows (direct tree children) for 'parent'.
	 **/
	int rowCount( const QModelIndex & parent ) const override;

	/**
	 * Return the number of columns for 'parent'.
	 **/
	int columnCount( const QModelIndex & ) const override { return ColCount; }

	/**
	 * Return data to be displayed for the specified model index and role.
	 **/
	QVariant data( const QModelIndex & index, int role ) const override;

	/**
	 * Return header data for the specified section.
	 **/
	QVariant headerData( int section, Qt::Orientation orientation, int role ) const override;

	/**
	 * Return item flags for the specified model index. This specifies if
	 * the item can be selected, edited etc.
	 *
	 * The base implementation returns flags indicating that the item is
	 * enabled and selectable, which is acceptable here.
	 **/
//	Qt::ItemFlags flags( const QModelIndex & ) const override
//	    { return Qt::ItemIsEnabled; }


    private:

	const FileSizeStats * _stats{ nullptr };

    }; // class BucketsTableModel



    /**
     * Data model for the buckets table in the file size statistics window.
     * This displays the data of that window's histogram view in a table.
     **/
    class PercentileTableModel: public QAbstractTableModel
    {
	Q_OBJECT

    public:

	enum PercentileTableColumn
	{
	    ValueCol,
	    CountCol,
	    SumCol,
	    CumCountCol,
	    CumSumCol,
	};


	/**
	 * Constructor.
	 **/
	PercentileTableModel( QObject * parent ):
	    QAbstractTableModel{ parent }
	{}

	/**
	 * Provide a pointer to the statistics to be used by
	 * the model.
	 *
	 * Note that this is wrapped in begin/endResetModel() for
	 * safety, although it is normally followed immediately
	 * by a call to resetModel().
	 **/
	void setStats( const FileSizeStats * stats )
	    { _stats = stats; }

	/**
	 * Reset the model, possibly with new contents, possibly
	 * with a new filter setting.
	 **/
	void resetModel( bool filterRows )
	    { beginResetModel(); _filterRows = filterRows; endResetModel(); }

	/**
	 * Map a row number to a percentile index.  The mapping is
	 * 1:1 if the rows are not being filtered.
	 *
	 * filterStep() gives the step size for filtering, for example
	 * 5 shows every 5th percentile.
	 *
	 * filterMargin() is the number of extra percentiles to show
	 * at the start and end.  So a margin of 1 would show
	 * percentiles 1 and 99 in addition to every 5th percentile.
	 **/
	int mapRow( int row ) const;


    protected:

	/**
	 * Return the number of rows (direct tree children) for 'parent'.
	 **/
	int rowCount( const QModelIndex & parent ) const override;

	/**
	 * Return the number of columns for 'parent'.
	 **/
	int columnCount( const QModelIndex & ) const override { return CumSumCol + 1; }

	/**
	 * Return data to be displayed for the specified model index and role.
	 **/
	QVariant data( const QModelIndex & index, int role ) const override;

	/**
	 * Return header data for the specified section.  The header contents
	 * are painted in the paintSection() override, but the tooltips are
	 * defined here.
	 **/
	QVariant headerData( int, Qt::Orientation, int ) const override;

	/**
	 * Return whether row 'index' should be highlighted.  Every 10th
	 * row is highlighted if all rows are being shown.
	 **/
	bool highlightRow( const QModelIndex & index ) const
	    { return !_filterRows && mapRow( index.row() ) % 10 == 0; }

	/**
	 * Return the filtered step size.
	 *
	 * Note that the program will run with any value, but should
	 * normally be a value that divides 100 to an exact round
	 * number: for example, 5, 10, or 25.  Values above 50 will
	 * filter out all rows (except min and max) and leave only
	 * the "margins".
	 **/
	constexpr static unsigned short filterStep() { return 5; }

	/**
	 * Return the filtered margin size.  This is the number of
	 * percentiles to show at the start and end of the range in
	 * addition to the min, max, and filter steps.
	 *
	 * Important: if this value is larger than the maximum
	 * percentile (100) then the results are undefined; the
	 * program will most likely crash.
	 **/
	constexpr static unsigned short filterMargin() { return 1; }

	/**
	 * Return the filtered margin size, adjusted for percentiles
	 * that would have been included just because of the step size.
	 * The expression deliberately uses integer arithmetic so that
	 * the result is rounded down.
	 **/
	constexpr static unsigned short marginAdjustment()
	    { return filterMargin() / filterStep(); }
	constexpr static unsigned short adjustedMargin()
	    { return filterMargin() - marginAdjustment(); }


    private:

	const FileSizeStats * _stats{ nullptr };

	bool _filterRows{ true };

    }; // class PercentileTableModel




    class PercentileTableHeader: public QHeaderView
    {
        Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	PercentileTableHeader( Qt::Orientation orientation, QWidget * parent ):
	    QHeaderView{ orientation, parent }
	{
	    setSectionsClickable( true );
	}


    protected:

	/**
	 * Paint rich text into the header.
	 **/
	void paintSection( QPainter * painter, const QRect & rect, int logicalIndex ) const override;

	/**
	 * Return the size of the painted header for sizing the header and
	 * column width.
	 **/
	QSize sectionSizeFromContents( int logicalIndex ) const override;

	/**
	 * Return the text for a header section, based on the header
	 * orientation.  For the vertical orientation, the row number
	 * 'index' is mapped to a percentile index if the header
	 * section count is unfiltered (ie. 101).
	 **/
	QString sectionText( int logicalIndex ) const;

	/**
	 * Return the model for the parent table, cast to
	 * PercentileTableModel *.
	 **/
	const QTableView * table() const
	    { return qobject_cast<QTableView *>( parent() ); }
	const PercentileTableModel * tableModel() const
	    { return qobject_cast<const PercentileTableModel *>( table()->model() ); }

	/**
	 * Spacing around each section of the header text.
	 **/
	constexpr static unsigned short horizontalMargin() { return 8; }
	constexpr static unsigned short verticalMargin()   { return 4; }

    }; // class PercentileTableHeader

} // namespace QDirStat

#endif // BucketsTableModel_h