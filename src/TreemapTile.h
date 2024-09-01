/*
 *   File name: TreemapTile.h
 *   Summary:   Treemap rendering for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef TreemapTile_h
#define TreemapTile_h

#include <QGraphicsRectItem>
#include <QVector>

#include "Typedefs.h" // FileSize


#define PAINT_DEBUGGING 0 // not completely thread-safe, do not use in release versions


class QGraphicsSceneMouseEvent;
class QTextStream;


namespace QDirStat
{
    class FileInfo;
    class BySizeIterator;
    class SelectedTileHighlighter;
    class TreemapView;

    enum Orientation
    {
	TreemapHorizontal,
	TreemapVertical,
    };

    /**
     * Lightweight class that contains a pre-calculated list of the cushion
     * heights for each depth in the tree.  It exposes an iterator so the
     * heights can be used in sequence and a special constLast() so that
     * iteration can be stopped on the last entry and can continue to be
     * used for any cushions created at a greater depth in the tree. The
     * list is only 10 entries long, but that is as deep as most trees and
     * there is very little visible change in cushion shading beyond that
     * point, if the tiles are even large enough to be visible.
     **/
    class CushionHeightSequence: public QVector<double>
    {
    public:
	CushionHeightSequence( double cushionHeight, double scaleFactor ):
	    QVector<double>( 10 ), // ten elements, not onex10.0
	    _constLast{ cend() - 1 } // iterator to last list entry
	{
	    // Start with the given cushion height, times 4 from the coefficients
	    double height = 4.0 * cushionHeight;

	    // Fill the sequence with heights calculated from the given scale factor
	    for ( auto it = begin(); it != end(); ++it, height *= scaleFactor )
		*it = height;
	}

	/**
	 * A convenience function so we can easily stop iterating at the last
	 * calculated height.
	 **/
	CushionHeightSequence::const_iterator constLast() const { return _constLast; }

    private:
	CushionHeightSequence::const_iterator _constLast;
    };


    /**
     * Helper class for cushioned treemaps: This class holds the polynome
     * parameters for the cushion surface. The height of each point of such a
     * surface is defined as:
     *
     *	   z(x, y) = a*x^2 + b*y^2 + c*x + d*y
     * or
     *	   z(x, y) = xx2*x^2 + yy2*y^2 + xx1*x + yy1*y
     *
     * to better keep track of which coefficient belongs where.
     **/
    class CushionSurface
    {
    public:

	/**
	 * Root tile constructor. All coefficients are set to 0 and the
	 * height to the start of the configured sequence.
	 **/
	CushionSurface( const CushionHeightSequence & heights ):
	    _xx2{ 0.0 },
	    _xx1{ 0.0 },
	    _yy2{ 0.0 },
	    _yy1{ 0.0 },
	    _height{ heights.cbegin() }
	{}

	/**
	 * Constructor for simple tiling, or the row cushion; copies
	 * the cushion from the parent tile and uses the next height
	 * in the sequence.
	 **/
	CushionSurface( const CushionSurface & parent, const CushionHeightSequence & heights ):
	    _xx2{ parent._xx2 },
	    _xx1{ parent._xx1 },
	    _yy2{ parent._yy2 },
	    _yy1{ parent._yy1 },
	    _height{ parent._height == heights.constLast() ? parent._height : parent._height + 1 }
	{}

	/**
	 * Adds a ridge of the specified height in dimension 'dir' within
	 * rectangle 'rect' to this surface.
	 *
	 * See the paper about "cushion treemaps" by Jarke J. van Wiik and
	 * Huub van de Wetering from the TU Eindhoven, NL for more details.
	 *
	 * If you don't want to get all that involved: the coefficients are
	 * changed in some way for each tile.
	 **/
	void addHorizontalRidge( double start, double end )
	{
	    const double reciprocal = coefficientReciprocal( start, end );
	    _xx2 -= squareCoefficient( reciprocal );
	    _xx1 += linearCoefficient( start, end, reciprocal );
	}
	void addVerticalRidge( double start, double end )
	{
	    const double reciprocal = coefficientReciprocal( start, end );
	    _yy2 -= squareCoefficient( reciprocal );
	    _yy1 += linearCoefficient( start, end, reciprocal );
	}

	/**
	 * Returns the polynomial coefficient of the second order for X
	 * direction.
	 **/
	double xx2() const { return _xx2; }

	/**
	 * Returns the polynomial coefficient of the first order for X
	 * direction.
	 **/
	double xx1() const { return _xx1; }

	/**
	 * Returns the polynomial coefficient of the second order for Y
	 * direction.
	 **/
	double yy2() const { return _yy2; }

	/**
	 * Returns the polynomial coefficient of the first order for Y
	 * direction.
	 **/
	double yy1() const { return _yy1; }


    protected:

	/**
	 * Helper function for calculating the polynomial coefficients.
	 * For speed, the reciprocal can be calculated just once for a pair of coefficients.
	 **/
	double coefficientReciprocal( double start, double end ) const
	    { return *_height / (end - start); }

	/**
	 * Calculate a new square polynomial coefficient.
	 * The full formula is -4h / (end - start)
	 * Currently a no-op kept for clarity, should be inlined away.
	 **/
	static double squareCoefficient( double reciprocal )
	    { return reciprocal; }

	/**
	 * Calculate a new linear polynomial coefficient.
	 * The full forumla is 4h * (start + end) / (end - start)
	 **/
	static double linearCoefficient( double start, double end, double reciprocal )
	    { return (start + end) * reciprocal; }

	// Data members

	double _xx2;
	double _xx1;
	double _yy2;
	double _yy1;

	CushionHeightSequence::const_iterator _height;

    }; // class CushionSurface


    /**
     * This is the basic building block of a treemap view: One single tile of a
     * treemap. If it corresponds to a leaf in the tree, it will be visible as
     * one tile (one rectangle) of the treemap. If it has children, it will be
     * subdivided again.
     **/
    class TreemapTile: public QGraphicsRectItem
    {

    protected:

	/**
	 * Constructor used for non-squarified children.  This is only
	 * used for delegation from the HorizontalTreemapTile and
	 * VerticalTreemapTile derived classes.
	 **/
	TreemapTile( TreemapTile  * parentTile,
	             FileInfo     * orig,
	             const QRectF & rect );

	/**
	 * Constructor used for squarified children.
	 **/
	TreemapTile( TreemapTile          * parentTile,
	             FileInfo             * orig,
	             const QRectF         & rect,
	             const CushionSurface & cushionSurface );

    public:

	/**
	 * Constructor: create a root treemap tile from 'orig' that fits into a
	 * rectangle 'rect'.  This is used for the root tile and is the only
	 * public constructor.
	 **/
	TreemapTile( TreemapView  * parentView,
	             FileInfo     * orig,
	             const QRectF & rect );

	/**
	 * Destructor.  Note that the highlighter is owned by the scene/view
	 * so nothing to do here.
	 **/
//	~TreemapTile() override = default;

	/**
	 * Returns a pointer to the original FileInfo item that corresponds to
	 * this treemap tile.
	 **/
	FileInfo * orig() const { return _orig; }

	/**
	 * Returns a pointer to the parent TreemapTile or 0 if there is none.
	 **/
	TreemapTile * parentTile() const { return static_cast<TreemapTile *>( parentItem() ); }

	/**
	 * Removes all the cushion surface pixmaps and plain tile brushes to force
	 * them to be re-rendered.
	 **/
	void invalidateCushions();

	/**
	 * Returns this tile's cushion surface parameters.
	 **/
	CushionSurface & cushionSurface() { return _cushionSurface; }

#if PAINT_DEBUGGING
	/**
	 * Sets a flag on the last tile that was constructed, for logging purposes.
	 * The flag will be set by the view after the map has finished building.
	 **/
	void setLastTile() { _lastTile = true; }
#endif

    protected:

	/**
	 * Create children (sub-tiles) using the simple treemap algorithm:
	 * alternate between horizontal and vertical subdivision in each
	 * level. Each child will get the entire height or width, respectively,
	 * of the specified rectangle. This algorithm is fast, but often
	 * results in very thin, elongated tiles.
	 **/
	void createChildrenHorizontal( const QRectF & rect );
	void createChildrenVertical( const QRectF & rect );

	/**
	 * Create a thread for rendering the cushions of the children of this tile.
	 *
	 * The algorithm adds threads for the largest possible tiles up to a certain
	 * threshold.  This attempts to balance having threads large enough to justify
	 * the overhead of creating them while ensuring that rendering begins early
	 * enough and in enough threads to speed up the whole process.  With more
	 * processors, it is more effective to spawn larger threads later in the build
	 * and have more of them running in parallel.  Very small tiles are also ignored
	 * to avoid multiple threads with almost no work.  Such tiles will be very
	 * quickly rendered in paint().
	 *
	 * The worst extremes are: small trees with only one child of the root, which
	 * are rendered in a single thread spawned when the build is essentially complete;
	 * they will be very fast with or without threading; and very "flat" trees such as
	 * the packages view, where essentially every packages gets a thread. Even in
	 * this last case, performance is good and despite the large number of threads
	 * created, they complete quickly enough that there are only generally a small
	 * number running in parallel.
	 **/
	void addRenderThread( TreemapTile * tile, int minThreadTileSize );

	/**
	 * Returns a pointer to the parent TreemapView.
	 **/
//	TreemapView * parentView() const { return _parentView; }

	/**
	 * Create children using the "squarified treemaps" algorithm as
	 * described by Mark Bruls, Kees Huizing, and Jarke J. van Wijk of the
	 * TU Eindhoven, NL.
	 *
	 * This algorithm is not quite so simple and involves more expensive
	 * operations, e.g., sorting the children of each node by size first,
	 * try some variations of the layout and maybe backtrack to the
	 * previous attempt. But it results in tiles that are much more
	 * square-like, i.e. have more reasonable width-to-height ratios. It is
	 * very much less likely to get thin, elongated tiles that are hard to
	 * point at and even harder to compare visually against each other.
	 *
	 * This implementation includes some improvements to that basic
	 * algorithm. For example, children below a certain size are
	 * disregarded completely since they will not get an adequate visual
	 * representation anyway (it would be way too small). They are
	 * summarized in some kind of 'misc stuff' area in the parent treemap
	 * tile - in fact, part of the parent directory's tile can be "seen
	 * through".
	 *
	 * In short, a lot of small children that don't have any useful effect
	 * for the user in finding wasted disk space are omitted from handling
	 * and, most important, don't need to be sorted by size (which has a
	 * cost of O(n*ln(n)) in the best case, so reducing n helps a lot).
	 **/
	void createSquarifiedChildren( const QRectF & rect );

	/**
	 * Lay out all members of 'row' within 'rect' along its longer side.
	 * Returns the new rectangle with the layouted area subtracted.
	 **/
	void layoutRow( Orientation      dir,
	                QRectF         & rect,
	                BySizeIterator & it,
	                const FileInfo * rowEnd,
	                FileSize         rowTotal,
	                double           primary,
	                double           secondary );

	/**
	 * Paint this tile.
	 *
	 * Reimplemented from QGraphicsRectItem.
	 **/
	void paint( QPainter                       * painter,
	            const QStyleOptionGraphicsItem * option,
	            QWidget                        * widget = nullptr ) override;

	/**
	 * Notification that item attributes (such as the 'selected' state)
	 * have changed.
	 *
	 * Reimplemented from QGraphicsItem.
	 **/
	QVariant itemChange( GraphicsItemChange   change,
	                     const QVariant     & value ) override;

	/**
	 * Mouse press event: Handle setting the current item.
	 *	left    button sets current item
	 *	middle  button highlights tile parents
	 *	right   brings up context menu and selects current item
	 *
	 * Reimplemented from QGraphicsItem.
	 **/
	void mousePressEvent( QGraphicsSceneMouseEvent * event ) override;

	/**
	 * Mouse double click event:
	 *	left    button double-click zooms in on the clicked tile,
	 *	middle  button double-click zooms out one level
	 *	right   button click brings up context menu, so nothing here
	 *
	 * Reimplemented from QGraphicsItem.
	 **/
	void mouseDoubleClickEvent( QGraphicsSceneMouseEvent * event ) override;

	/**
	 * Mouse release event: Handle marking item selection.
	 *	left    button selects tile (default for QGraphicsItem)
	 *	right   button brings up context menu, so nothing here
	 *
	 * Reimplemented from QGraphicsItem.
	 **/
	void mouseReleaseEvent( QGraphicsSceneMouseEvent * event ) override;

	/**
	 * Mouse wheel event: Zoom in or out
	 *
	 * Reimplemented from QGraphicsItem.
	 **/
	void wheelEvent( QGraphicsSceneWheelEvent * event) override;

	/**
	 * Context menu event.
	 *
	 * Reimplemented from QGraphicsItem.
	 **/
	void contextMenuEvent( QGraphicsSceneContextMenuEvent * event ) override;

	/**
	 * Hover enter event.
	 *
	 * Reimplemented from QGraphicsItem.
	 **/
	void hoverEnterEvent( QGraphicsSceneHoverEvent * ) override;

	/**
	 * Hover leave event.
	 *
	 * Reimplemented from QGraphicsItem.
	 **/
	void hoverLeaveEvent( QGraphicsSceneHoverEvent * ) override;

	/**
	 * Render a cushion as described in "cushioned treemaps" by Jarke
	 * J. van Wijk and Huub van de Wetering	 of the TU Eindhoven, NL.
	 **/
	QPixmap renderCushion( const QRectF & rect );

	/**
	 * Recursively iterate through all the children of this tile, rendering the cushions
	 * of any leaf-level tiles.
	 **/
	void renderChildCushions();

	/**
	 * Initialization common to all constructors.
	 **/
	void init();


    private:

	// Data members

	TreemapView * _parentView;
	FileInfo    * _orig;

#if PAINT_DEBUGGING
	bool          _firstTile;
	bool          _lastTile;
	QElapsedTimer _stopwatch;
#endif

	CushionSurface _cushionSurface;
	QPixmap        _cushion;

	SelectedTileHighlighter * _highlighter{ nullptr };

    }; // class TreemapTile

    /**
     * Derived class for tiles in the simple layout being laid out in the
     * horizontal direction.  The constructor delegates to the similar TreemapTile
     * constructor, the only difference being the createChildrenHorizontal() member
     * function that ends up getting called to layout the children.
     **/
    class HorizontalTreemapTile: private TreemapTile
    {
	friend class TreemapTile;

	/**
	 * Constructor for tiles in the simple layout being laid out in the
	 * horizontal direction.
	 **/
	HorizontalTreemapTile( TreemapTile * parentTile, FileInfo * orig, const QRectF & rect );
    };

    /**
     * Derived class for tiles in the simple layout being laid out in the
     * vertical direction.  The constructor delegates to the similar TreemapTile
     * constructor, the only difference being the createChildrenVertical() member
     * function that ends up getting called to layout the children.
     **/
    class VerticalTreemapTile: private TreemapTile
    {
	friend class TreemapTile;

	/**
	 * Constructor for tiles in the simple layout being laid out in the
	 * vertical direction.
	 **/
	VerticalTreemapTile( TreemapTile * parentTile, FileInfo * orig, const QRectF & rect );
    };

    inline QTextStream & operator<<( QTextStream & stream, TreemapTile * tile )
    {
	if ( tile )
	    stream << tile->orig();
	else
	    stream << "<NULL TreemapTile *>";

	return stream;
    }

}	// namespace QDirStat

#endif // ifndef TreemapTile_h
