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
#include "Subtree.h"
#include "Typedefs.h" // FileCount, FileSize


namespace QDirStat
{
    struct CountSize;
    class FileInfo;

    /**
     * Modeless dialog to display file type statistics, such as how much disk
     * space is used for each kind of filename extension (*.jpg, *.mp4 etc.).
     **/
    class FileTypeStatsWindow: public QDialog
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
	static void populateSharedInstance( QWidget  * mainWindow, FileInfo * subtree )
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
	inline void enableActions( bool enable );

	/**
	 * Return the suffix of the currently selected file type or an empty
	 * string if no suffix is selected.
	 **/
	QString currentItemSuffix() const;

	/**
	 * Key press event for detecting enter/return.
	 *
	 * Reimplemented from QWidget.
	 **/
	void keyPressEvent( QKeyEvent * event ) override;

	/**
	 * Resize event: elide the title to fit inside the current dialog width,
	 * so that they fill the available width but very long paths don't
	 * stretch the dialog.  A little extra room is left for the user to
	 * shrink the dialog, which would then force the label to be elided
	 * further.
	 *
	 * Reimplemented from QWidget.
	 **/
	void resizeEvent( QResizeEvent * ) override;


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
     * Item class for the file type tree widget, representing either a MIME
     * category or a suffix.
     **/
    class FileTypeItem: public QTreeWidgetItem
    {
    public:

	/**
	 * Constructor. After creating, this item has to be inserted into the
	 * tree at the appropriate place: toplevel for categories, below a
	 * category for suffixes.
	 **/
	FileTypeItem( const QString   & name,
	              const CountSize & countSize,
	              FileCount         parentCount,
	              FileSize          parentSize,
	              int               treeLevel = 0 );

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
     * Specialized item class for suffix file types.
     **/
    class SuffixFileTypeItem: public FileTypeItem
    {
    public:

	/**
	 * Constructor.
	 **/
	SuffixFileTypeItem( bool              otherCategory,
	                    const QString   & suffix,
	                    const CountSize & countSize,
	                    FileCount         parentCount,
	                    FileSize          parentSize ):
	    FileTypeItem{ itemName( otherCategory, suffix ), countSize, parentCount, parentSize, 1 },
	    _suffix{ suffix }
	{}

	/**
	 * Return this file type's suffix.
	 **/
	const QString & suffix() const { return _suffix; }


    protected:

	/**
	 * Returns the name to display for this tree item.  This
	 * is "*." plus the suffix.  If there is no suffix, two
	 * special names are used for the "Other" category and for
	 * all other categories.
	 **/
	static QString itemName( bool otherCategory, const QString & suffix )
	    { return suffix.isEmpty() ? otherCategory ? noExtension() : nonSuffix() : "*." + suffix; }

	/**
	 * Returns the name to be used for items with no suffix in
	 * the "Other" category.
	 **/
	 static QString noExtension()
	    { return QObject::tr( "<no extension>" ); }

	/**
	 * Returns the name to be used for items with no suffix in
	 * categories except "Other".
	 **/
	 static QString nonSuffix()
	    { return QObject::tr( "<non-suffix rule>" ); }


    private:

	QString _suffix;

    };	// class SuffixFileTypeItem

}	// namespace QDirStat

#endif	// FileTypeStatsWindow_h
