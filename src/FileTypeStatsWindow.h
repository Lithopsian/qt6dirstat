/*
 *   File name: FileTypeStatsWindow.h
 *   Summary:   QDirStat file type statistics window
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef FileTypeStatsWindow_h
#define FileTypeStatsWindow_h

#include <memory>

#include <QDialog>
#include <QTreeWidgetItem>

#include "ui_file-type-stats-window.h"
#include "FileTypeStats.h" // CountSize, PatternCategory
#include "Subtree.h"
#include "Typedefs.h" // FileCount, FileSize


namespace QDirStat
{
    class FileInfo;

    /**
     * Modeless dialog to display file type statistics, such as how much disk
     * space is used for each kind of filename extension (*.jpg, *.mp4 etc.).
     **/
    class FileTypeStatsWindow final : public QDialog
    {
	Q_OBJECT

	/**
	 * Constructor.  Private, use the populateSharedInstance() function
	 * for access to this window.
	 *
	 * This creates a file type statistics window, but it does not populate
	 * it with content yet.
	 *
	 * Note that this widget will destroy itself upon window close.
	 **/
	FileTypeStatsWindow( QWidget * parent );

	/**
	 * Destructor.
	 **/
	~FileTypeStatsWindow() override;

	/**
	 * Static method for using one shared instance of this class between
	 * multiple parts of the application. This will create a new instance
	 * if there is none yet (or any more).
	 **/
	static FileTypeStatsWindow * sharedInstance( QWidget * parent );


    public:

	/**
	 * Populate the shared singleton instance of this window for 'subtree'.
	 * Create a new instance if there isn't one currently.
	 **/
	static void populateSharedInstance( QWidget * mainWindow, FileInfo * subtree )
	    { if ( subtree ) sharedInstance( mainWindow )->populate( subtree ); }


    protected slots:

	/**
	 * Automatically update with the current main window selection, if the
	 * sync checkbox is checked and if the current item has changed.
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
	 * Open a "Locate File Type" window for the currently selected file
	 * type or re-populate it if it is still open.
	 **/
	void locateCurrentFileType();

	/**
	 * Open a "File Size Statistics" window for the currently selected file
	 * type and subtree, or re-populate it if it is still open.
	 **/
	void sizeStatsForCurrentFileType();

	/**
	 * An item in the tree was activated by mouse or keyboard.
	 **/
	void itemActivated();

	/**
	 * Enable or disable the actions depending on the current item.
	 **/
	void enableActions();

	/**
	 * Custom context menu signalled for the tree.
	 **/
	void contextMenu( const QPoint & pos );


    protected:

	/**
	 * Populate the widgets for a subtree.
	 **/
	void populate( FileInfo * subtree );

	/**
	 * Enable or disable the actions.
	 **/
	void enableActions( bool enable )
	    { _ui->locateButton->setEnabled( enable ); _ui->sizeStatsButton->setEnabled( enable ); }

	/**
	 * Return a PatternCategory struct for the currently selected
	 * file type or 0 if no PatternFileTypeItem is selected.
	 **/
	const PatternCategory * currentItemPattern() const;

	/**
	 * Key press event for detecting enter/return.
	 *
	 * Reimplemented from QWidget.
	 **/
	void keyPressEvent( QKeyEvent * event ) override;

	/**
	 * Event handler: elide the title to fit inside the current dialog
	 * width, so that they fill the available width but very long paths
	 * don't stretch the dialog.  A little extra room is left for the
	 * user to shrink the dialog, which would then force the label to
	 * be elided further.
	 *
	 * Reimplemented from QDialog/QWidget.
	 **/
	bool event( QEvent * event ) override;


    private:

	std::unique_ptr<Ui::FileTypeStatsWindow> _ui;
	Subtree _subtree;

    };	// class FileTypeStatsWindow


    /**
     * Column numbers for the file type tree widget
     **/
    enum FileTypeColumns
    {
	FT_NameCol = 0,
	FT_CountCol,
	FT_CountPercentBarCol,
	FT_CountPercentCol,
	FT_TotalSizeCol,
	FT_SizePercentBarCol,
	FT_SizePercentCol,
	FT_ColumnCount,
    };


    /**
     * Item class for the file type tree widget, representing either a
     * MIME category or a pattern.
     **/
    class FileTypeItem : public QTreeWidgetItem
    {
    public:

	/**
	 * Constructor with explicit parent count and size. After
	 * creating, this item has to be inserted into the tree at the
	 * appropriate place: toplevel for categories, below a category
	 * for patterns.
	 **/
	FileTypeItem( const QString   & name,
	              const CountSize & countSize,
	              FileCount         parentCount,
	              FileSize          parentSize,
	              int               treeLevel = 0 );

	/**
	 * Constructor with a parent FileTypeItem and tooltip.  Used for
	 * non-pattern child items: <no extension> and <other>.
	 **/
	FileTypeItem( FileTypeItem    * parentItem,
	              const QString   & name,
	              const QString   & tooltip,
	              const CountSize & countSize ):
	    FileTypeItem{ name, countSize, parentItem->count(), parentItem->totalSize(), 1 }
	{
	    setToolTip( FT_NameCol, tooltip );
	}

	/**
	 * Return the files count for this category item.
	 **/
	FileCount count() const { return _count; }

	/**
	 * Return the total files size for this category item.
	 **/
	FileSize totalSize() const { return _totalSize; }


    protected:

	/**
	 * Less-than operator for sorting.
	 *
	 * Reimplemented from QTreeWidgetItem.
	 **/
	 bool operator<(const QTreeWidgetItem & other) const override;


    private:

	FileCount _count;
	FileSize  _totalSize;

    };	// class FileTypeItem



    /**
     * Specialized item class for children of a top-level category
     * item.  These can be used as the basis for searches such as
     * by LocateFileTypeWindow and FileSizeStatsWindow.
     **/
    class PatternFileTypeItem final : public FileTypeItem
    {
    public:

	/**
	 * Constructor with a PatternCategory, a (possibly
	 * case-insensitive) pattern and a category.
	 **/
	PatternFileTypeItem( FileTypeItem          * parentItem,
	                     const QString         & name,
	                     const PatternCategory & patternCategory,
	                     const CountSize       & countSize ):
	    FileTypeItem{ name, countSize, parentItem->count(), parentItem->totalSize(), 1 },
	    _patternCategory{ patternCategory }
	{}

	/**
	 * Constructor with no pattern.
	 **/
	PatternFileTypeItem( FileTypeItem       * parentItem,
	                     const QString      & name,
	                     const QString      & tooltip,
	                     const MimeCategory * category,
	                     const CountSize    & countSize ):
	    FileTypeItem{ parentItem, name, tooltip, countSize },
	    _patternCategory{ QString{}, false, category }
	{}

	/**
	 * Return this file type item's PatternCategory struct.
	 **/
	const PatternCategory & patternCategory() const { return _patternCategory; }


    private:

	const PatternCategory _patternCategory;

    };	// class PatternFileTypeItem

}	// namespace QDirStat

#endif	// FileTypeStatsWindow_h
