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
     * Return a settings key for the width of a column.
     **/
    QString colWidthKey( DataColumn col )
    {
	return "Width_"_L1 % DataColumns::toString( col );
    }


    /**
     * Write the settings for a layout.
     **/
    void writeLayoutSettings( const ColumnLayout * layout )
    {
	CHECK_PTR( layout );

	Settings settings;
	settings.beginGroup( "TreeViewLayout_" + layout->name );
	settings.setValue( "Columns", DataColumns::toStringList( layout->columns ) );
	settings.endGroup();
    }


    /**
     * Ensure consistency of a layout.
     **/
    void fixupLayout( ColumnLayout * layout )
    {
	if ( layout->columns.isEmpty() )
	{
	    logDebug() << "Falling back to default visible columns" << Qt::endl;
	    layout->columns = layout->defaultColumns();
	}

	DataColumns::ensureNameColFirst( layout->columns );
    }


    /**
     * Read the settings for a layout.
     **/
    void readLayoutSettings( ColumnLayout * layout )
    {
	CHECK_PTR( layout );

	Settings settings;

	settings.beginGroup( "TreeViewLayout_" + layout->name );
	layout->columns = DataColumns::fromStringList( settings.value( "Columns" ).toStringList() );
	settings.endGroup();

	fixupLayout( layout );
    }


    /**
     * Add any columns that are missing from the default columns to
     * 'colList'.
     **/
    void addMissingColumns( DataColumnList & colList )
    {
	const auto defaultColumns = DataColumns::defaultColumns();
	for ( const DataColumn col : defaultColumns )
	{
	    if ( !colList.contains( col ) )
		 colList << col;
	}
    }


    /**
     * Order the columns according to 'colOrderList'.
     **/
    void setColumnOrder( QHeaderView * header, DataColumnList columns )
    {
	addMissingColumns( columns );

	int visualIndex = 0;

	for ( DataColumn col : asConst( columns ) )
	{
	    if ( visualIndex < header->count() )
	    {
		// logDebug() << "Moving " << col << " to position " << visualIndex << Qt::endl;
		header->moveSection( header->visualIndex( col ), visualIndex++ );
	    }
	    else
		logWarning() << "More columns than header sections" << Qt::endl;
	}
    }


    /**
     * Show the columns that are in 'columns'.
     **/
    void setColumnVisibility( QHeaderView * header, const DataColumnList & columns )
    {
	for ( int section = 0; section < header->count(); ++section )
	    header->setSectionHidden( section, !columns.contains( DataColumns::fromViewCol( section ) ) );
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

    createColumnLayouts();

    connect( _header, &QHeaderView::sectionCountChanged,
             this,    &HeaderTweaker::readSettings );

    connect( _header, &QHeaderView::customContextMenuRequested,
             this,    &HeaderTweaker::contextMenu );
}


HeaderTweaker::~HeaderTweaker()
{
    qDeleteAll( _layouts );
}


void HeaderTweaker::createColumnLayouts()
{
    /**
     * Create one column layout.
     **/
    const auto createColumnLayout = [ this ]( const QString & layoutName )
    {
	_layouts[ layoutName ] = new ColumnLayout{ layoutName };
    };

    // Layout L1: Short
    createColumnLayout( l1Name() );

    // L2: Classic QDirStat Style
    createColumnLayout( l2Name() );

    // L3: Full
    createColumnLayout( l3Name() );
}


QAction * HeaderTweaker::createAction( QMenu * menu, const QString & title, void( HeaderTweaker::*slot )( void ) )
{
    QAction * action = new QAction{ title, this };
    menu->addAction( action );
    connect( action, &QAction::triggered, this, slot );

    return action;
}


void HeaderTweaker::contextMenu( const QPoint & pos )
{
    _currentSection = _header->logicalIndexAt( pos );
    if ( _currentSection == -1 )
	return;

    const QString colName = this->colName( _currentSection );
    //logDebug() << colName << Qt::endl;

    QMenu menu;
    QAction * autoSizeCurrentCol = createAction( &menu,
                                                 tr( "Auto &Size" ),
                                                 &HeaderTweaker::autoSizeCurrentCol );
    autoSizeCurrentCol->setCheckable( true );
    autoSizeCurrentCol->setChecked( autoSizeCol( _currentSection ) );

    QAction * hideCurrentCol = createAction( &menu,
                                             tr( "&Hide \"%1\"" ).arg( colName ),
                                             &HeaderTweaker::hideCurrentCol );
    hideCurrentCol->setEnabled( _currentSection );

    menu.addSeparator();
    menu.addMenu( createHiddenColMenu( &menu ) );

    QMenu allColMenu{ tr( "&All Columns" ) };
    menu.addMenu( &allColMenu );
    createAction( &allColMenu, tr( "Auto &Size" ),         &HeaderTweaker::setAllColumnsAutoSize );
    createAction( &allColMenu, tr( "&Interactive Size" ),  &HeaderTweaker::setAllColumnsInteractiveSize );
    createAction( &allColMenu, tr( "&Reset to Defaults" ), &HeaderTweaker::resetToDefaults );

    menu.exec( _header->mapToGlobal( pos ) );
}


QMenu * HeaderTweaker::createHiddenColMenu( QWidget * parent )
{
    int actionCount = 0;
    QMenu * hiddenColMenu = new QMenu{ tr( "Hidden &Columns" ), parent };

    for ( int section = 0; section < _header->count(); ++section )
    {
	if ( _header->isSectionHidden( section ) )
	{
	    const QString text = tr( "Show Column \"%1\"" ).arg( this->colName( section ) );
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
	createAction( hiddenColMenu, tr( "Show &All Hidden Columns" ), &HeaderTweaker::showAllHiddenColumns );
    }

    return hiddenColMenu;
}


QString HeaderTweaker::colName( int section ) const
{
    if ( !DataColumns::isValidCol( section ) )
	logError() << "No column at section " << section << Qt::endl;

    const DataColumn col = DataColumns::fromViewCol( section );

    return _treeView->headerData( col, Qt::Horizontal, Qt::DisplayRole ).toString();
}


void HeaderTweaker::hideCurrentCol()
{
    if ( _currentSection >= 0 )
    {
	//logDebug() << "Hiding column \"" << colName( _currentSection ) << '"' << Qt::endl;
	_header->setSectionHidden( _currentSection, true );
    }

    _currentSection = -1;
}


void HeaderTweaker::autoSizeCurrentCol()
{
    if ( _currentSection >= 0 )
	setResizeMode( _currentSection, toggledResizeMode( _header->sectionResizeMode( _currentSection ) ) );
    else
	logWarning() << "No current section" << Qt::endl;

    _currentSection = -1;
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
	if ( section >= 0 && section < _header->count() )
	{
	    //logDebug() << "Showing column \"" << colName( section ) << '"' << Qt::endl;
	    _header->setSectionHidden( section, false );
	}
	else
	    logError() << "Section index out of range: " << section << Qt::endl;
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
	_header->setSectionHidden( section, false );
}


void HeaderTweaker::resetToDefaults()
{
    if ( _currentLayout )
    {
	_currentLayout->columns = _currentLayout->defaultColumns();
	applyLayout( _currentLayout );
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


void HeaderTweaker::changeLayout( const QString & layoutName )
{
    if ( !_layouts.contains( layoutName ) )
    {
	logError() << "No layout " << layoutName << Qt::endl;
	return;
    }

    // logDebug() << "Changing to layout " << name << Qt::endl;

    saveLayout();
    _currentLayout = _layouts[ layoutName ];
    applyLayout( _currentLayout );
}


void HeaderTweaker::saveLayout( ColumnLayout * layout )
{
    CHECK_PTR( layout );

    layout->columns.clear();

    for ( int visualIndex = 0; visualIndex < _header->count(); ++visualIndex )
    {
	const int logicalIndex = _header->logicalIndex( visualIndex );
	if ( DataColumns::isValidCol( logicalIndex ) )
	{
	    if ( !_header->isSectionHidden( logicalIndex ) )
		layout->columns << DataColumns::fromViewCol( logicalIndex );
	}
    }
}


void HeaderTweaker::applyLayout( ColumnLayout * layout )
{
    CHECK_PTR( layout );

    fixupLayout( layout );
    setColumnOrder( _header, layout->columns );
    setColumnVisibility( _header, layout->columns );
}




DataColumnList ColumnLayout::defaultColumns( const QString & layoutName )
{
    if ( layoutName == HeaderTweaker::l1Name() )
	return { NameCol,
	         PercentBarCol,
	         PercentNumCol,
	         SizeCol,
	         LatestMTimeCol
	       };

    if ( layoutName == HeaderTweaker::l2Name() )
	return { NameCol,
	         PercentBarCol,
	         PercentNumCol,
	         SizeCol,
	         TotalItemsCol,
	         TotalFilesCol,
	         TotalSubDirsCol,
	         LatestMTimeCol
	       };

    return DataColumns::allColumns();
}
