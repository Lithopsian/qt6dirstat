/*
 *   File name: DirTreeModel.h
 *   Summary:   Qt data model for directory tree
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
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
#include <QTimer>
#include <QTreeView>

#include "DataColumns.h"
#include "Typedefs.h" // _L1


namespace QDirStat
{
    class DirInfo;
    class DirTree;
    class FileInfo;
    class FileInfoSet;
    class PkgFilter;

    enum DirTreeItemSize
    {
	DTIS_Medium,
	DTIS_Small,
    };

    enum CustomRoles
    {
	PercentRole = Qt::UserRole,
	TreeLevelRole,
	SizeTextRole,
    };


    class DirTreeModel final : public QAbstractItemModel
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	DirTreeModel( QObject * parent );

	/**
	 * Destructor.
	 **/
	~DirTreeModel() override;

	/**
	 * Returns the internal DirTree this view works on.
	 *
	 * Handle with caution: this might be short-lived information.	The
	 * model might choose to create a new tree shortly after returning
	 * this, so don't store this pointer internally.
	 **/
	DirTree * tree() const { return _tree; }

	/**
	 * This is protected in the base class, but it's the only reasonable
	 * way for the view to figure out what items are expanded.
	 **/
	QModelIndexList persistentIndexList() const
	    { return QAbstractItemModel::persistentIndexList(); }

	/**
	 * Return a stylesheet string to set a label text to the configured
	 * directory read error colour.
	 **/
	QString errorStyleSheet() const
	    { return QString{ "QLabel { color: %1; }" }.arg( dirReadErrColor().name() ); }

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
	void setBaseFont( const QFont & font )
	    { _baseFont = font; }

	/**
	 * Return the icon indicate an item's type (file, directory etc.)
	 * or a null icon if the type cannot be determined.
	 **/
	QIcon itemTypeIcon( const FileInfo * item ) const;

	/**
	 * Open a pkg URL: Read installed packages that match the specified
	 * PkgFilter and their file lists from the system's package manager(s).
	 *
	 * Note that PkgFilter has a constructor that takes a QString and
	 * uses PkgFilter::Auto as the default filter mode to determine the
	 * filter mode from any special characters present in the URL, e.g.
	 *
	 * "Pkg:/"                     -> PkgFilter::SelectAll
	 * contains "*" or "?"         -> PkgFilter::Wildcard
	 * contains "^" or "$" or ".*" -> PkgFilter::RegExp
	 * "Pkg:/=foo"                 -> PkgFilter::ExactMatch
	 * otherwise                   -> PkgFilter::StartsWith
	 **/
//	void readPkg( const PkgFilter & pkgFilter );

	/**
	 * Open a pkg URL: read installed packages that match the specified
	 * PkgFilter and their file lists from the system's package manager(s).
	 *
	 * Note that PkgFilter has a constructor that takes a QString and
	 * uses PkgFilter::Auto as the default filter mode to determine the
	 * filter mode from any special characters present in the URL, e.g.
	 *
	 * "Pkg:/"                     -> PkgFilter::SelectAll
	 * contains "*" or "?"         -> PkgFilter::Wildcard
	 * contains "^" or "$" or ".*" -> PkgFilter::RegExp
	 * "Pkg:/=foo"                 -> PkgFilter::ExactMatch
	 * otherwise                   -> PkgFilter::StartsWith
	 **/
	void refresh( const PkgFilter & pkgFilter );

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
	QLatin1String treeIconDir() const { return treeIconDir( _treeItemSize ); }

	/**
	 * Returns the current tree item size setting.
	 **/
	DirTreeItemSize dirTreeItemSize() const { return _treeItemSize; }

	/**
	 * Update internal settings from the general configuration page.
	 * Any changes will be saved to the conf file in the destructor.
	 **/
	void updateSettings( bool crossFilesystems,
	                     bool useBoldForDominant,
	                     DirTreeItemSize dirTreeItemSize,
	                     int updateTimerMillisec );

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
	 * Set the icon size of a QTreeView's items based on
	 * the configured DirTree icon size.
	 **/
	void setTreeIconSize( QTreeView * tree ) const
	    { tree->setIconSize( dirTreeIconSize() ); }

#if 0
    public slots:

	/**
	 * Item clicked in the tree widget, for debugging.
	 **/
	void itemClicked( const QModelIndex & index );
#endif

    protected slots:

	/**
	 * Process notification that a tree read of some sort is
	 * starting.
	 **/
	void startingRead();

	/**
	 * Process notification that the read job for 'dir' is finished.
	 * Other read jobs might still be pending.
	 **/
	void readJobFinished( DirInfo * dir );

	/**
	 * Process notification that reading the dir tree is completely
	 * finished.
	 **/
	void readingFinished();

	/**
	 * Send all pending updates to the connected views, triggered from
	 * the update timer.  The list of pending updates can be long and
	 * not necessarily unique, but most entries are filtered out as
	 * not being "touched".  The dataChanged() signal is sent for the
	 * items that have been touched and so might be visible.
	 **/
	void sendPendingUpdates();

	/**
	 * Notification that items are going to be deleted from the tree.
	 * A set of items with the same parent can be notified in a single
	 * call.  The Qt documentation isn't clear on the matter, but in
	 * practice the rows don't have to be contiguous (possibly due to
	 * some specific aspect of our model).  In fact, all the child rows
	 * could be notified for simplicity and it still works.
	 **/
	void deletingChildren( DirInfo * parent, const FileInfoSet & children );

	/**
	 * Notification that a subtree is about to be cleared.
	 **/
	void clearingSubtree( DirInfo * subtree );

	/**
	 * Called when the tree is about to be cleared.  Kill the timer and
	 * any pending updates, and propagate the signal.
	 **/
	void beginResetModel();

	/**
	 * End removing rows.
	 *
	 * Unlike the QAbstractItemModel's implementation, this method checks
	 * if removing rows is in progress in the first place so there will not
	 * be a segfault (!) if endRemoveRows is called without a corresponding
	 * beginRemoveRows().
	 **/
	void endRemoveRows();


    protected:

	/**
	 * Read parameters from settings file.
	 **/
	void readSettings();

	/**
	 * Write parameters to settings file.
	 **/
	void writeSettings();

	/**
	 * Create a new tree (and delete the old one if there is one)
	 **/
	void createTree();

	/**
	 * Load all required icons.
	 **/
	void loadIcons();

	/**
	 * Returns the tree icon directory for the given enum value.
	 **/
	static QLatin1String treeIconDir( DirTreeItemSize treeItemSize )
	    { return treeItemSize == DTIS_Medium ? ":/icons/tree-medium/"_L1 : ":/icons/tree-small/"_L1; }

	/**
	 * Returns the tree item size setting for a given icon directory string.
	 **/
	static DirTreeItemSize dirTreeItemSize( const QString & treeIconDir )
	    { return treeIconDir.contains( "medium"_L1 ) ? DTIS_Medium : DTIS_Small; }

	/**
	 * Returns the configured tree icon size.
	 **/
	QSize dirTreeIconSize() const
	    { return _dirIcon.actualSize( QSize{ 1024, 1024 } ); }

	/**
	 * Notify the view (with beginInsertRows() and endInsertRows()) about
	 * new children (all the children of 'dir'). This might become
	 * recursive if any of those children in turn are already finished.
	 *
	 * Note that the rows have already been inserted.  Normally,
	 * beginInsertRows() is called before the rows are inserted so it is
	 * essential that the event loop is not allowed to run before this
	 * notification executes.
	 **/
	void newChildrenNotify( DirInfo * dir );

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
	 * Invalidate all persistent indexes in 'subtree'. 'includeParent'
	 * indicates if 'subtree' itself will become invalid.
	 **/
//	void invalidatePersistent( const FileInfo * subtree, bool includeParent );

	/**
	 * Return the icon to be displayed in the tree for this item.
	 **/
	QVariant columnIcon ( FileInfo * item, int col ) const;

	/**
	 * Return 'true' if the application uses a dark widget theme.
	 **/
	static bool usingDarkTheme()
	    { return QGuiApplication::palette().color( QPalette::Active, QPalette::Base ).lightness() < 160; }

	/**
	 * Find the child number 'childNo' among the children of 'parent'.
	 * Return 0 if not found.
	 **/
	FileInfo * findChild( DirInfo * parent, int childNo ) const;

	/**
	 * Find the row number (the index, starting with 0) of 'child' among
	 * its parent's children.
	 **/
	int rowNumber( const FileInfo * child ) const;

	/**
	 * Start removing rows.
	 **/
	void beginRemoveRows( const QModelIndex & parent, int first, int last );

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

	/**
	 * Return data to be displayed for the specified model index and role.
	 **/
	QVariant data( const QModelIndex & index, int role ) const override;

	/**
	 * Return header data (in this case: column header texts) for the
	 * specified section (column number).
	 **/
	QVariant headerData( int section, Qt::Orientation orientation, int role ) const override;

	/**
	 * Return item flags for the specified model index. This specifies if
	 * the item can be selected, edited etc.
	 **/
	Qt::ItemFlags flags( const QModelIndex & index ) const override;

	/**
	 * Return the model index for the specified row (direct tree child
	 * number) and column of item 'parent'.
	 **/
	QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex{} ) const override;

	/**
	 * Return the parent model index of item 'index'.
	 **/
	QModelIndex parent( const QModelIndex & index ) const override;


    private:

	DirTree * _tree;

	bool            _crossFilesystems;
	bool            _useBoldForDominantItems;
	DirTreeItemSize _treeItemSize;
	DataColumn      _sortCol{ ReadJobsCol };
	Qt::SortOrder   _sortOrder{ Qt::DescendingOrder };

	QSet<DirInfo *> _pendingUpdates;
	QTimer          _updateTimer;
	int             _updateTimerMillisec;
	int             _slowUpdateMillisec;
	bool            _slowUpdate{ false };
	bool            _removingRows{ false };

	// Colors and fonts
	QColor _dirReadErrLightTheme;
	QColor _subtreeReadErrLightTheme;
	QColor _dirReadErrDarkTheme;
	QColor _subtreeReadErrDarkTheme;
	QFont  _baseFont;

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

}	// namespace QDirStat

#endif	// DirTreeModel_h
