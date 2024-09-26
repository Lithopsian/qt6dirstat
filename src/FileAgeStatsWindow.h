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

#include <memory>

#include <QDialog>
#include <QTreeWidgetItem>

#include "ui_file-age-stats-window.h"
#include "Subtree.h"
#include "Typedefs.h" // FileSize


namespace QDirStat
{
    class YearListItem;
    class YearStats;

    /**
     * Modeless dialog to display file age statistics, i.e. statistics about
     * the years of last modification times of files in a subtree.
     **/
    class FileAgeStatsWindow: public QDialog
    {
	Q_OBJECT

	/**
	 * Constructor.  Private; access the window through the static
	 * populateSharedInstance().
	 *
	 * Note that this widget will destroy itself upon window close.
	 **/
	FileAgeStatsWindow( QWidget * parent );

	/**
	 * Destructor.
	 **/
	~FileAgeStatsWindow() override;

	/**
	 * Returns the shared instance pointer for this window.  It is created if
	 * it doesn't already exist.
	 **/
	static FileAgeStatsWindow * sharedInstance( QWidget * parent );


    public:

	/**
	 * Convenience function for creating, populating and showing the shared
	 * instance.
	 **/
	static void populateSharedInstance( QWidget * parent, FileInfo * fileInfo )
	    { if ( fileInfo ) sharedInstance( parent )->populate( fileInfo ); }


    protected slots:

	/**
	 * Automatically update with the current main window selection, if the
	 * checkbox is checked.
	 **/
	void syncedPopulate();

	/**
	 * Automatically update with the current main window selection, if the
	 * checkbox is checked.  This dos not compare the current item to the
	 * current subtree since the subtree is likely to fall back to root
	 * after a refresh of the tree.
	 **/
	void syncedRefresh();

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
	void populate( FileInfo * fileInfo );

	/**
	 * Key press event for detecting enter/return.
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
	void resizeEvent( QResizeEvent * ) override;


    private:

	std::unique_ptr<Ui::FileAgeStatsWindow> _ui;

	Subtree _subtree;

    };	// class FileAgeStatsWindow


    /**
     * Column numbers for the years tree widget
     **/
    enum YearListColumns
    {
	YL_YearMonthCol,
	YL_FilesCountCol,
	YL_FilesPercentBarCol,
	YL_FilesPercentCol,
	YL_SizeCol,
	YL_SizePercentBarCol,
	YL_SizePercentCol,
	YL_ColumnCount,
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
	 * Return the year for this item.
	 **/
	short year() const { return _year; }

	/**
	 * Return the month for this item.
	 **/
	short month() const { return _month; }

	/**
	 * Return the files count for this item.
	 **/
	int filesCount() const { return _filesCount; }


    protected:

	/**
	 * Less-than operator for sorting.
	 *
	 * Reimplemented from QTreeWidgetItem.
	 **/
	bool operator<( const QTreeWidgetItem & other ) const override;


    private:

	short    _year;
	short    _month;
	int      _filesCount;
	FileSize _size;

    };	// class YearListItem

}	// namespace QDirStat

#endif	// FileAgeStatsWindow_h
