/****************************************************************************
** Copyright (C) 2001-2013 Klaralvdalens Datakonsult AB.  All rights reserved.
**
** This file is part of the KD Chart library.
**
** Licensees holding valid commercial KD Chart licenses may use this file in
** accordance with the KD Chart Commercial License Agreement provided with
** the Software.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.GPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** Contact info@kdab.com if any conditions of this licensing are not
** clear to you.
**
**********************************************************************/

#include <QDebug>
#include <QPainter>
#include <QStack>

#include "KDChartPieDiagram.h"
#include "KDChartPieDiagram_p.h"

#include "KDChartPaintContext.h"
#include "KDChartPieAttributes.h"
#include "KDChartPolarCoordinatePlane_p.h"
#include "KDChartThreeDPieAttributes.h"
#include "KDChartPainterSaver_p.h"

#include <KDABLibFakes>


using namespace KDChart;

PieDiagram::Private::Private()
  : labelDecorations( PieDiagram::NoDecoration ),
    isCollisionAvoidanceEnabled( false )
{
}

PieDiagram::Private::~Private() {}

#define d d_func()

PieDiagram::PieDiagram( QWidget* parent, PolarCoordinatePlane* plane ) :
    AbstractPieDiagram( new Private(), parent, plane )
{
    init();
}

PieDiagram::~PieDiagram()
{
}

void PieDiagram::init()
{
}

/**
  * Creates an exact copy of this diagram.
  */
PieDiagram * PieDiagram::clone() const
{
    return new PieDiagram( new Private( *d ) );
}

void PieDiagram::setLabelDecorations( LabelDecorations decorations )
{
    d->labelDecorations = decorations;
}

PieDiagram::LabelDecorations PieDiagram::labelDecorations() const
{
    return d->labelDecorations;
}

void PieDiagram::setLabelCollisionAvoidanceEnabled( bool enabled )
{
    d->isCollisionAvoidanceEnabled = enabled;
}

bool PieDiagram::isLabelCollisionAvoidanceEnabled() const
{
    return d->isCollisionAvoidanceEnabled;
}

const QPair<QPointF, QPointF> PieDiagram::calculateDataBoundaries () const
{
    if ( !checkInvariants( true ) || model()->rowCount() < 1 ) return QPair<QPointF, QPointF>( QPointF( 0, 0 ), QPointF( 0, 0 ) );

    const PieAttributes attrs( pieAttributes() );

    QPointF bottomLeft( QPointF( 0, 0 ) );
    QPointF topRight;
    // If we explode, we need extra space for the slice that has the largest explosion distance.
    if ( attrs.explode() ) {
        const int colCount = columnCount();
        qreal maxExplode = 0.0;
        for ( int j = 0; j < colCount; ++j ) {
            const PieAttributes columnAttrs( pieAttributes( model()->index( 0, j, rootIndex() ) ) ); // checked
            maxExplode = qMax( maxExplode, columnAttrs.explodeFactor() );
        }
        topRight = QPointF( 1.0 + maxExplode, 1.0 + maxExplode );
    } else {
        topRight = QPointF( 1.0, 1.0 );
    }
    return QPair<QPointF, QPointF> ( bottomLeft,  topRight );
}


void PieDiagram::paintEvent( QPaintEvent* )
{
    QPainter painter ( viewport() );
    PaintContext ctx;
    ctx.setPainter ( &painter );
    ctx.setRectangle( QRectF ( 0, 0, width(), height() ) );
    paint ( &ctx );
}

void PieDiagram::resizeEvent( QResizeEvent* )
{
}

void PieDiagram::resize( const QSizeF& )
{
}

void PieDiagram::paint( PaintContext* ctx )
{
    // Painting is a two stage process
    // In the first stage we figure out how much space is needed
    // for text labels.
    // In the second stage, we make use of that information and
    // perform the actual painting.
    placeLabels( ctx );
    paintInternal( ctx );
}

void PieDiagram::calcSliceAngles()
{
    // determine slice positions and sizes
    const qreal sum = valueTotals();
    const qreal sectorsPerValue = 360.0 / sum;
    const PolarCoordinatePlane* plane = polarCoordinatePlane();
    qreal currentValue = plane ? plane->startPosition() : 0.0;

    const int colCount = columnCount();
    d->startAngles.resize( colCount );
    d->angleLens.resize( colCount );

    bool atLeastOneValue = false; // guard against completely empty tables
    for ( int iColumn = 0; iColumn < colCount; ++iColumn ) {
        bool isOk;
        const qreal cellValue = qAbs( model()->data( model()->index( 0, iColumn, rootIndex() ) ) // checked
            .toReal( &isOk ) );
        // toReal() returns 0.0 if there was no value or a non-numeric value
        atLeastOneValue = atLeastOneValue || isOk;

        d->startAngles[ iColumn ] = currentValue;
        d->angleLens[ iColumn ] = cellValue * sectorsPerValue;

        currentValue = d->startAngles[ iColumn ] + d->angleLens[ iColumn ];
    }

    // If there was no value at all, this is the sign for other code to bail out
    if ( !atLeastOneValue ) {
        d->startAngles.clear();
        d->angleLens.clear();
    }
}

void PieDiagram::calcPieSize( const QRectF &contentsRect )
{
    d->size = qMin( contentsRect.width(), contentsRect.height() );

    // if any slice explodes, the whole pie needs additional space so we make the basic size smaller
    qreal maxExplode = 0.0;
    const int colCount = columnCount();
    for ( int j = 0; j < colCount; ++j ) {
        const PieAttributes columnAttrs( pieAttributes( model()->index( 0, j, rootIndex() ) ) ); // checked
        maxExplode = qMax( maxExplode, columnAttrs.explodeFactor() );
    }
    d->size /= ( 1.0 + 1.0 * maxExplode );

    if ( d->size < 0.0 ) {
        d->size = 0;
    }
}

// this is the rect of the top surface of the pie, i.e. excluding the "3D" rim effect.
QRectF PieDiagram::twoDPieRect( const QRectF &contentsRect, const ThreeDPieAttributes& threeDAttrs ) const
{
    QRectF pieRect;
    if ( !threeDAttrs.isEnabled() ) {
        qreal x = ( contentsRect.width() - d->size ) / 2.0;
        qreal y = ( contentsRect.height() - d->size ) / 2.0;
        pieRect = QRectF( contentsRect.left() + x, contentsRect.top() + y, d->size, d->size );
    } else {
        // threeD: width is the maximum possible width; height is 1/2 of that
        qreal sizeFor3DEffect = 0.0;

        qreal x = ( contentsRect.width() - d->size ) / 2.0;
        qreal height = d->size;
        // make sure that the height plus the threeDheight is not more than the
        // available size
        if ( threeDAttrs.depth() >= 0.0 ) {
            // positive pie height: absolute value
            sizeFor3DEffect = threeDAttrs.depth();
            height = d->size - sizeFor3DEffect;
        } else {
            // negative pie height: relative value
            sizeFor3DEffect = - threeDAttrs.depth() / 100.0 * height;
            height = d->size - sizeFor3DEffect;
        }
        qreal y = ( contentsRect.height() - height - sizeFor3DEffect ) / 2.0;

        pieRect = QRectF( contentsRect.left() + x, contentsRect.top() + y, d->size, height );
    }
    return pieRect;
}

void PieDiagram::placeLabels( PaintContext* paintContext )
{
    if ( !checkInvariants(true) || model()->rowCount() < 1 ) {
        return;
    }
    if ( paintContext->rectangle().isEmpty() || valueTotals() == 0.0 ) {
        return;
    }

    const ThreeDPieAttributes threeDAttrs( threeDPieAttributes() );
    const int colCount = columnCount();

    d->reverseMapper.clear(); // on first call, this sets up the internals of the ReverseMapper.

    calcSliceAngles();
    if ( d->startAngles.isEmpty() ) {
        return;
    }

    calcPieSize( paintContext->rectangle() );

    // keep resizing the pie until the labels and the pie fit into paintContext->rectangle()

    bool tryAgain = true;
    while ( tryAgain ) {
        tryAgain = false;

        QRectF pieRect = twoDPieRect( paintContext->rectangle(), threeDAttrs );
        d->forgetAlreadyPaintedDataValues();
        d->labelPaintCache.clear();

        for ( int slice = 0; slice < colCount; slice++ ) {
            if ( d->angleLens[ slice ] != 0.0 ) {
                const QRectF explodedPieRect = explodedDrawPosition( pieRect, slice );
                addSliceLabel( &d->labelPaintCache, explodedPieRect, slice );
            }
        }

        QRectF textBoundingRect;
        d->paintDataValueTextsAndMarkers( paintContext, d->labelPaintCache, false, true,
                                          &textBoundingRect );
        if ( d->isCollisionAvoidanceEnabled ) {
            shuffleLabels( &textBoundingRect );
        }

        if ( !textBoundingRect.isEmpty() && d->size > 0.0 ) {
            const QRectF &clipRect = paintContext->rectangle();
            // see by how many pixels the text is clipped on each side
            qreal right = qMax( qreal( 0.0 ), textBoundingRect.right() - clipRect.right() );
            qreal left = qMax( qreal( 0.0 ), clipRect.left() - textBoundingRect.left() );
            // attention here - y coordinates in Qt are inverted compared to the convention in maths
            qreal top = qMax( qreal( 0.0 ), clipRect.top() - textBoundingRect.top() );
            qreal bottom = qMax( qreal( 0.0 ), textBoundingRect.bottom() - clipRect.bottom() );
            qreal maxOverhang = qMax( qMax( right, left ), qMax( top, bottom ) );

            if ( maxOverhang > 0.0 ) {
                // subtract 2x as much because every side only gets half of the total diameter reduction
                // and we have to make up for the overhang on one particular side.
                d->size -= qMin( d->size, maxOverhang * 2.0 );
                tryAgain = true;
            }
        }
    }
}

static int wraparound( int i, int size )
{
    while ( i < 0 ) {
        i += size;
    }
    while ( i >= size ) {
        i -= size;
    }
    return i;
}

//#define SHUFFLE_DEBUG

void PieDiagram::shuffleLabels( QRectF* textBoundingRect )
{
    // things that could be improved here:
    // - use a variable number (chosen using angle information) of neighbors to check
    // - try harder to arrange the labels to look nice

    // ideas:
    // - leave labels that don't collide alone (only if they their offset is zero)
    // - use a graphics view for collision detection

    LabelPaintCache& lpc = d->labelPaintCache;
    const int n = lpc.paintReplay.size();
    bool modified = false;
    qreal direction = 5.0;
    QVector< qreal > offsets;
    offsets.fill( 0.0, n );

    for ( bool lastRoundModified = true; lastRoundModified; ) {
        lastRoundModified = false;

        for ( int i = 0; i < n; i++ ) {
            const int neighborsToCheck = qMax( 10, lpc.paintReplay.size() - 1 );
            const int minComp = wraparound( i - neighborsToCheck / 2, n );
            const int maxComp = wraparound( i + ( neighborsToCheck + 1 ) / 2, n );

            QPainterPath& path = lpc.paintReplay[ i ].labelArea;

            for ( int j = minComp; j != maxComp; j = wraparound( j + 1, n ) ) {
                if ( i == j ) {
                    continue;
                }
                QPainterPath& otherPath = lpc.paintReplay[ j ].labelArea;

                while ( ( offsets[ i ] + direction > 0 ) && otherPath.intersects( path ) ) {
#ifdef SHUFFLE_DEBUG
                    qDebug() << "collision involving" << j << "and" << i << " -- n =" << n;
                    TextAttributes ta = lpc.paintReplay[ i ].attrs.textAttributes();
                    ta.setPen( QPen( Qt::white ) );
                    lpc.paintReplay[ i ].attrs.setTextAttributes( ta );
#endif
                    uint slice = lpc.paintReplay[ i ].index.column();
                    qreal angle = DEGTORAD( d->startAngles[ slice ] + d->angleLens[ slice ] / 2.0 );
                    qreal dx = cos( angle ) * direction;
                    qreal dy = -sin( angle ) * direction;
                    offsets[ i ] += direction;
                    path.translate( dx, dy );
                    lastRoundModified = true;
                }
            }
        }
        direction *= -1.07; // this can "overshoot", but avoids getting trapped in local minimums
        modified = modified || lastRoundModified;
    }

    if ( modified ) {
        for ( int i = 0; i < lpc.paintReplay.size(); i++ ) {
            *textBoundingRect |= lpc.paintReplay[ i ].labelArea.boundingRect();
        }
    }
}

static QPolygonF polygonFromPainterPath( const QPainterPath &pp )
{
    QPolygonF ret;
    for ( int i = 0; i < pp.elementCount(); i++ ) {
        const QPainterPath::Element& el = pp.elementAt( i );
        Q_ASSERT( el.type == QPainterPath::MoveToElement || el.type == QPainterPath::LineToElement );
        ret.append( el );
    }
    return ret;
}

// you can call it "normalizedProjectionLength" if you like
static qreal normProjection( const QLineF &l1, const QLineF &l2 )
{
    const qreal dotProduct = l1.dx() * l2.dx() + l1.dy() * l2.dy();
    return qAbs( dotProduct / ( l1.length() * l2.length() ) );
}

static QLineF labelAttachmentLine( const QPointF &center, const QPointF &start, const QPainterPath &label )
{
    Q_ASSERT ( label.elementCount() == 5 );

    // start is assumed to lie on the outer rim of the slice(!), making it possible to derive the
    // radius of the pie
    const qreal pieRadius = QLineF( center, start ).length();

    // don't draw a line at all when the label is connected to its slice due to at least one of its
    // corners falling inside the slice.
    for ( int i = 0; i < 4; i++ ) { // point 4 is just a duplicate of point 0
        if ( QLineF( label.elementAt( i ), center ).length() < pieRadius ) {
            return QLineF();
        }
    }

    // find the closest edge in the polygon, and its two neighbors
    QPointF closeCorners[3];
    {
        QPointF closest = QPointF( 1000000, 1000000 );
        int closestIndex = 0; // better misbehave than crash
        for ( int i = 0; i < 4; i++ ) { // point 4 is just a duplicate of point 0
            QPointF p = label.elementAt( i );
            if ( QLineF( p, center ).length() < QLineF( closest, center ).length() ) {
                closest = p;
                closestIndex = i;
            }
        }

        closeCorners[ 0 ] = label.elementAt( wraparound( closestIndex - 1, 4 ) );
        closeCorners[ 1 ] = closest;
        closeCorners[ 2 ] = label.elementAt( wraparound( closestIndex + 1, 4 ) );
    }

    QLineF edge1 = QLineF( closeCorners[ 0 ], closeCorners[ 1 ] );
    QLineF edge2 = QLineF( closeCorners[ 1 ], closeCorners[ 2 ] );
    QLineF connection1 = QLineF( ( closeCorners[ 0 ] + closeCorners[ 1 ] ) / 2.0, center );
    QLineF connection2 = QLineF( ( closeCorners[ 1 ] + closeCorners[ 2 ] ) / 2.0, center );
    QLineF ret;
    // prefer the connecting line meeting its edge at a more perpendicular angle
    if ( normProjection( edge1, connection1 ) < normProjection( edge2, connection2 ) ) {
        ret = connection1;
    } else {
        ret = connection2;
    }

    // This tends to look a bit better than not doing it *shrug*
    ret.setP2( ( start + center ) / 2.0 );

    // make the line end at the rim of the slice (not 100% accurate because the line is not precisely radial)
    qreal p1Radius = QLineF( ret.p1(), center ).length();
    ret.setLength( p1Radius - pieRadius );

    return ret;
}

void PieDiagram::paintInternal( PaintContext* paintContext )
{
    // note: Not having any data model assigned is no bug
    //       but we can not draw a diagram then either.
    if ( !checkInvariants( true ) || model()->rowCount() < 1 ) {
        return;
    }
    if ( d->startAngles.isEmpty() || paintContext->rectangle().isEmpty() || valueTotals() == 0.0 ) {
        return;
    }

    const ThreeDPieAttributes threeDAttrs( threeDPieAttributes() );
    const int colCount = columnCount();

    // Paint from back to front ("painter's algorithm") - first draw the backmost slice,
    // then the slices on the left and right from back to front, then the frontmost one.

    QRectF pieRect = twoDPieRect( paintContext->rectangle(), threeDAttrs );
    const int backmostSlice = findSliceAt( 90, colCount );
    const int frontmostSlice = findSliceAt( 270, colCount );
    int currentLeftSlice = backmostSlice;
    int currentRightSlice = backmostSlice;

    drawSlice( paintContext->painter(), pieRect, backmostSlice );

    if ( backmostSlice == frontmostSlice ) {
        const int rightmostSlice = findSliceAt( 0, colCount );
        const int leftmostSlice = findSliceAt( 180, colCount );

        if ( backmostSlice == leftmostSlice ) {
            currentLeftSlice = findLeftSlice( currentLeftSlice, colCount );
        }
        if ( backmostSlice == rightmostSlice ) {
            currentRightSlice = findRightSlice( currentRightSlice, colCount );
        }
    }

    while ( currentLeftSlice != frontmostSlice ) {
        if ( currentLeftSlice != backmostSlice ) {
            drawSlice( paintContext->painter(), pieRect, currentLeftSlice );
        }
        currentLeftSlice = findLeftSlice( currentLeftSlice, colCount );
    }

    while ( currentRightSlice != frontmostSlice ) {
        if ( currentRightSlice != backmostSlice ) {
            drawSlice( paintContext->painter(), pieRect, currentRightSlice );
        }
        currentRightSlice = findRightSlice( currentRightSlice, colCount );
    }

    // if the backmost slice is not the frontmost slice, we draw the frontmost one last
    if ( backmostSlice != frontmostSlice || ! threeDPieAttributes().isEnabled() ) {
        drawSlice( paintContext->painter(), pieRect, frontmostSlice );
    }

    d->paintDataValueTextsAndMarkers( paintContext, d->labelPaintCache, false, false );
    // it's safer to do this at the beginning of placeLabels, but we can save some memory here.
    d->forgetAlreadyPaintedDataValues();
    // ### maybe move this into AbstractDiagram, also make ReverseMapper deal better with multiple polygons
    const QPointF center = paintContext->rectangle().center();
    const PainterSaver painterSaver( paintContext->painter() );
    paintContext->painter()->setBrush( Qt::NoBrush );
    KDAB_FOREACH( const LabelPaintInfo &pi, d->labelPaintCache.paintReplay ) {
        // we expect the PainterPath to be a rectangle
        if ( pi.labelArea.elementCount() != 5 ) {
            continue;
        }

        paintContext->painter()->setPen( pen( pi.index ) );
        if ( d->labelDecorations & LineFromSliceDecoration ) {
            paintContext->painter()->drawLine( labelAttachmentLine( center, pi.markerPos, pi.labelArea ) );
        }
        if ( d->labelDecorations & FrameDecoration ) {
            paintContext->painter()->drawPath( pi.labelArea );
        }
        d->reverseMapper.addPolygon( pi.index.row(), pi.index.column(),
                                     polygonFromPainterPath( pi.labelArea ) );
    }
    d->labelPaintCache.clear();
    d->startAngles.clear();
    d->angleLens.clear();
}

#if defined ( Q_OS_WIN)
#define trunc(x) ((int)(x))
#endif

QRectF PieDiagram::explodedDrawPosition( const QRectF& drawPosition, uint slice ) const
{
    const QModelIndex index( model()->index( 0, slice, rootIndex() ) ); // checked
    const PieAttributes attrs( pieAttributes( index ) );

    QRectF adjustedDrawPosition = drawPosition;
    if ( attrs.explode() ) {
        qreal startAngle = d->startAngles[ slice ];
        qreal angleLen = d->angleLens[ slice ];
        qreal explodeAngle = ( DEGTORAD( startAngle + angleLen / 2.0 ) );
        qreal explodeDistance = attrs.explodeFactor() * d->size / 2.0;

        adjustedDrawPosition.translate( explodeDistance * cos( explodeAngle ),
                                        explodeDistance * - sin( explodeAngle ) );
    }
    return adjustedDrawPosition;
}

/**
  Internal method that draws one of the slices in a pie chart.

  \param painter the QPainter to draw in
  \param dataset the dataset to draw the pie for
  \param slice the slice to draw
  \param threeDPieHeight the height of the three dimensional effect
  */
void PieDiagram::drawSlice( QPainter* painter, const QRectF& drawPosition, uint slice)
{
    // Is there anything to draw at all?
    if ( d->angleLens[ slice ] == 0.0 ) {
        return;
    }
    const QRectF adjustedDrawPosition = explodedDrawPosition( drawPosition, slice );
    draw3DEffect( painter, adjustedDrawPosition, slice );
    drawSliceSurface( painter, adjustedDrawPosition, slice );
}

/**
  Internal method that draws the surface of one of the slices in a pie chart.

  \param painter the QPainter to draw in
  \param dataset the dataset to draw the slice for
  \param slice the slice to draw
  */
void PieDiagram::drawSliceSurface( QPainter* painter, const QRectF& drawPosition, uint slice )
{
    // Is there anything to draw at all?
    const qreal angleLen = d->angleLens[ slice ];
    const qreal startAngle = d->startAngles[ slice ];
    const QModelIndex index( model()->index( 0, slice, rootIndex() ) ); // checked

    const PieAttributes attrs( pieAttributes( index ) );
    const ThreeDPieAttributes threeDAttrs( threeDPieAttributes( index ) );

    painter->setRenderHint ( QPainter::Antialiasing );
    QBrush br = brush( index );
    if ( threeDAttrs.isEnabled() ) {
        br = threeDAttrs.threeDBrush( br, drawPosition );
    }
    painter->setBrush( br );

    QPen pen = this->pen( index );
    if ( threeDAttrs.isEnabled() ) {
        pen.setColor( Qt::black );
    }
    painter->setPen( pen );

    if ( angleLen == 360 ) {
        // full circle, avoid nasty line in the middle
        painter->drawEllipse( drawPosition );

        //Add polygon to Reverse mapper for showing tool tips.
        QPolygonF poly( drawPosition );
        d->reverseMapper.addPolygon( index.row(), index.column(), poly );
    } else {
        // draw the top of this piece
        // Start with getting the points for the arc.
        const int arcPoints = static_cast<int>(trunc( angleLen / granularity() ));
        QPolygonF poly( arcPoints + 2 );
        qreal degree = 0.0;
        int iPoint = 0;
        bool perfectMatch = false;

        while ( degree <= angleLen ) {
            poly[ iPoint ] = pointOnEllipse( drawPosition, startAngle + degree );
            //qDebug() << degree << angleLen << poly[ iPoint ];
            perfectMatch = ( degree == angleLen );
            degree += granularity();
            ++iPoint;
        }
        // if necessary add one more point to fill the last small gap
        if ( !perfectMatch ) {
            poly[ iPoint ] = pointOnEllipse( drawPosition, startAngle + angleLen );

            // add the center point of the piece
            poly.append( drawPosition.center() );
        } else {
            poly[ iPoint ] = drawPosition.center();
        }
        //find the value and paint it
        //fix value position
        d->reverseMapper.addPolygon( index.row(), index.column(), poly );

        painter->drawPolygon( poly );
    }
}

// calculate the position points for the label and pass them to addLabel()
void PieDiagram::addSliceLabel( LabelPaintCache* lpc, const QRectF& drawPosition, uint slice )
{
    const qreal angleLen = d->angleLens[ slice ];
    const qreal startAngle = d->startAngles[ slice ];
    const QModelIndex index( model()->index( 0, slice, rootIndex() ) ); // checked
    const qreal sum = valueTotals();

    // Position points are calculated relative to the slice.
    // They are calculated as if the slice was 'standing' on its tip and the rim was up,
    // so North is the middle (also highest part) of the rim and South is the tip of the slice.

    const QPointF south = drawPosition.center();
    const QPointF southEast = south;
    const QPointF southWest = south;
    const QPointF north = pointOnEllipse( drawPosition, startAngle + angleLen / 2.0 );

    const QPointF northEast = pointOnEllipse( drawPosition, startAngle );
    const QPointF northWest = pointOnEllipse( drawPosition, startAngle + angleLen );
    QPointF center = ( south + north ) / 2.0;
    const QPointF east = ( south + northEast ) / 2.0;
    const QPointF west = ( south + northWest ) / 2.0;

    PositionPoints points( center, northWest, north, northEast, east, southEast, south, southWest, west );
    qreal topAngle = startAngle - 90;
    if ( topAngle < 0.0 ) {
        topAngle += 360.0;
    }

    points.setDegrees( KDChartEnums::PositionEast, topAngle );
    points.setDegrees( KDChartEnums::PositionNorthEast, topAngle );
    points.setDegrees( KDChartEnums::PositionWest, topAngle + angleLen );
    points.setDegrees( KDChartEnums::PositionNorthWest, topAngle + angleLen );
    points.setDegrees( KDChartEnums::PositionCenter, topAngle + angleLen / 2.0 );
    points.setDegrees( KDChartEnums::PositionNorth, topAngle + angleLen / 2.0 );

    qreal favoriteTextAngle = 0.0;
    if ( autoRotateLabels() ) {
        favoriteTextAngle = - ( startAngle + angleLen / 2 ) + 90.0;
        while ( favoriteTextAngle <= 0.0 ) {
            favoriteTextAngle += 360.0;
        }
        // flip the label when upside down
        if ( favoriteTextAngle > 90.0 && favoriteTextAngle < 270.0 ) {
            favoriteTextAngle = favoriteTextAngle - 180.0;
        }
        // negative angles can have special meaning in addLabel; otherwise they work fine
        if ( favoriteTextAngle <= 0.0 ) {
            favoriteTextAngle += 360.0;
        }
    }

    d->addLabel( lpc, index, 0, points, Position::Center, Position::Center,
                 angleLen * sum / 360, favoriteTextAngle );
}

static bool doSpansOverlap( qreal s1Start, qreal s1End, qreal s2Start, qreal s2End )
{
    if ( s1Start < s2Start ) {
        return s1End >= s2Start;
    } else {
        return s1Start <= s2End;
    }
}

static bool doArcsOverlap( qreal a1Start, qreal a1End, qreal a2Start, qreal a2End )
{
    Q_ASSERT( a1Start >= 0 && a1Start <= 360 && a1End >= 0 && a1End <= 360 &&
              a2Start >= 0 && a2Start <= 360 && a2End >= 0 && a2End <= 360 );
    // all of this could probably be done better...
    if ( a1End < a1Start ) {
        a1End += 360;
    }
    if ( a2End < a2Start ) {
        a2End += 360;
    }

    if ( doSpansOverlap( a1Start, a1End, a2Start, a2End ) ) {
        return true;
    }
    if ( a1Start > a2Start ) {
        return doSpansOverlap( a1Start - 360.0, a1End - 360.0, a2Start, a2End );
    } else {
        return doSpansOverlap( a1Start + 360.0, a1End + 360.0, a2Start, a2End );
    }
}

/**
  Internal method that draws the shadow creating the 3D effect of a pie

  \param painter the QPainter to draw in
  \param drawPosition the position to draw at
  \param slice the slice to draw the shadow for
  */
void PieDiagram::draw3DEffect( QPainter* painter, const QRectF& drawPosition, uint slice )
{
    const QModelIndex index( model()->index( 0, slice, rootIndex() ) ); // checked
    const ThreeDPieAttributes threeDAttrs( threeDPieAttributes( index ) );
    if ( ! threeDAttrs.isEnabled() ) {
        return;
    }

    // NOTE: We cannot optimize away drawing some of the effects (even
    // when not exploding), because some of the pies might be left out
    // in future versions which would make some of the normally hidden
    // pies visible. Complex hidden-line algorithms would be much more
    // expensive than just drawing for nothing.

    // No need to save the brush, will be changed on return from this
    // method anyway.
    const QBrush brush = this->brush( model()->index( 0, slice, rootIndex() ) ); // checked
    if ( threeDAttrs.useShadowColors() ) {
        painter->setBrush( QBrush( brush.color().darker() ) );
    } else {
        painter->setBrush( brush );
    }

    qreal startAngle = d->startAngles[ slice ];
    qreal endAngle = startAngle + d->angleLens[ slice ];
    // Normalize angles
    while ( startAngle >= 360 )
        startAngle -= 360;
    while ( endAngle >= 360 )
        endAngle -= 360;
    Q_ASSERT( startAngle >= 0 && startAngle <= 360 );
    Q_ASSERT( endAngle >= 0 && endAngle <= 360 );

    // positive pie height: absolute value
    // negative pie height: relative value
    const int depth = threeDAttrs.depth() >= 0.0 ? threeDAttrs.depth() : -threeDAttrs.depth() / 100.0 * drawPosition.height();

    if ( startAngle == endAngle || startAngle == endAngle - 360 ) { // full circle
        draw3dOuterRim( painter, drawPosition, depth, 180, 360 );
    } else {
        if ( doArcsOverlap( startAngle, endAngle, 180, 360 ) ) {
            draw3dOuterRim( painter, drawPosition, depth, startAngle, endAngle );
        }

        if ( startAngle >= 270 || startAngle <= 90 ) {
            draw3dCutSurface( painter, drawPosition, depth, startAngle );
        }
        if ( endAngle >= 90 && endAngle <= 270 ) {
            draw3dCutSurface( painter, drawPosition, depth, endAngle );
        }
    }
}


/**
  Internal method that draws the cut surface of a slice (think of a real pie cut into slices)
  in 3D mode, for surfaces that are facing the observer.

  \param painter the QPainter to draw in
  \param rect the position to draw at
  \param threeDHeight the height of the shadow
  \param angle the angle of the segment
  */
void PieDiagram::draw3dCutSurface( QPainter* painter,
        const QRectF& rect,
        qreal threeDHeight,
        qreal angle )
{
    QPolygonF poly( 4 );
    const QPointF center = rect.center();
    const QPointF circlePoint = pointOnEllipse( rect, angle );
    poly[0] = center;
    poly[1] = circlePoint;
    poly[2] = QPointF( circlePoint.x(), circlePoint.y() + threeDHeight );
    poly[3] = QPointF( center.x(), center.y() + threeDHeight );
    // TODO: add polygon to ReverseMapper
    painter->drawPolygon( poly );
}

/**
  Internal method that draws the outer rim of a slice when the rim is facing the observer.

  \param painter the QPainter to draw in
  \param rect the position to draw at
  \param threeDHeight the height of the shadow
  \param startAngle the starting angle of the segment
  \param endAngle the ending angle of the segment
  */
void PieDiagram::draw3dOuterRim( QPainter* painter,
        const QRectF& rect,
        qreal threeDHeight,
        qreal startAngle,
        qreal endAngle )
{
    // Start with getting the points for the inner arc.
    if ( endAngle < startAngle ) {
        endAngle += 360;
    }
    startAngle = qMax( startAngle, qreal( 180.0 ) );
    endAngle = qMin( endAngle, qreal( 360.0 ) );

    int numHalfPoints = trunc( ( endAngle - startAngle ) / granularity() ) + 1;
    if ( numHalfPoints < 2 ) {
        return;
    }

    QPolygonF poly( numHalfPoints );

    qreal degree = endAngle;
    int iPoint = 0;
    bool perfectMatch = false;
    while ( degree >= startAngle ) {
        poly[ numHalfPoints - iPoint - 1 ] = pointOnEllipse( rect, degree );

        perfectMatch = (degree == startAngle);
        degree -= granularity();
        ++iPoint;
    }
    // if necessary add one more point to fill the last small gap
    if ( !perfectMatch ) {
        poly.prepend( pointOnEllipse( rect, startAngle ) );
        ++numHalfPoints;
    }

    poly.resize( numHalfPoints * 2 );

    // Now copy these arcs again into the final array, but in the
    // opposite direction and moved down by the 3D height.
    for ( int i = numHalfPoints - 1; i >= 0; --i ) {
        QPointF pointOnFirstArc( poly[ i ] );
        pointOnFirstArc.setY( pointOnFirstArc.y() + threeDHeight );
        poly[ numHalfPoints * 2 - i - 1 ] = pointOnFirstArc;
    }

    // TODO: Add polygon to ReverseMapper
    painter->drawPolygon( poly );
}

/**
  Internal method that finds the slice that is located at the position specified by \c angle.

  \param angle the angle at which to search for a slice
  \return the number of the slice found
  */
uint PieDiagram::findSliceAt( qreal angle, int colCount )
{
    for ( int i = 0; i < colCount; ++i ) {
        qreal endseg = d->startAngles[ i ] + d->angleLens[ i ];
        if ( d->startAngles[ i ] <= angle &&  endseg >= angle ) {
            return i;
        }
    }

    // If we have not found it, try wrap around
    // but only if the current searched angle is < 360 degree
    if ( angle < 360 )
        return findSliceAt( angle + 360, colCount );
    // otherwise - what ever went wrong - we return 0
    return 0;
}


/**
  Internal method that finds the slice that is located to the left of \c slice.

  \param slice the slice to start the search from
  \return the number of the pie to the left of \c pie
  */
uint PieDiagram::findLeftSlice( uint slice, int colCount )
{
    if ( slice == 0 ) {
        if ( colCount > 1 ) {
            return colCount - 1;
        } else {
            return 0;
        }
    } else {
        return slice - 1;
    }
}


/**
  Internal method that finds the slice that is located to the right of \c slice.

  \param slice the slice to start the search from
  \return the number of the slice to the right of \c slice
  */
uint PieDiagram::findRightSlice( uint slice, int colCount )
{
    int rightSlice = slice + 1;
    if ( rightSlice == colCount ) {
        rightSlice = 0;
    }
    return rightSlice;
}


/**
  * Auxiliary method returning a point to a given boundary
  * rectangle of the enclosed ellipse and an angle.
  */
QPointF PieDiagram::pointOnEllipse( const QRectF& boundingBox, qreal angle )
{
    qreal angleRad = DEGTORAD( angle );
    qreal cosAngle = cos( angleRad );
    qreal sinAngle = -sin( angleRad );
    qreal posX = cosAngle * boundingBox.width() / 2.0;
    qreal posY = sinAngle * boundingBox.height() / 2.0;
    return QPointF( posX + boundingBox.center().x(),
                    posY + boundingBox.center().y() );

}

/*virtual*/
qreal PieDiagram::valueTotals() const
{
    if ( !model() )
        return 0;
    const int colCount = columnCount();
    qreal total = 0.0;
    Q_ASSERT( model()->rowCount() >= 1 );
    for ( int j = 0; j < colCount; ++j ) {
      total += qAbs(model()->data( model()->index( 0, j, rootIndex() ) ).toReal()); // checked
    }
    return total;
}

/*virtual*/
qreal PieDiagram::numberOfValuesPerDataset() const
{
    return model() ? model()->columnCount( rootIndex() ) : 0.0;
}

/*virtual*/
qreal PieDiagram::numberOfGridRings() const
{
    return 1;
}
