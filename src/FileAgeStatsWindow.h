/*
 *   File name: FileAgeStatsWindow.h
 *   Summary:   QDirStat "File Age Statistics" window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef FileAgeStatsWindow_h
#define FileAgeStatsWindow_h

#include <QDialog>
#include <QTreeWidgetItem>

#include "ui_file-age-stats-window.h"
#include "FileAgeStats.h"
#include "Subtree.h"


namespace QDirStat
{
    class FileAgeStats;
    class PercentBarDelegate;
    class SelectionModel;
    class YearListItem;

    /**
     * Modeless dialog to display file age statistics, i.e. statistics about
     * the years of last modification times of files in a subtree.
     **/
    class FileAgeStatsWindow: public QDialog
    {
	Q_OBJECT

	/**
	 * Constructor.  Private.  Access the window through the static
	 * populateSharedInstance().
	 *
	 * Note that this widget will destroy itself upon window close.
	 **/
	FileAgeStatsWindow( QWidget         * parent,
			    SelectionModel  * selectionModel );

	/**
	 * Destructor.
	 **/
	~FileAgeStatsWindow() override;

	/**
	 * Returns the shared instance pointer for this windw.  It is created if
	 * it doesn't already exist.
	 **/
	static FileAgeStatsWindow * sharedInstance( QWidget         * parent,
						    SelectionModel  * selectionModel );


    public:

	/**
	 * Convenience function for creating, populating and showing the shared
	 * instance.
	 **/
	static void populateSharedInstance( QWidget         * parent,
                                            FileInfo        * subtree,
					    SelectionModel  * selectionModel );


    signals:

	/**
	 * Emitted when the user clicks the "Locate" button (which is only
	 * enabled when there are 1..10000 files for that year).
	 *
	 * 'path' is also sent because otherwise the main window will use the
	 * tree's root if a file and not a directory is currently
	 * selected. This is a common case after the user clicked on a file
	 * result in the "locate" window.
	 **/
	void locateFilesFromYear( const QString & path, short year );

	/**
	 * Similar than 'locateFilesFromYear()', but with year and month (1-12).
	 **/
	void locateFilesFromMonth( const QString & path, short year, short month );


    protected slots:

	/**
	 * Automatically update with the current main window selection, if the
	 * checkbox is checked.
	 **/
	void syncedPopulate( FileInfo * subtree );

	/**
	 * Refresh (reload) all data.
	 **/
	void refresh();

	/**
	 * Read settings from the config file
	 **/
	void readSettings();

	/**
	 * Write settings to the config file
	 **/
	void writeSettings();

	/**
	 * Emit the locateFilesFromYear() signal for the currently selected
	 * item's year. Do nothing if nothing is selected.
	 **/
	void locateFiles();

	/**
	 * Enable or disable actions and buttons depending on the internal
	 * state, e.g. if any item is selected and the number of files for the
	 * selected year are in the specified range (1..10000).
	 **/
	void enableActions();


    protected:

	/**
	 * Populate the window.
	 **/
	void populate( FileInfo * subtree );

	/**
	 * Clear all data and widget contents.
	 **/
	void clear();

	/**
	 * One-time initialization of the widgets in this window.
	 **/
	void initWidgets();

	/**
	 * Create an item in the years tree / list widget for each year
	 **/
	void populateListWidget();

	/**
	 * Fill the gaps between years.
	 **/
	void fillGaps();

	/**
	 * Find the gaps between years.
	 **/
	YearsList findGaps() const;

	/**
	 * Return the currently selected item in the tree widget or 0
	 * if there is none or if it is the wrong type.
	 **/
	const YearListItem * selectedItem() const;

	/**
	 * Returns whether a locate should be performed for the current selection.
	 **/
	bool canLocate( const YearListItem * item ) const;

	/**
	 * Key press event for detecting evnter/return.
	 *
	 * Reimplemented from QWidget.
	 **/
	void keyPressEvent( QKeyEvent * event ) override;

	/**
	 * Resize event, reimplemented from QWidget.
	 *
	 * Elide the title to fit inside the current dialog width, so that
	 * they fill the available width but very long paths don't stretch
	 * the dialog.  A little extra room is left for the user to
	 * shrink the dialog, which would then force the label to be elided
	 * further.
	 **/
	void resizeEvent( QResizeEvent * event ) override;


	//
	// Data members
	//

	Ui::FileAgeStatsWindow * _ui;
	FileAgeStats           * _stats 			{ nullptr };
	PercentBarDelegate     * _filesPercentBarDelegate	{ nullptr };
	PercentBarDelegate     * _sizePercentBarDelegate	{ nullptr };
	Subtree                  _subtree;
	bool                     _startGapsWithCurrentYear	{ true };

    };	// class FileAgeStatsWindow


    /**
     * Column numbers for the years tree widget
     **/
    enum YearListColumns
    {
	YearListYearCol,
	YearListFilesCountCol,
	YearListFilesPercentBarCol,
	YearListFilesPercentCol,
	YearListSizeCol,
	YearListSizePercentBarCol,
	YearListSizePercentCol,
	YearListColumnCount
    };


    /**
     * Item class for the years list (which is really a tree widget),
     * representing one year (or month) with accumulated values.
     **/
    class YearListItem: public QTreeWidgetItem
    {
    public:

	/**
	 * Constructor.
	 **/
	YearListItem( const YearStats & yearStats );

	/**
	 * Return the statistics values.
	 **/
	const YearStats & stats() const { return _stats; }


    protected:

	/**
	 * Less-than operator for sorting.
	 *
	 * Reimplemented from QTreeWidgetItem.
	 **/
	bool operator<( const QTreeWidgetItem & other ) const override;

	void set( YearListColumns col, Qt::Alignment alignment, const QString & text );


    private:

	const YearStats & _stats;

    };	// class YearListItem

}	// namespace QDirStat


#endif // FileAgeStatsWindow_h
