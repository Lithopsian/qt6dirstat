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
	 * Fill the percentiles table depending on the content of the filter
	 * combo box in the same tab.
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
	 * them and rebuild the histogram.
	 **/
	void autoPercentiles();

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
	 * Update the values for the option widgets from the current ones from
	 * the histogram
	 **/
	void updateOptions();

	/**
	 * Fill the buckets and histogram, and build the tables.
	 **/
	void loadHistogram();

	/**
	 * Fill the histogram with content
	 **/
	void fillHistogram();

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
