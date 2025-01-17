/*
 *   File name: TreemapView.cpp
 *   Summary:   View widget for treemap rendering for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <QResizeEvent>
#include <QtConcurrent/QtConcurrent>

#include "TreemapView.h"
#include "TreemapTile.h"
#include "DirInfo.h"
#include "DirTree.h"
#include "Exception.h"
#include "MimeCategorizer.h"
#include "SelectionModel.h"
#include "Settings.h"
#include "SignalBlocker.h"


using namespace QDirStat;


namespace
{
    /**
     * Writes a colour to the settings file.  If the color is invalid, it
     * will write an entry with an empty string.
     **/
    void writeOptionalColorEntry( Settings & settings, const char * setting, const QColor & color )
    {
        if ( color.isValid() )
            settings.colorValue( setting, color );
        else
            settings.setValue( setting, QString{} );
    }


    /**
     * Search the treemap for a tile that corresponds to the specified
     * FileInfo node. Returns 0 if there is none.
     *
     * Note that this is an expensive operation since all treemap tiles need
     * to be searched.
     **/
    TreemapTile * findTile( TreemapTile * rootTile, const FileInfo * node )
    {
        if ( !node || !rootTile )
            return nullptr;

        // common case that is easy
        if ( rootTile->orig() == node )
            return rootTile;

        // loop recursively through the children of each tile
        const auto childItems = rootTile->childItems();
        for ( QGraphicsItem * graphicsItem : childItems )
        {
            TreemapTile * tile = dynamic_cast<TreemapTile *>( graphicsItem );
            if ( tile )
            {
                tile = findTile( tile, node );
                if ( tile )
                    return tile;
            }
        }

        return nullptr;
    }

} // namespace


TreemapView::TreemapView( QWidget * parent ):
    QGraphicsView{ parent }
{
    // Only one scene, never destroyed, create it now for simplicity
    setScene( new QGraphicsScene{ this } );

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
        // Take out the tiles so we can delete them in the background
        scene()->removeItem( _rootTile );

        // Clear everything else, any highlighters and mask
        scene()->clear();

        // Deleting these can take a while, so delegate to a thread
        TreemapTile * rootTile = _rootTile;
        std::ignore = QtConcurrent::run( [ rootTile ]() { delete rootTile; } );
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

    // Always clear the treemap before the DirTree disappears ...
    // ... disable, although nobody should trigger us to rebuild until it is safe.
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
    delete _selectionModelProxy; // should always be 0 here anyway
    _selectionModelProxy = new SelectionModelProxy{ selectionModel, this };

    connect( _selectionModelProxy, &SelectionModelProxy::currentItemChanged,
             this,                 &TreemapView::updateCurrentItem );

    connect( _selectionModelProxy, &SelectionModelProxy::selectionChangedItems,
             this,                 &TreemapView::updateSelection);

    // Connect this one here because it is only relevant in the real treemap
    connect( MimeCategorizer::instance(), &MimeCategorizer::categoriesChanged,
             this,                        &TreemapView::changeTreemapColors );
}


void TreemapView::readSettings()
{
    Settings settings;
    settings.beginGroup( "Treemaps" );

    _colourPreviews     = settings.value( "ColourPreviews",    true ).toBool();

    _squarify           = settings.value( "Squarify",          true  ).toBool();
    _doCushionShading   = settings.value( "CushionShading",    true  ).toBool();
//    _enforceContrast    = settings.value( "EnforceContrast",   false ).toBool();
    _forceCushionGrid   = settings.value( "ForceCushionGrid",  false ).toBool();
    _useDirGradient     = settings.value( "UseDirGradient",    true  ).toBool();

    _ambientIntensity   = settings.value( "AmbientLight",      DefaultAmbientLight      ).toInt() / 255.0;
    _heightScaleFactor  = settings.value( "HeightScaleFactor", DefaultHeightScaleFactor ).toDouble();
    _cushionHeight      = settings.value( "CushionHeight",     DefaultCushionHeight     ).toDouble();
    _minTileSize        = settings.value( "MinTileSize",       DefaultMinTileSize       ).toInt();

    _tileFixedColor     = settings.colorValue( "TileFixedColor",     QColor{}                   );
    _currentItemColor   = settings.colorValue( "CurrentItemColor",   Qt::red                    );
    _selectedItemsColor = settings.colorValue( "SelectedItemsColor", Qt::yellow                 );
    _highlightColor     = settings.colorValue( "HighlightColor",     Qt::white                  );
    _cushionGridColor   = settings.colorValue( "CushionGridColor",   Qt::darkGray               );
    _outlineColor       = settings.colorValue( "OutlineColor",       Qt::black                  );
    _dirFillColor       = settings.colorValue( "DirFillColor",       QColor{ 0x60, 0x60, 0x60 } );
    _dirGradientStart   = settings.colorValue( "DirGradientStart",   QColor{ 0x60, 0x60, 0x70 } );
    _dirGradientEnd     = settings.colorValue( "DirGradientEnd",     QColor{ 0x70, 0x70, 0x80 } );

    settings.endGroup();

    calculateSettings();
}


void TreemapView::writeSettings()
{
    Settings settings;

    settings.beginGroup( "Treemaps" );

    settings.setValue( "ColourPreviews",    _colourPreviews    );
    settings.setValue( "Squarify",          _squarify          );
    settings.setValue( "CushionShading",    _doCushionShading  );
//    settings.setValue( "EnforceContrast",   _enforceContrast   );
    settings.setValue( "ForceCushionGrid",  _forceCushionGrid  );
    settings.setValue( "UseDirGradient",    _useDirGradient    );
    settings.setValue( "AmbientLight",      qRound( _ambientIntensity * 255 ) );
    settings.setValue( "HeightScaleFactor", _heightScaleFactor );
    settings.setValue( "CushionHeight",     _cushionHeight     );
    settings.setValue( "MinTileSize",       _minTileSize       );

    writeOptionalColorEntry( settings, "TileFixedColor", _tileFixedColor );

    settings.setColorValue( "CurrentItemColor",   _currentItemColor   );
    settings.setColorValue( "SelectedItemsColor", _selectedItemsColor );
    settings.setColorValue( "HighlightColor",     _highlightColor     );
    settings.setColorValue( "CushionGridColor",   _cushionGridColor   );
    settings.setColorValue( "OutlineColor",       _outlineColor       );
    settings.setColorValue( "DirFillColor",       _dirFillColor       );
    settings.setColorValue( "DirGradientStart",   _dirGradientStart   );
    settings.setColorValue( "DirGradientEnd",     _dirGradientEnd     );

    settings.endGroup();
}


void TreemapView::zoomTo()
{
    // this does all the sanity checks so we know we are good
    if ( !canZoomIn() )
        return;

    // Work from the FileInfo tree because there might not be a tile for the current item
    FileInfo * newNode = _selectionModel->currentItem();
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
    FileInfo * newNode = _selectionModel->currentItem();
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
    if ( _tree )
    {
        FileInfo * firstToplevel = _tree->firstToplevel();
        if ( firstToplevel )
            rebuildTreemap( firstToplevel );
    }
}


bool TreemapView::canZoomIn() const
{
    if ( !_rootTile || !_selectionModel )
        return false;

    // Work up the FileInfo tree because there might not be a tile for the current item
    const FileInfo * currentNode = _selectionModel->currentItem();
    if ( !currentNode )
        return false;

    const FileInfo * rootNode = _rootTile->orig();
    if ( currentNode == rootNode )
        return false;

    while ( currentNode && currentNode->parent() != rootNode )
        currentNode = currentNode->parent();

    if ( currentNode && currentNode->isDirInfo() )
        return true;

    return false;
}


bool TreemapView::canZoomOut() const
{
    if ( !_rootTile )
        return false;

    FileInfo * firstToplevel = _tree->firstToplevel();

    return firstToplevel ? _rootTile->orig() != firstToplevel : false;
}


void TreemapView::rebuildTreemap()
{
    //logDebug() << _savedRootUrl << Qt::endl;

    FileInfo * root = nullptr;

    if ( !_savedRootUrl.isEmpty() )
    {
        //logDebug() << "Restoring old treemap with root " << _savedRootUrl << Qt::endl;

        root = _tree->locate( _savedRootUrl );
    }

    if ( !root )
        root = _rootTile ? _rootTile->orig() : _tree->firstToplevel();

    clear();

    rebuildTreemap( root );

    _savedRootUrl = QString{};
    //logDebug() << _savedRootUrl << Qt::endl;
}


void TreemapView::rebuildTreemap( FileInfo * newRoot )
{
    if ( _disabled || !newRoot || !isVisible() )
        return;

    // Prevent division by zero in TreemapTile - also cleans all the summaries for this subtree
    if ( newRoot->totalAllocatedSize() == 0 )
        return;

    const QRectF rect = viewport()->rect();
    if ( rect.isEmpty() )
        return;

    if ( _treemapRunning )
    {
        // Restart in the watched finished() slot so we don't stamp on the future
        _newRoot = newRoot;
        _treemapCancel = TreemapCancelRestart;
        return;
    }

    //logDebug() << rect << Qt::endl;

    _treemapCancel = TreemapCancelNone;
    _treemapRunning = true;

    _stopwatch.start();

    _watcher.setFuture( QtConcurrent::run( [ this, newRoot, rect ]()
    {
        // By default the number of CPUs, which will sometimes block creation of render threads
        _threadPool = new QThreadPool{};
        _threadPool->setMaxThreadCount( _threadPool->maxThreadCount() * 2 );

        TreemapTile * tile = new TreemapTile{ this, newRoot, rect };

        delete _threadPool; // will wait for all the render threads to complete

        if ( treemapCancelled() )
        {
            // Logging is not thread-safe, use only for debugging
            //logDebug() << "treemap cancelled, delete tiles" << Qt::endl;
            delete tile;
            tile = nullptr;
        }

        return tile;
    } ) );

    //logDebug() << QThreadPool::globalInstance()->activeThreadCount() << " threads active" << Qt::endl;
}


void TreemapView::treemapFinished()
{
    TreemapTile * futureResult = _watcher.result();

    logDebug() << _stopwatch.restart() << "ms " << Qt::endl;

    _treemapRunning = false;

    if ( treemapCancelled() )
    {
        if ( futureResult )
        {
            // Rare, but it is possible that the build is cancelled, but the thread has already finished
            //logDebug() << "completed treemap has been cancelled, delete tiles" << Qt::endl;
            delete futureResult;
        }

        // We're finished with the future now, so can restart an interrupted build
        if ( _treemapCancel == TreemapCancelRestart )
            rebuildTreemap( _newRoot );

        return;
    }

    if ( !futureResult )
    {
        logWarning() << "unexpected null result from treemap build" << Qt::endl;
        return;
    }

    // Wipe the existing scene
    clear();
    resetTransform();

    // Add the new treemap to the scene
    _rootTile = futureResult;
    scene()->setSceneRect( _rootTile->rect() );
    scene()->addItem( _rootTile );

    if ( _selectionModel )
        updateSelection( _selectionModel->selectedItems() );

    emit treemapChanged();

    //logDebug() << _stopwatch.restart() << "ms" << Qt::endl;
#if PAINT_DEBUGGING
    _lastTile->setLastTile();
#endif
}


void TreemapView::configChanged( const QColor & fixedColor,
                                 bool           squarified,
                                 bool           cushionShading,
                                 double         cushionHeight,
                                 double         heightScaleFactor,
                                 int            minTileSize )
{
    //logDebug() << fixedColor.name() << Qt::endl;
    const bool treemapChanged = squarified        != _squarify ||
                                cushionHeight     != _cushionHeight ||
                                heightScaleFactor != _heightScaleFactor ||
                                minTileSize       != _minTileSize;
    const bool coloursChanged = cushionShading != _doCushionShading ||
                                fixedColor     != _tileFixedColor;
    if ( !treemapChanged && !coloursChanged )
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
        rebuildTreemap();
    else
        changeTreemapColors();
}


void TreemapView::calculateSettings()
{
    // Pre-calculate cushion heights from the configured starting height and scale factor.
    _cushionHeights.reset( new CushionHeightSequence{ _cushionHeight, _heightScaleFactor } );

    // Calculate thresholds for tile sizes that will be submitted to a render thread
    _maxTileThreshold = ( _squarify ? 150 : 75 ) + 10 * QThread::idealThreadCount();

    // Calculate the minimum height for generating a row of squarified tiles
    _minSquarifiedTileHeight = _minTileSize == 0 ? 0 : _minTileSize - 0.5;

    // Directory gradient can't currently change after startup, but calculate it here anyway
    if ( _useDirGradient )
    {
        _dirGradient = QLinearGradient{};
        _dirGradient.setCoordinateMode( QGradient::ObjectMode );
        _dirGradient.setColorAt( 0, _dirGradientStart );
        _dirGradient.setColorAt( 1, _dirGradientEnd   );
    }

    // Cushion shading coefficients for a light source above, somewhat behind, and slightly left
    const double intensityScaling = 1.0 - _ambientIntensity;
    _lightX = 0.09759 * intensityScaling;
    _lightY = 0.19518 * intensityScaling;
    _lightZ = 0.97590 * intensityScaling;
}


void TreemapView::changeTreemapColors()
{
    if ( _rootTile )
    {
        _rootTile->invalidateCushions();
        _rootTile->update( _rootTile->rect() );
    }
}

void TreemapView::deleteNotify( FileInfo * )
{
    if ( _rootTile )
    {
        if ( _rootTile->orig() != _tree->firstToplevel() )
        {
            // If the user zoomed the treemap in, save the root's URL so the
            // current state can be restored when the treemap is rebuilt.
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
            _savedRootUrl = QString{};
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
    if ( !_tree )
        return;

    if ( _rootTile )
    {
        //logDebug() << "Auto-resizing treemap" << Qt::endl;
        rebuildTreemap( _rootTile->orig() );

        const QSizeF & newSize = event->size();
        const QSizeF & oldSize = event->oldSize();
        if ( !newSize.isEmpty() && !oldSize.isEmpty() )
            scale( newSize.width() / oldSize.width(), newSize.height() / oldSize.height() );
    }
    else if ( _tree )
    {
        FileInfo * firstToplevel = _tree->firstToplevel();
        if ( firstToplevel )
            rebuildTreemap( firstToplevel );
    }
}


void TreemapView::hideTreemap()
{
    //logDebug() << "Hiding treemap view" << Qt::endl;

    clear();
    hide();

    emit treemapChanged();
}


void TreemapView::showTreemap()
{
    //logDebug() << "Showing treemap view " << Qt::endl;

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
    rebuildTreemap(); // will emit treemapChanged() when complete
}


void TreemapView::setCurrentTile( const TreemapTile * tile )
{
    // Always clear the current highlight
    delete _currentTileHighlighter;
    _currentTileHighlighter = nullptr;

    if ( !tile )
        return;

    //logDebug() << tile << " " << _stopwatch.restart() << "ms" << Qt::endl;

    //logDebug() << highlightedParent() << " " << tile->parentTile() << Qt::endl;

    // Clear the parent highlights if the current tile parent has changed
    if ( highlightedParent() != tile->parentTile() )
        clearParentsHighlight();

    // Don't highlight the root tile
    if ( tile == _rootTile )
        return;

    //logDebug() " Highlighting the current tile" << Qt::endl;
    _currentTileHighlighter = new CurrentTileHighlighter{ this, tile, tile->isSelected() };

    if ( _selectionModel->currentItem() != tile->orig() && _selectionModelProxy )
    {
        //logDebug() << "Sending currentItemChanged " << tile << Qt::endl;

        // Prevent signal ping-pong from sending us the current item again
        SignalBlocker sigBlocker{ _selectionModelProxy };
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

    if ( treemapRoot != _rootTile->orig() ) // need to zoom out?
    {
        //logDebug() << "Zooming out to " << treemapRoot << " to make current item visible" << Qt::endl;
        rebuildTreemap( treemapRoot );
    }

    setCurrentTile( findTile( _rootTile, node ) );
}


void TreemapView::updateSelection( const FileInfoSet & newSelection )
{
    if ( !_rootTile || ( newSelection.size() == 0 && !_selectionModel->currentItem() ) )
        return;

    //logDebug() << newSelection.size() << " items selected (after " << _stopwatch.restart() << "ms) " << Qt::endl;

    // Don't send a signal that we changed the selection when someone else did it
    SignalBlocker sigBlocker{ this };
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

    //logDebug() << " map built in " << _stopwatch.restart() << "ms" << (map.isEmpty() ? " (empty) " : QString{}) << Qt::endl;

    for ( const FileInfo * item : newSelection )
    {
        TreemapTile * tile = map.isEmpty() ? findTile( _rootTile, item ) : map.value( item, nullptr );
        if ( tile )
            tile->setSelected( true );
    }

    const FileInfo * currentItem = _selectionModel->currentItem();
    const TreemapTile * tile = map.isEmpty() ? findTile( _rootTile, currentItem ) : map.value( currentItem, nullptr );
    if ( tile )
        setCurrentTile( tile );

    //logDebug() << newSelection.size() << " items selected " << _stopwatch.restart() << "ms" << Qt::endl;
}


void TreemapView::sendSelection( const TreemapTile * tile)
{
    if ( !_selectionModel )
        return;

//    SignalBlocker sigBlocker{ _selectionModelProxy };

    const QList<QGraphicsItem *> selectedTiles = scene()->selectedItems();
    if ( selectedTiles.size() == 1 && selectedTiles.first() == tile )
    {
        // For just one selected tile that is the current item, only send one signal
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

    // Don't send a signal that we changed the current item when someone else did it
    SignalBlocker sigBlocker{ this };
    setCurrentItem( currentItem );
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
        return;

    const TreemapTile * currentHighlight = highlightedParent();
    const TreemapTile * parent = tile->parentTile();

    // If the same parent, then keep the existing highlights and mask
    if ( currentHighlight && currentHighlight == parent )
        return;

    // Simplest to start from scratch even if some of the ancestors are the same
    clearParentsHighlight();

    while ( parent && parent != _rootTile )
    {
        _parentHighlightList << new ParentTileHighlighter{ this, parent, parent->orig()->debugUrl() };
        parent = parent->parentTile();
    }

    if ( !_parentHighlightList.isEmpty() )
        _sceneMask = new SceneMask{ _parentHighlightList.last()->tile(), qRound( 0.6 * 255 ) };
}


void TreemapView::clearParentsHighlight()
{
    qDeleteAll( _parentHighlightList );
    _parentHighlightList.clear();

    delete _sceneMask;
    _sceneMask = nullptr;
}


void TreemapView::toggleParentsHighlight( const TreemapTile * tile )
{
    if ( !_parentHighlightList.isEmpty() && tile->orig() == _selectionModel->currentItem() )
        clearParentsHighlight();
    else
        highlightParents( tile );
}


const TreemapTile * TreemapView::highlightedParent() const
{
    return _parentHighlightList.empty() ? nullptr : _parentHighlightList.first()->tile();
}


void TreemapView::saveTreemapRoot()
{
    _savedRootUrl = _rootTile ? _rootTile->orig()->debugUrl() : QString{};
    //logDebug() << _savedRootUrl << Qt::endl;
}



HighlightRect::HighlightRect( const TreemapTile * tile,
                              const QColor      & color,
                              int                 lineWidth,
                              Qt::PenStyle        lineStyle,
                              qreal               zValue ):
    QGraphicsRectItem{ tile->rect() }
{
    setPen( QPen{ color, static_cast<qreal>( lineWidth ), lineStyle } );
    setZValue( zValue );

    tile->scene()->addItem( this );
}



QPainterPath ParentTileHighlighter::shape() const
{
    // Return just the outline as the shape so any tooltip is only displayed on
    // the outline, not inside the rectangle as well; but use more than the line
    // thickness of 1 or 2 pixels to make it humanly possible to position the
    // mouse cursor close enough.
    //
    // Note that it's still only on the inside of the line to avoid side effects.
    const int thickness = 5;

    QPainterPath path;
    path.addRect( rect() );
    path.addRect( rect().adjusted( thickness, thickness, -thickness, -thickness ) );

    return path;
}



SceneMask::SceneMask( const TreemapTile * tile, int opacity ):
    QGraphicsPathItem{}
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

    setBrush( QColor{ 0x30, 0x30, 0x30, opacity } );
    setZValue( SceneMaskLayer );

    tile->scene()->addItem( this );
}
