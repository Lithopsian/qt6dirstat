/*
 *   File name: HeaderTweaker.cpp
 *   Summary:   Helper class for DirTreeView
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QMenu>
#include <QAction>

#include "HeaderTweaker.h"
#include "DirTreeView.h"
#include "Settings.h"
#include "Logger.h"
#include "Exception.h"


using namespace QDirStat;


HeaderTweaker::HeaderTweaker( QHeaderView * header, DirTreeView * parent ):
    QObject ( parent ),
    _treeView { parent },
    _header { header }
{
    CHECK_PTR( _header );

    _header->setContextMenuPolicy( Qt::CustomContextMenu );
    _header->setDefaultAlignment( Qt::AlignVCenter | Qt::AlignHCenter );

    createColumnLayouts();

    connect( _header, &QHeaderView::sectionCountChanged,
	     this,    &HeaderTweaker::initHeader );

    connect( _header, &QHeaderView::customContextMenuRequested,
	     this,    &HeaderTweaker::contextMenu );
}


HeaderTweaker::~HeaderTweaker()
{
    if ( _currentLayout )
	saveLayout( _currentLayout );

    writeSettings();

    qDeleteAll( _layouts );
}


void HeaderTweaker::initHeader()
{
    // Initialize stuff when the header actually has sections: It's constructed
    // empty. It is only populated when the tree view model requests header
    // data from the data model.

    // logDebug() << "Header count: " << _header->count() << Qt::endl;
    readSettings();
}


void HeaderTweaker::createColumnLayout( const QString & layoutName)
{
    ColumnLayout * layout = new ColumnLayout( layoutName );
    CHECK_PTR( layout );
    _layouts[ layoutName ] = layout;
}


void HeaderTweaker::createColumnLayouts()
{
    // Layout L1: Short
    createColumnLayout( "L1" );

    // L2: Classic QDirStat Style
    createColumnLayout( "L2" );

    // L3: Full
    createColumnLayout( "L3" );
}


QAction * HeaderTweaker::createAction( QMenu * menu, const QString & title, void( HeaderTweaker::*slot )( void ) )
{
    QAction * action = new QAction( title, this );
    CHECK_NEW( action );
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
//    menu.addAction( tr( "Column \"%1\"" ).arg( colName ) );
//    menu.addSeparator();
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

    QMenu allColMenu( tr( "&All Columns" ) );
    menu.addMenu( &allColMenu );
    createAction( &allColMenu, tr( "Auto &Size" ),         &HeaderTweaker::setAllColumnsAutoSize );
    createAction( &allColMenu, tr( "&Interactive Size" ),  &HeaderTweaker::setAllColumnsInteractiveSize );
    createAction( &allColMenu, tr( "&Reset to Defaults" ), &HeaderTweaker::resetToDefaults );

    menu.exec( _header->mapToGlobal( pos ) );
}


QMenu * HeaderTweaker::createHiddenColMenu( QWidget * parent )
{
    int actionCount = 0;
    QMenu * hiddenColMenu = new QMenu( tr( "Hidden &Columns" ), parent );

    for ( int section = 0; section < _header->count(); ++section )
    {
	if ( _header->isSectionHidden( section ) )
	{
	    const QString text = tr( "Show Column \"%1\"" ).arg( this->colName( section ) );
	    QAction * showAction = createAction( hiddenColMenu, text, &HeaderTweaker::showHiddenCol );
	    showAction->setData( section );
//	    hiddenColMenu->addAction( showAction );
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

    return _treeView->model()->headerData( col, Qt::Horizontal, Qt::DisplayRole ).toString();
}


void HeaderTweaker::hideCurrentCol()
{
    if ( _currentSection >= 0 )
    {
	//logDebug() << "Hiding column \"" << colName( _currentSection ) << "\"" << Qt::endl;
	_header->setSectionHidden( _currentSection, true );
    }

    _currentSection = -1;
}


void HeaderTweaker::autoSizeCurrentCol()
{
    if ( _currentSection >= 0 )
	setResizeMode( _currentSection, toggleResizeMode( _header->sectionResizeMode( _currentSection ) ) );
    else
	logWarning() << "No current section" << Qt::endl;

    _currentSection = -1;
}


void HeaderTweaker::setAllColumnsResizeMode( bool autoSize )
{
    const QHeaderView::ResizeMode newResizeMode = resizeMode( autoSize );

    for ( int section = 0; section < _header->count(); ++section )
	setResizeMode( section, newResizeMode );
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
	    //logDebug() << "Showing column \"" << colName( section ) << "\"" << Qt::endl;
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


void HeaderTweaker::setColumnOrder( const DataColumnList & columns )
{
    DataColumnList colOrderList = columns;
    addMissingColumns( colOrderList );

    int visualIndex = 0;

    for ( DataColumn col : colOrderList )
    {
	if ( visualIndex < _header->count() )
	{
	    // logDebug() << "Moving " << col << " to position " << visualIndex << Qt::endl;
	    _header->moveSection( _header->visualIndex( col ), visualIndex++ );
	}
	else
	    logWarning() << "More columns than header sections" << Qt::endl;
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

	    const int width = settings.value( "Width_" + DataColumns::toString( col ), -1 ).toInt();
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

    for ( ColumnLayout * layout : _layouts )
	readLayoutSettings( layout );
}


void HeaderTweaker::readLayoutSettings( ColumnLayout * layout )
{
    CHECK_PTR( layout );

    Settings settings;
    settings.beginGroup( "TreeViewLayout_" + layout->name );

    layout->columns = DataColumns::fromStringList( settings.value( "Columns" ).toStringList() );

    fixupLayout( layout );

    settings.endGroup();
}


void HeaderTweaker::writeSettings()
{
    Settings settings;
    settings.beginGroup( "TreeViewColumns" );

    // Remove any leftovers from old config file versions
    settings.remove( "" ); // Remove all keys in this settings group

    // Save column widths
    for ( int visualIndex = 0; visualIndex < _header->count(); ++visualIndex )
    {
	const int logicalIndex = _header->logicalIndex( visualIndex );
	if ( DataColumns::isValidCol( logicalIndex ) )
	{
	    const DataColumn col   = DataColumns::fromViewCol( logicalIndex );
	    const QString widthKey = "Width_" + DataColumns::toString( col );

	    if ( autoSizeCol( logicalIndex ) )
		settings.setValue( widthKey, "auto" );
	    else
		settings.setValue( widthKey, _header->sectionSize( logicalIndex ) );
	}
    }

    settings.endGroup();

    // Write column layouts to settings
    for ( const ColumnLayout * layout : _layouts )
	writeLayoutSettings( layout );
}


void HeaderTweaker::writeLayoutSettings( const ColumnLayout * layout ) const
{
    CHECK_PTR( layout );

    Settings settings;
    settings.beginGroup( "TreeViewLayout_" + layout->name );
    settings.setValue( "Columns", DataColumns::toStringList( layout->columns ) );
    settings.endGroup();
}


void HeaderTweaker::setColumnVisibility( const DataColumnList & columns )
{
    for ( int section = 0; section < _header->count(); ++section )
	_header->setSectionHidden( section, !columns.contains( DataColumns::fromViewCol( section ) ) );
}


void HeaderTweaker::addMissingColumns( DataColumnList & colList )
{
    for ( const DataColumn col : DataColumns::defaultColumns() )
    {
	if ( !colList.contains( col ) )
	     colList << col;
    }
}


void HeaderTweaker::changeLayout( const QString & name )
{
    if ( !_layouts.contains( name ) )
    {
	logError() << "No layout " << name << Qt::endl;
	return;
    }

    // logDebug() << "Changing to layout " << name << Qt::endl;

    if ( _currentLayout )
	saveLayout( _currentLayout );

    _currentLayout = _layouts[ name ];
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
	    const DataColumn col   = DataColumns::fromViewCol( logicalIndex );

	    if ( !_header->isSectionHidden( logicalIndex ) )
		layout->columns << col;
	}
    }
}


void HeaderTweaker::applyLayout( ColumnLayout * layout )
{
    CHECK_PTR( layout );

    fixupLayout( layout );
    setColumnOrder( layout->columns );
    setColumnVisibility( layout->columns );
}


void HeaderTweaker::fixupLayout( ColumnLayout * layout )
{
    CHECK_PTR( layout );

    if ( layout->columns.isEmpty() )
    {
	logDebug() << "Falling back to default visible columns" << Qt::endl;
	layout->columns = layout->defaultColumns();
    }

    DataColumns::ensureNameColFirst( layout->columns );
}


void HeaderTweaker::resizeToContents( QHeaderView * header )
{
    for ( int col = 0; col < header->count(); ++col )
	header->setSectionResizeMode( col, QHeaderView::ResizeToContents );
}



DataColumnList ColumnLayout::defaultColumns( const QString & layoutName )
{
    if ( layoutName == QLatin1String( "L1" ) )
	return { NameCol,
                 PercentBarCol,
                 PercentNumCol,
                 SizeCol,
                 LatestMTimeCol
	       };

    if ( layoutName == QLatin1String( "L2" ) )
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
