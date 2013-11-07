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

#include "KDChartCartesianGrid.h"
#include "KDChartAbstractCartesianDiagram.h"
#include "KDChartPaintContext.h"
#include "KDChartPainterSaver_p.h"
#include "KDChartPrintingParameters.h"
#include "KDChartFrameAttributes.h"
#include "KDChartCartesianAxis_p.h"

#include <QPainter>

#include <KDABLibFakes>

#include <limits>

using namespace KDChart;

CartesianGrid::CartesianGrid()
    : AbstractGrid(), m_minsteps( 2 ), m_maxsteps( 12 )
{
}

CartesianGrid::~CartesianGrid()
{
}
        
int CartesianGrid::minimalSteps() const
{
    return m_minsteps;
}

void CartesianGrid::setMinimalSteps(int minsteps)
{
    m_minsteps = minsteps;
}

int CartesianGrid::maximalSteps() const
{
    return m_maxsteps;
}

void CartesianGrid::setMaximalSteps(int maxsteps)
{
    m_maxsteps = maxsteps;
}

void CartesianGrid::drawGrid( PaintContext* context )
{
    CartesianCoordinatePlane* plane = qobject_cast< CartesianCoordinatePlane* >( context->coordinatePlane() );
    const GridAttributes gridAttrsX( plane->gridAttributes( Qt::Horizontal ) );
    const GridAttributes gridAttrsY( plane->gridAttributes( Qt::Vertical ) );
    if ( !gridAttrsX.isGridVisible() && !gridAttrsX.isSubGridVisible() &&
         !gridAttrsY.isGridVisible() && !gridAttrsY.isSubGridVisible() ) {
        return;
    }
    // This plane is used for translating the coordinates - not for the data boundaries
    QPainter *p = context->painter();
    PainterSaver painterSaver( p );
    // sharedAxisMasterPlane() changes the painter's coordinate transformation(!)
    plane = qobject_cast< CartesianCoordinatePlane* >( plane->sharedAxisMasterPlane( context->painter() ) );
    Q_ASSERT_X ( plane, "CartesianGrid::drawGrid",
                 "Bad function call: PaintContext::coodinatePlane() NOT a cartesian plane." );

    // update the calculated mDataDimensions before using them
    updateData( context->coordinatePlane() ); // this, in turn, calls our calculateGrid().
    Q_ASSERT_X ( mDataDimensions.count() == 2, "CartesianGrid::drawGrid",
                 "Error: updateData did not return exactly two dimensions." );
    if ( !isBoundariesValid( mDataDimensions ) ) {
        return;
    }

    const DataDimension dimX = mDataDimensions.first();
    const DataDimension dimY = mDataDimensions.last();
    const bool isLogarithmicX = dimX.calcMode == AbstractCoordinatePlane::Logarithmic;
    const bool isLogarithmicY = dimY.calcMode == AbstractCoordinatePlane::Logarithmic;

    qreal minValueX = qMin( dimX.start, dimX.end );
    qreal maxValueX = qMax( dimX.start, dimX.end );
    qreal minValueY = qMin( dimY.start, dimY.end );
    qreal maxValueY = qMax( dimY.start, dimY.end );
    {
        bool adjustXLower = !isLogarithmicX && gridAttrsX.adjustLowerBoundToGrid();
        bool adjustXUpper = !isLogarithmicX && gridAttrsX.adjustUpperBoundToGrid();
        bool adjustYLower = !isLogarithmicY && gridAttrsY.adjustLowerBoundToGrid();
        bool adjustYUpper = !isLogarithmicY && gridAttrsY.adjustUpperBoundToGrid();
        AbstractGrid::adjustLowerUpperRange( minValueX, maxValueX, dimX.stepWidth, adjustXLower, adjustXUpper );
        AbstractGrid::adjustLowerUpperRange( minValueY, maxValueY, dimY.stepWidth, adjustYLower, adjustYUpper );
    }

   if ( plane->frameAttributes().isVisible() ) {
        const qreal radius = plane->frameAttributes().cornerRadius();
        QPainterPath path;
        path.addRoundedRect( QRectF( plane->translate( QPointF( minValueX, minValueY ) ),
                                     plane->translate( QPointF( maxValueX, maxValueY ) ) ),
                             radius, radius );
        context->painter()->setClipPath( path );
    }


    /* TODO features from old code:
       - MAYBE coarsen the main grid when it gets crowded (do it in calculateGrid or here?)
        if ( ! dimX.isCalculated ) {
            while ( screenRangeX / numberOfUnitLinesX <= MinimumPixelsBetweenLines ) {
                dimX.stepWidth *= 10.0;
                dimX.subStepWidth *= 10.0;
                numberOfUnitLinesX = qAbs( dimX.distance() / dimX.stepWidth );
            }
        }
       - MAYBE deactivate the sub-grid when it gets crowded
        if ( dimX.subStepWidth && (screenRangeX / (dimX.distance() / dimX.subStepWidth)
             <= MinimumPixelsBetweenLines) ) {
            // de-activating grid sub steps: not enough space
            dimX.subStepWidth = 0.0;
        }
    */

    const int labelThinningFactor = 1; // TODO: do we need this here?

    for ( int i = 0; i < 2; i++ ) {
        XySwitch xy( i == 1 ); // first iteration paints the X grid lines, second paints the Y grid lines
        const GridAttributes& gridAttrs = xy( gridAttrsX, gridAttrsY );
        bool hasMajorLines = gridAttrs.isGridVisible();
        bool hasMinorLines = hasMajorLines && gridAttrs.isSubGridVisible();
        if ( !hasMajorLines && !hasMinorLines ) {
            continue;
        }

        const DataDimension& dimension = xy( dimX, dimY );
        const bool drawZeroLine = dimension.isCalculated && gridAttrs.zeroLinePen().style() != Qt::NoPen;

        QPointF lineStart = QPointF( minValueX, minValueY ); // still need transformation to screen space
        QPointF lineEnd = QPointF( maxValueX, maxValueY );

        TickIterator it( xy.isY, dimension, hasMajorLines, hasMinorLines, plane, labelThinningFactor );
        for ( ; !it.isAtEnd(); ++it ) {
            if ( !gridAttrs.isOuterLinesVisible() &&
                 ( it.areAlmostEqual( it.position(), xy( minValueX, minValueY ) ) ||
                   it.areAlmostEqual( it.position(), xy( maxValueX, maxValueY ) ) ) ) {
                continue;
            }
            xy.lvalue( lineStart.rx(), lineStart.ry() ) = it.position();
            xy.lvalue( lineEnd.rx(), lineEnd.ry() ) = it.position();
            QPointF transLineStart = plane->translate( lineStart );
            QPointF transLineEnd = plane->translate( lineEnd );
            if ( ISNAN( transLineStart.x() ) || ISNAN( transLineStart.y() ) ||
                 ISNAN( transLineEnd.x() ) || ISNAN( transLineEnd.y() ) ) {
                // ### can we catch NaN problems earlier, wasting fewer cycles?
                continue;
            }
            if ( it.position() == 0.0 && drawZeroLine ) {
                p->setPen( PrintingParameters::scalePen( gridAttrsX.zeroLinePen() ) );
            } else if ( it.type() == TickIterator::MinorTick ) {
                p->setPen( PrintingParameters::scalePen( gridAttrs.subGridPen() ) );
            } else {
                p->setPen( PrintingParameters::scalePen( gridAttrs.gridPen() ) );
            }
            p->drawLine( transLineStart, transLineEnd );
        }
    }
}


DataDimensionsList CartesianGrid::calculateGrid(
    const DataDimensionsList& rawDataDimensions ) const
{
    Q_ASSERT_X ( rawDataDimensions.count() == 2, "CartesianGrid::calculateGrid",
                 "Error: calculateGrid() expects a list with exactly two entries." );

    CartesianCoordinatePlane* plane = qobject_cast< CartesianCoordinatePlane* >( mPlane );
    Q_ASSERT_X ( plane, "CartesianGrid::calculateGrid",
                 "Error: PaintContext::calculatePlane() called, but no cartesian plane set." );

    DataDimensionsList l( rawDataDimensions );
#if 0
    qDebug() << Q_FUNC_INFO << "initial grid X-range:" << l.first().start << "->" << l.first().end
             << "   substep width:" << l.first().subStepWidth;
    qDebug() << Q_FUNC_INFO << "initial grid Y-range:" << l.last().start << "->" << l.last().end
             << "   substep width:" << l.last().subStepWidth;
#endif
    // rule:  Returned list is either empty, or it is providing two
    //        valid dimensions, complete with two non-Zero step widths.
    if ( isBoundariesValid( l ) ) {
        const QPointF translatedBottomLeft( plane->translateBack( plane->geometry().bottomLeft() ) );
        const QPointF translatedTopRight( plane->translateBack( plane->geometry().topRight() ) );

        const GridAttributes gridAttrsX( plane->gridAttributes( Qt::Horizontal ) );
        const GridAttributes gridAttrsY( plane->gridAttributes( Qt::Vertical ) );

        const DataDimension dimX
                = calculateGridXY( l.first(), Qt::Horizontal,
                                   gridAttrsX.adjustLowerBoundToGrid(),
                                   gridAttrsX.adjustUpperBoundToGrid() );
        if ( dimX.stepWidth ) {
            //qDebug("CartesianGrid::calculateGrid()   l.last().start:  %f   l.last().end:  %f", l.last().start, l.last().end);
            //qDebug("                                 l.first().start: %f   l.first().end: %f", l.first().start, l.first().end);

            // one time for the min/max value
            const DataDimension minMaxY
                    = calculateGridXY( l.last(), Qt::Vertical,
                                       gridAttrsY.adjustLowerBoundToGrid(),
                                       gridAttrsY.adjustUpperBoundToGrid() );

            if ( plane->autoAdjustGridToZoom()
                && plane->axesCalcModeY() == CartesianCoordinatePlane::Linear
                && plane->zoomFactorY() > 1.0 )
            {
                l.last().start = translatedBottomLeft.y();
                l.last().end   = translatedTopRight.y();
            }
            // and one other time for the step width
            const DataDimension dimY
                    = calculateGridXY( l.last(), Qt::Vertical,
                                       gridAttrsY.adjustLowerBoundToGrid(),
                                       gridAttrsY.adjustUpperBoundToGrid() );
            if ( dimY.stepWidth ) {
                l.first().start        = dimX.start;
                l.first().end          = dimX.end;
                l.first().stepWidth    = dimX.stepWidth;
                l.first().subStepWidth = dimX.subStepWidth;
                l.last().start        = minMaxY.start;
                l.last().end          = minMaxY.end;
                l.last().stepWidth    = dimY.stepWidth;
                l.last().subStepWidth    = dimY.subStepWidth;
                //qDebug() << "CartesianGrid::calculateGrid()  final grid y-range:" << l.last().end - l.last().start << "   step width:" << l.last().stepWidth << endl;
                // calculate some reasonable subSteps if the
                // user did not set the sub grid but did set
                // the stepWidth.
                
                // FIXME (Johannes)
                // the last (y) dimension is not always the dimension for the ordinate!
                // since there's no way to check for the orientation of this dimension here,
                // we cannot automatically assume substep values
                //if ( dimY.subStepWidth == 0 )
                //    l.last().subStepWidth = dimY.stepWidth/2;
                //else
                //    l.last().subStepWidth = dimY.subStepWidth;
            }
        }
    }
#if 0
    qDebug() << Q_FUNC_INFO << "final grid X-range:" << l.first().start << "->" << l.first().end
             << "   substep width:" << l.first().subStepWidth;
    qDebug() << Q_FUNC_INFO << "final grid Y-range:" << l.last().start << "->" << l.last().end
             << "   substep width:" << l.last().subStepWidth;
#endif
    return l;
}


qreal fastPow10( int x )
{
    qreal res = 1.0;
    if ( 0 <= x ) {
        for ( int i = 1; i <= x; ++i )
            res *= 10.0;
    } else {
        for ( int i = -1; i >= x; --i )
            res /= 10.0;
    }
    return res;
}

#if defined ( Q_OS_WIN)
#define trunc(x) ((int)(x))
#endif

DataDimension CartesianGrid::calculateGridXY(
    const DataDimension& rawDataDimension,
    Qt::Orientation orientation,
    bool adjustLower, bool adjustUpper ) const
{
    CartesianCoordinatePlane* const plane = dynamic_cast<CartesianCoordinatePlane*>( mPlane );
    if ( ( orientation == Qt::Vertical && plane->autoAdjustVerticalRangeToData() >= 100 ) ||
         ( orientation == Qt::Horizontal && plane->autoAdjustHorizontalRangeToData() >= 100 ) ) {
        adjustLower = false;
        adjustUpper = false;
    }

    DataDimension dim( rawDataDimension );
    if ( dim.isCalculated && dim.start != dim.end ) {
        if ( dim.calcMode == AbstractCoordinatePlane::Linear ) {
            // linear ( == not-logarithmic) calculation
            if ( dim.stepWidth == 0.0 ) {
                QList<qreal> granularities;
                switch ( dim.sequence ) {
                    case KDChartEnums::GranularitySequence_10_20:
                        granularities << 1.0 << 2.0;
                        break;
                    case KDChartEnums::GranularitySequence_10_50:
                        granularities << 1.0 << 5.0;
                        break;
                    case KDChartEnums::GranularitySequence_25_50:
                        granularities << 2.5 << 5.0;
                        break;
                    case KDChartEnums::GranularitySequence_125_25:
                        granularities << 1.25 << 2.5;
                        break;
                    case KDChartEnums::GranularitySequenceIrregular:
                        granularities << 1.0 << 1.25 << 2.0 << 2.5 << 5.0;
                        break;
                }
                //qDebug("CartesianGrid::calculateGridXY()   dim.start: %f   dim.end: %f", dim.start, dim.end);
                calculateStepWidth(
                    dim.start, dim.end, granularities, orientation,
                    dim.stepWidth, dim.subStepWidth,
                    adjustLower, adjustUpper );
            }
            // if needed, adjust start/end to match the step width:
            //qDebug() << "CartesianGrid::calculateGridXY() has 1st linear range: min " << dim.start << " and max" << dim.end;

            AbstractGrid::adjustLowerUpperRange( dim.start, dim.end, dim.stepWidth,
                    adjustLower, adjustUpper );
            //qDebug() << "CartesianGrid::calculateGridXY() returns linear range: min " << dim.start << " and max" << dim.end;
        } else {
            // logarithmic calculation with negative values
            if ( dim.end <= 0 )
            {
                qreal min;
                const qreal minRaw = qMin( dim.start, dim.end );
                const int minLog = -static_cast<int>(trunc( log10( -minRaw ) ) );
                if ( minLog >= 0 )
                    min = qMin( minRaw, -std::numeric_limits< qreal >::epsilon() );
                else
                    min = -fastPow10( -(minLog-1) );
            
                qreal max;
                const qreal maxRaw = qMin( -std::numeric_limits< qreal >::epsilon(), qMax( dim.start, dim.end ) );
                const int maxLog = -static_cast<int>(ceil( log10( -maxRaw ) ) );
                if ( maxLog >= 0 )
                    max = -1;
                else if ( fastPow10( -maxLog ) < maxRaw )
                    max = -fastPow10( -(maxLog+1) );
                else
                    max = -fastPow10( -maxLog );
                if ( adjustLower )
                    dim.start = min;
                if ( adjustUpper )
                    dim.end   = max;
                dim.stepWidth = -pow( 10.0, ceil( log10( qAbs( max - min ) / 10.0 ) ) );
            }
            // logarithmic calculation (ignoring all negative values)
            else
            {
                qreal min;
                const qreal minRaw = qMax( qMin( dim.start, dim.end ), qreal( 0.0 ) );
                const int minLog = static_cast<int>(trunc( log10( minRaw ) ) );
                if ( minLog <= 0 && dim.end < 1.0 )
                    min = qMax( minRaw, std::numeric_limits< qreal >::epsilon() );
                else if ( minLog <= 0 )
                    min = qMax( qreal(0.00001), dim.start );
                else
                    min = fastPow10( minLog-1 );

                // Uh oh. Logarithmic scaling doesn't work with a lower or upper
                // bound being 0.
                const bool zeroBound = dim.start == 0.0 || dim.end == 0.0;

                qreal max;
                const qreal maxRaw = qMax( qMax( dim.start, dim.end ), qreal( 0.0 ) );
                const int maxLog = static_cast<int>(ceil( log10( maxRaw ) ) );
                if ( maxLog <= 0 )
                    max = 1;
                else if ( fastPow10( maxLog ) < maxRaw )
                    max = fastPow10( maxLog+1 );
                else
                    max = fastPow10( maxLog );
                if ( adjustLower || zeroBound )
                    dim.start = min;
                if ( adjustUpper || zeroBound )
                    dim.end   = max;
                dim.stepWidth = pow( 10.0, ceil( log10( qAbs( max - min ) / 10.0 ) ) );
            }
        }
    } else {
        //qDebug() << "CartesianGrid::calculateGridXY() returns stepWidth 1.0  !!";
        // Do not ignore the user configuration
        dim.stepWidth = dim.stepWidth ? dim.stepWidth : 1.0;
    }
    return dim;
}


static void calculateSteps(
    qreal start_, qreal end_, const QList<qreal>& list,
    int minSteps, int maxSteps,
    int power,
    qreal& steps, qreal& stepWidth,
    bool adjustLower, bool adjustUpper )
{
    //qDebug("-----------------------------------\nstart: %f   end: %f   power-of-ten: %i", start_, end_, power);

    qreal distance = 0.0;
    steps = 0.0;

    const int lastIdx = list.count()-1;
    for ( int i = 0;  i <= lastIdx;  ++i ) {
        const qreal testStepWidth = list.at(lastIdx - i) * fastPow10( power );
        //qDebug( "testing step width: %f", testStepWidth);
        qreal start = qMin( start_, end_ );
        qreal end   = qMax( start_, end_ );
        //qDebug("pre adjusting    start: %f   end: %f", start, end);
        AbstractGrid::adjustLowerUpperRange( start, end, testStepWidth, adjustLower, adjustUpper );
        //qDebug("post adjusting   start: %f   end: %f", start, end);

        const qreal testDistance = qAbs(end - start);
        const qreal testSteps    = testDistance / testStepWidth;

        //qDebug() << "testDistance:" << testDistance << "  distance:" << distance;
        if ( (minSteps <= testSteps) && (testSteps <= maxSteps)
              && ( (steps == 0.0) || (testDistance <= distance) ) ) {
            steps     = testSteps;
            stepWidth = testStepWidth;
            distance  = testDistance;
            //qDebug( "start: %f   end: %f   step width: %f   steps: %f   distance: %f", start, end, stepWidth, steps, distance);
        }
    }
}


void CartesianGrid::calculateStepWidth(
    qreal start_, qreal end_,
    const QList<qreal>& granularities,
    Qt::Orientation orientation,
    qreal& stepWidth, qreal& subStepWidth,
    bool adjustLower, bool adjustUpper ) const
{
    Q_UNUSED( orientation );

    Q_ASSERT_X ( granularities.count(), "CartesianGrid::calculateStepWidth",
                 "Error: The list of GranularitySequence values is empty." );
    QList<qreal> list( granularities );
    qSort( list );

    const qreal start = qMin( start_, end_);
    const qreal end   = qMax( start_, end_);
    const qreal distance = end - start;
    //qDebug( "raw data start: %f   end: %f", start, end);

    qreal steps;
    int power = 0;
    while ( list.last() * fastPow10( power ) < distance ) {
        ++power;
    };
    // We have the sequence *two* times in the calculation test list,
    // so we will be sure to find the best match:
    const int count = list.count();
    QList<qreal> testList;

    for ( int dec = -1; dec == -1 || fastPow10( dec + 1 ) >= distance; --dec )
        for ( int i = 0;  i < count;  ++i )
            testList << list.at(i) * fastPow10( dec );

    testList << list;

    do {
        calculateSteps( start, end, testList, m_minsteps, m_maxsteps, power,
                        steps, stepWidth,
                        adjustLower, adjustUpper );
        --power;
    } while ( steps == 0.0 );
    ++power;
    //qDebug( "steps calculated:  stepWidth: %f   steps: %f", stepWidth, steps);

    // find the matching sub-grid line width in case it is
    // not set by the user

    if ( subStepWidth == 0.0 ) {
        if ( stepWidth == list.first() * fastPow10( power ) ) {
            subStepWidth = list.last() * fastPow10( power-1 );
            //qDebug("A");
        } else if ( stepWidth == list.first() * fastPow10( power-1 ) ) {
            subStepWidth = list.last() * fastPow10( power-2 );
            //qDebug("B");
        } else {
            qreal smallerStepWidth = list.first();
            for ( int i = 1;  i < list.count();  ++i ) {
                if ( stepWidth == list.at( i ) * fastPow10( power ) ) {
                    subStepWidth = smallerStepWidth * fastPow10( power );
                    break;
                }
                if ( stepWidth == list.at( i ) * fastPow10( power-1 ) ) {
                    subStepWidth = smallerStepWidth * fastPow10( power-1 );
                    break;
                }
                smallerStepWidth = list.at( i );
            }
        }
    }
    //qDebug("CartesianGrid::calculateStepWidth() found stepWidth %f (%f steps) and sub-stepWidth %f", stepWidth, steps, subStepWidth);
}
