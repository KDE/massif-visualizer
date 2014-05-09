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

#include <limits>

#include <QAbstractItemModel>

#include "KDChartBarDiagram.h"
#include "KDChartLineDiagram.h"
#include "KDChartTextAttributes.h"
#include "KDChartAttributesModel.h"
#include "KDChartAbstractCartesianDiagram.h"
#include "KDChartNormalLineDiagram_p.h"
#include "PaintingHelpers_p.h"

using namespace KDChart;
using namespace std;

NormalLineDiagram::NormalLineDiagram( LineDiagram* d )
    : LineDiagramType( d )
{
}

LineDiagram::LineType NormalLineDiagram::type() const
{
    return LineDiagram::Normal;
}

const QPair< QPointF, QPointF > NormalLineDiagram::calculateDataBoundaries() const
{
    return compressor().dataBoundaries();
}

void NormalLineDiagram::paint( PaintContext* ctx )
{
    reverseMapper().clear();
    Q_ASSERT( dynamic_cast<CartesianCoordinatePlane*>( ctx->coordinatePlane() ) );
    CartesianCoordinatePlane* plane = static_cast<CartesianCoordinatePlane*>( ctx->coordinatePlane() );
    const int columnCount = compressor().modelDataColumns();
    const int rowCount = compressor().modelDataRows();
    if ( columnCount == 0 || rowCount == 0 ) return; // maybe blank out the area?

    // Reverse order of data sets?
    bool rev = diagram()->reverseDatasetOrder();
    LabelPaintCache lpc;
    LineAttributesInfoList lineList;

    const int step = rev ? -1 : 1;
    const int end = rev ? -1 : columnCount;
    for ( int column = rev ? columnCount - 1 : 0; column != end; column += step ) {
        LineAttributes laPreviousCell;
        CartesianDiagramDataCompressor::DataPoint lastPoint;
        qreal lastAreaBoundingValue = 0;

        // Get min. y value, used as lower or upper bounding for area highlighting
        const qreal minYValue = qMin(plane->visibleDataRange().bottom(), plane->visibleDataRange().top());

        CartesianDiagramDataCompressor::CachePosition previousCellPosition;
        for ( int row = 0; row < rowCount; ++row ) {
            const CartesianDiagramDataCompressor::CachePosition position( row, column );
            // get where to draw the line from:
            CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );
            if ( point.hidden ) {
                continue;
            }

            const QModelIndex sourceIndex = attributesModel()->mapToSource( point.index );

            const LineAttributes laCell = diagram()->lineAttributes( sourceIndex );
            const LineAttributes::MissingValuesPolicy policy = laCell.missingValuesPolicy();

            // lower or upper bounding for the highlighted area
            qreal areaBoundingValue;
            if ( laCell.areaBoundingDataset() != -1 ) {
                const CartesianDiagramDataCompressor::CachePosition areaBoundingCachePosition( row, laCell.areaBoundingDataset() );
                areaBoundingValue = compressor().data( areaBoundingCachePosition ).value;
            } else {
                // Use min. y value (i.e. zero line in most cases) if no bounding dataset is set
                areaBoundingValue = minYValue;
            }

            if ( ISNAN( point.value ) )
            {
                switch ( policy )
                {
                case LineAttributes::MissingValuesAreBridged:
                    // we just bridge both values
                    continue;
                case LineAttributes::MissingValuesShownAsZero:
                    // set it to zero
                    point.value = 0.0;
                    break;
                case LineAttributes::MissingValuesHideSegments:
                    // they're just hidden
                    break;
                default:
                    break;
                    // hm....
                }
            }

            if ( !ISNAN( point.value ) ) {
                // area corners, a + b are the line ends:
                const qreal offset = diagram()->centerDataPoints() ? 0.5 : 0;
                const QPointF a( plane->translate( QPointF( lastPoint.key + offset, lastPoint.value ) ) );
                const QPointF b( plane->translate( QPointF( point.key + offset, point.value ) ) );
                const QPointF c( plane->translate( QPointF( lastPoint.key + offset, lastAreaBoundingValue ) ) );
                const QPointF d( plane->translate( QPointF( point.key + offset, areaBoundingValue ) ) );
                const PositionPoints pts = PositionPoints( b, a, d, c );

                // add label
                m_private->addLabel( &lpc, sourceIndex, &position, pts, Position::NorthWest,
                                     Position::NorthWest, point.value );

                // add line and area, if switched on and we have a current and previous value
                if ( !ISNAN( lastPoint.value ) ) {
                    lineList.append( LineAttributesInfo( sourceIndex, a, b ) );

                    if ( laCell.displayArea() ) {
                        QList<QPolygonF> areas;
                        areas << ( QPolygonF() << a << b << d << c );
                        PaintingHelpers::paintAreas( m_private, ctx, attributesModel()->mapToSource( lastPoint.index ),
                                                     areas, laCell.transparency() );
                    }
                }
            }

            previousCellPosition = position;
            laPreviousCell = laCell;
            lastAreaBoundingValue = areaBoundingValue;
            lastPoint = point;
        }
    }

    // paint the lines
    PaintingHelpers::paintElements( m_private, ctx, lpc, lineList );
}
