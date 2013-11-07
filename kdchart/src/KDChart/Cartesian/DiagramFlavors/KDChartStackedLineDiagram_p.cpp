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

#include "KDChartStackedLineDiagram_p.h"

#include <QAbstractItemModel>

#include "KDChartBarDiagram.h"
#include "KDChartLineDiagram.h"
#include "KDChartTextAttributes.h"
#include "KDChartAttributesModel.h"
#include "KDChartAbstractCartesianDiagram.h"
#include "PaintingHelpers_p.h"

using namespace KDChart;
using namespace std;

StackedLineDiagram::StackedLineDiagram( LineDiagram* d )
    : LineDiagramType( d )
{
}

LineDiagram::LineType StackedLineDiagram::type() const
{
    return LineDiagram::Stacked;
}

const QPair<QPointF, QPointF> StackedLineDiagram::calculateDataBoundaries() const
{
    const int rowCount = compressor().modelDataRows();
    const int colCount = compressor().modelDataColumns();
    const qreal xMin = 0;
    qreal xMax = diagram()->model() ? diagram()->model()->rowCount( diagram()->rootIndex() ) : 0;
    if ( !diagram()->centerDataPoints() && diagram()->model() )
        xMax -= 1;
    qreal yMin = 0, yMax = 0;

    bool bStarting = true;
    for ( int row = 0; row < rowCount; ++row )
    {
        // calculate sum of values per column - Find out stacked Min/Max
        qreal stackedValues = 0.0;
        qreal negativeStackedValues = 0.0;
        for ( int col = datasetDimension() - 1; col < colCount; col += datasetDimension() ) {
            const CartesianDiagramDataCompressor::CachePosition position( row, col );
            const CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );

            if ( ISNAN( point.value ) )
                continue;

            if ( point.value >= 0.0 )
                stackedValues += point.value;
            else
                negativeStackedValues += point.value;
        }

        if ( bStarting ) {
            yMin = stackedValues;
            yMax = stackedValues;
            bStarting = false;
        } else {
            // take in account all stacked values
            yMin = qMin( qMin( yMin, negativeStackedValues ), stackedValues );
            yMax = qMax( qMax( yMax, negativeStackedValues ), stackedValues );
        }
    }

    const QPointF bottomLeft( xMin, yMin );
    const QPointF topRight( xMax, yMax );

    return QPair<QPointF, QPointF> ( bottomLeft, topRight );
}

void StackedLineDiagram::paint( PaintContext* ctx )
{
    reverseMapper().clear();

    const int columnCount = compressor().modelDataColumns();
    const int rowCount = compressor().modelDataRows();

// FIXME integrate column index retrieval to compressor:
//    int maxFound = 0;
//    {   // find the last column number that is not hidden
//        for ( int iColumn =  datasetDimension() - 1;
//             iColumn <  columnCount;
//             iColumn += datasetDimension() )
//            if ( ! diagram()->isHidden( iColumn ) )
//                maxFound = iColumn;
//    }
    //maxFound = columnCount;
    // ^^^ temp

    LabelPaintCache lpc;
    LineAttributesInfoList lineList;

    QVector< qreal > percentSumValues;

    QList<QPointF> bottomPoints;
    bool bFirstDataset = true;

    for ( int column = 0; column < columnCount; ++column )
    {
        CartesianDiagramDataCompressor::CachePosition previousCellPosition;

        //display area can be set by dataset ( == column) and/or by cell
        LineAttributes laPreviousCell; // by default no area is drawn
        QModelIndex indexPreviousCell;
        QList<QPolygonF> areas;
        QList<QPointF> points;

        for ( int row = 0; row < rowCount; ++row ) {
            const CartesianDiagramDataCompressor::CachePosition position( row, column );
            CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );
            const QModelIndex sourceIndex = attributesModel()->mapToSource( point.index );

            const LineAttributes laCell = diagram()->lineAttributes( sourceIndex );
            const bool bDisplayCellArea = laCell.displayArea();

            const LineAttributes::MissingValuesPolicy policy = laCell.missingValuesPolicy();

            if ( ISNAN( point.value ) && policy == LineAttributes::MissingValuesShownAsZero )
                point.value = 0.0;

            qreal stackedValues = 0, nextValues = 0, nextKey = 0;
            for ( int column2 = column; column2 >= 0; --column2 )
            {
                const CartesianDiagramDataCompressor::CachePosition position( row, column2 );
                const CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );
                if ( !ISNAN( point.value ) )
                {
                    stackedValues += point.value;
                }
                else if ( policy == LineAttributes::MissingValuesAreBridged )
                {
                    const qreal interpolation = interpolateMissingValue( position );
                    if ( !ISNAN( interpolation ) )
                        stackedValues += interpolation;
                }

                //qDebug() << valueForCell( iRow, iColumn2 );
                if ( row + 1 < rowCount ) {
                    const CartesianDiagramDataCompressor::CachePosition position( row + 1, column2 );
                    const CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );
                    if ( !ISNAN( point.value ) )
                    {
                        nextValues += point.value;
                    }
                    else if ( policy == LineAttributes::MissingValuesAreBridged )
                    {
                        const qreal interpolation = interpolateMissingValue( position );
                        if ( !ISNAN( interpolation ) )
                            nextValues += interpolation;
                    }
                    nextKey = point.key;
                }
            }
            //qDebug() << stackedValues << endl;
            const QPointF nextPoint = ctx->coordinatePlane()->translate( QPointF( diagram()->centerDataPoints() ? point.key + 0.5 : point.key, stackedValues ) );
            points << nextPoint;

            const QPointF ptNorthWest( nextPoint );
            const QPointF ptSouthWest(
                bDisplayCellArea
                ? ( bFirstDataset
                    ? ctx->coordinatePlane()->translate( QPointF( diagram()->centerDataPoints() ? point.key + 0.5 : point.key, 0.0 ) )
                    : bottomPoints.at( row )
                    )
                : nextPoint );
            QPointF ptNorthEast;
            QPointF ptSouthEast;

            if ( row + 1 < rowCount ) {
                QPointF toPoint = ctx->coordinatePlane()->translate( QPointF( diagram()->centerDataPoints() ? nextKey + 0.5 : nextKey, nextValues ) );
                lineList.append( LineAttributesInfo( sourceIndex, nextPoint, toPoint ) );
                ptNorthEast = toPoint;
                ptSouthEast =
                    bDisplayCellArea
                    ? ( bFirstDataset
                        ? ctx->coordinatePlane()->translate( QPointF( diagram()->centerDataPoints() ? nextKey + 0.5 : nextKey, 0.0 ) )
                        : bottomPoints.at( row + 1 )
                        )
                    : toPoint;
                if ( areas.count() && laCell != laPreviousCell ) {
                    PaintingHelpers::paintAreas( m_private, ctx, indexPreviousCell, areas, laPreviousCell.transparency() );
                    areas.clear();
                }
                if ( bDisplayCellArea ) {
                    QPolygonF poly;
                    poly << ptNorthWest << ptNorthEast << ptSouthEast << ptSouthWest;
                    areas << poly;
                    laPreviousCell = laCell;
                    indexPreviousCell = sourceIndex;
                } else {
                    //qDebug() << "no area shown for row"<<iRow<<"  column"<<iColumn;
                }
            } else {
                ptNorthEast = ptNorthWest;
                ptSouthEast = ptSouthWest;
            }

            const PositionPoints pts( ptNorthWest, ptNorthEast, ptSouthEast, ptSouthWest );
            if ( !ISNAN( point.value ) )
                m_private->addLabel( &lpc, sourceIndex, &position, pts, Position::NorthWest,
                                     Position::NorthWest, point.value );
        }
        if ( areas.count() ) {
            PaintingHelpers::paintAreas( m_private, ctx, indexPreviousCell, areas, laPreviousCell.transparency() );
            areas.clear();
        }
        bottomPoints = points;
        bFirstDataset = false;
    }
    PaintingHelpers::paintElements( m_private, ctx, lpc, lineList );
}
