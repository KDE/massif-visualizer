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

#include "KDChartNormalPlotter_p.h"
#include "KDChartPlotter.h"
#include "PaintingHelpers_p.h"

#include <limits>

using namespace KDChart;
using namespace std;

NormalPlotter::NormalPlotter( Plotter* d )
    : PlotterType( d )
{
}

Plotter::PlotType NormalPlotter::type() const
{
    return Plotter::Normal;
}

const QPair< QPointF, QPointF > NormalPlotter::calculateDataBoundaries() const
{
    if ( diagram()->useDataCompression() != Plotter::NONE )
        return plotterCompressor().dataBoundaries();
    else
        return compressor().dataBoundaries();
}

static bool isFinite( const QPointF &point )
{
    return !ISINF( point.x() ) && !ISNAN( point.x() ) && !ISINF( point.y() ) && !ISNAN( point.y() );
}

void NormalPlotter::paint( PaintContext* ctx )
{
    reverseMapper().clear();

    Q_ASSERT( dynamic_cast< CartesianCoordinatePlane* >( ctx->coordinatePlane() ) );
    const CartesianCoordinatePlane* const plane = static_cast< CartesianCoordinatePlane* >( ctx->coordinatePlane() );
    const int colCount = compressor().modelDataColumns();
    const int rowCount = compressor().modelDataRows();    

    LabelPaintCache lpc;

    if ( diagram()->useDataCompression() != Plotter::NONE )
    {
        for ( int dataset = 0; dataset < plotterCompressor().datasetCount(); ++dataset )
        {
            LineAttributesInfoList lineList;
            LineAttributes laPreviousCell;
            PlotterDiagramCompressor::DataPoint lastPoint;
            for ( PlotterDiagramCompressor::Iterator it = plotterCompressor().begin( dataset ); it != plotterCompressor().end( dataset ); ++ it )
            {
                const PlotterDiagramCompressor::DataPoint point = *it;

                const QModelIndex sourceIndex = attributesModel()->mapToSource( point.index );
                LineAttributes laCell = diagram()->lineAttributes( sourceIndex );
                const LineAttributes::MissingValuesPolicy policy = laCell.missingValuesPolicy();

                if ( ISNAN( point.key ) || ISNAN( point.value ) )
                {
                    switch ( policy )
                    {
                    case LineAttributes::MissingValuesAreBridged: // we just bridge both values
                        continue;
                    case LineAttributes::MissingValuesShownAsZero: // fall-through since that attribute makes no sense for the plotter
                    case LineAttributes::MissingValuesHideSegments: // fall-through since they're just hidden
                    default:
                        lastPoint = PlotterDiagramCompressor::DataPoint();
                        continue;
                    }
                }

                // area corners, a + b are the line ends:
                const QPointF a( plane->translate( QPointF( lastPoint.key, lastPoint.value ) ) );
                const QPointF b( plane->translate( QPointF( point.key, point.value ) ) );
                if ( a.toPoint() == b.toPoint() )
                    continue;

                const QPointF c( plane->translate( QPointF( lastPoint.key, 0.0 ) ) );
                const QPointF d( plane->translate( QPointF( point.key, 0.0 ) ) );

                // add the pieces to painting if this is not hidden:
                if ( !point.hidden /*&& !ISNAN( lastPoint.key ) && !ISNAN( lastPoint.value ) */) {
                    // add data point labels:
                    const PositionPoints pts = PositionPoints( b, a, d, c );
                    // if necessary, add the area to the area list:
                    QList<QPolygonF> areas;
                    if ( laCell.displayArea() ) {
                        QPolygonF polygon;
                        polygon << a << b << d << c;
                        areas << polygon;
                    }
                    m_private->addLabel( &lpc, sourceIndex, 0, pts, Position::NorthWest,
                                         Position::NorthWest, point.value );
                    if ( !ISNAN( lastPoint.key ) && !ISNAN( lastPoint.value ) )
                    {
                        PaintingHelpers::paintAreas( m_private, ctx,
                                                     attributesModel()->mapToSource( lastPoint.index ),
                                                     areas, laCell.transparency() );
                        lineList.append( LineAttributesInfo( sourceIndex, a, b ) );
                    }
                }

                // wrap it up:
                laPreviousCell = laCell;
                lastPoint = point;
            }
            PaintingHelpers::paintElements( m_private, ctx, lpc, lineList );
        }
    }
    else
    {
        if ( colCount == 0 || rowCount == 0 )
            return;
        for ( int column = 0; column < colCount; ++column )
        {
            LineAttributesInfoList lineList;
            LineAttributes laPreviousCell;
            CartesianDiagramDataCompressor::CachePosition previousCellPosition;
            CartesianDiagramDataCompressor::DataPoint lastPoint;

            for ( int row = 0; row < rowCount; ++row )
            {
                const CartesianDiagramDataCompressor::CachePosition position( row, column );
                const CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );

                const QModelIndex sourceIndex = attributesModel()->mapToSource( point.index );
                LineAttributes laCell = diagram()->lineAttributes( sourceIndex );
                const LineAttributes::MissingValuesPolicy policy = laCell.missingValuesPolicy();

                if ( ISNAN( point.key ) || ISNAN( point.value ) )
                {
                    switch ( policy )
                    {
                    case LineAttributes::MissingValuesAreBridged: // we just bridge both values
                        continue;
                    case LineAttributes::MissingValuesShownAsZero: // fall-through since that attribute makes no sense for the plotter
                    case LineAttributes::MissingValuesHideSegments: // fall-through since they're just hidden
                    default:
                        previousCellPosition = CartesianDiagramDataCompressor::CachePosition();
                        lastPoint = CartesianDiagramDataCompressor::DataPoint();
                        continue;
                    }
                }

                // area corners, a + b are the line ends:
                const QPointF a( plane->translate( QPointF( lastPoint.key, lastPoint.value ) ) );
                const QPointF b( plane->translate( QPointF( point.key, point.value ) ) );
                if ( a.toPoint() == b.toPoint() || !isFinite( a ) || !isFinite( b ) ) {
                    lastPoint = point;
                    continue;
                }

                const QPointF c( plane->translate( QPointF( lastPoint.key, 0.0 ) ) );
                const QPointF d( plane->translate( QPointF( point.key, 0.0 ) ) );

                // add the pieces to painting if this is not hidden:
                if ( !point.hidden ) {
                    // add data point labels:
                    const PositionPoints pts = PositionPoints( b, a, d, c );
                    // if necessary, add the area to the area list:
                    QList<QPolygonF> areas;
                    if ( laCell.displayArea() ) {
                        QPolygonF polygon;
                        polygon << a << b << d << c;
                        areas << polygon;
                    }
                    m_private->addLabel( &lpc, sourceIndex, 0, pts, Position::NorthWest,
                                         Position::NorthWest, point.value );
                    PaintingHelpers::paintAreas( m_private, ctx,
                                                 attributesModel()->mapToSource( lastPoint.index ),
                                                 areas, laCell.transparency() );
                    lineList.append( LineAttributesInfo( sourceIndex, a, b ) );
                }

                // wrap it up:
                previousCellPosition = position;
                laPreviousCell = laCell;
                lastPoint = point;
            }
            PaintingHelpers::paintElements( m_private, ctx, lpc, lineList );
        }
    }
}
