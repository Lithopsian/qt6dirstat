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
	BucketsTableModel( QObject * parent ):
	    QAbstractTableModel { parent }
	{}

	/**
	 * Provide a pointer to the statistics to be used by
	 * the model.
	 **/
	void setStats( const FileSizeStats * stats )
	    { _stats = stats; }

	/**
	 * Wrappers around beginResetModel() and endResetModel() when the
	 * buckets contents are being replaced.
	 **/
	void beginReset() { beginResetModel(); }
	void endReset() { endResetModel(); }


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
	 **/
	Qt::ItemFlags flags( const QModelIndex & index ) const override
	    { return QAbstractTableModel::flags( index ) | Qt::ItemIsSelectable; }


    private:

        const FileSizeStats * _stats { nullptr };

    }; // class BucketsTableModel

} // namespace QDirStat

#endif // BucketsTableModel_h
