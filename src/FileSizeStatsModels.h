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
#include <QPainter>
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

	friend class PercentileTableHeader;

	enum PercentileTableColumn
	{
	    ValueCol,
	    CountCol,
	    SumCol,
	    CumCountCol,
	    CumSumCol,
	};


    public:

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
	 * Note that this call is not wrapped in begin/endResetModel().
	 * It must either be called within begin/endReset() or must be
	 * followed by calls to begin/endReset() before the event loop
	 * spins.
	 **/
	void setStats( const FileSizeStats * stats )
	    { _stats = stats; }

	/**
	 * Reset the model, possibly with new contents, possibly
	 * with a new filter setting.
	 **/
	void resetModel( bool filterRows )
	    { beginResetModel(); _filterRows = filterRows; endResetModel(); }


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
	 * Map a row number to a percentile index.  The mapping is
	 * 1:1 if the rows are not being filtered.
	 *
	 * The mapping was previously much more complicated due to
	 * "margins", extra rows shown at the start and end of the table
	 * in addition to the filtered rows.
	 **/
	int mapRow( int row ) const
	    { return row * filterStep(); }

	/**
	 * Return whether row 'index' should be highlighted.  Every 10th
	 * row is highlighted if more than every 5th row is being shown.
	 **/
	bool highlightRow( const QModelIndex & index ) const
	    { return filterStep() < 5 && mapRow( index.row() ) % 10 == 0; }

	/**
	 * Return the filtered step size.  For example, 5 shows every
	 * 5th percentile.  Currently this is either 5 or 1, set by a
	 * simple checkbox in FileSizeStatsWindow.
	 *
	 * Note that the program will run with any value, but should
	 * normally be a value that divides 100 to an exact round
	 * number: for example, 5, 10, or 25.  Values above 50 will
	 * filter out all rows except min and max.
	 **/
	unsigned short filterStep() const { return _filterRows ? 5 : 1; }


    private:

	const FileSizeStats * _stats{ nullptr };

	bool _filterRows{ true };

    }; // class PercentileTableModel




    class PercentileTableHeader: public QHeaderView
    {
        Q_OBJECT

    public:

	/**
	 * Constructor.  'parent' must be the QTableView that
	 * contains the header; this is used to access the model.
	 **/
	PercentileTableHeader( Qt::Orientation orientation, QTableView * parent ):
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
