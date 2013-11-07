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

#include "KDChartPercentPlotter_p.h"
#include "KDChartPlotter.h"
#include "PaintingHelpers_p.h"

#include <limits>

using namespace KDChart;
using namespace std;

PercentPlotter::PercentPlotter( Plotter* d )
    : PlotterType( d )
{
}

Plotter::PlotType PercentPlotter::type() const
{
    return Plotter::Percent;
}

const QPair< QPointF, QPointF > PercentPlotter::calculateDataBoundaries() const
{
    const int rowCount = compressor().modelDataRows();
    const int colCount = compressor().modelDataColumns();
    qreal xMin = std::numeric_limits< qreal >::quiet_NaN();
    qreal xMax = std::numeric_limits< qreal >::quiet_NaN();
    const qreal yMin = 0.0;
    const qreal yMax = 100.0;

    for ( int column = 0; column < colCount; ++column )
    {
        for ( int row = 0; row < rowCount; ++row )
        {
            const CartesianDiagramDataCompressor::CachePosition position( row, column );
            const CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );

            const qreal valueX = ISNAN( point.key ) ? 0.0 : point.key;

            if ( ISNAN( xMin ) )
            {
                xMin = valueX;
                xMax = valueX;
            }
            else
            {
                xMin = qMin( xMin, valueX );
                xMax = qMax( xMax, valueX );
            }
        }
    }

    // NOTE: calculateDataBoundaries must return the *real* data boundaries!
    //       i.e. we may NOT fake yMin to be qMin( 0.0, yMin )
    //       (khz, 2008-01-24)
    const QPointF bottomLeft( QPointF( xMin, yMin ) );
    const QPointF topRight( QPointF( xMax, yMax ) );
    return QPair< QPointF, QPointF >( bottomLeft, topRight );
}

class Value
{
public:
    Value()
        : value( std::numeric_limits< qreal >::quiet_NaN() )
    {
    }
    // allow implicit conversion
    Value( qreal value )
        : value( value )
    {
    }
    operator qreal() const
    {
        return value;
    }

private:
    qreal value;
};

void PercentPlotter::paint( PaintContext* ctx )
{
    reverseMapper().clear();

    Q_ASSERT( dynamic_cast< CartesianCoordinatePlane* >( ctx->coordinatePlane() ) );
    const CartesianCoordinatePlane* const plane = static_cast< CartesianCoordinatePlane* >( ctx->coordinatePlane() );
    const int colCount = compressor().modelDataColumns();
    const int rowCount = compressor().modelDataRows();

    if ( colCount == 0 || rowCount == 0 )
        return;

    LabelPaintCache lpc;

    // this map contains the y-values to each x-value
    QMap< qreal, QVector< QPair< Value, QModelIndex > > > diagramValues;

    for ( int col = 0; col < colCount; ++col )
    {
        for ( int row = 0; row < rowCount; ++row )
        {
            const CartesianDiagramDataCompressor::CachePosition position( row, col );
            const CartesianDiagramDataCompressor::DataPoint point = compressor().data( position );
            diagramValues[ point.key ].resize( colCount );
            diagramValues[ point.key ][ col ].first = point.value;
            diagramValues[ point.key ][ col ].second = point.index;
        }
    }

    // the sums of the y-values per x-value
    QMap< qreal, qreal > yValueSums;
    // the x-values
    QList< qreal > xValues = diagramValues.keys();
    // make sure it's sorted
    qSort( xValues );
    Q_FOREACH( const qreal xValue, xValues )
    {
        // the y-values to the current x-value
        QVector< QPair< Value, QModelIndex > >& yValues = diagramValues[ xValue ];
        Q_ASSERT( yValues.count() == colCount );

        for ( int column = 0; column < colCount; ++column )
        {
            QPair< Value, QModelIndex >& data = yValues[ column ];
            // if the index is invalid, there was no value. Let's interpolate.
            if ( !data.second.isValid() )
            {
                QPair< QPair< qreal, Value >, QModelIndex > left;
                QPair< QPair< qreal, Value >, QModelIndex > right;
                int xIndex = 0;
                // let's find the next lower value
                for ( xIndex = xValues.indexOf( xValue ); xIndex >= 0; --xIndex )
                {
                    if ( diagramValues[ xValues[ xIndex ] ][ column ].second.isValid() )
                    {
                        left.first.first = xValues[ xIndex ];
                        left.first.second = diagramValues[ left.first.first ][ column ].first;
                        left.second = diagramValues[ xValues[ xIndex ] ][ column ].second;
                        break;
                    }
                }
                // let's find the next higher value
                for ( xIndex = xValues.indexOf( xValue ); xIndex < xValues.count(); ++xIndex )
                {
                    if ( diagramValues[ xValues[ xIndex ] ][ column ].second.isValid() )
                    {
                        right.first.first = xValues[ xIndex ];
                        right.first.second = diagramValues[ right.first.first ][ column ].first;
                        right.second = diagramValues[ xValues[ xIndex ] ][ column ].second;
                        break;
                    }
                }

                // interpolate out of them (left and/or right might be invalid, but this doesn't matter here)
                const qreal leftX = left.first.first;
                const qreal rightX = right.first.first;
                const qreal leftY = left.first.second;
                const qreal rightY = right.first.second;

                data.first = leftY + ( rightY - leftY ) * ( xValue - leftX ) / ( rightX - leftX );
                // if the result is a valid value, let's assign the index, too
                if ( !ISNAN( data.first.operator qreal() ) )
                    data.second = left.second;
            }

            // sum it up
            if ( !ISNAN( yValues[ column ].first.operator qreal() ) )
                yValueSums[ xValue ] += yValues[ column ].first;
        }
    }

    for ( int column = 0; column < colCount; ++column )
    {
        LineAttributesInfoList lineList;
        LineAttributes laPreviousCell;
        CartesianDiagramDataCompressor::CachePosition previousCellPosition;

        CartesianDiagramDataCompressor::DataPoint lastPoint;

        qreal lastExtraY = 0.0;
        qreal lastValue = 0.0;

        QMapIterator< qreal, QVector< QPair< Value, QModelIndex > > >  i( diagramValues );
        while ( i.hasNext() )
        {
            i.next();
            CartesianDiagramDataCompressor::DataPoint point;
            point.key = i.key();
            const QPair< Value, QModelIndex >& data = i.value().at( column );
            point.value = data.first;
            point.index = data.second;

            if ( ISNAN( point.key ) || ISNAN( point.value ) )
            {
                previousCellPosition = CartesianDiagramDataCompressor::CachePosition();
                continue;
            }

            qreal extraY = 0.0;
            for ( int col = column - 1; col >= 0; --col )
            {
                const qreal y = i.value().at( col ).first;
                if ( !ISNAN( y ) )
                    extraY += y;
            }

            LineAttributes laCell;

            const qreal value = ( point.value + extraY ) / yValueSums[ i.key() ] * 100;

            const QModelIndex sourceIndex = attributesModel()->mapToSource( point.index );
            // area corners, a + b are the line ends:
            const QPointF a( plane->translate( QPointF( lastPoint.key, lastValue ) ) );
            const QPointF b( plane->translate( QPointF( point.key, value ) ) );
            const QPointF c( plane->translate( QPointF( lastPoint.key, lastExtraY / yValueSums[ i.key() ] * 100 ) ) );
            const QPointF d( plane->translate( QPointF( point.key, extraY / yValueSums[ i.key() ] * 100 ) ) );
            // add the line to the list:
            laCell = diagram()->lineAttributes( sourceIndex );
            // add data point labels:
            const PositionPoints pts = PositionPoints( b, a, d, c );
            // if necessary, add the area to the area list:
            QList<QPolygonF> areas;
            if ( laCell.displayArea() ) {
                QPolygonF polygon;
                polygon << a << b << d << c;
                areas << polygon;
            }
            // add the pieces to painting if this is not hidden:
            if ( !point.hidden /*&& !ISNAN( lastPoint.key ) && !ISNAN( lastPoint.value ) */) {
                m_private->addLabel( &lpc, sourceIndex, 0, pts, Position::NorthWest,
                                     Position::NorthWest, value );
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
            lastExtraY = extraY;
            lastValue = value;
        }
        PaintingHelpers::paintElements( m_private, ctx, lpc, lineList );
    }
}
