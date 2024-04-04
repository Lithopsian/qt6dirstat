/*
 *   File name: BucketsTableModel.h
 *   Summary:	Data model for buckets table
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#ifndef BucketsTableModel_h
#define BucketsTableModel_h

#include <QAbstractTableModel>


namespace QDirStat
{
    class HistogramView;

    /**
     * Data model for the buckets table in the file size statistics window.
     * This displays the data of that window's histogram view in a table.
     **/
    class BucketsTableModel: public QAbstractTableModel
    {
        Q_OBJECT

    public:
        enum Column
        {
            StartCol,
            EndCol,
            ValueCol,
            ColCount
        };

        /**
         * Constructor.
         **/
        BucketsTableModel( QObject * parent, const HistogramView * histogram ):
	    QAbstractTableModel ( parent ),
	    _histogram { histogram }
	{}

        /**
         * Destructor.
         **/
//        virtual ~BucketsTableModel() {}

        /**
         * Return the associated histogram view.
         **/
        const HistogramView * histogram() const { return _histogram; }

        /**
         * Notification that data in the histogram have been reset.
         **/
        void reset();


    protected:

        //
        // Overloaded model methods
        //

        /**
	 * Return the number of rows (direct tree children) for 'parent'.
	 **/
	int rowCount( const QModelIndex & parent ) const override;

	/**
	 * Return the number of columns for 'parent'.
	 **/
	int columnCount( const QModelIndex & ) const override
	    { return ColCount; }

	/**
	 * Return data to be displayed for the specified model index and role.
	 **/
	QVariant data( const QModelIndex & index, int role ) const override;

	/**
	 * Return header data for the specified section.
	 **/
	QVariant headerData( int	     section,
			     Qt::Orientation orientation,
			     int	     role ) const override;

	/**
	 * Return item flags for the specified model index. This specifies if
	 * the item can be selected, edited etc.
	 **/
	Qt::ItemFlags flags( const QModelIndex &index ) const override
	    { return QAbstractTableModel::flags( index ) | Qt::ItemIsSelectable; }


    private:

        const HistogramView * _histogram;
    };
}

#endif // BucketsTableModel_h
