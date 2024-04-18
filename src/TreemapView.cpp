/*
 *   File name: TreemapView.cpp
 *   Summary:   View widget for treemap rendering for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QResizeEvent>

#include "TreemapView.h"
#include "TreemapTile.h"
#include "DirInfo.h"
#include "DirTree.h"
#include "MimeCategorizer.h"
#include "SelectionModel.h"
#include "Settings.h"
#include "SettingsHelpers.h"
#include "SignalBlocker.h"
#include "Exception.h"
#include "Logger.h"


using namespace QDirStat;


TreemapView::TreemapView( QWidget * parent ):
    QGraphicsView ( parent )
{
    // logDebug() << Qt::endl;

    readSettings();

    // We only ever need one thread at a time, and having more just chews up memory
    QThreadPool::globalInstance()->setMaxThreadCount( 1 );

    connect( &_watcher,  &QFutureWatcher<TreemapTile *>::finished,
             this,       &TreemapView::treemapFinished );
}


TreemapView::~TreemapView()
{
    // Write settings back to file, but only if we are the real treemapView
    if ( _selectionModel )
        writeSettings();
}


void TreemapView::cancelTreemap()
{
    _treemapCancel = TreemapCancelCancel;
    _watcher.waitForFinished();
}


void TreemapView::clear()
{
    cancelTreemap();

    if ( _rootTile )
    {
        if ( scene() )
        {
            // Take out the tiles so we can delete them in the background
            scene()->removeItem( _rootTile );

            // Clear everything else, any highlighters and mask
            scene()->clear();
        }

        // Deleting these can take a while, so delegate to a thread
        TreemapTile * rootTile = _rootTile;
        QtConcurrent::run( [ rootTile ]() { delete rootTile; } );
        _rootTile = nullptr;
    }

    _currentTileHighlighter = nullptr;
    _sceneMask              = nullptr;

    _parentHighlightList.clear();
}


void TreemapView::setDirTree( const DirTree * newTree )
{
    if ( !newTree )
        return;

    _tree = newTree;

    // This signal indicates that a subtree is going to be removed.  This occurs
    // for cleanups with refresh policy AssumeDeleted and when a cache file is
    // automatically read during a tree read.  It is always followed by
    // childDeleted, but the tree may still be being read at that point.  The
    // assumedDeleted signal from the cleanup (connected in MainWindow) indicates
    // that it has finished.  An ongoing tree read will send a normal finished() signal
    // when it completes.
    connect( _tree, &DirTree::deletingChild,
             this,  &TreemapView::deleteNotify );

    // Clear the treemap before the DirTree disappears
    // Also disable, although nobody should trigger us to rebuild until it is safe.
    connect( _tree, &DirTree::clearing,
             this,  &TreemapView::disable );
    connect( _tree, &DirTree::clearingSubtree,
             this,  &TreemapView::disable );
}


void TreemapView::setSelectionModel( SelectionModel * selectionModel )
{
    if ( !selectionModel )
        return;

    _selectionModel = selectionModel;

    connect( this,            &TreemapView::currentItemChanged,
             _selectionModel, &SelectionModel::updateCurrentBranch );

    // Use the proxy for all selection model receiving signals
    delete _selectionModelProxy;
    _selectionModelProxy = new SelectionModelProxy( selectionModel, this );
    CHECK_NEW( _selectionModelProxy );

    connect( _selectionModelProxy, &SelectionModelProxy::currentItemChanged,
             this,                 &TreemapView::updateCurrentItem );

    connect( _selectionModelProxy, qOverload<const FileInfoSet &>( &SelectionModelProxy::selectionChanged ),
             this,                 &TreemapView::updateSelection);

    // Connect this one here because it is only relevant in the real treemap
    connect( MimeCategorizer::instance(), &MimeCategorizer::categoriesChanged,
             this,                        &TreemapView::changeTreemapColors );
}


void TreemapView::readSettings()
{
    Settings settings;
    settings.beginGroup( "Treemaps" );

    _colourPreviews     = settings.value( "ColourPreviews"   , true ).toBool();

    _squarify           = settings.value( "Squarify"         , true  ).toBool();
    _doCushionShading   = settings.value( "CushionShading"   , true  ).toBool();
//    _enforceContrast    = settings.value( "EnforceContrast"  , false ).toBool();
    _forceCushionGrid   = settings.value( "ForceCushionGrid" , false ).toBool();
    _useDirGradient     = settings.value( "UseDirGradient"   , true  ).toBool();

    _ambientLight       = settings.value( "AmbientLight"     , DefaultAmbientLight ).toInt();
    _heightScaleFactor  = settings.value( "HeightScaleFactor", DefaultHeightScaleFactor ).toDouble();
    _cushionHeight      = settings.value( "CushionHeight"    , DefaultCushionHeight ).toDouble();
    _minTileSize        = settings.value( "MinTileSize"      , DefaultMinTileSize ).toInt();

    _tileFixedColor     = readColorEntry( settings, "TileFixedColor"    , QColor()                   );
    _currentItemColor   = readColorEntry( settings, "CurrentItemColor"  , Qt::red                    );
    _selectedItemsColor = readColorEntry( settings, "SelectedItemsColor", Qt::yellow                 );
    _highlightColor     = readColorEntry( settings, "HighlightColor"    , Qt::white                  );
    _cushionGridColor   = readColorEntry( settings, "CushionGridColor"  , QColor( 0x80, 0x80, 0x80 ) );
    _outlineColor       = readColorEntry( settings, "OutlineColor"      , QColor()                   );
    _dirFillColor       = readColorEntry( settings, "DirFillColor"      , QColor( 0x60, 0x60, 0x60 ) );
    _dirGradientStart   = readColorEntry( settings, "DirGradientStart"  , QColor( 0x60, 0x60, 0x70 ) );
    _dirGradientEnd     = readColorEntry( settings, "DirGradientEnd"    , QColor( 0x70, 0x70, 0x80 ) );

    settings.endGroup();

    calculateSettings();
}


void TreemapView::writeSettings()
{
    // logDebug() << Qt::endl;

    Settings settings;
    settings.beginGroup( "Treemaps" );

    settings.setValue( "ColourPreviews"   , _colourPreviews    );

    settings.setValue( "Squarify"         , _squarify          );
    settings.setValue( "CushionShading"   , _doCushionShading  );
//    settings.setValue( "EnforceContrast"  , _enforceContrast   );
    settings.setValue( "ForceCushionGrid" , _forceCushionGrid  );
    settings.setValue( "UseDirGradient"   , _useDirGradient    );

    settings.setValue( "AmbientLight"     , _ambientLight      );
    settings.setValue( "HeightScaleFactor", _heightScaleFactor );
    settings.setValue( "CushionHeight"    , _cushionHeight     );
    settings.setValue( "MinTileSize"      , _minTileSize       );

    writeOptionalColorEntry( settings, "TileFixedColor", _tileFixedColor );
    writeColorEntry( settings, "CurrentItemColor"      , _currentItemColor   );
    writeColorEntry( settings, "SelectedItemsColor"    , _selectedItemsColor );
    writeColorEntry( settings, "HighlightColor"        , _highlightColor     );
    writeColorEntry( settings, "CushionGridColor"      , _cushionGridColor   );
    writeOptionalColorEntry( settings, "OutlineColor"  , _outlineColor       );
    writeColorEntry( settings, "DirFillColor"          , _dirFillColor       );
    writeColorEntry( settings, "DirGradientStart"      , _dirGradientStart   );
    writeColorEntry( settings, "DirGradientEnd"        , _dirGradientEnd     );

    settings.endGroup();
}


void TreemapView::writeOptionalColorEntry( Settings & settings, const char * setting, const QColor & color )
{
    if ( color.isValid() )
        writeColorEntry( settings, setting, color );
    else
        settings.setValue( setting, "" );
}


void TreemapView::zoomTo()
{
    // this does all the sanity checks so we know we are good
    if ( !canZoomIn() )
        return;

    // Work from the FileInfo tree because there might not be a tile for the current item
    FileInfo *newNode = _selectionModel->currentItem();
    if ( !newNode->isDirInfo() )
        newNode = newNode->parent();

    rebuildTreemap( newNode );
}


void TreemapView::zoomIn()
{
    // this does all the sanity checks so we know we are good
    if ( !canZoomIn() )
        return;

    // Work up the FileInfo tree because there might not be a tile for the current item
    FileInfo *newNode = _selectionModel->currentItem();
    while ( newNode && newNode->parent() != _rootTile->orig() )
        newNode = newNode->parent();

    rebuildTreemap( newNode );
}


void TreemapView::zoomOut()
{
    // this does all the sanity checks so we know we are good
    if ( !canZoomOut() )
        return;

    FileInfo * newRoot = _rootTile->orig();

    if ( newRoot->parent() && newRoot->parent() != _tree->root() )
        newRoot = newRoot->parent();

    rebuildTreemap( newRoot );
}


void TreemapView::resetZoom()
{
    if ( _tree && _tree->firstToplevel() )
        rebuildTreemap( _tree->firstToplevel() );
}


bool TreemapView::canZoomIn() const
{
    if ( !_rootTile || !_selectionModel )
        return false;

    // Work up the FileInfo tree because there might not be a tile for the current item
    const FileInfo *currentNode = _selectionModel->currentItem();
    if (!currentNode)
        return false;

    const FileInfo *rootNode = _rootTile->orig();
    if (currentNode == rootNode)
        return false;

    while (currentNode && currentNode->parent() != rootNode)
        currentNode = currentNode->parent();

    if (currentNode && currentNode->isDirInfo())
        return true;

    return false;
}


bool TreemapView::canZoomOut() const
{
    if ( !_rootTile || !_tree->firstToplevel() )
        return false;

    return _rootTile->orig() != _tree->firstToplevel();
}


void TreemapView::rebuildTreemapSlot()
{
    logDebug() << _savedRootUrl << Qt::endl;
    FileInfo * root = nullptr;

    if ( !_savedRootUrl.isEmpty() )
    {
        logDebug() << "Restoring old treemap with root " << _savedRootUrl << Qt::endl;

        root = _tree->locate( _savedRootUrl, true );        // node, findPseudoDirs
    }

    if ( !root )
        root = _rootTile ? _rootTile->orig() : _tree->firstToplevel();

    clear();

    rebuildTreemap( root );

    _savedRootUrl = "";
    //logDebug() << _savedRootUrl << Qt::endl;
}


void TreemapView::rebuildTreemap( FileInfo * newRoot )
{
    logDebug() << newRoot << ( isVisible() ? " - visible" : " - not visible" ) << Qt::endl;
    if ( _disabled || !newRoot || !isVisible() )
        return;

    const QRectF rect = viewport()->rect();
    if ( rect.isEmpty() )
        return;

    if ( _treemapRunning )
    {
        _newRoot = newRoot;
        _treemapCancel = TreemapCancelRestart;
        return;
    }

    logDebug() << rect << Qt::endl;

    _treemapCancel = TreemapCancelNone;
    _treemapRunning = true;

    _stopwatch.start();

    _watcher.setFuture( QtConcurrent::run( [ this, newRoot, rect ]() {
        _threadPool = new QThreadPool();
        CHECK_NEW( _threadPool );
        _threadPool->setMaxThreadCount( _threadPool->maxThreadCount() * 2 );

        TreemapTile *tile = new TreemapTile( this,           // parentView
                                             newRoot,        // orig
                                             rect );

        delete _threadPool;

        if ( treemapCancelled() )
        {
            // Logging is not thread-safe, use only for debugging
//            logDebug() << "treemap cancelled, delete tiles" << Qt::endl;
            delete tile;
            tile = nullptr;
        }

        return tile;
    } ) );
}


void TreemapView::treemapFinished()
{
    TreemapTile *futureResult = _watcher.result();

    logDebug() << _stopwatch.restart() << "ms " << Qt::endl;

    _treemapRunning = false;

    if ( treemapCancelled() )
    {
        if ( futureResult )
        {
            logDebug() << "completed treemap has been cancelled, delete tiles" << Qt::endl;
            delete futureResult;
        }

        if ( _treemapCancel == TreemapCancelRestart )
            rebuildTreemap( _newRoot );

        return;
    }

    if ( !futureResult )
    {
        logWarning() << "unexpected null result from treemap build" << Qt::endl;
        return;
    }

    clear();

    if ( !scene() )
    {
        QGraphicsScene * newScene = new QGraphicsScene( this );
        CHECK_NEW( newScene);
        setScene( newScene );
    }
    resetTransform();

    // Switch to the new scene
    _rootTile = futureResult;
    scene()->setSceneRect( _rootTile->rect() );
    scene()->addItem( _rootTile );

    if ( _selectionModel )
        updateSelection( _selectionModel->selectedItems() );

    emit treemapChanged();

    //logDebug() << _stopwatch.restart() << "ms" << Qt::endl;
//    _lastTile->setLastTile();
}


void TreemapView::configChanged( const QColor & fixedColor,
                                 bool squarified,
                                 bool cushionShading,
                                 double cushionHeight,
                                 double heightScaleFactor,
                                 int minTileSize )
{
    //logDebug() << fixedColor.name() << Qt::endl;
    const bool treemapChanged = squarified        != _squarify ||
                                cushionHeight     != _cushionHeight ||
                                heightScaleFactor != _heightScaleFactor ||
                                minTileSize       != _minTileSize;
    const bool colorsChanged = cushionShading != _doCushionShading || fixedColor != _tileFixedColor;
    if ( !treemapChanged && !colorsChanged )
        return;

    // We're about to change data used by the treemap build thread
    cancelTreemap();

    _tileFixedColor    = fixedColor;
    _squarify          = squarified;
    _doCushionShading  = cushionShading;
    _cushionHeight     = cushionHeight;
    _heightScaleFactor = heightScaleFactor;
    _minTileSize       = minTileSize;

    calculateSettings();

    if ( treemapChanged )
        rebuildTreemapSlot();
    else
        changeTreemapColors();
}


void TreemapView::calculateSettings()
{
    // Pre-calculate cushion heights from the configured starting height and scale factor.
    _cushionHeights = TreemapTile::calculateCushionHeights( _cushionHeight, _heightScaleFactor );

    // Calculate thresholds for tile sizes that will be submitted to a render thread
    _maxTileThreshold = ( _squarify ? 150 : 75 ) + 10 * QThread::idealThreadCount();

    // Calculate the minimum height for generating a row of squarified tiles
    _minSquarifiedTileHeight = _minTileSize == 0 ? 0 : _minTileSize - 0.5;

    // Directory gradient can't currently change after startup, but calculate it here anyway
    if ( _useDirGradient )
    {
        _dirGradient = QLinearGradient();
        _dirGradient.setCoordinateMode( QGradient::ObjectMode );
        _dirGradient.setColorAt( 0.0, _dirGradientStart );
        _dirGradient.setColorAt( 1.0, _dirGradientEnd   );
    }
}


void TreemapView::changeTreemapColors()
{
    //logDebug() << "change treemap colours to " << Qt::endl;

    if ( !_rootTile )
        return;

    _rootTile->invalidateCushions();
    _rootTile->update( _rootTile->rect() );
}

void TreemapView::deleteNotify( FileInfo * )
{
    if ( _rootTile )
    {
        if ( _rootTile->orig() != _tree->firstToplevel() )
        {
            // If the user zoomed the treemap in, save the root's URL so the
            // current state can be restored upon the next rebuildTreemapSlot()
            // call (which is triggered by the childDeleted() signal that the
            // tree emits after deleting is done).
            //
            // Intentionally using debugUrl() here rather than just url() so
            // the correct zoom can be restored even when a dot entry is the
            // current treemap root.

            _savedRootUrl = _rootTile->orig()->debugUrl();
        }
        else
        {
            // A shortcut for the most common case: No zoom. Simply use the
            // tree's root for the next treemap rebuild.

            _savedRootUrl = "";
        }
    }
    else
    {
        // Intentionally leaving _savedRootUrl alone: Otherwise multiple
        // deleteNotify() calls might cause a previously saved _savedRootUrl to
        // be unnecessarily deleted, thus the treemap couldn't be restored as
        // it was.
    }

    // Not safe to try building a treemap at this point as the tree is being modified
    disable();
}


void TreemapView::resizeEvent( QResizeEvent * event )
{
    // logDebug() << Qt::endl;
    QGraphicsView::resizeEvent( event );

    if ( !_tree )
        return;

     if ( !_rootTile )
    {
        if ( _tree && _tree->firstToplevel() )
        {
            //logDebug() << "Redisplaying suppressed treemap" << Qt::endl;
            rebuildTreemap( _tree->firstToplevel() );
        }
    }
    else
    {
        //logDebug() << "Auto-resizing treemap" << Qt::endl;
        rebuildTreemap( _rootTile->orig() );

        const QSize *newSize = &event->size();
        const QSize *oldSize = &event->oldSize();
        if ( !newSize->isEmpty() && !oldSize->isEmpty() )
            scale( ( double )newSize->width() / oldSize->width(), ( double )newSize->height() / oldSize->height() );
    }
}


void TreemapView::hideTreemap()
{
    logDebug() << "Hiding treemap view" << Qt::endl;

    clear();
    hide();

    emit treemapChanged();
}


void TreemapView::showTreemap()
{
    logDebug() << "Showing treemap view " << Qt::endl;

    if ( !isVisible() )
        show();

    enable();
}


void TreemapView::disable()
{
//    logDebug() << "Disabling treemap view" << Qt::endl;
    _disabled = true;
    clear();

    emit treemapChanged();
}


void TreemapView::enable()
{
//    logDebug() << "Enabling treemap view" << Qt::endl;
    _disabled = false;

    // Use the slow function to pick up any saved root on a refresh
    rebuildTreemapSlot(); // will emit treemapChanged() when complete
}


void TreemapView::setCurrentTile( const TreemapTile * tile )
{
    //logDebug() << tile << " " << _stopwatch.restart() << "ms" << Qt::endl;

    if ( tile )
    {
        //logDebug() << highlightedParent() << " " << tile->parentTile() << Qt::endl;

        if ( highlightedParent() != tile->parentTile() )
            clearParentsHighlight();

        if ( !_currentTileHighlighter )
            _currentTileHighlighter = new CurrentTileHighlighter( this );
    }

    if ( _currentTileHighlighter )
    {
        //logDebug() " Highting the current tile" << Qt::endl;

        if ( tile == _rootTile )
            _currentTileHighlighter->hide(); // Don't highlight the root tile
        else
            _currentTileHighlighter->highlight( tile );
    }

    if ( tile && _selectionModel->currentItem() != tile->orig() && _selectionModelProxy )
    {
        //logDebug() << "Sending currentItemChanged " << tile << Qt::endl;

        SignalBlocker sigBlocker( _selectionModelProxy ); // Prevent signal ping-pong
        emit currentItemChanged( tile->orig() );
    }
}


void TreemapView::setCurrentItem( FileInfo * node )
{
    //logDebug() << node << " " << _stopwatch.restart() << "ms" << Qt::endl;

    if ( !_rootTile || !node )
        return;

    FileInfo * treemapRoot = _rootTile->orig();

    // Check if the new current item is inside the current treemap
    // (it might be zoomed).

    while ( !node->isInSubtree( treemapRoot ) &&
            treemapRoot->parent() &&
            treemapRoot->parent() != _tree->root() )
        treemapRoot = treemapRoot->parent(); // try one level higher

    if ( treemapRoot != _rootTile->orig() )          // need to zoom out?
    {
        //logDebug() << "Zooming out to " << treemapRoot << " to make current item visible" << Qt::endl;
        rebuildTreemap( treemapRoot );
    }

    setCurrentTile( findTile( _rootTile, node ) );
}


void TreemapView::updateSelection( const FileInfoSet & newSelection )
{
    if ( !scene() || !_rootTile || ( newSelection.size() == 0 && !_selectionModel->currentItem() ) )
        return;

    //logDebug() << newSelection.size() << " items selected (after " << _stopwatch.restart() << "ms) " << Qt::endl;

    SignalBlocker sigBlocker( this );
    scene()->clearSelection();

    QHash<const FileInfo *, TreemapTile *> map;
    if ( newSelection.size() > 10 )
    {
        // Build a mapping of all fileInfo objects to tiles for scaling to very large selections
        const auto items = scene()->items();
        for ( QGraphicsItem * graphicsItem : items )
        {
            TreemapTile * tile = dynamic_cast<TreemapTile *>( graphicsItem );
            if ( tile )
                map.insert( tile->orig(), tile );
        }
    }

    //logDebug() << " map built in " << _stopwatch.restart() << "ms" << (map.isEmpty() ? " (empty) " : "") << Qt::endl;

    for ( auto it = newSelection.constBegin(); it != newSelection.constEnd(); ++it )
    {
        TreemapTile *tile = map.isEmpty() ? findTile( _rootTile, *it ) : map.value( *it, nullptr );
        if ( tile )
            tile->setSelected( true );
    }

    const FileInfo *currentItem = _selectionModel->currentItem();
    const TreemapTile *tile = map.isEmpty() ? findTile( _rootTile, currentItem ) : map.value( currentItem, nullptr );
    if ( tile )
        setCurrentTile( tile );

    //logDebug() << newSelection.size() << " items selected " << _stopwatch.restart() << "ms" << Qt::endl;
}


void TreemapView::sendSelection( const TreemapTile *tile)
{
    if ( !scene() || !_selectionModel )
        return;

    SignalBlocker sigBlocker( _selectionModelProxy );
    const QList<QGraphicsItem *> selectedTiles = scene()->selectedItems();

    if ( selectedTiles.size() == 1 && selectedTiles.first() == tile )
    {
        // For just one selected tile, only send one signal
        _selectionModel->setCurrentItem( tile->orig(),
                                         true ); // select
    }
    else // Multi-selection
    {
        FileInfoSet selectedItems;

        for ( const QGraphicsItem * selectedItem : selectedTiles )
        {
            const TreemapTile * selectedTile = dynamic_cast<const TreemapTile *>( selectedItem );
            if ( selectedTile )
                selectedItems << selectedTile->orig();
        }

        _selectionModel->setSelectedItems( selectedItems );
        _selectionModel->setCurrentItem( tile ? tile->orig() : nullptr );
    }
}


void TreemapView::updateCurrentItem( FileInfo * currentItem )
{
    //logDebug() << currentItem << " " << _stopwatch.restart() << "ms" << Qt::endl;

    if ( !scene() )
        return;

    SignalBlocker sigBlocker( this );
    setCurrentItem( currentItem );
}


TreemapTile * TreemapView::findTile( TreemapTile *rootTile, const FileInfo *node ) const
{
    if ( !node || !rootTile )
        return nullptr;

    // common case that is easy
    if ( rootTile->orig() == node )
        return rootTile;

    // loop recursively through the children of each tile
    for ( QGraphicsItem *graphicsItem : rootTile->childItems() )
    {
        TreemapTile *tile = dynamic_cast<TreemapTile *>(graphicsItem);
        if ( tile )
        {
            tile = findTile( tile, node );
            if ( tile )
                return tile;
        }
    }

    return nullptr;
}


void TreemapView::setFixedColor( const QColor & color )
{
    //logDebug() << color.name() << Qt::endl;

    // We're about to change data used in the treemap build thread
    cancelTreemap();

    _tileFixedColor = color;
    changeTreemapColors();
}


void TreemapView::highlightParents( const TreemapTile * tile )
{
    if ( !tile )
    {
        clearParentsHighlight();
        return;
    }

    const TreemapTile * parent = tile->parentTile();
    const TreemapTile * currentHighlight = highlightedParent();

    if ( currentHighlight && currentHighlight != parent )
        clearParentsHighlight();

    const TreemapTile * topParent = nullptr;

    while ( parent && parent != _rootTile )
    {
        ParentTileHighlighter * highlight = new ParentTileHighlighter( this, parent, parent->orig()->debugUrl() );
        CHECK_NEW( highlight );
        _parentHighlightList << highlight;

        topParent = parent;
        parent = parent->parentTile();
    }

    if ( topParent )
    {
        clearSceneMask();
        _sceneMask = new SceneMask( topParent, 0.6 );
    }
}


void TreemapView::clearParentsHighlight()
{
    qDeleteAll( _parentHighlightList );
    _parentHighlightList.clear();
    clearSceneMask();
}


void TreemapView::toggleParentsHighlight( const TreemapTile * tile )
{
    if ( !_parentHighlightList.isEmpty() && tile->orig() == _selectionModel->currentItem() )
        clearParentsHighlight();
    else
        highlightParents( tile );
}


void TreemapView::clearSceneMask()
{
    delete _sceneMask;
    _sceneMask = nullptr;
}


const TreemapTile * TreemapView::highlightedParent() const
{
    return _parentHighlightList.empty() ? nullptr : _parentHighlightList.first()->tile();
}


void TreemapView::saveTreemapRoot()
{
    _savedRootUrl = _rootTile ? _rootTile->orig()->debugUrl() : QString();
    //logDebug() << _savedRootUrl << Qt::endl;
}


void TreemapView::sendHoverEnter( FileInfo * node )
{
    if ( _useTreemapHover )
        emit hoverEnter( node );
}


void TreemapView::sendHoverLeave( FileInfo * node )
{
    if ( _useTreemapHover )
        emit hoverLeave( node );
}



HighlightRect::HighlightRect( QGraphicsScene * scene,
                              const TreemapTile * tile,
                              const QColor & color,
                              int lineWidth,
                              float zValue ):
    QGraphicsRectItem (),
    _tile { tile }
{
    setPen( QPen( color, lineWidth ) );
    setZValue( zValue );
    highlight();

    scene->addItem( this );
}


QPainterPath HighlightRect::shape() const
{
    if ( !_tile )
        return QGraphicsRectItem::shape();

    // Return just the outline as the shape so any tooltip is only displayed on
    // the outline, not inside as well; but use more than the line thickness of
    // 1 or 2 pixels to make it humanly possible to position the mouse cursor
    // close enough.
    //
    // Note that it's still only on the inside of the line to avoid bad side
    // effects with QGraphicsView's internal mechanisms.
    const int thickness = 10;

    QPainterPath path;
    path.addRect( _tile->rect() );
    path.addRect( _tile->rect().adjusted( thickness,   thickness,
                                          -thickness, -thickness ) );
    return path;
}


void HighlightRect::highlight()
{
    //logDebug() << Qt::endl;

    if ( _tile )
    {
        QRectF tileRect = _tile->rect();
        tileRect.moveTo( mapFromScene( _tile->mapToScene( tileRect.topLeft() ) ) );
        setRect( tileRect );

        if ( !isVisible() )
            show();
    }
    else
    {
        if ( isVisible() )
            hide();
    }
}



void CurrentTileHighlighter::highlight( const TreemapTile * tile )
{
    _tile = tile;

    QPen highlightPen = pen();
    highlightPen.setStyle( _tile && _tile->isSelected() ? Qt::SolidLine : Qt::DotLine );
    setPen( highlightPen );

    HighlightRect::highlight();
}




SceneMask::SceneMask( const TreemapTile * tile, float opacity ):
    QGraphicsPathItem()
{
    // logDebug() << "Adding scene mask for " << tile->orig() << Qt::endl;
    CHECK_PTR( tile );

    QPainterPath path;
    path.addRect( tile->scene()->sceneRect() );

    // Since the default OddEvenFillRule leaves overlapping areas unfilled,
    // adding the tile's rect that is inside the scene rect leaves the tile
    // "cut out", i.e. unobscured.
    path.addRect( tile->rect() );
    setPath( path );

    setBrush( QColor( 0x30, 0x30, 0x30, opacity * 255 ) );

    setZValue( SceneMaskLayer );
    tile->scene()->addItem( this );
}
