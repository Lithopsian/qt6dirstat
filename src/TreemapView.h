/*
 *   File name: TreemapView.h
 *   Summary:	View widget for treemap rendering for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#ifndef TreemapView_h
#define TreemapView_h


#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsPathItem>
#include <QList>
#include <QElapsedTimer>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>


#define DefaultAmbientLight       40
#define DefaultHeightScaleFactor   0.8
#define DefaultCushionHeight       0.5
#define DefaultMinTileSize         3


namespace QDirStat
{
    class TreemapTile;
    class HighlightRect;
    class SceneMask;
    class CushionHeightSequence;
    class CleanupCollection;
    class DirTree;
    class FileInfo;
    class FileInfoSet;
    class SelectionModel;
    class SelectionModelProxy;
    class Settings;

    enum TreemapCancel
    {
	TreemapCancelNone,
	TreemapCancelCancel,
	TreemapCancelRestart,
    };

    typedef QList<HighlightRect *> HighlightRectList;

    // Treemap layers (Z values)
    constexpr double TileLayer             = 0.0;
    constexpr double SceneMaskLayer        = 1e5;
    constexpr double TileHighlightLayer    = 1e6;
    constexpr double CurrentHighlightLayer = 1e8;
    constexpr double SceneHighlightLayer   = 1e10;

    /**
     * View widget that displays a DirTree as a treemap.
     **/
    class TreemapView: public QGraphicsView
    {
	Q_OBJECT

    public:

	/**
	 * Constructor. Remember to set the directory tree with setDirTree()
	 * and the selection model with setSelectionModel() after creating this
	 * widget.
	 **/
	TreemapView( QWidget * parent = nullptr );

	/**
	 * Destructor.
	 **/
	~TreemapView() override;

	/**
	 * Set the directory tree to work on. Without this, this widget will
	 * not display anything.
	 **/
	void setDirTree( const DirTree * tree );

	/**
	 * Returns the minimum recommended size for this widget.
	 * Reimplemented from QWidget.
	 **/
	QSize minimumSizeHint() const override { return QSize( 0, 0 ); }

	/**
	 * Returns this view's current item.
	 **/
//	TreemapTile * currentTile() const { return _currentTile; }

	/**
	 * Returns this treemap view's root treemap tile or 0 if there is none.
	 **/
	TreemapTile * rootTile() const { return _rootTile; }

        /**
         * Returns the currently highlighted treemap tile (that was highlighted
         * with a middle click) or 0 if there is none.
         **/
//        TreemapTile * highlightedTile() const { return _highlightedTile; }

	/**
	 * Returns this treemap view's DirTree.
	 **/
//	DirTree * tree() const { return _tree; }

	/**
	 * Set the selection model. This is important to synchronize current /
	 * selected items between a DirTreeView and this TreemapView.
	 **/
	void setSelectionModel( SelectionModel * selectionModel );

	/**
	 * Return this treemap view's SelectionModel.
	 **/
	SelectionModel * selectionModel() const { return _selectionModel; }

	/**
	 * Set the cleanup collection.  This connects signals, but does not
	 * take ownership or even remember the CleanupCollection object.
	 **/
	void setCleanupCollection( const CleanupCollection * collection );

	/**
	 * Return the cleanup collection or 0 if it is not set.
	 **/
//	const CleanupCollection * cleanupCollection() const { return _cleanupCollection; }

	/**
	 * Use a fixed color for all tiles. To undo this, set an invalid QColor
	 * with the QColor default constructor.
	 **/
	const QColor & fixedColor() const { return _tileFixedColor; }

	/**
	 * Use a fixed color for all tiles. To undo this, set an invalid QColor
	 * with the QColor default constructor.
	 **/
	void setFixedColor( const QColor & fixedColor );

        /**
         * Remember the current treemap zoom depth so it can be restored later (eg.
	 * after a refresh).
         **/
        void saveTreemapRoot();

	/**
	 * Hide this treemap view: clear its contents, hide the viewport, and disable
	 * any further builds.  Emits the treemapChanged() signal.
	 **/
	void hideTreemap();

	/**
	 * Show the treemap: show the viewport if it is not currently visible and
	 * enable builds (this will start a build and eventually emit the
	 * treemapChanged() signal).
	 **/
	void showTreemap();

	/**
	 * Disable this treemap view: clear its contents and prevent it from
	 * rebuilding, but leave the viewport visible.  Emits the treemapChanged()
	 * signal.
	 **/
	void disable();

	/**
	 * Re-enable this treemap view after disabling it: enable treemap builds
	 * and start a build.  The treemapChanged() signal will be emitted once the
	 * build finishes.
	 **/
	void enable();

	/**
	 * Returns a value used by the treemap to create render threads.  Directories
	 * smaller than this value that have parents larger than this value are
	 * submitted to be rendered in a thread.  The value is adjusted based on the
	 * number of available processors.  Larger values mean that larger directories
	 * will be processed in each thread, but threads will not be submitted until
	 * later in the treemap build, useful for reducing the number of threads created
	 * when they can be rendered in parallel on multiple processors.
	 **/
	int maxTileThreshold() const { return _maxTileThreshold; }

	/**
	 * Returns whether the treemap has been asked to stop building.
	 **/
	bool treemapCancelled() const { return _treemapCancel != TreemapCancelNone; }

	/**
	 * Adds the future for a tile-rendering thread to a list.  The treemap build will
	 * wait for all the rendering threads to finish before returning.  The rendering
	 * threads have no return value.
	 **/
	void appendTileFuture( QFuture<void> future ) { _tileFutures.append( future ); }

	/**
	 * Iterators for the tile future list.
	 **/
	const auto tileFuturesBegin() { return _tileFutures.begin(); }
	const auto tileFuturesEnd() const { return _tileFutures.cend(); }

	/**
	 * Returns true if it is possible to zoom in with the currently
	 * selected tile, false if not.
	 **/
	bool canZoomIn() const;

	/**
	 * Returns true if it is possible to zoom out with the currently
	 * selected tile, false if not.
	 **/
	bool canZoomOut() const;

	/**
	 * Make a treemap tile this treemap's current item. 'tile' may be 0 and
	 * in this case the previous selection is deselected.
	 **/
	void setCurrentTile( const TreemapTile * tile );

	/**
	 * Sync the selected items and the current item to the selection model.
	 **/
	void sendSelection( const TreemapTile *tile );

        /**
         * Returns the value of the UseTreemapHover setting.
         **/
        bool useTreemapHover() const { return _useTreemapHover; }

        /**
         * Returns the value of the UseTreemapHover setting.
         **/
        void setUseTreemapHover( bool useTreemapHover ) { _useTreemapHover = useTreemapHover; }

        /**
         * Send a hoverEnter() signal for 'node'.
         **/
        void sendHoverEnter( FileInfo * node );

        /**
         * Send a hoverLeave() signal for 'node'.
         **/
        void sendHoverLeave( FileInfo * node );

        /**
         * Highlight the parent tiles of item 'tile' if that tile is not
         * currently highlighted, or clear the highlight if it is.
         **/
        void toggleParentsHighlight( const TreemapTile * tile );

	/**
	 * Returns 'true' if the category colours should be shown next to each
	 * category in the configuration dialog list.
	 **/
	bool colourPreviews() const { return _colourPreviews; }

	/**
	 * Returns 'true' if the category colours should be shown next to each
	 * category in the configuration dialog list.
	 **/
	void setColourPreviews( bool colourPreviews ) { _colourPreviews = colourPreviews; }

	/**
	 * Returns 'true' if treemap tiles are to be squarified upon creation,
	 * 'false' if not.
	 **/
	bool squarify() const { return _squarify; }

	/**
	 * Returns 'true' if cushion shading is to be used, 'false' if not.
	 **/
	bool doCushionShading() const { return _doCushionShading; }

	/**
	 * Returns the brush to be used for filling visible directory tiles.
	 **/
	QBrush dirBrush() const { return _useDirGradient ? QBrush( _dirGradient ) : QBrush( _dirFillColor ); }

	/**
         * Returns 'true' if directories should be rendered with a gradient,
         * 'false' if not.
         **/
//        bool useDirGradient() const { return _useDirGradient; }

	/**
	 * Returns 'true' if treemap tiles are to be separated by a grid,
	 * 'false' if not.  Applies even without cushion shading, in fact is
	 * almost essential without cushion shading.
	 **/
	bool forceCushionGrid() const { return _forceCushionGrid; }

	/**
	 * Returns 'true' if tile boundary lines should be drawn for cushion
	 * treemaps, 'false'  if not.
	 **/
//	bool enforceContrast() const { return _enforceContrast; }

	/**
	 * Returns the minimum tile size in pixels. No treemap tiles less than
	 * this in width or height are desired.
	 **/
	double minTileSize() const { return _minTileSize; }

	/**
	 * Returns the minimum height of a row of squarified tiles.  The value is
	 * smaller than minTileSize to allow for rounding, but is clamped to a
	 * minimum size of zero to prevent infinite loops.
	 **/
	double minSquarifiedTileHeight() const { return _minSquarifiedTileHeight; }

	/**
	 * Returns the cushion grid color.
	 **/
	const QColor & cushionGridColor() const { return _cushionGridColor; }

	/**
	 * Return the frame color for selected items.
	 **/
	const QColor & currentItemColor() const { return _currentItemColor; }

	/**
	 * Return the frame color for selected items.
	 **/
	const QColor & selectedItemsColor() const { return _selectedItemsColor; }

	/**
	 * Return the frame color for highlighted parents.
	 **/
	const QColor & highlightColor() const { return _highlightColor; }

	/**
	 * Returns the outline color to use if cushion shading is not used.
	 **/
	const QColor & outlineColor() const { return _outlineColor; }

	/**
	 * Returns the fill color for non-directory treemap tiles when cushion
	 * shading is not used.
	 **/
//	const QColor & fileFillColor() const { return _fileFillColor; }

	/**
	 * Returns the fill color for directory (or "dotentry") treemap tiles
	 * when cushion shading is not used.
	 **/
//	const QColor & dirFillColor() const { return _dirFillColor; }

        /**
         * Returns the start color for directory (or "dotentry") treemap tiles
         * if a directory gradient should be used.
         **/
//        const QColor & dirGradientStart() const { return _dirGradientStart; }

        /**
         * Returns the end color for directory (or "dotentry") treemap tiles
         * if a directory gradient should be used.
         **/
//        const QColor & dirGradientEnd() const { return _dirGradientEnd; }

	/**
	 * Returns the intensity of ambient light for cushion shading
	 * [0..255]
	 **/
	int ambientLight() const { return _ambientLight; }

	/**
	 * These return the relative light levels in the x, y, and z directions,
	 * corresponding to over the left shoulder.  This avoids the steep top
	 * and left sides of the largest tiles being in deep shadow, while the
	 * smaller tiles to the bottom and right are still illuminated and the
	 * highlight is reasonably centered.
	 **/
	double lightX() const { return _lightX; }
	double lightY() const { return _lightY; }
	double lightZ() const { return _lightZ; }

	/**
	 * Returns cushion ridge height degradation factor (0 .. 1.0) for each
	 * level of subdivision.
	 **/
	double heightScaleFactor() const { return _heightScaleFactor; }

	/**
	 * Returns cushion initial height.
	 **/
	double cushionHeight() const { return _cushionHeight; }

	/**
	 * Returns a map of cushion heights:
	 * key: current cushion height
	 * value: next cushion height
	 **/
	const CushionHeightSequence * cushionHeights() const { return _cushionHeights; }

	/**
	 * Called from the main window when settings related to the treemap may have
	 * changed.
	 **/
	void configChanged( const QColor & fixedColor,
	                    bool squarified,
	                    bool cushionShading,
	                    double cushionHeight,
			    double heightScaleFactor,
			    int minTileSize );

        /**
         * Return the tile of the deepest-level highlighted parent or 0 if no
         * parent is currently highlighted. Notice that this returns the real
         * tile corresponding to a directory, not the HighlightRect.
         **/
        const TreemapTile * highlightedParent() const;

        /**
         * Sets a pointer to a treemap tile.  Each tile updates this as it is
	 * constructed, so that after the build it points to the last tile to be
	 * built, and hence the last to be painted.  This is used for logging
	 * purposes as it is difficult to identify the end of the paint any other way.
         **/
//	 void setLastTile( TreemapTile * tile ) { _lastTile = tile; }


    public slots:

	/**
	 * Zoom in to the level of the currently selected treemap tile:
	 * The entire treemap will be rebuilt with the near-topmost ancestor of
	 * the selected tile as the new root.
	 **/
	void zoomTo();

	/**
	 * Zoom in one level towards the currently selected treemap tile:
	 * The entire treemap will be rebuilt with the near-topmost ancestor of
	 * the selected tile as the new root.
	 **/
	void zoomIn();

	/**
	 * Zoom out one level: The parent (if there is any) FileInfo node of
	 * the current treemap root becomes the new root. This usually works
	 * only after zoomIn().
	 **/
	void zoomOut();

	/**
	 * Reset the zoom level: Zoom out as far as possible.
	 **/
	void resetZoom();

	/**
	 * Completely rebuild the entire treemap from the internal tree's root
	 * on.
	 **/
	void rebuildTreemapSlot();

	/**
	 * Notification that a dir tree node is about to be deleted, with no
	 * subsequent reads.
	 **/
	void deleteNotify( FileInfo * );

    private:

	/**
	 * Read parameters from the settings file.
	 **/
	void readSettings();

	/**
	 * Calculate some values from the settings.
	 **/
	void calculateSettings();

	/**
	 * Write parameters to the settings file.
	 *
	 * Unlike other classes in this program, this is not done from the
	 * corresponding settings dialog - because there is none. The settings
	 * here are very obscure - strictly for experts; nothing to bother a
	 * normal user with.
	 *
	 * Experts can edit them in the settings file, typically in
	 * ~/.config/QDirStat/QDirStat.conf ; this class writes the settings to
	 * that file in its destructor so those experts can find and edit them.
	 *
	 * If you misconfigured things and want to go back to the defaults,
	 * simply delete that one setting or the section in the settings or the
	 * complete settings file; missing settings are restored to the
	 * defaults when the program exits the next time.
	 **/
	void writeSettings();

	/**
	 * Writes a colour to the settings file.  If the color is invalid, it
	 * will write an entry with an empty string.
	 **/
	void writeOptionalColorEntry( Settings & settings, const char * setting, const QColor & color );

	/**
	 * Rebuild the treemap with 'newRoot' as the new root.
	 **/
	void rebuildTreemap( FileInfo * newRoot );

	/**
	 * Returns the visible size of the viewport presuming no scrollbars are
	 * needed - which makes a lot more sense than fiddling with scrollbars
	 * since treemaps can be scaled to make scrollbars unnecessary.
	 **/
//	QSize visibleSize() const { return viewport()->size(); }

	/**
	 * Resize the treemap view. Suppress the treemap contents if the size
	 * falls below a minimum size, redisplay it if it grows above that
	 * minimum size.
	 *
	 * Reimplemented from QFrame.
	 **/
	void resizeEvent( QResizeEvent * event ) override;

	/**
	 * Search the treemap for a tile that corresponds to the specified
	 * FileInfo node. Returns 0 if there is none.
	 *
	 * Notice: This is an expensive operation since all treemap tiles need
	 * to be searched.
	 **/
	TreemapTile * findTile( TreemapTile *rootTile, const FileInfo *node ) const;

        /**
         * Highlight the parent tiles of item 'tile'.
         **/
        void highlightParents( const TreemapTile * tile );

        /**
         * Clear previous parent highlights.
         **/
        void clearParentsHighlight();

        /**
         * Clear the old scene mask if there is one.
         **/
        void clearSceneMask();

        /**
         * Cancels any treemap builds.
         **/
        void cancelTreemap();


    signals:

	/**
	 * Emitted when the currently selected item changes.
	 * Caution: 'item' may be 0 when the selection is cleared.
	 *
	 * Unused.
	 **/
//	void selectionChanged( FileInfo * item );

	/**
	 * Emitted when the current item changes.
	 **/
	void currentItemChanged( FileInfo * newCurrent );

	/**
	 * Emitted when the treemap changes, e.g. is rebuilt, zoomed in, or
	 * zoomed out.
	 **/
	void treemapChanged();

        /**
         * Emitted when the mouse cursor enters a hover over 'item'.
         **/
        void hoverEnter( FileInfo * item );

        /**
         * Emitted when the mouse cursor leaves the hover over 'item'.
         **/
        void hoverLeave( FileInfo * item );


    protected slots:

	/**
	 * Clear the treemap contents.
	 **/
	void clear();

	/**
	 * The Mime categories have changed and the map needs to be re-coloured.
	 **/
	void changeTreemapColors();

	/**
	 * Update the selected items that have been selected in another view.
	 **/
	void updateSelection( const FileInfoSet & newSelection );

	/**
	 * Update the current item that has been changed in another view.
	 **/
	void updateCurrentItem( FileInfo * currentItem );

	/**
	 * Search the treemap for a tile with the specified FileInfo node and
	 * make that tile the current item if it is found. If nothing is found
	 * or if 'node' is 0, the highlighting is removed from the previous
	 * current item.
	 **/
	void setCurrentItem( FileInfo * node );

	/**
	 * The treemap thread has finished.
	 **/
	void treemapFinished();


    private:

	// Data members
	const DirTree		* _tree			{ nullptr };
	SelectionModel		* _selectionModel	{ nullptr };
	SelectionModelProxy	* _selectionModelProxy	{ nullptr };

	TreemapTile	* _rootTile			{ nullptr };
	HighlightRect	* _currentTileHighlighter	{ nullptr };
        SceneMask       * _sceneMask			{ nullptr };
	FileInfo	* _newRoot			{ nullptr };
        HighlightRectList _parentHighlightList;
	QString		  _savedRootUrl;

	bool   _colourPreviews;
	bool   _squarify;
	bool   _doCushionShading;
	bool   _forceCushionGrid;
//	bool   _enforceContrast;
//	bool   _useFixedColor;
        bool   _useDirGradient;
        bool   _useTreemapHover;

	QColor _tileFixedColor;
	QColor _currentItemColor;
	QColor _selectedItemsColor;
	QColor _highlightColor;
	QColor _cushionGridColor;
	QColor _outlineColor;
//	QColor _fileFillColor;
	QColor _dirFillColor;
        QColor _dirGradientStart;
        QColor _dirGradientEnd;
	QLinearGradient _dirGradient;

	const double _lightX { 0.09759 };
	const double _lightY { 0.19518 };
	const double _lightZ { 0.97590 };

	int    _ambientLight;
	double _heightScaleFactor;
	double _cushionHeight;
	double _minTileSize;
	double _minSquarifiedTileHeight;
	int    _maxTileThreshold; // largest sub-tree size at which to spawn a rendering thread
	const CushionHeightSequence * _cushionHeights;

	bool _disabled		{ false }; // flag to disable all treemap builds even though the view may still be visible
	bool _treemapRunning	{ false }; // internal flag to avoid race conditions when cancelling builds
	std::atomic<TreemapCancel>	_treemapCancel { TreemapCancelNone }; // flag to the treemap build thread
	QFuture<TreemapTile *>		_future;
	QFutureWatcher<TreemapTile *>	_watcher;
	QVector<QFuture<void>>		_tileFutures;

	// just for logging
	QElapsedTimer _stopwatch;
//	TreemapTile * _lastTile;

    }; // class TreemapView



    /**
     * Transparent rectangle to make a treemap tile clearly visible as the
     * current item or as selected.
     *
     * Leaf tiles can do that themselves, but directory tiles are typically
     * completely obscured by their children, so no highlight border they draw
     * themselves will ever become visible.
     *
     * This highlight rectangle simply draws a colored red outline on top
     * (i.e., great z-height) of everything else. The rectangle is transparent,
     * so the treemap tile contents remain visible.
     **/
    class HighlightRect: public QGraphicsRectItem
    {
    private:

	/**
	 * Private constructor that the others will delegate to.
	 **/
	HighlightRect( QGraphicsScene * scene, const TreemapTile * tile, const QColor & color, int lineWidth, float zValue );

    public:

	/**
	 * Create a highlight rectangle for the entire scene. This is most
	 * useful for the current item.
	 **/
	HighlightRect( QGraphicsScene * scene, const QColor & color, int lineWidth, float zValue );

	/**
	 * Create a highlight rectangle for one specific tile and highlight it
	 * right away. This is most useful for selected items if more than one
	 * item is selected. The z-height of this is lower than for the
	 * scene-wide highlight rectangle.
	 **/
	HighlightRect( const TreemapTile * tile, const QColor & color, int lineWidth, float zValue );

	/**
	 * Highlight the current treemap tile: resize this selection
	 * rectangle to match this tile and move it to this tile's
	 * position. Show the selection rectangle if it is currently
	 * invisible.
	 **/
	void highlight();

	/**
	 * Highlight the specified treemap tile: can be re-implemented by a child class.
	 * Used by the current item highlighter to reset the current tile, choose the
	 * correct highlight style, and then highlight it.
	 **/
	virtual void highlight( const TreemapTile * ) {}

        /**
         * Return the tile that this highlights or 0 if there is none.
         **/
        const TreemapTile * tile() const { return _tile; }

        /**
         * Return the shape of this item; in this case only the outline,
         * leaving the inside hollow to avoid displaying the tooltip there as
         * well.
         *
         * Reimplemented from QGraphicsRectItem / QGraphicsItem.
         **/
        QPainterPath shape() const override;


    protected:

	// Data members
        const TreemapTile * _tile;

    }; // class TreemapSelectionRect


    /**
     * Highlighter for the treemap view's current tile.
     * This one is shared; it moves around from tile to tile.
     **/
    class CurrentTileHighlighter: public HighlightRect
    {
    public:
	CurrentTileHighlighter( TreemapView * treemapView ):
	    HighlightRect ( treemapView->scene(), treemapView->currentItemColor(), 2, CurrentHighlightLayer )
	{}

	void highlight( const TreemapTile * tile ) override;
    };


    /**
     * Highlighter for the treemap view's current item.
     *
     * This one is created on demand for each directory when the directory is
     * selected; this cannot be done in the tile's paint() method since the
     * tile will mostly be obscured by its children. This highlighter hovers
     * above the children as long as the directory is selected.
     **/
    class SelectedTileHighlighter: public HighlightRect
    {
    public:
	SelectedTileHighlighter( TreemapView * treemapView, const TreemapTile * tile ):
	    HighlightRect ( tile, treemapView->selectedItemsColor(), 2, TileHighlightLayer )
	{}
    };



    /**
     * Highlighter for the treemap view's parent tiles.  There will (sometimes) be a list
     * of these for all the parents of the current tile.  The first tile in the list,
     * the immediate parent of the current tile, the highlight is 2 pixels wide, all
     * the others just 1 pixel.
     **/
    class ParentTileHighlighter: public HighlightRect
    {
    public:
	ParentTileHighlighter( TreemapView * treemapView, const TreemapTile * tile, const QString & tooltip ):
	    HighlightRect ( tile, treemapView->highlightColor(), outlineWidth( treemapView ), SceneHighlightLayer )
	{
	    setToolTip( tooltip );
	}

    private:
	static int outlineWidth( TreemapView * treemapView )
	    { return treemapView->highlightedParent() ? 1 : 2; }
    };


    /**
     * Semi-transparent mask that covers the complete scene except for one
     * tile.
     **/
    class SceneMask: public QGraphicsPathItem
    {
    public:

        /**
         * Constructor: Create a semi-transparent mask that covers the complete
         * scene (the complete treemap), but leaves 'tile' uncovered.
         *
         * 'opacity' (0.0 .. 1.0) indicates how transparent the mask is:
         * 0.0 -> completely transparent; 1.0 -> solid.
         **/
        SceneMask( const TreemapTile * tile, float opacity );
    };

}	// namespace QDirStat


#endif // ifndef TreemapView_h
