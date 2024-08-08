/*
 *   File name: MainWindowLayout.cpp
 *   Summary:   QDirStat main window layout-related functions
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QActionGroup>
#include <QContextMenuEvent>

#include "MainWindow.h"
#include "ActionManager.h"
#include "HeaderTweaker.h"
#include "Logger.h"
#include "QDirStatApp.h"
#include "Settings.h"


using namespace QDirStat;


QString MainWindow::layoutName( const QAction * action ) const
{
    if ( action == _ui->actionLayout1 ) return HeaderTweaker::l1Name();
    if ( action == _ui->actionLayout2 ) return HeaderTweaker::l2Name();
    if ( action == _ui->actionLayout3 ) return HeaderTweaker::l3Name();
    return QString{};
}


QAction * MainWindow::layoutAction( const QString & layoutName ) const
{
    if ( layoutName == HeaderTweaker::l1Name() ) return _ui->actionLayout1;
    if ( layoutName == HeaderTweaker::l2Name() ) return _ui->actionLayout2;
    if ( layoutName == HeaderTweaker::l3Name() ) return _ui->actionLayout3;
    return nullptr;
}


QString MainWindow::currentLayoutName() const
{
    return layoutName( _layoutActionGroup->checkedAction() );
}


void MainWindow::initLayouts( const QString & currentLayoutName )
{
    // Qt Designer does not support QActionGroups; it was there for Qt 3, but
    // they dropped that feature for Qt 4/5.
    _layoutActionGroup = new QActionGroup{ this };

    // Note that the column layouts are handled in the HeaderTweaker and its
    // ColumnLayout helper class; see also HeaderTweaker.h and .cpp.
    initLayout( HeaderTweaker::l1Name(), currentLayoutName );
    initLayout( HeaderTweaker::l2Name(), currentLayoutName );
    initLayout( HeaderTweaker::l3Name(), currentLayoutName );
}


void MainWindow::initLayout( const QString & layoutName, const QString & currentLayoutName )
{
    readLayoutSetting( layoutName );

    QAction * action = layoutAction( layoutName );
    _layoutActionGroup->addAction( action );

    if ( layoutName == currentLayoutName )
    {
	action->setChecked( true );
	changeLayout( layoutName ); // setChecked() doesn't fire triggered() and it isn't connected yet anyway
    }
}


void MainWindow::changeLayoutSlot()
{
    // Get the layout to use from data() from the QAction that sent the signal.
    const QAction * action = qobject_cast<QAction *>( sender() );
    changeLayout( layoutName( action ) );
}


void MainWindow::changeLayout( const QString & name )
{
    //logDebug() << "Changing to layout " << name << Qt::endl;
    _ui->dirTreeView->headerTweaker()->changeLayout( name );

    QAction * action = layoutAction( name );
    if ( action )
    {
	// Just set the actions, toggled signals will actually change the widget visibility
	_ui->actionShowBreadcrumbs->setChecked ( layoutShowBreadcrumbs ( action ) );
	_ui->actionShowDetailsPanel->setChecked( layoutShowDetailsPanel( action ) );
	_ui->actionShowTreemap->setChecked     ( layoutShowTreemap     ( action ) );
	_ui->actionShowDirTree->setChecked     ( layoutShowDirTree     ( action ) );
//	_ui->actionTreemapOnSide->setChecked   ( layoutTreemapOnSide   ( action ) );
    }
    else
	logError() << "No layout " << name << Qt::endl;
}


void MainWindow::updateLayoutBreadcrumbs( bool breadcrumbsVisible )
{
    //logDebug() << breadcrumbsVisible << Qt::endl;
    _ui->breadcrumbNavigator->setVisible( breadcrumbsVisible );
    setData( LayoutShowBreadcrumbs, breadcrumbsVisible );
}


void MainWindow::updateLayoutDetailsPanel( bool detailsPanelVisible )
{
    //logDebug() << detailsPanelVisible << Qt::endl;
    if ( detailsPanelVisible )
    {
	detailsWithTreemap( _ui->actionDetailsWithTreemap->isChecked() );
	updateFileDetailsView();
    }
    else
    {
	_ui->topFileDetailsPanel->hide();
	_ui->bottomFileDetailsPanel->hide();
    }

    setData( LayoutShowDetails, detailsPanelVisible );
}


void MainWindow::updateLayoutDirTree( bool dirTreeVisible )
{
    //logDebug() << dirTreeVisible << Qt::endl;
    _ui->treeViewContainer->setVisible( dirTreeVisible );
    setData( LayoutShowDirTree, dirTreeVisible );
}


void MainWindow::updateLayoutTreemap( bool treemapVisible )
{
    //logDebug() << treemapVisible << Qt::endl;
    showTreemapView( treemapVisible );
    setData( LayoutShowTreemap, treemapVisible );
}

/*
void MainWindow::updateLayoutTreemapOnSide( bool treemapOnSide )
{
    //logDebug() << treemapVisible << Qt::endl;
    treemapAsSidePanel( treemapOnSide );
    setData( LayoutTreemapOnSide, treemapOnSide );
}
*/

void MainWindow::setData( LayoutSettings setting, bool value )
{
    QAction * action = _layoutActionGroup->checkedAction();
    auto layoutDetails = action->data().toList();
    layoutDetails.replace( setting, value );
    action->setData( layoutDetails );
}


void MainWindow::readLayoutSetting( const QString & layoutName )
{
    Settings settings;

    settings.beginGroup( "TreeViewLayout_" + layoutName );
    const bool showBreadcrumbs    = settings.value( "ShowCurrentPath",    true ).toBool();
    const bool showDetailsPanel   = settings.value( "ShowDetailsPanel",   true ).toBool();
    const bool showDirTree        = settings.value( "ShowDirTree",        true ).toBool();
    const bool showTreemap        = settings.value( "ShowTreemap",        true ).toBool();
//    const bool treemapOnSide      = settings.value( "TreemapOnSide",      false ).toBool();
//    const bool detailsWithTreemap = settings.value( "DetailsWithTreemap", false ).toBool();
    settings.endGroup();

    const QList<QVariant> data{ showBreadcrumbs, showDetailsPanel, showDirTree, showTreemap };
    layoutAction( layoutName )->setData( data );
}


void MainWindow::writeLayoutSetting( const QAction * action )
{
    Settings settings;

    settings.beginGroup( "TreeViewLayout_" + layoutName( action ) );
    settings.setValue( "ShowCurrentPath",  layoutShowBreadcrumbs ( action ) );
    settings.setValue( "ShowDetailsPanel", layoutShowDetailsPanel( action ) );
    settings.setValue( "ShowDirTree",      layoutShowDirTree     ( action ) );
    settings.setValue( "ShowTreemap",      layoutShowTreemap     ( action ) );
//    settings.setValue( "TreemapOnSide",    layoutTreemapOnSide   ( action ) );
//    settings.setValue( "DetailsWithTreemap", layoutdetailsWithTreemap( action ) );
    settings.endGroup();
}


void MainWindow::writeLayoutSettings()
{
    writeLayoutSetting( _ui->actionLayout1 );
    writeLayoutSetting( _ui->actionLayout2 );
    writeLayoutSetting( _ui->actionLayout3 );
}


void MainWindow::showBars()
{
    menuBar()->setVisible( _ui->actionShowMenuBar->isChecked() );
    statusBar()->setVisible( _ui->actionShowStatusBar->isChecked() );
}


void MainWindow::contextMenuEvent( QContextMenuEvent * event )
{
    if ( _ui->centralWidget->underMouse() )
    {
        const QStringList actions{ "actionLayout1",
                                   "actionLayout2",
                                   "actionLayout3",
                                   ActionManager::separator(),
                                   "actionShowBreadcrumbs",
                                   "actionShowDetailsPanel",
                                   "actionShowDirTree",
                                   "actionShowTreemap",
//                                   ActionManager::separator(),
//                                   "actionTreemapOnSide",
//                                   "actionDetailsWithTreemap",
                                  };
        QMenu * menu = ActionManager::createMenu( actions, {} );
        menu->exec( event->globalPos() );

        return;
    }

    QMenu * menu = createPopupMenu();
    QAction * toolbarAction = menu->actions().first();
    toolbarAction->setText( tr( "Show &Toolbar" ) );

    menu->insertAction( toolbarAction, _ui->actionShowMenuBar );
    menu->addAction( _ui->actionShowStatusBar );

    menu->exec( event->globalPos() );

    showBars();
}
