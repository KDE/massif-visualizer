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

#include "KDChartNormalLyingBarDiagram_p.h"

#include <QModelIndex>

#include "KDChartBarDiagram.h"
#include "KDChartTextAttributes.h"
#include "KDChartAttributesModel.h"
#include "KDChartAbstractCartesianDiagram.h"

using namespace KDChart;
using namespace std;

NormalLyingBarDiagram::NormalLyingBarDiagram( BarDiagram* d )
    : BarDiagramType( d )
{
}

BarDiagram::BarType NormalLyingBarDiagram::type() const
{
    return BarDiagram::Normal;
}

// TODO there is a lot of duplication between this and the non-lying bar diagram, fix it someday...
const QPair<QPointF, QPointF> NormalLyingBarDiagram::calculateDataBoundaries() const
{
    const int rowCount = compressor().modelDataRows();
    const int colCount = compressor().modelDataColumns();

    qreal xMin = 0.0;
    qreal xMax = rowCount;
    qreal yMin = 0.0;
    qreal yMax = 0.0;

    bool isFirst = true;
    for ( int column = 0; column < colCount; ++column ) {
        for ( int row = 0; row < rowCount; ++row ) {
            const CartesianDiagramDataCompressor::CachePosition position( row, column );
            const CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );
            const qreal value = ISNAN( point.value ) ? 0.0 : point.value;
            // this is always true yMin can be 0 in case all values
            // are the same
            // same for yMax it can be zero if all values are negative
            if ( isFirst ) {
                yMin = value;
                yMax = value;
                isFirst = false;
            } else {
                yMin = qMin( yMin, value );
                yMax = qMax( yMax, value );
            }
        }
    }

    // special cases
    if ( yMax == yMin ) {
        if ( yMin == 0.0 ) {
            yMax = 0.1; // we need at least a range
        } else if ( yMax < 0.0 ) {
            yMax = 0.0; // they are the same and negative
        } else if ( yMin > 0.0 ) {
            yMin = 0.0; // they are the same but positive
        }
    }
    const QPointF bottomLeft( QPointF( yMin, xMin ) );
    const QPointF topRight( QPointF( yMax, xMax ) );

    return QPair< QPointF, QPointF >( bottomLeft, topRight );
}

void NormalLyingBarDiagram::paint( PaintContext* ctx )
{
    // FIXME: in all LyingBarDiagram types, the datasets are rendered top to bottom, but the abscissa
    // (in that case Y axis) ticks are still bottom to top. So tick labels are in reverse order.
    reverseMapper().clear();

    const QPair<QPointF,QPointF> boundaries = diagram()->dataBoundaries(); // cached

    const QPointF boundLeft = ctx->coordinatePlane()->translate( boundaries.first ) ;
    const QPointF boundRight = ctx->coordinatePlane()->translate( boundaries.second );

    const int rowCount = attributesModel()->rowCount( attributesModelRootIndex() );
    const int colCount = attributesModel()->columnCount( attributesModelRootIndex() );

    BarAttributes ba = diagram()->barAttributes();
    qreal barWidth = 0;
    qreal maxDepth = 0;
    qreal width = boundLeft.y() - boundRight.y();
    qreal groupWidth = width / rowCount;
    qreal spaceBetweenBars = 0;
    qreal spaceBetweenGroups = 0;

    if ( ba.useFixedBarWidth() ) {

        barWidth = ba.fixedBarWidth();
        groupWidth += barWidth;

        // Pending Michel set a min and max value for the groupWidth
        // related to the area.width
        if ( groupWidth < 0 )
            groupWidth = 0;

        if ( groupWidth * rowCount > width )
            groupWidth = width / rowCount;
    }

    // maxLimit: allow the space between bars to be larger until area.width()
    // is covered by the groups.
    qreal maxLimit = rowCount * ( groupWidth + ( colCount - 1 ) * ba.fixedDataValueGap() );

    //Pending Michel: FixMe
    if ( ba.useFixedDataValueGap() ) {
        if ( width > maxLimit )
            spaceBetweenBars += ba.fixedDataValueGap();
        else
            spaceBetweenBars = ( width / rowCount - groupWidth ) / ( colCount - 1 );
    }

    if ( ba.useFixedValueBlockGap() ) {
        spaceBetweenGroups += ba.fixedValueBlockGap();
    }

    calculateValueAndGapWidths( rowCount, colCount,groupWidth,
                                barWidth, spaceBetweenBars, spaceBetweenGroups );

    LabelPaintCache lpc;

    for ( int row = 0; row < rowCount; row++ ) {
    qreal offset = -groupWidth / 2 + spaceBetweenGroups / 2;

        if ( ba.useFixedDataValueGap() ) {
            if ( spaceBetweenBars > 0 ) {
                if ( width > maxLimit ) {
                    offset -= ba.fixedDataValueGap();
                } else {
                    offset -= ( width / rowCount - groupWidth ) / ( colCount - 1 );
                }
            } else {
                offset += barWidth / 2;
            }
        }

        for ( int column = 0; column < colCount; column++ ) {
            // paint one group
            const CartesianDiagramDataCompressor::CachePosition position( row,  column );
            const CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );
            const QModelIndex sourceIndex = attributesModel()->mapToSource( point.index );
            const qreal value = point.value;//attributesModel()->data( sourceIndex ).toReal();
            QPointF topPoint = ctx->coordinatePlane()->translate( QPointF( value, rowCount - ( point.key + 0.5 ) ) );
            QPointF bottomPoint =  ctx->coordinatePlane()->translate( QPointF( 0, rowCount - point.key ) );
            const qreal barHeight = topPoint.x() - bottomPoint.x();
            topPoint.ry() += offset;
            topPoint.rx() -= barHeight;
            const QRectF rect( topPoint, QSizeF( barHeight, barWidth ) );
            m_private->addLabel( &lpc, sourceIndex, 0, PositionPoints( rect ), Position::North,
                                 Position::South, point.value );
            paintBars( ctx, sourceIndex, rect, maxDepth );

            offset += barWidth + spaceBetweenBars;
        }
    }
    m_private->paintDataValueTextsAndMarkers( ctx, lpc, false );
}
