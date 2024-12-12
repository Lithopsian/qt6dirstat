/*
 *   File name: HeaderTweaker.cpp
 *   Summary:   Helper class for DirTreeView
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QAction>
#include <QMenu>

#include "HeaderTweaker.h"
#include "DirTreeView.h"
#include "Exception.h"
#include "Settings.h"


using namespace QDirStat;


namespace
{
    /**
     * Save the state of 'header' in 'layout'.
     **/
    void saveLayout( const QHeaderView * header, ColumnLayout * layout )
    {
	layout->columns.clear();

	for ( int visualIndex = 0; visualIndex < header->count(); ++visualIndex )
	{
	    const int logicalIndex = header->logicalIndex( visualIndex );
	    if ( DataColumns::isValidCol( logicalIndex ) )
	    {
		if ( !header->isSectionHidden( logicalIndex ) )
		    layout->columns << DataColumns::fromViewCol( logicalIndex );
	    }
	}
    }


    /**
     * Return the column name for 'section' in 'treeView'.
     **/
    QString colName( const DirTreeView * treeView, int section )
    {
	if ( !DataColumns::isValidCol( section ) )
	    logError() << "No column at section " << section << Qt::endl;

	const DataColumn col = DataColumns::fromViewCol( section );

	return treeView->headerData( col, Qt::Horizontal, Qt::DisplayRole ).toString();
    }


    /**
     * Set 'section' in 'header' to be not hidden and move it to the end of the
     * header for consistent positioning.  This should normally only be called
     * if 'section' is currently hidden to avoid unexpectedly moving visible
     * sections.
     **/
    void showSection( QHeaderView * header, int section )
    {
	header->moveSection( header->visualIndex( section ), header->count() - 1 );
	header->showSection( section );
    }

    /**
     * Ensure consistency of a layout.  If the settings have been corrupted
     * so that there are no columns for this layout, show a default set and
     * make sure that the name column is first.
     **/
    void fixupLayout( ColumnLayout * layout )
    {
	if ( layout->columns.isEmpty() )
	{
	    logInfo() << "No columns configured: falling back to default columns" << Qt::endl;
	    layout->columns = layout->defaultColumns();
	}

	DataColumns::ensureNameColFirst( layout->columns );
    }


    /**
     * Return a settings key for the width of a column.
     **/
    QString colWidthKey( DataColumn col )
    {
	return "Width_"_L1 % DataColumns::toString( col );
    }


    /**
     * Read the settings for a layout.
     **/
    void readLayoutSettings( ColumnLayout * layout )
    {
	Settings settings;

	settings.beginGroup( "TreeViewLayout_" + layout->name );
	layout->columns = DataColumns::fromStringList( settings.value( "Columns" ).toStringList() );
	settings.endGroup();

	fixupLayout( layout );
    }


    /**
     * Write the settings for a layout.
     **/
    void writeLayoutSettings( const ColumnLayout * layout )
    {
	Settings settings;
	settings.beginGroup( "TreeViewLayout_" + layout->name );
	settings.setValue( "Columns", DataColumns::toStringList( layout->columns ) );
	settings.endGroup();
    }


    /**
     * Order the columns as they are ordered in the 'columns' list, starting at
     * visual index 0.  The remaining columns not in 'columns' will end up
     * after these, and will be hidden later.
     **/
    void setColumnOrder( QHeaderView * header, const DataColumnList & columns )
    {
	int visualIndex = 0;

	for ( DataColumn col : columns )
	{
	    if ( visualIndex >= header->count() )
	    {
		logWarning() << "More configured columns than header sections" << Qt::endl;
		return;
	    }

	    // logDebug() << "Moving " << col << " to position " << visualIndex << Qt::endl;
	    header->moveSection( header->visualIndex( DataColumns::toViewCol( col ) ), visualIndex++ );
	}
    }


    /**
     * Show the columns that are in 'columns', and hide the rest.
     **/
    void setColumnVisibility( QHeaderView * header, const DataColumnList & columns )
    {
	for ( int section = 0; section < header->count(); ++section )
	    header->setSectionHidden( section, !columns.contains( DataColumns::fromViewCol( section ) ) );
    }


    /**
     * Create the column layouts.
     **/
    void createColumnLayouts( ColumnLayoutList & layouts )
    {
	/**
	 * Create one column layout.
	 **/
	const auto createColumnLayout = [ &layouts ]( const QString & layoutName )
	{
	    layouts[ layoutName ] = new ColumnLayout{ layoutName };
	};

	// Layout L1: Short
	createColumnLayout( HeaderTweaker::l1Name() );

	// L2: Classic QDirStat Style
	createColumnLayout( HeaderTweaker::l2Name() );

	// L3: Full
	createColumnLayout( HeaderTweaker::l3Name() );
    }


    /**
     * Apply the settings from 'layout'.
     **/
    void applyLayout( QHeaderView * header, ColumnLayout * layout )
    {
	fixupLayout( layout );
	setColumnOrder( header, layout->columns );
	setColumnVisibility( header, layout->columns );
    }

} // namespace


HeaderTweaker::HeaderTweaker( QHeaderView * header, DirTreeView * parent ):
    QObject{ parent },
    _treeView{ parent },
    _header{ header }
{
    CHECK_PTR( parent );
    CHECK_PTR( header );

    _header->setContextMenuPolicy( Qt::CustomContextMenu );
    _header->setDefaultAlignment( Qt::AlignVCenter | Qt::AlignHCenter );

    createColumnLayouts( _layouts );

    connect( _header, &QHeaderView::sectionCountChanged,
             this,    &HeaderTweaker::readSettings );

    connect( _header, &QHeaderView::customContextMenuRequested,
             this,    &HeaderTweaker::contextMenu );
}


HeaderTweaker::~HeaderTweaker()
{
    qDeleteAll( _layouts );
}


void HeaderTweaker::contextMenu( const QPoint & pos )
{
    QMenu menu;

    /**
     * Create an action with 'title' in 'menu' and connect it to 'slot'.
     **/
    const auto createAction = [ this ]( QMenu * menu, const QString & title, void( HeaderTweaker::*slot )( void ) )
    {
	QAction * action = menu->addAction( title );
	connect( action, &QAction::triggered, this, slot );

	return action;
    };

    /**
     * Create a submenu for the currently-hidden columns.
     **/
    const auto createHiddenColMenu = [ this, createAction, &menu ]()
    {
	QMenu * hiddenColMenu = menu.addMenu( tr( "Hidden &Columns" ) );

	int actionCount = 0;
	for ( int section = 0; section < _header->count(); ++section )
	{
	    if ( _header->isSectionHidden( section ) )
	    {
		const QString text = tr( "Show \"%1\"" ).arg( colName( _treeView, section ) );
		QAction * showAction = createAction( hiddenColMenu, text, &HeaderTweaker::showHiddenCol );
		showAction->setData( section );
		++actionCount;
	    }
	}

	if ( actionCount == 0 )
	{
	    hiddenColMenu->setEnabled( false );
	}
	else if ( actionCount > 1 )
	{
	    hiddenColMenu->addSeparator();
	    createAction( hiddenColMenu,
	                  tr( "Show &All Hidden Columns" ),
	                  &HeaderTweaker::showAllHiddenColumns );
	}
    };

    /**
     * Create menu actions to control the sizing and visibility of the section
     * under the mouse.
     **/
    const auto createSectionActions = [ this, createAction, &menu ]()
    {
	QAction * autoSizeCurrentCol = createAction( &menu,
	                                             tr( "Auto &Size" ),
	                                             &HeaderTweaker::autoSizeCurrentCol );
	autoSizeCurrentCol->setCheckable( true );
	autoSizeCurrentCol->setChecked( autoSizeCol( _currentSection ) );

	const QString currentColName = colName( _treeView, _currentSection );
	//logDebug() << currentColName << Qt::endl;
	QAction * hideCurrentCol = createAction( &menu,
	                                         tr( "&Hide \"%1\"" ).arg( currentColName ),
	                                         &HeaderTweaker::hideCurrentCol );
	hideCurrentCol->setEnabled( _currentSection );

	menu.addSeparator();
    };

    // Populate the menu
    _currentSection = _header->logicalIndexAt( pos );
    if ( _currentSection != -1 )
	createSectionActions();

    createHiddenColMenu();

    QMenu * allColMenu = menu.addMenu( tr( "&All Columns" ) );
    createAction( allColMenu, tr( "Auto &Size" ),         &HeaderTweaker::setAllColumnsAutoSize );
    createAction( allColMenu, tr( "&Interactive Size" ),  &HeaderTweaker::setAllColumnsInteractiveSize );
    createAction( allColMenu, tr( "&Reset to Defaults" ), &HeaderTweaker::resetToDefaults );

    // Show the menu
    menu.exec( _header->mapToGlobal( pos ) );
}


void HeaderTweaker::hideCurrentCol()
{
    if ( DataColumns::isValidCol( _currentSection ) )
    {
	//logDebug() << "Hiding column \"" << colName( _treeView, _currentSection ) << '"' << Qt::endl;
	_header->hideSection( _currentSection );
    }
}


void HeaderTweaker::autoSizeCurrentCol()
{
    if ( DataColumns::isValidCol( _currentSection ) )
	setResizeMode( _currentSection, toggledResizeMode( _header->sectionResizeMode( _currentSection ) ) );
}


void HeaderTweaker::setAllColumnsAutoSize()
{
    setAllColumnsResizeMode( true );
}


void HeaderTweaker::setAllColumnsInteractiveSize()
{
    setAllColumnsResizeMode( false );
}


void HeaderTweaker::showHiddenCol()
{
    QAction * action = qobject_cast<QAction *>( sender() );
    if ( !action )
    {
	logError() << "Wrong sender type: " << sender()->metaObject()->className() << Qt::endl;
	return;
    }

    if ( action->data().isValid() )
    {
	const int section = action->data().toInt();
	const int count = _header->count();
	if ( section >= 0 && section < count )
	{
	    //logDebug() << "Showing column \"" << colName( _treeView, section ) << '"' << Qt::endl;
	    showSection( _header, section );
	}
	else
	{
	    logError() << "Section index out of range: " << section << Qt::endl;
	}
    }
    else
    {
	logError() << "No data() set for this QAction" << Qt::endl;
    }
}


void HeaderTweaker::showAllHiddenColumns()
{
    //logDebug() << "Showing all columns" << Qt::endl;

    for ( int section = 0; section < _header->count(); ++section )
    {
	if ( _header->isSectionHidden( section ) )
	    showSection( _header, section );
    }
}


void HeaderTweaker::resetToDefaults()
{
    if ( _currentLayout )
    {
	_currentLayout->columns = _currentLayout->defaultColumns();
	applyLayout( _header, _currentLayout );
    }
}


void HeaderTweaker::readSettings()
{
    Settings settings;
    settings.beginGroup( "TreeViewColumns" );

    // Set all column widths
    for ( int section = 0; section < _header->count(); ++section )
    {
	if ( DataColumns::isValidCol( section ) )
	{
	    const DataColumn col = DataColumns::fromViewCol( section );

	    const int width = settings.value( colWidthKey( col ), -1 ).toInt();
	    if ( width > 0 )
	    {
		// Fixed width, can be changed by the user
		setResizeMode( section, QHeaderView::Interactive );
		_header->resizeSection( section, width );
	    }
	    else
	    {
		// Auto width
		setResizeMode( section, QHeaderView::ResizeToContents );
	    }
	}
    }

    settings.endGroup();

    for ( ColumnLayout * layout : asConst( _layouts ) )
	readLayoutSettings( layout );
}


void HeaderTweaker::writeSettings()
{
    saveCurrentLayout();

    Settings settings;
    settings.beginGroup( "TreeViewColumns" );

    // Remove any leftovers from old config file versions
    settings.remove( QString{} ); // Remove all keys in this settings group

    // Save column widths
    for ( int visualIndex = 0; visualIndex < _header->count(); ++visualIndex )
    {
	const int logicalIndex = _header->logicalIndex( visualIndex );
	if ( DataColumns::isValidCol( logicalIndex ) )
	{
	    const DataColumn col   = DataColumns::fromViewCol( logicalIndex );
	    const QString widthKey = colWidthKey( col );

	    if ( autoSizeCol( logicalIndex ) )
		settings.setValue( widthKey, "auto" );
	    else
		settings.setValue( widthKey, _header->sectionSize( logicalIndex ) );
	}
    }

    settings.endGroup();

    // Write column layouts to settings
    for ( const ColumnLayout * layout : asConst( _layouts ) )
	writeLayoutSettings( layout );
}


void HeaderTweaker::saveCurrentLayout()
{
    if ( _currentLayout )
	saveLayout( _header, _currentLayout );
}


void HeaderTweaker::changeLayout( const QString & layoutName )
{
    if ( !_layouts.contains( layoutName ) )
    {
	logError() << "No layout " << layoutName << Qt::endl;
	return;
    }

    // logDebug() << "Changing to layout " << name << Qt::endl;

    saveCurrentLayout();
    _currentLayout = _layouts[ layoutName ];
    applyLayout( _header, _currentLayout );
}




DataColumnList ColumnLayout::defaultColumns( const QString & layoutName )
{
    if ( layoutName == HeaderTweaker::l1Name() )
	return { NameCol,
	         PercentBarCol,
	         PercentNumCol,
	         SizeCol,
	         LatestMTimeCol,
	       };

    if ( layoutName == HeaderTweaker::l2Name() )
	return { NameCol,
	         PercentBarCol,
	         PercentNumCol,
	         SizeCol,
	         TotalItemsCol,
	         TotalFilesCol,
	         TotalSubDirsCol,
	         LatestMTimeCol,
	       };

    return DataColumns::allColumns();
}
