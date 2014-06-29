/* -*- Mode: C++ -*-
   KDChart - a multi-platform charting engine
   */

/****************************************************************************
 ** Copyright (C) 2005-2007 Klarälvdalens Datakonsult AB.  All rights reserved.
 **
 ** This file is part of the KD Chart library.
 **
 ** This file may be used under the terms of the GNU General Public
 ** License versions 2.0 or 3.0 as published by the Free Software
 ** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
 ** included in the packaging of this file.  Alternatively you may (at
 ** your option) use any later version of the GNU General Public
 ** License if such license has been publicly approved by
 ** Klarälvdalens Datakonsult AB (or its successors, if any).
 **
 ** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
 ** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
 ** A PARTICULAR PURPOSE. Klarälvdalens Datakonsult AB reserves all rights
 ** not expressly granted herein.
 **
 ** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 ** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 **
 **********************************************************************/

#include "KDChartStackedPlotter_p.h"
#include "KDChartPlotter.h"

#include <limits>

using namespace KDChart;
using namespace std;

StackedPlotter::StackedPlotter( Plotter* d )
    : PlotterType( d )
{
}

Plotter::PlotType StackedPlotter::type() const
{
    return Plotter::Stacked;
}

const QPair< QPointF, QPointF > StackedPlotter::calculateDataBoundaries() const
{
    const int rowCount = compressor().modelDataRows();
    const int colCount = compressor().modelDataColumns();
    double xMin = 0, xMax = 0;
    double yMin = 0, yMax = 0;

    bool bStarting = true;
    for( int row = 0; row < rowCount; ++row )
    {
        // calculate sum of values per column - Find out stacked Min/Max
        double stackedValues = 0.0;
        double negativeStackedValues = 0.0;
        for( int col = 0; col < colCount; ++col ) {
            const CartesianDiagramDataCompressor::CachePosition position( row, col );
            const CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );

            if( ISNAN( point.value ) )
                continue;

            if( point.value >= 0.0 )
                stackedValues += point.value;
            else
                negativeStackedValues += point.value;
        }

        //assume that each value in this row has the same x-value
        //very unintuitive...
        const CartesianDiagramDataCompressor::CachePosition xPosition( row, 0 );
        const CartesianDiagramDataCompressor::DataPoint xPoint = compressor().data( xPosition );
        if( bStarting ){
            yMin = stackedValues;
            yMax = stackedValues;
            xMax = xPoint.key;
            xMin = xPoint.key;
            bStarting = false;
        }else{
            // take in account all stacked values
            yMin = qMin( qMin( yMin, negativeStackedValues ), stackedValues );
            yMax = qMax( qMax( yMax, negativeStackedValues ), stackedValues );
            xMin = qMin( qreal(xMin), xPoint.key );
            xMax = qMax( qreal(xMax), xPoint.key );
        }
    }

    const QPointF bottomLeft( xMin, yMin );
    const QPointF topRight( xMax, yMax );

    return QPair<QPointF, QPointF> ( bottomLeft, topRight );
}

void StackedPlotter::paint( PaintContext* ctx )
{
    reverseMapper().clear();

    const QPair<QPointF, QPointF> boundaries = diagram()->dataBoundaries();
    const QPointF bottomLeft = boundaries.first;
    const QPointF topRight = boundaries.second;

    const int columnCount = compressor().modelDataColumns();
    const int rowCount = compressor().modelDataRows();

// FIXME integrate column index retrieval to compressor:
    int maxFound = 0;
//    {   // find the last column number that is not hidden
//        for( int iColumn =  /*datasetDimension()*/2 - 1;
//             iColumn <  columnCount;
//             iColumn += /*datasetDimension()*/2 )
//            if( ! diagram()->isHidden( iColumn ) )
//                maxFound = iColumn;
//    }
    maxFound = columnCount;
    // ^^^ temp

    DataValueTextInfoList list;
    LineAttributesInfoList lineList;

    //FIXME(khz): add LineAttributes::MissingValuesPolicy support for LineDiagram::Stacked and ::Percent

    QVector <double > percentSumValues;

    QList<QPointF> bottomPoints;
    bool bFirstDataset = true;

    for( int column = 0; column < columnCount; ++column )
    {
        CartesianDiagramDataCompressor::CachePosition previousCellPosition;

        //display area can be set by dataset ( == column) and/or by cell
        QList<QPointF> points;

        for ( int row = 0; row < rowCount; ++row ) {
            const CartesianDiagramDataCompressor::CachePosition position( row, column );
            CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );
            const QModelIndex sourceIndex = attributesModel()->mapToSource( point.index );

            const LineAttributes laCell = diagram()->lineAttributes( sourceIndex );
            const bool bDisplayCellArea = laCell.displayArea();

            const LineAttributes::MissingValuesPolicy policy = laCell.missingValuesPolicy();

            if( ISNAN( point.value ) && policy == LineAttributes::MissingValuesShownAsZero )
                point.value = 0.0;

            double stackedValues = 0, nextValues = 0, nextKey = 0;
            for ( int column2 = column; column2 >= 0; --column2 )
            {
                const CartesianDiagramDataCompressor::CachePosition position( row, column2 );
                const CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );
                const QModelIndex sourceIndex = attributesModel()->mapToSource( point.index );
                if( !ISNAN( point.value ) )
                {
                    stackedValues += point.value;
                }
                else if( policy == LineAttributes::MissingValuesAreBridged )
                {
                    const double interpolation = interpolateMissingValue( position );
                    if( !ISNAN( interpolation ) )
                        stackedValues += interpolation;
                }

                //qDebug() << valueForCell( iRow, iColumn2 );
                if ( row + 1 < rowCount ){
                    const CartesianDiagramDataCompressor::CachePosition position( row + 1, column2 );
                    const CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );
                    if( !ISNAN( point.value ) )
                    {
                        nextValues += point.value;
                    }
                    else if( policy == LineAttributes::MissingValuesAreBridged )
                    {
                        const double interpolation = interpolateMissingValue( position );
                        if( !ISNAN( interpolation ) )
                            nextValues += interpolation;
                    }
                    nextKey = point.key;
                }
            }
            //qDebug() << stackedValues << endl;
            const QPointF nextPoint = ctx->coordinatePlane()->translate( QPointF( point.key, stackedValues ) );
            points << nextPoint;

            const QPointF ptNorthWest( nextPoint );
            const QPointF ptSouthWest(
                bDisplayCellArea
                ? ( bFirstDataset
                    ? ctx->coordinatePlane()->translate( QPointF( point.key, 0.0 ) )
                    : bottomPoints.at( row )
                    )
                : nextPoint );
            QPointF ptNorthEast;
            QPointF ptSouthEast;

            if ( row + 1 < rowCount ){
                QPointF toPoint = ctx->coordinatePlane()->translate( QPointF( nextKey, nextValues ) );
                lineList.append( LineAttributesInfo( sourceIndex, nextPoint, toPoint ) );
                ptNorthEast = toPoint;
                ptSouthEast =
                    bDisplayCellArea
                    ? ( bFirstDataset
                        ? ctx->coordinatePlane()->translate( QPointF( nextKey, 0.0 ) )
                        : bottomPoints.at( row + 1 )
                        )
                    : toPoint;
                if( bDisplayCellArea ){
                    QPolygonF poly;
                    poly << ptNorthWest << ptNorthEast << ptSouthEast << ptSouthWest;
                    QList<QPolygonF> areas;
                    areas << poly;
                    paintAreas( ctx, sourceIndex, areas, laCell.transparency() );
                }else{
                    //qDebug() << "no area shown for row"<<iRow<<"  column"<<iColumn;
                }
            }else{
                ptNorthEast = ptNorthWest;
                ptSouthEast = ptSouthWest;
            }

            if( !ISNAN( point.value ) ) {
                const PositionPoints pts( ptNorthWest, ptNorthEast, ptSouthEast, ptSouthWest );
                appendDataValueTextInfoToList( diagram(), list, sourceIndex,
                                               pts, Position::NorthWest, Position::SouthWest,
                                               point.value );
            }
        }
        bottomPoints = points;
        bFirstDataset = false;
    }
    paintElements( ctx, list, lineList, KDChart::LineAttributes::MissingValuesAreBridged );
}

double StackedPlotter::interpolateMissingValue( const CartesianDiagramDataCompressor::CachePosition& pos ) const
{
    double leftValue = std::numeric_limits< double >::quiet_NaN();
    double rightValue = std::numeric_limits< double >::quiet_NaN();
    int missingCount = 1;

    const int column = pos.second;
    const int row = pos.first;
    const int rowCount = compressor().modelDataRows();

    // iterate back and forth to find valid values
    for( int r1 = row - 1; r1 > 0; --r1 )
    {
        const CartesianDiagramDataCompressor::CachePosition position( r1, column );
        const CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );
        leftValue = point.value;
        if( !ISNAN( point.value ) )
            break;
        ++missingCount;
    }
    for( int r2 = row + 1; r2 < rowCount; ++r2 )
    {
        const CartesianDiagramDataCompressor::CachePosition position( r2, column );
        const CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );
        rightValue = point.value;
        if( !ISNAN( point.value ) )
            break;
        ++missingCount;
    }
    if( !ISNAN( leftValue ) && !ISNAN( rightValue ) )
        return leftValue + ( rightValue - leftValue ) / ( missingCount + 1 );
    else
        return std::numeric_limits< double >::quiet_NaN();
}
