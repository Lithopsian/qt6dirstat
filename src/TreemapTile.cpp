/*
 *   File name: TreemapTile.cpp
 *   Summary:   Treemap rendering for QDirStat
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#include <cmath> // round(), sqrt()

#include <QElapsedTimer>
#include <QGraphicsSceneMouseEvent>
#include <QImage>
#include <QMenu>
#include <QPainter>
#include <QtConcurrent/QtConcurrent>

#include "TreemapTile.h"
#include "ActionManager.h"
#include "FileInfoIterator.h"
#include "Logger.h"
#include "MimeCategorizer.h"
#include "SelectionModel.h"
#include "TreemapView.h"


using namespace QDirStat;


namespace
{
    /**
     * Returns a suitable color for 'file' based on a set of internal rules
     * (according to filename extension, MIME type or permissions).
     *
     * This function is defined here primarily to let the compiler inline
     * it as a performance-critical call.
     **/
    const QColor & tileColor( const TreemapView * parentView, const FileInfo * file )
    {
        if ( parentView->fixedColor().isValid() )
            return parentView->fixedColor();

        return MimeCategorizer::instance()->color( file );
    }


    /**
     * Try to include members referred to by 'it' into 'rect' so that they achieve
     * the most "square" appearance.  Items are added until the aspect ratio of the
     * first and last items doesn't get better any more.  Returns the total size of
     * the items for the row.
     **/
    FileSize squarify( const QRectF   & rect,
                       BySizeIterator & it,
                       FileSize         remainingTotal )
    {
        //logDebug() << "squarify() " << this << " " << rect << Qt::endl;

        // We only care about ratios, so scale everything for speed of calculation
        // rowHeightScale = rowHeight / remainingTotal, scale this to 1
        // rowWidthScale = rowWidth, scaled to rowWidth / rowHeight * remainingTotal
        const double width    = rect.width();
        const double height   = rect.height();
        const double rowRatio = width < height ? width / height : height / width;
        const double rowWidthScale = rowRatio * remainingTotal; // really rectWidth

        const FileSize firstSize = it->itemTotalSize();
        FileSize sum = 0LL;
        double bestAspectRatio = 0.0;
        while ( *it )
        {
            const FileSize size = it->itemTotalSize();
            if ( size > 0 )
            {
                sum += size;

                // Again, only ratios matter, so avoid the size / sum division by multiplying both by sum
                const double rowHeight = 1.0 * sum * sum; // * really sum * rowHeight / remainingTotal
                const double rowScale = rowWidthScale; // really rowWidth * size / sum
                const double aspectRatio = qMin( rowHeight / (rowScale * firstSize), rowScale * size / rowHeight );
                if ( aspectRatio < bestAspectRatio )
                {
                    // "Forget" the offending tile that made things worse
                    // Leave the iterator pointing to the first item after this row
                    sum -= size;
                    break;
                }

                // Aspect ratio of the two (or perhaps only one so far) end tiles still approaching one
                bestAspectRatio = aspectRatio;
            }

            ++it;
        }

        return sum;
    }

#if 0
    /**
     * Returns a color that gives a reasonable contrast to 'col': Lighter
     * if 'col' is dark, darker if 'col' is light.
     **/
    QRgb contrastingColor( QRgb col )
    {
        if ( qGray( col ) < 128 )
            return qRgb( qRed( col ) * 2, qGreen( col ) * 2, qBlue( col ) * 2 );
        else
            return qRgb( qRed( col ) / 2, qGreen( col ) / 2, qBlue( col ) / 2 );
    }


    /**
     * Check if the contrast of the specified image is sufficient to
     * visually distinguish an outline at the right and bottom borders
     * and add a grey line there, if necessary.
     **/
    void enforceContrast( QImage & image )
    {
        if ( image.width() > 5 )
        {
            // Check contrast along the right image boundary:
            //
            // Compare samples from the outmost boundary to samples a few pixels to
            // the inside and count identical pixel values. A number of identical
            // pixels are tolerated, but not too many.
            int x1 = image.width() - 6;
            int x2 = image.width() - 1;
            int interval = qMax( image.height() / 10, 5 );
            int sameColorCount = 0;

            // Take samples
            for ( int y = interval; y < image.height(); y+= interval )
            {
                if ( image.pixel( x1, y ) == image.pixel( x2, y ) )
                    ++sameColorCount;
            }

            if ( sameColorCount * 10 > image.height() )
            {
                // Add a line at the right boundary
                QRgb val = contrastingColor( image.pixel( x2, image.height() / 2 ) );
                for ( int y = 0; y < image.height(); y++ )
                    image.setPixel( x2, y, val );
            }
        }

        if ( image.height() > 5 )
        {
            // Check contrast along the bottom boundary

            int y1 = image.height() - 6;
            int y2 = image.height() - 1;
            int interval = qMax( image.width() / 10, 5 );
            int sameColorCount = 0;

            for ( int x = interval; x < image.width(); x += interval )
            {
                if ( image.pixel( x, y1 ) == image.pixel( x, y2 ) )
                    ++sameColorCount;
            }

            if ( sameColorCount * 10 > image.height() )
            {
                // Add a grey line at the bottom boundary
                QRgb val = contrastingColor( image.pixel( image.width() / 2, y2 ) );
                for ( int x = 0; x < image.width(); x++ )
                    image.setPixel( x, y2, val );
            }
        }
    }
#endif

    /**
     * Draws a thin outline.  Only draw on the top and left sides to keep the outline as
     * thin as possible.  Lines on small tiles will be drawn barrower than 1 pixel.  Using
     * painter->drawLine() is relatively slow, but the quality of these sub-pixel lines is
     * high.
     **/
    void drawOutline( QPainter * painter, const QRectF & rect, const QColor & color, int penScale )
    {
        // Draw the outline as thin as practical
        const qreal sizeForPen = qMin( rect.width(), rect.height() );
        const qreal penSize = sizeForPen < penScale ? sizeForPen / penScale : 1;
        painter->setPen( QPen{ color, penSize } );

        // Draw along only the top and left edges to avoid doubling the line thickness
        if ( rect.x() > 0 )
            painter->drawLine( rect.topLeft(), rect.bottomLeft() );
        if ( rect.y() > 0 )
            painter->drawLine( rect.topLeft(), rect.topRight() );
    }

} // namespace


TreemapTile::TreemapTile( TreemapView  * parentView,
                          FileInfo     * orig,
                          const QRectF & rect ):
    QGraphicsRectItem{ rect },
    _parentView{ parentView },
    _orig{ orig },
#if PAINT_DEBUGGING
    _firstTile{ true },
    _lastTile{ false },
#endif
    _cushionSurface{ _parentView->cushionHeights() } // initial cushion surface
{
    //logDebug() << "Creating root tile " << orig << "    " << rect << Qt::endl;

    // constructor with no parent tile, only used for the root tile
    init();

    if ( _parentView->squarify() )
        createSquarifiedChildren(rect);
    else if ( rect.width() > rect.height() )
        createChildrenHorizontal( rect );
    else
        createChildrenVertical( rect );

//    _stopwatch.start();

//    _parentView->threadPool()->waitForDone(); // destructor already waits

//    logDebug() << _stopwatch.restart() << "ms for " << threads << " threads to finish" << (_parentView->treemapCancelled() ? " (cancelled)" : QString{}) << Qt::endl;
}


// constructor for simple (non-squarified) children
TreemapTile::TreemapTile( TreemapTile  * parentTile,
                          FileInfo     * orig,
                          const QRectF & rect ):
    QGraphicsRectItem{ rect, parentTile },
    _parentView{ parentTile->_parentView },
    _orig{ orig },
#if PAINT_DEBUGGING
    _firstTile{ false },
    _lastTile{ false },
#endif
    _cushionSurface{ parentTile->_cushionSurface, _parentView->cushionHeights() } // copy the parent cushion and scale the height
{
//    logDebug() << "Creating non-squarified child for " << orig << " (in " << parentTile->_orig << ")" << Qt::endl;

    init();
}


HorizontalTreemapTile::HorizontalTreemapTile( TreemapTile  * parentTile,
                                              FileInfo     * orig,
                                              const QRectF & rect ) :
    TreemapTile{ parentTile, orig, rect }
{
    if ( orig->isDirInfo() )
        createChildrenHorizontal( rect );
}


VerticalTreemapTile::VerticalTreemapTile( TreemapTile  * parentTile,
                                          FileInfo     * orig,
                                          const QRectF & rect ) :
    TreemapTile{ parentTile, orig, rect }
{
    if ( orig->isDirInfo() )
        createChildrenVertical( rect );
}


TreemapTile::TreemapTile( TreemapTile          * parentTile,
                          FileInfo             * orig,
                          const QRectF         & rect,
                          const CushionSurface & cushionSurface ):
    QGraphicsRectItem{ rect, parentTile },
    _parentView{ parentTile->_parentView },
    _orig{ orig },
#if PAINT_DEBUGGING
    _firstTile{ false },
    _lastTile{ false },
#endif
    _cushionSurface{ cushionSurface } // uses the default copy constructor on a row cushion
{
    //logDebug() << "Creating squarified tile for " << orig << "  " << rect << Qt::endl;

    // constructor for squarified layout, with the cushion specified explicitly to allow for a row cushion
    init();

    if ( orig->isDirInfo() )
        createSquarifiedChildren( rect );
}


void TreemapTile::init()
{
    setPen( Qt::NoPen );

#if PAINT_DEBUGGING
    _parentView->setLastTile( this ); // only for logging
#endif

    setFlags( ItemIsSelectable );

    if ( ( _orig->isDir() && _orig->totalSubDirsConst() == 0 ) || _orig->isDotEntry() )
        setAcceptHoverEvents( true );
}


void TreemapTile::createChildrenHorizontal( const QRectF & rect )
{
    BySizeIterator it{ _orig };
    FileSize totalSize = it.totalSize();

    if ( totalSize == 0 )
        return;

    _cushionSurface.addVerticalRidge( rect.top(), rect.bottom() );

    // All stripes are scaled by the same amount
    const double width = rect.width();
    const double scale = width / totalSize;

    // To avoid rounding errors accumulating, every tile is positioned relative to the parent
    // Items that don't reach a pixel from the previous item are silently dropped
    FileSize cumulativeSize = 0LL;
    double offset = 0.0;
    double nextOffset = qMin( width, _parentView->minTileSize() );
    while ( *it && offset < width )
    {
        cumulativeSize += it->itemTotalSize();
        const double newOffset = std::round( scale * cumulativeSize );
        if ( newOffset >= nextOffset && !_parentView->treemapCancelled() )
        {
            QRectF childRect{ rect.left() + offset, rect.top(), newOffset - offset, rect.height() };
            TreemapTile * tile = new VerticalTreemapTile{ this, *it, childRect };
            tile->cushionSurface().addHorizontalRidge( childRect.left(), childRect.right() );

            if ( it->isDirInfo() )
                addRenderThread( tile, 4 );
//                tile->_cushion = tile->renderCushion( childRect );

            offset = newOffset;
            nextOffset = qMin( static_cast<double>( rect.width() ), newOffset + _parentView->minTileSize() );
        }

        ++it;
    }
}


void TreemapTile::createChildrenVertical( const QRectF & rect )
{
    BySizeIterator it{ _orig };
    FileSize totalSize = it.totalSize();

    if (totalSize == 0)
        return;

    _cushionSurface.addHorizontalRidge( rect.left(), rect.right() );

    // All stripes are scaled by the same amount
    const double height = rect.height();
    const double scale = height / totalSize;

    // To avoid rounding errors accumulating, every tile is positioned relative to the parent
    // Items that don't reach a pixel from the previous item are silently dropped
    FileSize cumulativeSize = 0LL;
    double offset = 0.0;
    double nextOffset = qMin( height, _parentView->minTileSize() );
    while ( *it && offset < height )
    {
        cumulativeSize += it->itemTotalSize();
        const double newOffset = std::round( scale * cumulativeSize );
        if ( newOffset >= nextOffset && !_parentView->treemapCancelled() )
        {
            QRectF childRect{ rect.left(), rect.top() + offset, rect.width(), newOffset - offset };
            TreemapTile * tile = new HorizontalTreemapTile{ this, *it, childRect };
            tile->cushionSurface().addVerticalRidge( childRect.top(), childRect.bottom() );

            if ( it->isDirInfo() )
                addRenderThread( tile, 4 );
//                tile->_cushion = tile->renderCushion( childRect );

            offset = newOffset;
            nextOffset = qMin( static_cast<double>( rect.height() ), newOffset + _parentView->minTileSize() );
        }

        ++it;
    }
}


void TreemapTile::createSquarifiedChildren( const QRectF & rect )
{
    // Get all the children of this tile and total them up
    BySizeIterator it{ _orig };
    FileSize remainingTotal = it.totalSize();

    // Don't show completely empty directories in the treemap, avoids divide by zero issues
    if ( remainingTotal == 0 )
        return;

    QRectF childrenRect = rect;
    const FileInfo * rowEnd = *it;
    while ( rowEnd && childrenRect.height() >= 0 && childrenRect.width() >= 0 )
    {
        // Square treemaps always layout the next row of tiles along the shortest dimension
        const Orientation dir = childrenRect.width() < childrenRect.height() ? TreemapHorizontal : TreemapVertical;
        const double primary = dir == TreemapHorizontal ? childrenRect.width() : childrenRect.height();
        const double secondary = dir == TreemapHorizontal ? childrenRect.height() : childrenRect.width();

        // Find the set of items that fill a row with tiles as near as possible to squares
        auto rowStartIt = it.currentPos();
        FileSize rowTotal = squarify( childrenRect, it, remainingTotal );

        // Rows 0.5-1.0 pixels high all get rounded up so we'll probably run out of space, but just in case ...
        // ... rows < 0.5 pixels high will never get rounded up, so force them
        double height = secondary * rowTotal / remainingTotal;
        while ( height <= _parentView->minSquarifiedTileHeight() && height < secondary )
        {
            // Aspect ratio hardly matters any more, so fast forward enough items to make half a pixel
            // (many of these tiny items will be dropped while laying out a row of tiles)
            if ( *it )
            {
                rowTotal += it->itemTotalSize();
                ++it;
            }
            else
                // If we run out of items, force the dregs to take up any space still left
                rowTotal = remainingTotal;

            height = secondary * rowTotal / remainingTotal;
        }
        rowEnd = *it;

        it.setPos( rowStartIt );
        layoutRow( dir, childrenRect, it, rowEnd, rowTotal, primary, std::round( height ) );

        remainingTotal -= rowTotal;
    }
}


void TreemapTile::layoutRow( Orientation      dir,
                             QRectF         & rect,
                             BySizeIterator & it,
                             const FileInfo * rowEnd,
                             FileSize         rowTotal,
                             double           primary,
                             double           height )
{

    //logDebug() << this << " - " << rect << " - height= " << height << Qt::endl;

    const double rectX = rect.x();
    const double rectY = rect.y();

    // All the row tiles have the same coefficients on the short axis of the row
    // .. so just calculate them once on a hypothetical row cushion
    CushionSurface rowCushionSurface{ _cushionSurface, _parentView->cushionHeights() };
    if ( dir == TreemapHorizontal )
    {
        const double newY = rectY + height;
        rowCushionSurface.addVerticalRidge( rectY, newY );
        rect.setY( newY );
    }
    else
    {
        const double newX = rectX + height;
        rowCushionSurface.addHorizontalRidge( rectX, newX );
        rect.setX( newX );
    }

    // logDebug() << this << " - " << rect << " - height= " << height << Qt::endl;

    const double rowScale = primary / rowTotal;
    double cumulativeSize = 0;
    double offset = 0;
    double nextOffset = qMin( primary, _parentView->minTileSize() );
    while ( *it != rowEnd && offset < primary )
    {
        // Position tiles relative to the row start based on the cumulative size of tiles
        //logDebug() << rect << *it << Qt::endl;
        cumulativeSize += it->itemTotalSize();
        const double newOffset = std::round( cumulativeSize * rowScale );

        // Drop tiles that don't reach to the minimum pixel size or fill the row
        if ( newOffset >= nextOffset && !_parentView->treemapCancelled() )
        {
            QRectF childRect = dir == TreemapHorizontal ?
                QRectF{ rectX + offset, rectY, newOffset - offset, height } :
                QRectF{ rectX, rectY + offset, height, newOffset - offset };

            TreemapTile * tile = new TreemapTile{ this, *it, childRect, rowCushionSurface };

            // Don't need to finish calculating cushions once all the leaf-level children have been created
            if ( it->isDirInfo() )
//                tile->_cushion = tile->renderCushion( childRect );
                addRenderThread( tile, 6 );
            else if ( dir == TreemapHorizontal )
                tile->_cushionSurface.addHorizontalRidge( childRect.left(), childRect.right() );
            else
                tile->_cushionSurface.addVerticalRidge( childRect.top(), childRect.bottom() );

            offset = newOffset;
            nextOffset = qMin( primary, newOffset + _parentView->minTileSize() );
        }

        ++it;
    }
}


void TreemapTile::addRenderThread( TreemapTile * tile, int minThreadTileSize )
{
    // If the tile's parent is smaller than the threshold and not the root tile, then no thread
    if ( parentItem() && rect().width() < _parentView->maxTileThreshold() &&
         rect().height() < _parentView->maxTileThreshold() )
        return;

    // Not worth a thread for a tiny directory
    if ( tile->rect().width() < minThreadTileSize || tile->rect().height() < minThreadTileSize )
        return;

    // If the tile itself is larger than the threshold and its children are sub-directories, no thread
    if ( ( tile->rect().width() >= _parentView->maxTileThreshold() ||
           tile->rect().height() >= _parentView->maxTileThreshold() ) &&
         ( !tile->_orig->firstChild() || tile->_orig->firstChild()->isDirInfo() ) )
         return;

    //logDebug() << QThreadPool::globalInstance()->activeThreadCount() << " threads active" << Qt::endl;
    // The API changed in Qt6!
#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
    QtConcurrent::run( _parentView->threadPool(), tile, &TreemapTile::renderChildCushions );
#else
    std::ignore = QtConcurrent::run( _parentView->threadPool(), &TreemapTile::renderChildCushions, tile );
#endif
}


void TreemapTile::renderChildCushions()
{
    if ( _parentView->treemapCancelled() )
        return;

    const auto items = childItems();
    for ( QGraphicsItem * graphicsItem : items )
    {
        // nothing other than tiles in the tree at this point
        TreemapTile * tile = static_cast<TreemapTile * >( graphicsItem );

        if ( tile->_orig->isDirInfo() )
            tile->renderChildCushions();
        else if ( _parentView->doCushionShading() )
            tile->_cushion = tile->renderCushion( tile->rect() );
        else
            //tile->_pixmap = tile->renderPlainTile( tile->rect() );
            tile->setBrush( tileColor( _parentView, tile->_orig ) );
    }
}


QPixmap TreemapTile::renderCushion( const QRectF & rect )
{
    //logDebug() << rect << Qt::endl;

    const QColor & color = tileColor( _parentView, _orig );

    // These don't need rounding, they're already whole pixels, but make the narrowing explicit
    const int width = static_cast<int>( rect.width() );
    const int height = static_cast<int>( rect.height() );
    QImage image{ width, height, QImage::Format_RGB32 };
    QRgb * data = reinterpret_cast<QRgb *>( image.bits() );

    const double xx22 = 2.0 * _cushionSurface.xx2();
    const double yy22 = 2.0 * _cushionSurface.yy2();
    const double nx0 = _cushionSurface.xx1() + xx22 * ( rect.x() + 0.5 );
    const double ny0 = _cushionSurface.yy1() + yy22 * ( rect.y() + 0.5 );

    double nx, ny;
    int x, y;
    for ( y = 0, ny = ny0; y < rect.height(); ++y, ny += yy22 )
    {
        for ( x = 0, nx = nx0; x < rect.width(); ++data, ++x, nx += xx22 )
        {
            const double num   = _parentView->lightZ() + ny*_parentView->lightY() + nx*_parentView->lightX();
            const double denom = sqrt( nx*nx + ny*ny + 1.0 );
            const double cosa  = _parentView->ambientIntensity() + qMax( 0.0, num / denom );

            const int red   = 0.5 + cosa * color.red();
            const int green = 0.5 + cosa * color.green();
            const int blue  = 0.5 + cosa * color.blue();
            *data = qRgb( red, green, blue );
        }
    }

//    if ( _parentView->enforceContrast() )
//        enforceContrast( image );

    return QPixmap::fromImage( image );
}


void TreemapTile::invalidateCushions()
{
    _cushion = QPixmap{};
    setBrush( QBrush{} );

    const auto items = childItems();
    for ( QGraphicsItem * graphicsItem : items )
    {
        TreemapTile * tile = dynamic_cast<TreemapTile *>( graphicsItem );
        if ( tile )
            tile->invalidateCushions();
    }
}


void TreemapTile::paint( QPainter                       * painter,
                         const QStyleOptionGraphicsItem * option,
                         QWidget                        * widget )
{
#if PAINT_DEBUGGING
    if ( _firstTile )
    {
        logDebug() << Qt::endl;
        _parentView->rootTile()->_stopwatch.start();
    }
#endif
    // Don't paint tiles with children, the children will cover the parent, but double-check
    // it actually has child tiles (no tile will be created for zero-sized children)
    if ( _orig->hasChildren() && childItems().size() > 0 )
    {
//        logDebug() << this << "(" << boundingRect() << ")" << ( isObscured() ? " - obscured" : QString{} ) << Qt::endl;
        return;
    }

    const QRectF rect = QGraphicsRectItem::rect();

    if ( _orig->isDirInfo() )
    {
//        logDebug() << _parentView->rootTile()->_stopwatch.restart() << "ms for " << rect << Qt::endl;

        // Relatively rare visible directory, fill it with a gradient or plain colour
        if ( brush().style() == Qt::NoBrush )
            setBrush( _parentView->dirBrush() );
        QGraphicsRectItem::paint( painter, option, widget );

        // Outline otherwise completely-plain empty-directory tiles
        if ( brush().style() == Qt::SolidPattern && _parentView->outlineColor().isValid() )
            drawOutline( painter, rect, _parentView->outlineColor(), 5 );
    }
    else if ( _parentView->doCushionShading() )
    {
        //logDebug() << rect << ", " << opaqueArea().boundingRect() << Qt::endl;

        // The cushion pixmap is rendered when the treemap is built, but may be deleted to re-colour the map
        if ( _cushion.isNull() )
            _cushion = renderCushion( rect );

        if ( !_cushion.isNull() )
            painter->drawPixmap( rect.topLeft(), _cushion );

        // Draw a clearly visible tile boundary if configured
        if ( _parentView->forceCushionGrid() )
            drawOutline( painter, rect, _parentView->cushionGridColor(), 10 );
    }
    else
    {
        if ( brush().style() == Qt::NoBrush )
            setBrush( tileColor( _parentView, _orig ) );
        QGraphicsRectItem::paint( painter, option, widget );

        // Always try to draw an outline since there is no other indication of the tiles
        if ( _parentView->outlineColor().isValid() )
            drawOutline( painter, rect, _parentView->outlineColor(), 5 );
    }

    if ( isSelected() )
    {
        //logDebug() << " highlight " << _parentView->rootTile()->_stopwatch.restart() << "ms for " << rect << Qt::endl;

        // Highlight this tile. This only makes sense if this is a leaf
        // tile, (i.e. if the corresponding FileInfo doesn't have any
        // children) because otherwise the children will obscure this
        // tile anyway. In that case, we have to rely on a HighlightRect
        // to be created. But we can save some memory if we don't do
        // that for every tile, so we draw that highlight frame manually
        // if this is a leaf tile.
        painter->setBrush( Qt::NoBrush );
        QRectF selectionRect{ rect };
        selectionRect.setSize( rect.size() - QSizeF{ 1.0, 1.0 } );
        painter->setPen( QPen{ _parentView->selectedItemsColor(), 1 } );
        painter->drawRect( selectionRect );
    }

#if PAINT_DEBUGGING
    if (_lastTile)
        logDebug() << _parentView->rootTile()->_stopwatch.restart() << "ms" << Qt::endl;
#endif
}


QVariant TreemapTile::itemChange( GraphicsItemChange   change,
                                  const QVariant     & value)
{
    //logDebug() << this << Qt::endl;

    if ( change == ItemSelectedChange &&
         _orig->hasChildren() &&             // tiles with no children are highlighted in paint()
         this != _parentView->rootTile() )   // don't highlight the root tile
    {
        const bool selected = value.toBool();
        //logDebug() << this << ( selected ? " is selected" : " is deselected" ) << Qt::endl;

        if ( !selected )
        {
            delete _highlighter;
            _highlighter = nullptr;
        }
        else if ( !_highlighter )
        {
            //logDebug() << "Creating highlighter for " << this << Qt::endl;
            _highlighter = new SelectedTileHighlighter{ _parentView, this };
        }
    }

    return QGraphicsRectItem::itemChange( change, value );
}


void TreemapTile::mousePressEvent( QGraphicsSceneMouseEvent * event )
{
    if ( !_parentView->selectionModel() )
        return;

    switch ( event->button() )
    {
        case Qt::LeftButton:
            //logDebug() << this << " mouse pressed (" << event->modifiers() << ")" << Qt::endl;

            // isSelected() is unreliable here since in QGraphicsItem some
            // stuff is done in the mousePressEvent, while some other stuff is
            // done in the mouseReleaseEvent. Just setting the current item
            // here to avoid having a yellow highlighter rectangle upon mouse
            // press and then a red one upon mouse release. No matter if the
            // item ends up selected or not, the mouse press makes it the
            // current item, so let's update the red highlighter rectangle
            // here.
            QGraphicsRectItem::mousePressEvent( event );
            _parentView->setCurrentTile( this );
            break;

        case Qt::MiddleButton:
            // logDebug() << "Middle click on " << _orig << Qt::endl;

            // Handle item selection (with or without Ctrl) ourselves here;
            // unlike for a left click, the QGraphicsItem base class does
            // not do this for us already.
            if ( ( event->modifiers() & Qt::ControlModifier ) == 0 )
                scene()->clearSelection();
            setSelected( !isSelected() );
            _parentView->toggleParentsHighlight( this );
            _parentView->setCurrentTile( this );
            break;

        case Qt::RightButton:
            // logDebug() << this << " right mouse pressed" << Qt::endl;
            _parentView->setCurrentTile( this );
            _parentView->sendSelection( this ); // won't get a mouse release for this
            break;

        case Qt::BackButton:
        case Qt::ForwardButton:
            event->ignore();
            break;

        default:
            QGraphicsRectItem::mousePressEvent( event );
            break;
    }
}


void TreemapTile::mouseDoubleClickEvent( QGraphicsSceneMouseEvent * event )
{
    if ( !_parentView->selectionModel() )
        return;

    switch ( event->button() )
    {
        case Qt::LeftButton:
            // logDebug() << "Zooming treemap in" << Qt::endl;
            _parentView->zoomIn();
            break;

        case Qt::MiddleButton:
            // logDebug() << "Zooming treemap out" << Qt::endl;
            _parentView->zoomOut();
            break;

        case Qt::RightButton:
            // This doesn't work at all since the first click already opens the
            // context menu which grabs the focus to that pop-up menu.
        case Qt::BackButton:
        case Qt::ForwardButton:
            // Used for history navigation, so ignore here (in mousePressEvent)
            // or the history button doesn't get the second click (and we also
            // get an unnecessary release event here)
        default:
            QGraphicsRectItem::mousePressEvent( event );
            break;
    }
}


void TreemapTile::mouseReleaseEvent( QGraphicsSceneMouseEvent * event )
{
    if ( !_parentView->selectionModel() )
        return;

    switch ( event->button() )
    {
        case Qt::LeftButton:
            // The current item was already set in the mouse press event,
            // but the selected status might be changed on the release.
            QGraphicsRectItem::mouseReleaseEvent( event );
            _parentView->setCurrentTile( this );
            // logDebug() << this << " clicked; selected: " << isSelected() << Qt::endl;
            break;

        default:
            break;
    }

    _parentView->sendSelection( this );
}


void TreemapTile::wheelEvent( QGraphicsSceneWheelEvent * event )
{
    if ( !_parentView->selectionModel() )
        return;

    if ( event->delta() < 0 )
    {
        _parentView->zoomOut();
    }
    else
    {
        // If no current item, or it is the root already, pick a new current item so we can zoom
        FileInfo * currentItem = _parentView->selectionModel()->currentItem();
        if ( !currentItem || currentItem == _parentView->rootTile()->_orig )
            if ( this->parentItem() != _parentView->rootTile() ) // ... unless we just can't zoom any further
            _parentView->setCurrentTile( this );

        _parentView->zoomIn();
    }
}


void TreemapTile::contextMenuEvent( QGraphicsSceneContextMenuEvent * event )
{
    if ( !_parentView->selectionModel() )
        return;

    FileInfoSet selectedItems = _parentView->selectionModel()->selectedItems();
    if ( !selectedItems.contains( _orig ) )
    {
        //logDebug() << "Abandoning old selection" << Qt::endl;
        _parentView->selectionModel()->setCurrentItem( _orig, true );
        selectedItems = _parentView->selectionModel()->selectedItems();
    }

    //logDebug() << "Context menu for " << this << Qt::endl;

    // The first action should not be a destructive one like "move to trash":
    // It's just too easy to select and execute the first action accidentially,
    // especially on a laptop touchpad.
    const QStringList actions{ "actionTreemapZoomTo",
                               "actionTreemapZoomIn",
                               "actionTreemapZoomOut",
                               "actionResetTreemapZoom",
                               ActionManager::separator(),
                               "actionCopyPath",
                               "actionMoveToTrash",
                             };

    const QStringList enabledActions{ ActionManager::separator(),
                                      ActionManager::cleanups(),
                                    };

    QMenu * menu = ActionManager::createMenu( actions, enabledActions );
    menu->exec( event->screenPos() );
}


void TreemapTile::hoverEnterEvent( QGraphicsSceneHoverEvent * )
{
    // logDebug() << "Hovering over " << this << Qt::endl;
    _parentView->sendHoverEnter( _orig );
}


void TreemapTile::hoverLeaveEvent( QGraphicsSceneHoverEvent * )
{
    // logDebug() << "  Leaving " << this << Qt::endl;
    _parentView->sendHoverLeave( _orig );
}
