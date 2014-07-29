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

#include "KDChartRingDiagram.h"
#include "KDChartRingDiagram_p.h"

#include "KDChartAttributesModel.h"
#include "KDChartPaintContext.h"
#include "KDChartPainterSaver_p.h"
#include "KDChartPieAttributes.h"
#include "KDChartPolarCoordinatePlane_p.h"
#include "KDChartThreeDPieAttributes.h"
#include "KDChartDataValueAttributes.h"

#include <QPainter>

#include <KDABLibFakes>

using namespace KDChart;

RingDiagram::Private::Private()
    : relativeThickness( false )
    , expandWhenExploded( false )
{
}

RingDiagram::Private::~Private() {}

#define d d_func()

RingDiagram::RingDiagram( QWidget* parent, PolarCoordinatePlane* plane ) :
    AbstractPieDiagram( new Private(), parent, plane )
{
    init();
}

RingDiagram::~RingDiagram()
{
}

void RingDiagram::init()
{
}

/**
  * Creates an exact copy of this diagram.
  */
RingDiagram * RingDiagram::clone() const
{
    return new RingDiagram( new Private( *d ) );
}

bool RingDiagram::compare( const RingDiagram* other ) const
{
    if ( other == this ) return true;
    if ( ! other ) {
        return false;
    }
    return  // compare the base class
            ( static_cast<const AbstractPieDiagram*>(this)->compare( other ) ) &&
            // compare own properties
            (relativeThickness()  == other->relativeThickness()) &&
            (expandWhenExploded() == other->expandWhenExploded());
}

void RingDiagram::setRelativeThickness( bool relativeThickness )
{
    d->relativeThickness = relativeThickness;
}

bool RingDiagram::relativeThickness() const
{
    return d->relativeThickness;
}

void RingDiagram::setExpandWhenExploded( bool expand )
{
	d->expandWhenExploded = expand;
}

bool RingDiagram::expandWhenExploded() const
{
	return d->expandWhenExploded;
}

const QPair<QPointF, QPointF> RingDiagram::calculateDataBoundaries () const
{
    if ( !checkInvariants( true ) ) return QPair<QPointF, QPointF>( QPointF( 0, 0 ), QPointF( 0, 0 ) );

    const PieAttributes attrs( pieAttributes() );

    QPointF bottomLeft( 0, 0 );
    QPointF topRight;
    // If we explode, we need extra space for the pie slice that has the largest explosion distance.
    if ( attrs.explode() ) {
        const int rCount = rowCount();
        const int colCount = columnCount();
        qreal maxExplode = 0.0;
        for ( int i = 0; i < rCount; ++i ) {
            qreal maxExplodeInThisRow = 0.0;
            for ( int j = 0; j < colCount; ++j ) {
                const PieAttributes columnAttrs( pieAttributes( model()->index( i, j, rootIndex() ) ) ); // checked
                maxExplodeInThisRow = qMax( maxExplodeInThisRow, columnAttrs.explodeFactor() );
            }
            maxExplode += maxExplodeInThisRow;

            // FIXME: What if explode factor of inner ring is > 1.0 ?
            if ( !d->expandWhenExploded ) {
                break;
            }
        }
        // explode factor is relative to width (outer r - inner r) of one ring
        maxExplode /= ( rCount + 1);
        topRight = QPointF( 1.0 + maxExplode, 1.0 + maxExplode );
    } else {
        topRight = QPointF( 1.0, 1.0 );
    }
    return QPair<QPointF, QPointF>( bottomLeft, topRight );
}

void RingDiagram::paintEvent( QPaintEvent* )
{
    QPainter painter ( viewport() );
    PaintContext ctx;
    ctx.setPainter ( &painter );
    ctx.setRectangle( QRectF ( 0, 0, width(), height() ) );
    paint ( &ctx );
}

void RingDiagram::resizeEvent( QResizeEvent* )
{
}

void RingDiagram::paint( PaintContext* ctx )
{
    // note: Not having any data model assigned is no bug
    //       but we can not draw a diagram then either.
    if ( !checkInvariants(true) )
        return;

    d->reverseMapper.clear();

    const PieAttributes attrs( pieAttributes() );

    const int rCount = rowCount();
    const int colCount = columnCount();

    QRectF contentsRect = PolarCoordinatePlane::Private::contentsRect( polarCoordinatePlane() );
    contentsRect = ctx->rectangle();
    if ( contentsRect.isEmpty() )
        return;

    d->startAngles = QVector< QVector<qreal> >( rCount, QVector<qreal>( colCount ) );
    d->angleLens = QVector< QVector<qreal> >( rCount, QVector<qreal>( colCount ) );

    // compute position
    d->size = qMin( contentsRect.width(), contentsRect.height() ); // initial size

    // if the slices explode, we need to give them additional space =>
    // make the basic size smaller
    qreal totalOffset = 0.0;
    for ( int i = 0; i < rCount; ++i ) {
        qreal maxOffsetInThisRow = 0.0;
        for ( int j = 0; j < colCount; ++j ) {
            const PieAttributes cellAttrs( pieAttributes( model()->index( i, j, rootIndex() ) ) ); // checked
            //qDebug() << cellAttrs.explodeFactor();
            const qreal explode = cellAttrs.explode() ? cellAttrs.explodeFactor() : 0.0;
            maxOffsetInThisRow = qMax( maxOffsetInThisRow, cellAttrs.gapFactor( false ) + explode );
        }
        if ( !d->expandWhenExploded ) {
            maxOffsetInThisRow -= qreal( i );
        }
        totalOffset += qMax( maxOffsetInThisRow, 0.0 );
        // FIXME: What if explode factor of inner ring is > 1.0 ?
        //if ( !d->expandWhenExploded )
        //	break;
    }

    // explode factor is relative to width (outer r - inner r) of one ring
    if ( rCount > 0 )
    	totalOffset /= ( rCount + 1 );
    d->size /= ( 1.0 + totalOffset );


    qreal x = ( contentsRect.width() == d->size ) ? 0.0 : ( ( contentsRect.width() - d->size ) / 2.0 );
    qreal y = ( contentsRect.height() == d->size ) ? 0.0 : ( ( contentsRect.height() - d->size ) / 2.0 );
    d->position = QRectF( x, y, d->size, d->size );
    d->position.translate( contentsRect.left(), contentsRect.top() );

    const PolarCoordinatePlane * plane = polarCoordinatePlane();

    QVariant vValY;

    d->forgetAlreadyPaintedDataValues();
    for ( int iRow = 0; iRow < rCount; ++iRow ) {
        const qreal sum = valueTotals( iRow );
        if ( sum == 0.0 ) //nothing to draw
            continue;
        qreal currentValue = plane ? plane->startPosition() : 0.0;
        const qreal sectorsPerValue = 360.0 / sum;

        for ( int iColumn = 0; iColumn < colCount; ++iColumn ) {
            // is there anything at all at this column?
            bool bOK;
            const qreal cellValue = qAbs( model()->data( model()->index( iRow, iColumn, rootIndex() ) ) // checked
                                    .toReal( &bOK ) );

            if ( bOK ) {
                d->startAngles[ iRow ][ iColumn ] = currentValue;
                d->angleLens[ iRow ][ iColumn ] = cellValue * sectorsPerValue;
            } else { // mark as non-existent
                d->angleLens[ iRow ][ iColumn ] = 0.0;
                if ( iColumn > 0.0 ) {
                    d->startAngles[ iRow ][ iColumn ] = d->startAngles[ iRow ][ iColumn - 1 ];
                } else {
                    d->startAngles[ iRow ][ iColumn ] = currentValue;
                }
            }

            currentValue = d->startAngles[ iRow ][ iColumn ] + d->angleLens[ iRow ][ iColumn ];

            drawOneSlice( ctx->painter(), iRow, iColumn, granularity() );
        }
    }
}

#if defined ( Q_WS_WIN)
#define trunc(x) ((int)(x))
#endif

/**
  \param painter the QPainter to draw in
  \param dataset the dataset to draw the slice for
  \param slice the slice to draw
  */
void RingDiagram::drawOneSlice( QPainter* painter, uint dataset, uint slice, qreal granularity )
{
    // Is there anything to draw at all?
    const qreal angleLen = d->angleLens[ dataset ][ slice ];
    if ( angleLen ) {
        drawPieSurface( painter, dataset, slice, granularity );
    }
}

void RingDiagram::resize( const QSizeF& )
{
}

/**
  Internal method that draws the top surface of one of the slices in a ring chart.

  \param painter the QPainter to draw in
  \param dataset the dataset to draw the slice for
  \param slice the slice to draw
  */
void RingDiagram::drawPieSurface( QPainter* painter, uint dataset, uint slice, qreal granularity )
{
    // Is there anything to draw at all?
    qreal angleLen = d->angleLens[ dataset ][ slice ];
    if ( angleLen ) {
        qreal startAngle = d->startAngles[ dataset ][ slice ];

        QModelIndex index( model()->index( dataset, slice, rootIndex() ) ); // checked
        const PieAttributes attrs( pieAttributes( index ) );
        const ThreeDPieAttributes threeDAttrs( threeDPieAttributes( index ) );

        const int rCount = rowCount();
        const int colCount = columnCount();

        int iPoint = 0;

        QRectF drawPosition = d->position;

        painter->setRenderHint ( QPainter::Antialiasing );

        QBrush br = brush( index );
        if ( threeDAttrs.isEnabled() ) {
            br = threeDAttrs.threeDBrush( br, drawPosition );
        }
        painter->setBrush( br );

        painter->setPen( pen( index ) );

        if ( angleLen == 360 ) {
            // full circle, avoid nasty line in the middle
            // FIXME: Draw a complete ring here
            //painter->drawEllipse( drawPosition );
        } else {
            bool perfectMatch = false;

            qreal circularGap = 0.0;

            if ( attrs.gapFactor( true ) > 0.0 ) {
                // FIXME: Measure in degrees!
                circularGap = attrs.gapFactor( true );
            }

            QPolygonF poly;

            qreal degree = 0;

            qreal actualStartAngle = startAngle + circularGap;
            qreal actualAngleLen = angleLen - 2 * circularGap;

            qreal totalRadialExplode = 0.0;
            qreal maxRadialExplode = 0.0;

            qreal totalRadialGap = 0.0;
            qreal maxRadialGap = 0.0;
            for ( uint i = rCount - 1; i > dataset; --i ) {
                qreal maxRadialExplodeInThisRow = 0.0;
                qreal maxRadialGapInThisRow = 0.0;
                for ( int j = 0; j < colCount; ++j ) {
                    const PieAttributes cellAttrs( pieAttributes( model()->index( i, j, rootIndex() ) ) ); // checked
                    if ( d->expandWhenExploded ) {
                        maxRadialGapInThisRow = qMax( maxRadialGapInThisRow, cellAttrs.gapFactor( false ) );
                    }

                    // Don't use a gap for the very inner circle
                    if ( cellAttrs.explode() && d->expandWhenExploded ) {
                        maxRadialExplodeInThisRow = qMax( maxRadialExplodeInThisRow, cellAttrs.explodeFactor() );
                    }
                }
                maxRadialExplode += maxRadialExplodeInThisRow;
                maxRadialGap += maxRadialGapInThisRow;

                // FIXME: What if explode factor of inner ring is > 1.0 ?
                //if ( !d->expandWhenExploded )
                //    break;
            }
            totalRadialGap = maxRadialGap + attrs.gapFactor( false );
            totalRadialExplode = attrs.explode() ? maxRadialExplode + attrs.explodeFactor() : maxRadialExplode;

            while ( degree <= actualAngleLen ) {
            const QPointF p = pointOnEllipse( drawPosition, dataset, slice, false, actualStartAngle + degree,
                                              totalRadialGap, totalRadialExplode );
                poly.append( p );
                degree += granularity;
                iPoint++;
            }
            if ( ! perfectMatch ) {
                poly.append( pointOnEllipse( drawPosition, dataset, slice, false, actualStartAngle + actualAngleLen,
                                             totalRadialGap, totalRadialExplode ) );
                iPoint++;
            }

            // The center point of the inner brink
            const QPointF innerCenterPoint( poly[ int(iPoint / 2) ] );

            actualStartAngle = startAngle + circularGap;
            actualAngleLen = angleLen - 2 * circularGap;

            degree = actualAngleLen;

            const int lastInnerBrinkPoint = iPoint;
            while ( degree >= 0 ) {
                poly.append( pointOnEllipse( drawPosition, dataset, slice, true, actualStartAngle + degree,
                                             totalRadialGap, totalRadialExplode ) );
                perfectMatch = (degree == 0);
                degree -= granularity;
                iPoint++;
            }
            // if necessary add one more point to fill the last small gap
            if ( ! perfectMatch ) {
                poly.append( pointOnEllipse( drawPosition, dataset, slice, true, actualStartAngle,
                                             totalRadialGap, totalRadialExplode ) );
                iPoint++;
            }

            // The center point of the outer brink
            const QPointF outerCenterPoint( poly[ lastInnerBrinkPoint + int((iPoint - lastInnerBrinkPoint) / 2) ] );
            //qDebug() << poly;
            //find the value and paint it
            //fix value position
            const qreal sum = valueTotals( dataset );
            painter->drawPolygon( poly );

            d->reverseMapper.addPolygon( index.row(), index.column(), poly );

            const QPointF centerPoint = (innerCenterPoint + outerCenterPoint) / 2.0;

            const PainterSaver ps( painter );
            const TextAttributes ta = dataValueAttributes( index ).textAttributes();
            if ( !ta.hasRotation() && autoRotateLabels() )
            {
                const QPointF& p1 = poly.last();
                const QPointF& p2 = poly[ lastInnerBrinkPoint ];
                const QLineF line( p1, p2 );
                // TODO: do the label rotation like in PieDiagram
                const qreal angle = line.dx() == 0 ? 0.0 : atan( line.dy() / line.dx() );
                painter->translate( centerPoint );
                painter->rotate( angle / 2.0 / 3.141592653589793 * 360.0 );
                painter->translate( -centerPoint );
            }

            paintDataValueText( painter, index, centerPoint, angleLen*sum / 360 );
        }
    }
}


/**
  * Auxiliary method returning a point to a given boundary
  * rectangle of the enclosed ellipse and an angle.
  */
QPointF RingDiagram::pointOnEllipse( const QRectF& rect, int dataset, int slice, bool outer, qreal angle,
                                     qreal totalGapFactor, qreal totalExplodeFactor )
{
    qreal angleLen = d->angleLens[ dataset ][ slice ];
    qreal startAngle = d->startAngles[ dataset ][ slice ];

    const int rCount = rowCount() * 2;

    qreal level = outer ? ( rCount - dataset - 1 ) + 2 : ( rCount - dataset - 1 ) + 1;

    const qreal offsetX = rCount > 0 ? level * rect.width() / ( ( rCount + 1 ) * 2 ) : 0.0;
    const qreal offsetY = rCount > 0 ? level * rect.height() / ( ( rCount + 1 ) * 2 ) : 0.0;
    const qreal centerOffsetX = rCount > 0 ? totalExplodeFactor * rect.width() / ( ( rCount + 1 ) * 2 ) : 0.0;
    const qreal centerOffsetY = rCount > 0 ? totalExplodeFactor * rect.height() / ( ( rCount + 1 ) * 2 ) : 0.0;
    const qreal gapOffsetX = rCount > 0 ? totalGapFactor * rect.width() / ( ( rCount + 1 ) * 2 ) : 0.0;
    const qreal gapOffsetY = rCount > 0 ? totalGapFactor * rect.height() / ( ( rCount + 1 ) * 2 ) : 0.0;

    qreal explodeAngleRad = DEGTORAD( angle );
    qreal cosAngle = cos( explodeAngleRad );
    qreal sinAngle = -sin( explodeAngleRad );
    qreal explodeAngleCenterRad = DEGTORAD( startAngle + angleLen / 2.0 );
    qreal cosAngleCenter = cos( explodeAngleCenterRad );
    qreal sinAngleCenter = -sin( explodeAngleCenterRad );
    return QPointF( ( offsetX + gapOffsetX ) * cosAngle + centerOffsetX * cosAngleCenter + rect.center().x(),
                    ( offsetY + gapOffsetY ) * sinAngle + centerOffsetY * sinAngleCenter + rect.center().y() );
}

/*virtual*/
qreal RingDiagram::valueTotals() const
{
    const int rCount = rowCount();
    const int colCount = columnCount();
    qreal total = 0.0;
    for ( int i = 0; i < rCount; ++i ) {
        for ( int j = 0; j < colCount; ++j ) {
            total += qAbs( model()->data( model()->index( i, j, rootIndex() ) ).toReal() ); // checked
        }
    }
    return total;
}

qreal RingDiagram::valueTotals( int dataset ) const
{
    Q_ASSERT( dataset < model()->rowCount() );
    const int colCount = columnCount();
    qreal total = 0.0;
    for ( int j = 0; j < colCount; ++j ) {
      total += qAbs( model()->data( model()->index( dataset, j, rootIndex() ) ).toReal() ); // checked
    }
    return total;
}

/*virtual*/
qreal RingDiagram::numberOfValuesPerDataset() const
{
    return model() ? model()->columnCount( rootIndex() ) : 0.0;
}

qreal RingDiagram::numberOfDatasets() const
{
    return model() ? model()->rowCount( rootIndex() ) : 0.0;
}

/*virtual*/
qreal RingDiagram::numberOfGridRings() const
{
    return 1;
}
