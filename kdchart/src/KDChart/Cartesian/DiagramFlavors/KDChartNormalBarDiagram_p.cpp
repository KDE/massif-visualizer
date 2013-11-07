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

#include "KDChartNormalBarDiagram_p.h"

#include <QModelIndex>

#include "KDChartBarDiagram.h"
#include "KDChartTextAttributes.h"
#include "KDChartAttributesModel.h"
#include "KDChartAbstractCartesianDiagram.h"

using namespace KDChart;
using namespace std;

NormalBarDiagram::NormalBarDiagram( BarDiagram* d )
    : BarDiagramType( d )
{
}

BarDiagram::BarType NormalBarDiagram::type() const
{
    return BarDiagram::Normal;
}

const QPair<QPointF, QPointF> NormalBarDiagram::calculateDataBoundaries() const
{
    const int rowCount = compressor().modelDataRows();
    const int colCount = compressor().modelDataColumns();

    const qreal xMin = 0.0;
    const qreal xMax = rowCount;
    qreal yMin = 0.0;
    qreal yMax = 0.0;

    qreal usedDepth = 0;

    bool isFirst = true;
    for ( int column = 0; column < colCount; ++column ) {
        for ( int row = 0; row < rowCount; ++row ) {
            const CartesianDiagramDataCompressor::CachePosition position( row, column );
            const CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );
            const qreal value = ISNAN( point.value ) ? 0.0 : point.value;

            QModelIndex sourceIndex = attributesModel()->mapToSource( point.index );
            ThreeDBarAttributes threeDAttrs = diagram()->threeDBarAttributes( sourceIndex );

            if ( threeDAttrs.isEnabled() )
                usedDepth = qMax( usedDepth, threeDAttrs.depth() );

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
            yMax = 0.0; // extend the range to zero
        } else if ( yMin > 0.0 ) {
            yMin = 0.0; // dito
        }
    }

    return QPair< QPointF, QPointF >( QPointF( xMin, yMin ), QPointF( xMax, yMax ) );
}

void NormalBarDiagram::paint( PaintContext* ctx )
{
    reverseMapper().clear();

    const QPair<QPointF,QPointF> boundaries = diagram()->dataBoundaries(); // cached

    const QPointF boundLeft = ctx->coordinatePlane()->translate( boundaries.first ) ;
    const QPointF boundRight = ctx->coordinatePlane()->translate( boundaries.second );

    const int rowCount = attributesModel()->rowCount(attributesModelRootIndex());
    const int colCount = attributesModel()->columnCount(attributesModelRootIndex());

    BarAttributes ba = diagram()->barAttributes();
    ThreeDBarAttributes threeDAttrs = diagram()->threeDBarAttributes();

    //we need some margin (hence the 2.5) for the three dimensional depth
    const qreal threeDepthMargin = ( threeDAttrs.isEnabled() ) ? 2.5 * threeDAttrs.depth() : 0;

    qreal barWidth = 0;
    qreal maxDepth = 0;
    qreal width = boundRight.x() - boundLeft.x() - threeDepthMargin;
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

        if ( groupWidth  * rowCount > width )
            groupWidth = width / rowCount;
    }

    // maxLimit: allow the space between bars to be larger until area.width()
    // is covered by the groups.
    qreal maxLimit = rowCount * ( groupWidth + ( ( colCount - 1 ) * ba.fixedDataValueGap() ) );

    //Pending Michel: FixMe
    if ( ba.useFixedDataValueGap() ) {
        if ( width > maxLimit ) {
            spaceBetweenBars += ba.fixedDataValueGap();
        } else {
            spaceBetweenBars = ( ( width / rowCount ) - groupWidth ) / ( colCount - 1 );
        }
    }

    if ( ba.useFixedValueBlockGap() ) {
        spaceBetweenGroups += ba.fixedValueBlockGap();
    }

    calculateValueAndGapWidths( rowCount, colCount,groupWidth,
                                barWidth, spaceBetweenBars, spaceBetweenGroups );

    LabelPaintCache lpc;

    for ( int row = 0; row < rowCount; ++row ) {
        qreal offset = -groupWidth / 2 + spaceBetweenGroups / 2;

        if ( ba.useFixedDataValueGap() ) {
            if ( spaceBetweenBars > 0 ) {
                if ( width > maxLimit ) {
                    offset -= ba.fixedDataValueGap();
                } else {
                    offset -= ( ( width / rowCount ) - groupWidth ) / ( colCount - 1 );
                }
            } else {
                offset += barWidth / 2;
            }
        }

        for ( int column = 0; column < colCount; ++column ) {
            // paint one group
            const CartesianDiagramDataCompressor::CachePosition position( row,  column );
            const CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );
            const QModelIndex sourceIndex = attributesModel()->mapToSource( point.index );
            const qreal value = point.value;//attributesModel()->data( sourceIndex ).toReal();
            if ( ! point.hidden && !ISNAN( value ) ) {
                QPointF topPoint = ctx->coordinatePlane()->translate( QPointF( point.key + 0.5, value ) );
                QPointF bottomPoint =  ctx->coordinatePlane()->translate( QPointF( point.key, 0 ) );

                if ( threeDAttrs.isEnabled() ) {
                    const qreal usedDepth = threeDAttrs.depth() / 4;
                    topPoint.setY( topPoint.y() + usedDepth + 1.0 );
                }

                const qreal barHeight = bottomPoint.y() - topPoint.y();
                topPoint.setX( topPoint.x() + offset );
                const QRectF rect( topPoint, QSizeF( barWidth, barHeight ) );
                m_private->addLabel( &lpc, sourceIndex, 0, PositionPoints( rect ), Position::North,
                                     Position::South, point.value );
                paintBars( ctx, sourceIndex, rect, maxDepth );
            }
            offset += barWidth + spaceBetweenBars;
        }
    }
    m_private->paintDataValueTextsAndMarkers( ctx, lpc, false );
}
