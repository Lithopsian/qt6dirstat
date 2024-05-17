/*
 *   File name: HeaderTweaker.h
 *   Summary:   Helper class for DirTreeView
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef HeaderTweaker_h
#define HeaderTweaker_h

#include <QHeaderView>

#include "DataColumns.h"


class QHeaderView;
class QAction;
class QMenu;


namespace QDirStat
{
    class DirTreeView;
    class ColumnLayout;

    /**
     * Decorator class for a DirTreeView's QHeaderView that takes care about
     * the header's context menu and the corresponding actions and saving and
     * restoring state.
     **/
    class HeaderTweaker: public QObject
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	HeaderTweaker( QHeaderView * header, DirTreeView * parent );

	/**
	 * Destructor.
	 **/
	~HeaderTweaker() override;

	/**
	 * Resize a header view to contents.
	 **/
	static void resizeToContents( QHeaderView * header );

	/**
	 * Switch the layout to the one with the specified name.
	 **/
	void changeLayout( const QString & layoutName );

	/**
	 * Return the names of the three layouts.
	 **/
	inline static QLatin1String l1Name() { return QLatin1String( "L1" ); }
	inline static QLatin1String l2Name() { return QLatin1String( "L2" ); }
	inline static QLatin1String l3Name() { return QLatin1String( "L3" ); }


    protected slots:

	/**
	 * Set auto size mode for all columns on.
	 **/
	void setAllColumnsAutoSize();

	/**
	 * Set interactive size mode (i.e. auto size mode off) for all columns.
	 **/
	void setAllColumnsInteractiveSize();

	/**
	 * Make all hidden columns visible again.
	 **/
	void showAllHiddenColumns();

	/**
	 * Reset all columns to defaults: Column order, visibility, auto size.
	 **/
	void resetToDefaults();

	/**
	 * Initialize the header view. This makes sense only when it has
	 * columns, i.e. when the model is set, and the parent QTreeView
	 * requested header data. It makes most sense to connect this slot with
	 * the header's sectionCountChanged() signal.
	 **/
	void initHeader();

	/**
	 * Post a context menu for the header at 'pos'.
	 **/
	void contextMenu( const QPoint & pos );

	/**
	 * Hide the current column.
	 **/
	void hideCurrentCol();

	/**
	 * Show the hidden column from sender()->data().
	 **/
	void showHiddenCol();

	/**
	 * Toggle auto size of the current column.
	 **/
	void autoSizeCurrentCol();


    protected:

	/**
	 * Create one action and connect to the given slot.
	 **/
	QAction * createAction( QMenu * menu, const QString & title, void( HeaderTweaker::*slot )( void ) );

	/**
	 * Create internally used actions and connect them to the appropriate
	 * slots.
	 **/
//	void createActions();

	/**
	 * Create one column layout.
	 **/
	void createColumnLayout( const QString & layoutName );

	/**
	 * Create the column layouts.
	 **/
	void createColumnLayouts();

	/**
	 * Update all actions for a context menu for logical section
	 * 'section'.
	 **/
//	void updateActions( int section );

	/**
	 * Create a submenu for the currently hidden columns.
	 **/
	QMenu * createHiddenColMenu( QWidget * parent );

	/**
	 * Set auto size mode for all columns on or off.
	 **/
	void setAllColumnsResizeMode( bool autoSize );

	/**
	 * Set auto size mode for all columns on or off.
	 **/
	 QHeaderView::ResizeMode toggleResizeMode( QHeaderView::ResizeMode resizeMode )
		{ return resizeMode == QHeaderView::Interactive ? QHeaderView::ResizeToContents : QHeaderView::Interactive; }

	/**
	 * Set auto size mode for all columns on or off.
	 **/
	QHeaderView::ResizeMode resizeMode( bool autoSize ) const
	    { return autoSize ? QHeaderView::ResizeToContents : QHeaderView::Interactive; }

	/**
	 * Save the current status in 'layout'.
	 **/
	void saveLayout( ColumnLayout * layout );

	/**
	 * Apply the settings from 'layout'.
	 **/
	void applyLayout( ColumnLayout * layout );

	/**
	 * Ensure consistency of a layout.
	 **/
	void fixupLayout( ColumnLayout * layout );

	/**
	 * Order the columns according to 'colOrderList'.
	 **/
	void setColumnOrder( const DataColumnList & colOrderList);

	/**
	 * Show the columns that are in 'columns'.
	 **/
	void setColumnVisibility( const DataColumnList & columns );

	/**
	 * Return the column name for the specified logical section number.
	 **/
	QString colName( int section ) const;

	/**
	 * Return 'true' if logical section no. 'section' has auto resize mode.
	 **/
	bool autoSizeCol( int section ) const
	    { return resizeMode( section ) == QHeaderView::ResizeToContents; }

	/**
	 * Add any columns that are missing from the default columns to
	 * 'colList'.
	 **/
	void addMissingColumns( DataColumnList & colList );

	/**
	 * Return the resize mode for the specified section.
	 **/
	QHeaderView::ResizeMode resizeMode( int section ) const
	    { return _header->sectionResizeMode( section ); }

	/**
	 * Set the resize mode for the specified section.
	 **/
	void setResizeMode( int section, QHeaderView::ResizeMode resizeMode )
	    { _header->setSectionResizeMode( section, resizeMode ); }

	/**
	 * Read parameters from the settings file.
	 **/
	void readSettings();

	/**
	 * Write parameters to the settings file.
	 **/
	void writeSettings();

	/**
	 * Read the settings for a layout.
	 **/
	void readLayoutSettings( ColumnLayout * layout );

	/**
	 * Write the settings for a layout.
	 **/
	void writeLayoutSettings( const ColumnLayout * layout ) const;


    private:

	//
	// Data members
	//

	DirTreeView                  * _treeView;
	QHeaderView                  * _header;
	int                            _currentSection	{ -1 };
	QHash<QString, ColumnLayout *> _layouts;
	ColumnLayout                 * _currentLayout	{ nullptr };

    };	// class HeaderTweaker


    /**
     * Helper class to store information about different column layouts.
     **/
    class ColumnLayout
    {
    public:
	ColumnLayout( const QString & name ):
	    name { name }
	{}

	QString        name;
	DataColumnList columns;

	/**
	 * Return the default column list for this layout.
	 **/
	DataColumnList defaultColumns() const { return defaultColumns( name ); }

	/**
	 * Return the default column list for a layout.
	 **/
	static DataColumnList defaultColumns( const QString & layoutName );

    };	// class ColumnLayout

}	// namespace QDirStat

#endif	// HeaderTweaker_h
