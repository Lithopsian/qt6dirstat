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
#include "Subtree.h"
#include "Wildcard.h"


namespace QDirStat
{
    class FileInfo;
    class FileSizeStats;

    /**
     * Modeless dialog to display file size statistics:
     * median, min, max, quartiles; histogram; percentiles table.
     **/
    class FileSizeStatsWindow final : public QDialog
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
	 * Convenience functions for creating, populating and showing the
	 * shared instance.
	 **/
	static void populateSharedInstance( QWidget * mainWindow, FileInfo * fileInfo )
	    { if ( fileInfo ) sharedInstance( mainWindow )->populate( fileInfo, WildcardCategory{} ); }
	static void populateSharedInstance( QWidget                * mainWindow,
	                                    FileInfo               * fileInfo,
	                                    const WildcardCategory & wildcardCategory )
	    { if ( fileInfo ) sharedInstance( mainWindow )->populate( fileInfo, wildcardCategory ); }


    protected slots:

	/**
	 * Re-populate with the existing subtree, pattern, and dialog
	 * settings.  This is used when excludeSymlinks is changed.
	 * The statistics are reloaded, the buckets are re-filled,
	 * the models are all reset, and the histogram is rebuilt,
	 * but the percentile range is not changed.
	 **/
	void refresh();

	/**
	 * Load the nominal percentiles label and reset the model with the
	 * current filter setting.
	 **/
	void setPercentileTable();

	/**
	 * Respond to changes in the markers combobox or from the context menu.
	 **/
	void markersChanged();

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
	 *
	 * Noe that this function may trigger up to two histogram rebuilds, through
	 * the slider signals, as the start and end percentiles are changed.  Or it
	 * may do nothing if both values are unchanged.
	 **/
	void autoPercentileRange();

	/**
	 * Fill the buckets with the start and end percentiles currently
	 * set on the options widgets, then pass the values to the
	 * histogram.  HistogramView will automatically regenerate the
	 * histogram with the new values.
	 **/
	void setPercentileRange();

	/**
	 * Toggle the histogram y-axis scale between log and linear and disable
	 * the automatic selection of the log scale setting.
	 **/
	void logHeights();

	/**
	 * Set the histogram y-axis scale to log or linear depending on the
	 * bucket size distribution and continue to automatically set the
	 * scaling.
	 **/
	void autoLogHeights();

	/**
	 * Set the start percentile to the minimum (0), or set the end
	 * percentile to the maximum (100).
	 **/
	void setMinPercentile();
	void setMaxPercentile();

	/**
	 * Show help for a topic determined by the sender of this signal.
	 **/
	void showHelp() const;


    protected:

	/**
	 * Connect the widget actions.
	 **/
	void connectActions();

	/**
	 * Populate with new content.  The titles are initialised, the
	 * statistics are loaded, the buckets are filled, the models
	 * are all reset, the percentile range is set automatically,
	 * and the histogram is rebuilt.
	 **/
	void populate( FileInfo * fileInfo, const WildcardCategory & wildcardCategory );

	/**
	 * (Re-)load the statistics from disk, including calculating
	 * the percentiles.  Notify the models and reset the percentile
	 * table model.
	 *
	 * Note that the buckets are not filled, the buckets table
	 * model is not reset, and the histogram is not rebuilt.  These
	 * must all be done immediately after a call to loadStats().
	 **/
	void loadStats( FileInfo * fileInfo );

	/**
	 * Initialise the histogram data.
	 **/
	void initHistogram();

	/**
	 * Elide the title to fit inside the current dialog width, so that
	 * they fill the available width but very long paths don't stretch
	 * the dialog.  A little extra room is left for the user to
	 * shrink the dialog, which would then force the label to be elided
	 * further.
	 *
	 * Also dect palette changes which affect the table shading.
	 *
	 * Reimplemented from QDialog/QWidget.
	 **/
	bool event( QEvent * event ) override;

	/**
	 * Context menu event.  Build and show a context menu.
	 * There are currently only context menus for the histogram
	 * and buckets table tabs.
	 *
	 * Reimplemented from QDialog/QWidget.
	 **/
	void contextMenuEvent( QContextMenuEvent * event ) override;


    private:

	std::unique_ptr<Ui::FileSizeStatsWindow> _ui;
	std::unique_ptr<FileSizeStats>           _stats;

	Subtree          _subtree;
	WildcardCategory _wildcardCategory;

    };	// class FileSizeStatsWindow

}	// namespace QDirStat

#endif	// FileSizeStatsWindow_h
