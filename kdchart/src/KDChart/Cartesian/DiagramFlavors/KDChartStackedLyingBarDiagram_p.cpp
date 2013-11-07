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

#include <QModelIndex>

#include "KDChartBarDiagram.h"
#include "KDChartTextAttributes.h"
#include "KDChartAttributesModel.h"
#include "KDChartAbstractCartesianDiagram.h"
#include "KDChartStackedLyingBarDiagram_p.h"

using namespace KDChart;

StackedLyingBarDiagram::StackedLyingBarDiagram( BarDiagram* d )
    : BarDiagramType( d )
{
}

BarDiagram::BarType StackedLyingBarDiagram::type() const
{
    return BarDiagram::Stacked;
}

const QPair<QPointF, QPointF> StackedLyingBarDiagram::calculateDataBoundaries() const
{
    const int rowCount = compressor().modelDataRows();
    const int colCount = compressor().modelDataColumns();

    const qreal xMin = 0;
    const qreal xMax = rowCount;
    qreal yMin = 0;
    qreal yMax = 0;

    bool isFirst = true;
    for ( int row = 0; row < rowCount; ++row ) {
        // calculate sum of values per column - Find out stacked Min/Max
        qreal stackedValues = 0.0;
        qreal negativeStackedValues = 0.0;
        for ( int col = 0; col < colCount; ++col ) {
            const CartesianDiagramDataCompressor::CachePosition position( row, col );
            const CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );

            if ( point.value > 0.0 )
                stackedValues += point.value;
            else
                negativeStackedValues += point.value;

            // this is always true yMin can be 0 in case all values
            // are the same
            // same for yMax it can be zero if all values are negative
            if ( isFirst ) {
                yMin = negativeStackedValues < 0.0 ? negativeStackedValues : stackedValues;
                yMax = stackedValues > 0.0 ? stackedValues : negativeStackedValues;
                isFirst = false;
            } else {
                yMin = qMin( qMin( yMin, stackedValues ), negativeStackedValues );
                yMax = qMax( qMax( yMax, stackedValues ), negativeStackedValues );
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

    return QPair< QPointF, QPointF >( QPointF( yMin, xMin ), QPointF( yMax, xMax ) );
}

void StackedLyingBarDiagram::paint( PaintContext* ctx )
{
    reverseMapper().clear();

    const QPair<QPointF,QPointF> boundaries = diagram()->dataBoundaries(); // cached

    const QPointF boundLeft = ctx->coordinatePlane()->translate( boundaries.first ) ;
    const QPointF boundRight = ctx->coordinatePlane()->translate( boundaries.second );

    const int rowCount = compressor().modelDataRows();
    const int colCount = compressor().modelDataColumns();

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

        if ( groupWidth  * rowCount > width )
            groupWidth = width / rowCount;
    }

    // maxLimit: allow the space between bars to be larger until area.width()
    // is covered by the groups.
    qreal maxLimit = rowCount * (groupWidth + ((colCount-1) * ba.fixedDataValueGap()) );


    //Pending Michel: FixMe
    if ( ba.useFixedDataValueGap() ) {
        if ( ctx->rectangle().width() > maxLimit )
            spaceBetweenBars += ba.fixedDataValueGap();
        else
            spaceBetweenBars = ((width/rowCount) - groupWidth)/(colCount-1);
    }

    if ( ba.useFixedValueBlockGap() )
        spaceBetweenGroups += ba.fixedValueBlockGap();

    calculateValueAndGapWidths( rowCount, colCount,groupWidth,
                                barWidth, spaceBetweenBars, spaceBetweenGroups );

    LabelPaintCache lpc;
    for ( int row = rowCount - 1; row >= 0; --row )
    {
        qreal offset = spaceBetweenGroups;
        if ( ba.useFixedBarWidth() )
            offset -= ba.fixedBarWidth();
        
        if ( offset < 0 )
            offset = 0;

        for ( int col = 0; col < colCount; ++col )
        {
        	qreal threeDOffset = 0.0;
            const CartesianDiagramDataCompressor::CachePosition position( row, col );
            const CartesianDiagramDataCompressor::DataPoint p = compressor().data( position );
 
            const QModelIndex index = attributesModel()->mapToSource( p.index );
            ThreeDBarAttributes threeDAttrs = diagram()->threeDBarAttributes( index );
            const qreal value = p.value;
            qreal stackedValues = 0.0;
            qreal key = 0.0;

            if ( threeDAttrs.isEnabled() ) {
                if ( barWidth > 0 ) {
                    barWidth =  (width - ((offset+(threeDAttrs.depth()))*rowCount))/ rowCount;
                	threeDOffset = threeDAttrs.depth();
                }
                if ( barWidth <= 0 ) {
                    barWidth = 0.1;
                    threeDOffset = (width - (offset*rowCount))/ rowCount;
                }
            } else {
                barWidth = (width - (offset*rowCount))/ rowCount;
            }

            for ( int k = col; k >= 0; --k )
            {
                const CartesianDiagramDataCompressor::CachePosition position( row, k );
                const CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );
                if ( ( p.value >= 0.0 && point.value >= 0.0 ) || ( p.value < 0.0 && point.value < 0.0 ) )
                    stackedValues += point.value;
                key = point.key;
            }
            QPointF point = ctx->coordinatePlane()->translate( QPointF( stackedValues, rowCount - key ) );
            point.ry() += offset / 2 + threeDOffset;
            const QPointF previousPoint = ctx->coordinatePlane()->translate( QPointF( stackedValues - value, rowCount - key ) );
            const qreal barHeight = point.x() - previousPoint.x();
            point.rx() -= barHeight;

            const QRectF rect( point, QSizeF( barHeight , barWidth ) );
            m_private->addLabel( &lpc, index, 0, PositionPoints( rect ), Position::North,
                                 Position::South, value );
            paintBars( ctx, index, rect, maxDepth );
        }
    }
    m_private->paintDataValueTextsAndMarkers( ctx, lpc, false );
}
