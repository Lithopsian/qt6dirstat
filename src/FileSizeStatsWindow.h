/*
 *   File name: FileSizeStatsWindow.h
 *   Summary:   QDirStat file size statistics window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef FileSizeStatsWindow_h
#define FileSizeStatsWindow_h

#include <memory>

#include <QDialog>

#include "ui_file-size-stats-window.h"


class QTableWidget;


namespace QDirStat
{
    class BucketsTableModel;
    class FileInfo;
    class FileSizeStats;

    /**
     * Modeless dialog to display file size statistics:
     * median, min, max, quartiles; histogram; percentiles table.
     **/
    class FileSizeStatsWindow: public QDialog
    {
	Q_OBJECT

	/**
	 * Constructor.  Private, use the populateSharedInstance() function
	 * to access this window.
	 *
	 * Note that this widget will destroy itself upon window close.
	 **/
	FileSizeStatsWindow( QWidget  * parent );

	/**
	 * Destructor.
	 **/
	~FileSizeStatsWindow() override;

	/**
	 * Static method for using one shared instance of this class between
	 * multiple parts of the application. This will create a new instance
	 * if there is none yet (or any more).
	 **/
	static FileSizeStatsWindow * sharedInstance( QWidget * mainWindow );


    public:

	/**
	 * Convenience function for creating, populating and showing the shared
	 * instance.
	 *
	 * Any suffix should start with '.', but not '*.".
	 **/
	static void populateSharedInstance( QWidget       * mainWindow,
	                                    FileInfo      * fileInfo,
	                                    const QString & suffix = "" );


    protected slots:

	/**
	 * Fill the percentiles table depending on the content of the
	 * checkbox in the same tab.
	 **/
	void fillPercentileTable();

	/**
	 * Make the histogram options visible or invisible.
	 **/
	void openOptions();
	void closeOptions();

	/**
	 * Respond to changes in the slider values or markers combobox.
	 **/
	void startValueChanged( int newStart );
	void endValueChanged( int newEnd );
	void markersChanged( int markersIndex );

	/**
	 * Calculate automatic values for the start and end percentiles, apply
	 * them, and rebuild the histogram.
	 **/
//	void autoPercentiles();

	/**
	 * Show help for a topic determined by the sender of this signal.
	 **/
	void showHelp();


    protected:

	/**
	 * One-time initialization of the widgets in this window
	 **/
	void initWidgets();

	/**
	 * Return the abstract model (cast as BucketsTableModel) for
	 * the QTableView.
	 **/
	BucketsTableModel * bucketsTableModel() const;

	/**
	 * Populate with new content.
	 **/
	void populate( FileInfo * fileInfo, const QString & suffix );

	/**
	 * Initialise the histogram data.
	 **/
	void initHistogram();

	/**
	 * Automatically determine the best start and end percentiles.  This is
	 * done by identifying the percentiles corresponding to outliers at the
	 * top and bottom end of the distribution.
	 *
	 * Outliers are statistically defined as being more than three times the
	 * inter-quartile range (IQR) below the first quartile or above the third
	 * quartile.  File size distributions are heavily weighted to small files
	 * and there are effectively never outliers at the small end by this
	 * definition, so low size outliers are defined as being the IQR beyond
	 * the 1st quartile.
	 *
	 * The high end outlier calculation can theoretically overflow a (64 bit!)
	 * FileSize integer, so this is first calculated as a floating point value
	 * and only calculated as an integer if it is less than the maximum
	 * FileSize value in the statistics.
	 **/
	void autoStartEndPercentiles();

	/**
	 * Update the values for the option widgets from the current ones from
	 * the histogram
	 **/
//	void updateOptions();

	/**
	 * Fill the buckets and histogram, and build the tables.
	 **/
	void loadHistogram();

	/**
	 * Resize event, reimplemented from QWidget.
	 *
	 * Elide the title to fit inside the current dialog width, so that
	 * they fill the available width but very long paths don't stretch
	 * the dialog.  A little extra room is left for the user to
	 * shrink the dialog, which would then force the label to be elided
	 * further.
	 **/
	void resizeEvent( QResizeEvent * ) override;


    private:

	//
	// Data members
	//

	std::unique_ptr<Ui::FileSizeStatsWindow> _ui;
	std::unique_ptr<FileSizeStats>           _stats;

    }; // class FileSizeStatsWindow

} // namespace QDirStat

#endif // FileSizeStatsWindow_h
