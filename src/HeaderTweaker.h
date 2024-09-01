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


class QAction;
class QMenu;


namespace QDirStat
{
    class DirTreeView;

    /**
     * Helper class to store information about different column layouts.
     **/
    struct ColumnLayout
    {
	ColumnLayout( const QString & name ):
	    name{ name }
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

    };	// struct ColumnLayout


    typedef QHash<QString, ColumnLayout *> ColumnLayoutList;


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
	constexpr static QLatin1String l1Name() { return "L1"_L1; }
	constexpr static QLatin1String l2Name() { return "L2"_L1; }
	constexpr static QLatin1String l3Name() { return "L3"_L1; }

	/**
	 * Save the current status in 'layout'.
	 **/
	void saveLayout() { if ( _currentLayout ) saveLayout( _currentLayout ); }

	/**
	 * Write parameters to the settings file.
	 **/
	void writeSettings();


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
	 * Create one column layout.
	 **/
	void createColumnLayout( const QString & layoutName )
	    { _layouts[ layoutName ] = new ColumnLayout{ layoutName }; }

	/**
	 * Create the column layouts.
	 **/
	void createColumnLayouts();

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
	 * Return the column name for the specified logical section number.
	 **/
	QString colName( int section ) const;

	/**
	 * Return 'true' if logical section no. 'section' has auto resize mode.
	 **/
	bool autoSizeCol( int section ) const
	    { return resizeMode( section ) == QHeaderView::ResizeToContents; }

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


    private:

	//
	// Data members
	//

	DirTreeView      * _treeView;
	QHeaderView      * _header;
	int                _currentSection{ -1 };
	ColumnLayoutList   _layouts;
	ColumnLayout     * _currentLayout{ nullptr };

    };	// class HeaderTweaker

}	// namespace QDirStat

#endif	// HeaderTweaker_h
