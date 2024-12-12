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

#include <QAction>
#include <QHeaderView>
#include <QMenu>
#include <QTreeView>

#include "DataColumns.h"


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
    class HeaderTweaker final : public QObject
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
	 * Switch the layout to the one with the specified name.
	 **/
	void changeLayout( const QString & layoutName );

	/**
	 * Return the names of the three layouts.
	 **/
	constexpr static QLatin1String l1Name() { return QLatin1String{ "L1" }; }
	constexpr static QLatin1String l2Name() { return QLatin1String{ "L2" }; }
	constexpr static QLatin1String l3Name() { return QLatin1String{ "L3" }; }

	/**
	 * Write parameters to the settings file.
	 **/
	void writeSettings();


    protected slots:

	/**
	 * Read parameters from the settings file.
	 *
	 * This only makes sense only when there are columns, i.e. after the
	 * model is set and the parent QTreeView has requested header data.
	 * It makes most sense to connect this slot with the header's
	 * sectionCountChanged() signal, which is emitted once when the
	 * header sections are first created.
	 **/
	void readSettings();

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
	 * Set auto size mode for all columns on or off.
	 **/
	void setAllColumnsResizeMode( bool autoSize )
	    { _header->setSectionResizeMode( resizeMode( autoSize ) ); }

	/**
	 * Return the opposite resize mode to 'currentResizeMode'.
	 **/
	 static QHeaderView::ResizeMode toggledResizeMode( QHeaderView::ResizeMode currentResizeMode )
	    { return resizeMode( currentResizeMode == QHeaderView::Interactive ); }

	/**
	 * Return the ResizeMode for either auto-size or interactive sizing,
	 * corresponding to 'autoSize'.
	 **/
	static QHeaderView::ResizeMode resizeMode( bool autoSize )
	    { return autoSize ? QHeaderView::ResizeToContents : QHeaderView::Interactive; }

	/**
	 * Save the current layout status.
	 **/
	void saveCurrentLayout();

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


    private:

	const DirTreeView * _treeView;
	QHeaderView       * _header;
	int                 _currentSection{ -1 };
	ColumnLayoutList    _layouts;
	ColumnLayout      * _currentLayout{ nullptr };

    };	// class HeaderTweaker

}	// namespace QDirStat

#endif	// HeaderTweaker_h
