/*
 *   File name: DirTreeModel.h
 *   Summary:	Qt data model for directory tree
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */

#ifndef DirTreeModel_h
#define DirTreeModel_h


#include <QAbstractItemModel>
#include <QColor>
#include <QFont>
#include <QGuiApplication>
#include <QIcon>
#include <QPalette>
#include <QSet>
#include <QTextStream>
#include <QTimer>

#include "DataColumns.h"


namespace QDirStat
{
    class DirInfo;
    class DirTree;
    class FileInfo;
    class PkgFilter;

    enum DirTreeItemSize
    {
	DTIS_Medium,
	DTIS_Small,
    };

    enum CustomRoles
    {
	RawDataRole = Qt::UserRole
    };

    class DirTreeModel: public QAbstractItemModel
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 *
	 * 'parent' is just the parent in the QObject / QWidget hierarchy that
	 * will take of deleting this model upon shutdown. It is typically NOT
	 * the corresponding view. As a matter of fact, there might be any
	 * number of views connected.
	 **/
	DirTreeModel( QObject * parent = nullptr );

	/**
	 * Destructor.
	 **/
	~DirTreeModel() override;

	/**
	 * Returns the internal DirTree this view works on.
	 *
	 * Handle with caution: This might be short-lived information.	The
	 * model might choose to create a new tree shortly after returning
	 * this, so don't store this pointer internally.
	 **/
	DirTree *tree()	{ return _tree; }

	/**
	 * Set the column order and what columns to display.
	 *
	 * Example:
	 *
	 *   DataColumnList col;
	 *   col << QDirStat::NameCol,
	 *	 << QDirStat::PercentBarCol,
	 *	 << QDirStat::PercentNumCol,
	 *	 << QDirStat::SizeCol;
	 *   dirTreeModel->setColumns( col );
	 */
//	void setColumns( const DataColumnList & columns );

	/**
	 * This is protected in the base class, but it's the only reasonable
	 * way for the view to figure out what items are expanded: The
	 * QTreeViewPrivate knows, but IT DOESN'T TELL that secret.
	 *
	 * Trolltech, you fucked up big time here. Seriously, that QTreeView is
	 * the most limiting tree widget you ever made. It's a far cry of the
	 * Qt3 QListView: It can't do shit. No wonder all apps using it suffer
	 * from poor usability! No way to set a separate icon for expanded
	 * vs. collapsed items, no way to close all branches except the current
	 * one.
	 *
	 * I am not going to reimplement all that stuff the private class
	 * already does. Seriously, life is too short for that kind of
	 * bullshit. I am not going to do my own bookkeeping based on
	 * expanded() and collapsed() signals and always suffer from not
	 * receiving the odd one and being out of sync with lists that are kept
	 * in the private class. No fucking way.
	 *
	 * Trolls, if you ever actually used your own stuff like you did in the
	 * old days before you abandoned the X11 widgets and spent all your
	 * time and energy on that QML crap, you'd know that this QTreeView is
	 * a sorry excuse for the old QListView.
	 *
	 * Didn't it ever occur to you that if you are constantly using
	 * d->expandedIndexes in your QTreeView.cpp, derived classes might have
	 * the same need to know about that? Why hide it in the private class
	 * if it's that obvious that this is frequently needed information?
	 *
	 * It doesn't happen often in Qt - in all other aspects, it's a great,
	 * well thought-out and very practical oriented framework. But
	 * QTreeView is poorly designed in many aspects; hiding this
	 * d->expandedIndexes is just one example.
	 *
	 * OK, I'm fed up with working around this poor design. Let's make our
	 * own kludge to find out what items are expanded: Check the persistent
	 * model indexes: The private class creates a persistent model index
	 * for each item that is expanded.
	 **/
	QModelIndexList persistentIndexList() const
	    { return QAbstractItemModel::persistentIndexList(); }

	/**
	 * Return 'true' if the application uses a dark widget theme.
	 **/
	static bool usingDarkTheme()
	    { return QGuiApplication::palette().color( QPalette::Active, QPalette::Base ).lightness() < 160; }

	/**
	 * Return the color configured for directories with a read error.
	 **/
	const QColor & dirReadErrColor() const
	    { return usingDarkTheme() ? _dirReadErrDarkTheme : _dirReadErrLightTheme; }

	/**
	 * Return the color configured for directories with a read error in a sub-directory.
	 **/
	const QColor & subtreeReadErrColor() const
	    { return usingDarkTheme() ? _subtreeReadErrDarkTheme : _subtreeReadErrLightTheme; }

	/**
	 * Set the base font for the tree view,
	 **/
	void setBaseFont( const QFont & font );

        /**
         * Return the icon indicate an item's type (file, directory etc.)
         * or a null icon if the type cannot be determined.
         **/
        QIcon itemTypeIcon( FileInfo * item ) const;

	/**
	 * Open a directory URL.
	 **/
	void openUrl( const QString & url );

	/**
	 * Open a pkg URL: Read installed packages that match the specified
	 * PkgFilter and their file lists from the system's package manager(s).
	 *
	 * Notice that PkgFilter has a constructor that takes a QString and
	 * uses PkgFilter::Auto as the default filter mode to determine the
	 * filter mode from any special characters present in the URL, e.g.
	 *
	 * "Pkg:/"		       -> PkgFilter::SelectAll
	 * contains "*" or "?"	       -> PkgFilter::Wildcard
	 * contains "^" or "$" or ".*" -> PkgFilter::RegExp
	 * "Pkg:/=foo"		       -> PkgFilter::ExactMatch
	 * otherwise		       -> PkgFilter::StartsWith
	 **/
	void readPkg( const PkgFilter & pkgFilter );

	/**
	 * Clear this view's contents.
	 **/
	void clear();

	/**
	 * Return the setting for CrossFilesystems.  This is the value
	 * from the configuration file, potentially updated from the
	 * general config page.  It will be written to the settings when
	 * the program closes.  There is another crossFilesystems flag
	 * held by DirTree, which is used for reading and may be updated
	 * by the user, but won't be written to the settings.
	 **/
	bool crossFilesystems() const { return _crossFilesystems; }

	/**
	 * Return 'true' if dominant tree items should be shown in bold font,
	 * 'false' if not.
	 **/
	bool useBoldForDominantItems() const { return _useBoldForDominantItems; }

	/**
	 * Set to 'true' if dominant tree items should be shown in bold font,
	 * 'false' if not.
	 **/
	void setUseBoldForDominantItems( bool val )
	    { _useBoldForDominantItems = val; }

	/**
	 * Return the setting for UpdateTimerMillisec
	 **/
	int updateTimerMillisec() const { return _updateTimerMillisec; }

	/**
	 * Return the icon directory for the current tree item size setting.
	 **/
	QString treeIconDir() const { return treeIconDir( _treeItemSize ); }

	/**
	 * Returns the current tree item size setting.
	 **/
	DirTreeItemSize dirTreeItemSize() const { return _treeItemSize; }

	/**
	 * Returns the tree icon directory for the given enum value.
	 **/
	static QString treeIconDir( DirTreeItemSize treeItemSize )
	    { return treeItemSize == DTIS_Medium ? ":/icons/tree-medium/" : ":/icons/tree-small/"; }

	/**
	 * Returns the tree item size setting for a given icon directory string.
	 **/
	static DirTreeItemSize dirTreeItemSize( const QString & treeIconDir )
	    { return treeIconDir.contains( "medium" ) ? DTIS_Medium : DTIS_Small; }

	/**
	 * Returns the configured tree icon size.
	 **/
	QSize dirTreeIconSize() const
	    { return _dirIcon.actualSize( QSize( 1024, 1024 ) ); }

	/**
	 * Update internal settings from the general configuration page.
	 * Any changes will be saved to the conf file in the destructor.
	 **/
	void updateSettings( bool crossFilesystems,
			     bool useBoldForDominant,
			     DirTreeItemSize dirTreeItemSize,
			     int updateTimerMillisec );

	/**
	 * Read parameters from settings file.
	 **/
	void readSettings();

	/**
	 * Write parameters to settings file.
	 **/
	void writeSettings();

	/**
	 * Refresh the selected items: Re-read their contents from disk.
	 * This requires a selection model to be set.
	 **/
	void refreshSelected();

	/**
	 * Set the update speed to slow (default 3 sec instead of 250 millisec).
	 **/
	void setSlowUpdate();

	/**
	 * Return the slow update flag.
	 **/
//	bool slowUpdate() const { return _slowUpdate; }

	// Mapping of tree items to model rows and vice versa.
	//
	// This is in the model and not in the FileInfo class to encapsulate
	// handling of the "dot entry". In this case, it is handled as a
	// separate subdirectory. But this might change in the future, or it
	// might even become configurable.

	/**
	 * Get the FileInfo for a model index. This may return 0 if the index
	 * is invalid.
	 **/
	FileInfo * itemFromIndex( const QModelIndex & index );

	/**
	 * Return a model index for 'item' and 'column'.
	 **/
	QModelIndex modelIndex( FileInfo * item, int column = 0 ) const;

	/**
	 * Return the current sort column.
	 **/
	DataColumn sortColumn() const { return _sortCol; }

	/**
	 * Return the current sort order
	 * (Qt::AscendingOrder or Qt::DescendingOrder).
	 **/
	Qt::SortOrder sortOrder() const { return _sortOrder; }

	/**
	 * Return the formatted tooltip for the size column.
	 **/
	QVariant sizeColTooltip( FileInfo * item ) const;

	/**
	 * Return data to be displayed for the specified model index and role.
	 **/
	QVariant data( const QModelIndex & index, int role ) const override;

	/**
	 * Return header data (in this case: column header texts) for the
	 * specified section (column number).
	 **/
	QVariant headerData( int	     section,
			     Qt::Orientation orientation,
			     int	     role ) const override;

	/**
	 * Return item flags for the specified model index. This specifies if
	 * the item can be selected, edited etc.
	 **/
	Qt::ItemFlags flags( const QModelIndex & index ) const override;

	/**
	 * Return the model index for the specified row (direct tree child
	 * number) and column of item 'parent'.
	 **/
	QModelIndex index( int row,
			   int column,
			   const QModelIndex & parent = QModelIndex() ) const override;

	/**
	 * Return the parent model index of item 'index'.
	 **/
	QModelIndex parent( const QModelIndex & index ) const override;

	/**
	 * Sort the model.
	 **/
	void sort( int column,
		   Qt::SortOrder order = Qt::AscendingOrder ) override;

	/**
	 * Return the resource path of the directory icon.
	 **/
	const QIcon & dirIcon() const { return _dirIcon; }

	/**
	 * Return the resource path of the unreadable directory icon.
	 **/
	const QIcon & unreadableDirIcon() const { return _unreadableDirIcon; }

	/**
	 * Return the resource path of the mount point icon.
	 **/
	const QIcon & mountPointIcon() const { return _mountPointIcon; }

	/**
	 * Return the resource path of the network mount point icon.
	 **/
	const QIcon & networkIcon() const { return _networkIcon; }

	/**
	 * Return whether the FileInfo item exists and is still valid.
	 **/
	static bool checkMagicNumber( const FileInfo * item );


    public slots:

	/**
	 * Fix up sort order while reading: Sort by read jobs if the sort
	 * column is the PercentBarCol.
	 **/
	void busyDisplay();


    protected slots:

	/**
	 * Fix up sort order after reading is finished: No longer Sort by read
	 * jobs if the sort column is the PercentBarCol.
	 **/
	void idleDisplay();

	/**
	 * Process notification that the read job for 'dir' is finished.
	 * Other read jobs might still be pending.
	 **/
	void readJobFinished( DirInfo *dir );

	/**
	 * Process notification that reading the dir tree is completely
	 * finished.
	 **/
	void readingFinished();

	/**
	 * Send all pending updates to the connected views.
	 * This is triggered by the update timer.
	 **/
	void sendPendingUpdates();

	/**
	 * Notification that a subtree is about to be deleted.
	 **/
	void deletingChild( FileInfo * child );

	/**
	 * Notification that deleting a subtree is done.
	 **/
	void childDeleted();

	/**
	 * Notification that a subtree is about to be cleared.
	 **/
	void clearingSubtree( DirInfo * subtree );

	/**
	 * Notification that clearing a subtree is done.
	 **/
	void subtreeCleared( DirInfo * subtree );


    protected:

	/**
	 * Create a new tree (and delete the old one if there is one)
	 **/
	void createTree();

	/**
	 * Load all required icons.
	 **/
	void loadIcons();

	/**
	 * Notify the view (with beginInsertRows() and endInsertRows()) about
	 * new children (all the children of 'dir'). This might become
	 * recursive if any of those children in turn are already finished.
	 **/
	void newChildrenNotify( DirInfo * dir );

	/**
	 * Notify the view about changed data of 'dir'.
	 **/
	void dataChangedNotify( DirInfo * dir );

	/**
	 * Update the persistent indexes with current row after sorting etc.
	 **/
	void updatePersistentIndexes();

	/**
	 * Delayed update of the data fields in the view for 'dir':
	 * Store 'dir' and all its ancestors in _pendingUpdates.
	 *
	 * The updates will be sent several times per second to the views with
	 * 'sendPendingUpdates()'.
	 **/
	void delayedUpdate( DirInfo * dir );

	/**
	 * Return 'true' if 'item' or any ancestor (parent or parent's parent
	 * etc.) is still busy, i.e. the read job for the directory itself (not
	 * any children!) is still queued or currently reading.
	 **/
	bool anyAncestorBusy( FileInfo * item ) const;

	/**
	 * Invalidate all persistent indexes in 'subtree'. 'includeParent'
	 * indicates if 'subtree' itself will become invalid.
	 **/
	void invalidatePersistent( FileInfo * subtree,
				   bool       includeParent );


	/**
	 * Data for different roles for each item (row) and column
	 **/
	QVariant columnText     ( FileInfo * item, int col ) const;
	QVariant columnIcon     ( FileInfo * item, int col ) const;
	QVariant columnAlignment( FileInfo * item, int col ) const;
	QVariant columnFont     ( FileInfo * item, int col ) const;

	/**
	 * Raw data for direct communication with our item delegates
	 * (PercentBarDelegate, SizeColDelegate)
	 **/
	QVariant columnRawData ( FileInfo * item, int col ) const;

	/**
	 * Return the number of direct children (plus the attic if there is
	 * one) of a subtree.
	 **/
	int directChildrenCount( FileInfo * subtree ) const;

	/**
	 * Return the text for the size for 'item'
	 **/
	QVariant sizeColText( FileInfo * item ) const;

	/**
	 * Find the child number 'childNo' among the children of 'parent'.
	 * Return 0 if not found.
	 **/
	FileInfo * findChild( DirInfo * parent, int childNo ) const;

	/**
	 * Find the row number (the index, starting with 0) of 'child' among
	 * its parent's children.
	 **/
	int rowNumber( FileInfo * child ) const;

	/**
	 * Start removing rows.
	 **/
	void beginRemoveRows( const QModelIndex & parent, int first, int last );

	/**
	 * End removing rows.
	 *
	 * Unlike the QAbstractItemModel's implementation, this method checks
	 * if removing rows is in progress in the first place so there will not
	 * be a segfault (!) if endRemoveRows is called without a corresponding
	 * beginRemoveRows().
	 *
	 * As usual, Qt's item classes don't even give it an honest try to do
	 * the most basic checking. This implementation does.
	 **/
	void endRemoveRows();

	//
	// Reimplemented from QAbstractItemModel:
	//

	/**
	 * Return the number of rows (direct tree children) for 'parent'.
	 **/
	int rowCount( const QModelIndex & parent ) const override;

	/**
	 * Return the number of columns for 'parent'.
	 **/
	int columnCount( const QModelIndex & ) const override
	    { return DataColumns::colCount(); }


    private:

	//
	// Data members
	//
	DirTree       * _tree			{ nullptr };

	bool            _crossFilesystems;
	DirTreeItemSize _treeItemSize;
	QSet<DirInfo *> _pendingUpdates;
	QTimer          _updateTimer;
	int             _updateTimerMillisec;
	int             _slowUpdateMillisec;
	bool            _slowUpdate		{ false };
	DataColumn      _sortCol		{ ReadJobsCol };
	Qt::SortOrder   _sortOrder		{ Qt::DescendingOrder };
	bool            _removingRows		{ false };
	bool            _useBoldForDominantItems;

	// Colors and fonts
	QColor _dirReadErrLightTheme;
	QColor _subtreeReadErrLightTheme;
	QColor _dirReadErrDarkTheme;
	QColor _subtreeReadErrDarkTheme;

	QFont  _baseFont;
	QFont  _themeFont;

	// The various tree icons
	QIcon  _dirIcon;
	QIcon  _dotEntryIcon;
	QIcon  _atticIcon;
	QIcon  _fileIcon;
	QIcon  _symlinkIcon;
	QIcon  _unreadableDirIcon;
	QIcon  _mountPointIcon;
	QIcon  _networkIcon;
	QIcon  _stopIcon;
	QIcon  _excludedIcon;
	QIcon  _blockDeviceIcon;
	QIcon  _charDeviceIcon;
	QIcon  _specialIcon;
	QIcon  _pkgIcon;

    };	// class DirTreeModel


    /**
     * Print a QModelIndex of this model in text form to a debug stream.
     **/
    inline QTextStream & operator<< ( QTextStream & stream, const QModelIndex & index )
    {
	if ( ! index.isValid() )
	    stream << "<Invalid QModelIndex>";
	else
	{
	    FileInfo * item = static_cast<FileInfo *>( index.internalPointer() );
	    stream << "<QModelIndex row: " << index.row()
		   << " col: " << index.column();

	    if ( DirTreeModel::checkMagicNumber( item ) )
		stream << " <INVALID FileInfo *>";
	    else
		stream << " " << item;

	    stream << " >";
	}

	return stream;
    }

}	// namespace QDirStat

#endif // DirTreeModel_h
