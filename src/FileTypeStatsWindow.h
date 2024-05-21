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

#include <QDialog>
#include <QTreeWidgetItem>

#include "ui_file-type-stats-window.h"
#include "Subtree.h"
#include "Typedefs.h" // FileSize


namespace QDirStat
{
    class FileInfo;
    class FileTypeStats;
    class MimeCategory;
    class FileTypeItem;
    class SelectionModel;
    class SuffixFileTypeItem;

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
	FileTypeStatsWindow( QWidget        * parent,
			     SelectionModel * selectionModel );

	/**
	 * Destructor.
	 **/
	~FileTypeStatsWindow() override;

	/**
	 * Static method for using one shared instance of this class between
	 * multiple parts of the application. This will create a new instance
	 * if there is none yet (or anymore).
	 **/
	static FileTypeStatsWindow * sharedInstance( QWidget        * parent,
						     SelectionModel * selectionModel );


    public:

	/**
	 * Obtain the subtree from the last used URL.
	 **/
//        const Subtree & subtree() const { return _subtree; }

	/**
	 * Convenience function for creating, populating and showing the shared
	 * instance.
	 **/
	static void populateSharedInstance( QWidget        * mainWindow,
					    FileInfo       * subtree,
					    SelectionModel * selectionModel );


    protected slots:

	/**
	 * Automatically update with the current main window selection, if the
	 * checkbox is checked.
	 **/
	void syncedPopulate( FileInfo * );

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
	 * type or re-popuolate it if it is still open.
	 **/
	void sizeStatsForCurrentFileType();

	/**
	 * Enable or disable the actions depending on the current item.
	 **/
	void enableActions( const QTreeWidgetItem * currentItem );


    protected:

	/**
	 * Clear all data and widget contents.
	 **/
	void clear();

	/**
	 * One-time initialization of the widgets in this window.
	 **/
	void initWidgets();

	/**
	 * Populate the widgets for a subtree.
	 **/
	void populate( FileInfo * subtree );

	/**
	 * Create a tree item for a category and add it to the tree.
	 **/
	FileTypeItem * addCategoryItem( const QString & name,
                                        int             count,
                                        FileSize        sum,
                                        float           percent );

	/**
	 * Create a file type item for files matching a non-suffix rule of a
	 * category. This does not yet add it to the category parent item.
	 **/
//	SuffixFileTypeItem * addNonSuffixRuleItem( const MimeCategory * category,
//                                                   int                  count,
//                                                   FileSize             sum );

	/**
	 * Create a file type item. This does not yet add it to a category
	 * item.
	 *
	 * This is important for file type items below the "Other" category:
	 * Those are created and collected first, but only the top X of them
	 * are actually added to the other category, the others are deleted.
	 **/
	SuffixFileTypeItem * addSuffixFileTypeItem( const QString & suffix,
                                                    int             count,
                                                    FileSize        sum,
                                                    float           percent );

	/**
	 * Add the top X of 'otherItems' to 'otherCategory' and delete the
	 * rest.
	 **/
	void addTopXOtherItems( FileTypeItem          * otherCategoryItem,
                                QList<FileTypeItem *> & otherItems );

	/**
	 * Return the suffix of the currently selected file type or an empty
	 * string if no suffix is selected.
	 **/
	QString currentSuffix() const;

	/**
	 * Custom context menu signalled.
	 **/
	void contextMenu( const QPoint & pos );

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


    private:

	//
	// Data members
	//

	Ui::FileTypeStatsWindow * _ui;
	Subtree                   _subtree;

    };


    /**
     * Column numbers for the file type tree widget
     **/
    enum FileTypeColumns
    {
	FT_NameCol = 0,
	FT_CountCol,
	FT_TotalSizeCol,
	FT_PercentageCol,
	FT_ColumnCount
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
	 * tree at the appropriate place: Toplevel for categories, below a
	 * category for suffixes.
	 **/
	FileTypeItem( const QString & name,
		      int             count,
		      FileSize        totalSize,
		      float           percentage );

	//
	// Getters
	//

	const QString & name()       const { return _name; }
	int             count()      const { return _count; }
	FileSize        totalSize()  const { return _totalSize; }
	float           percentage() const { return _percentage; }

	/**
	 * Set the font to bold face for all columns.
	 **/
//	void setBold();

	/**
	 * Less-than operator for sorting.
	 **/
	 bool operator<(const QTreeWidgetItem & other) const override;


    private:

	QString  _name;
	int      _count;
	FileSize _totalSize;
	float    _percentage;
    };

#if 0
    /**
     * Specialized item class for MIME categories.
     *
     * So specialized that it doesn't do anything differently
     * to the base class.
     **/
    class CategoryFileTypeItem: public FileTypeItem
    {
    public:

	/**
	 * Constructor.
	 **/
	CategoryFileTypeItem( const QString & name,
			      int             count,
			      FileSize        totalSize,
			      float            percentage ):
	    FileTypeItem ( name,
			   count,
			   totalSize,
			   percentage )
	{}

    };
#endif

    /**
     * Specialized item class for suffix file types.
     **/
    class SuffixFileTypeItem: public FileTypeItem
    {
    public:

	/**
	 * Constructor.
	 **/
	SuffixFileTypeItem( const QString & suffix,
			    int	            count,
			    FileSize        totalSize,
			    float           percentage ):
	    FileTypeItem ( itemName( suffix ), count, totalSize, percentage ),
	    _suffix { suffix }
	{}

	/**
	 * Return this file type's suffix.
	 **/
	const QString & suffix() const { return _suffix; }


    protected:

	/**
	 * Returns a string to be used as the name for this item.
	 **/
	QString itemName( const QString & suffix );


    private:

	QString _suffix;
    };


    /**
     * Functor for std::sort to compare FileTypeItems by size descending.
     **/
    struct FileTypeItemCompare
    {
	bool operator() ( FileTypeItem * a, FileTypeItem * b )
	    { return a->totalSize() > b->totalSize(); }
    };

} // namespace QDirStat


#endif // FileTypeStatsWindow_h
