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

#include "KDChartCartesianCoordinatePlane.h"
#include "KDChartCartesianCoordinatePlane_p.h"

#include "KDChartAbstractDiagram.h"
#include "KDChartAbstractDiagram_p.h"
#include "KDChartAbstractCartesianDiagram.h"
#include "CartesianCoordinateTransformation.h"
#include "KDChartGridAttributes.h"
#include "KDChartPaintContext.h"
#include "KDChartPainterSaver_p.h"
#include "KDChartBarDiagram.h"
#include "KDChartStockDiagram.h"

#include <KDABLibFakes>

#include <QApplication>
#include <QFont>
#include <QList>
#include <QtDebug>
#include <QPainter>
#include <QTime>


using namespace KDChart;

#define d d_func()

CartesianCoordinatePlane::Private::Private()
    : AbstractCoordinatePlane::Private()
    , bPaintIsRunning( false )
    , hasOwnGridAttributesHorizontal( false )
    , hasOwnGridAttributesVertical( false )
    , isometricScaling( false )
    , horizontalMin( 0 )
    , horizontalMax( 0 )
    , verticalMin( 0 )
    , verticalMax( 0 )
    , autoAdjustHorizontalRangeToData( 67 )
    , autoAdjustVerticalRangeToData( 67 )
    , autoAdjustGridToZoom( true )
    , fixedDataCoordinateSpaceRelation( false )
    , xAxisStartAtZero( true )
    , reverseVerticalPlane( false )
    , reverseHorizontalPlane( false )
{
}

CartesianCoordinatePlane::CartesianCoordinatePlane ( Chart* parent )
    : AbstractCoordinatePlane ( new Private(), parent )
{
    // this bloc left empty intentionally
}

CartesianCoordinatePlane::~CartesianCoordinatePlane()
{
    // this bloc left empty intentionally
}

void CartesianCoordinatePlane::init()
{
    // this bloc left empty intentionally
}


void CartesianCoordinatePlane::addDiagram ( AbstractDiagram* diagram )
{
    Q_ASSERT_X ( dynamic_cast<AbstractCartesianDiagram*> ( diagram ),
                 "CartesianCoordinatePlane::addDiagram", "Only cartesian "
                 "diagrams can be added to a cartesian coordinate plane!" );
    AbstractCoordinatePlane::addDiagram ( diagram );
    connect ( diagram,  SIGNAL ( layoutChanged ( AbstractDiagram* ) ),
              SLOT ( slotLayoutChanged ( AbstractDiagram* ) ) );

    connect( diagram, SIGNAL( propertiesChanged() ),this, SIGNAL( propertiesChanged() ) );
}


void CartesianCoordinatePlane::paint ( QPainter* painter )
{
    // prevent recursive call:
    if ( d->bPaintIsRunning ) {
        return;
    }
    d->bPaintIsRunning = true;

    AbstractDiagramList diags = diagrams();
    if ( !diags.isEmpty() )
    {
        PaintContext ctx;
        ctx.setPainter ( painter );
        ctx.setCoordinatePlane ( this );
        const QRectF drawArea( drawingArea() );
        ctx.setRectangle ( drawArea );

        // enabling clipping so that we're not drawing outside
        PainterSaver painterSaver( painter );
        QRect clipRect = drawArea.toRect().adjusted( -1, -1, 1, 1 );
        QRegion clipRegion( clipRect );
        painter->setClipRegion( clipRegion );

        // paint the coordinate system rulers:
        d->grid->drawGrid( &ctx );

        // paint the diagrams:
        for ( int i = 0; i < diags.size(); i++ )
        {
            if ( diags[i]->isHidden() ) {
                continue;
            }
            bool doDumpPaintTime = AbstractDiagram::Private::get( diags[ i ] )->doDumpPaintTime;
            QTime stopWatch;
            if ( doDumpPaintTime ) {
                stopWatch.start();
            }

            PainterSaver diagramPainterSaver( painter );
            diags[i]->paint ( &ctx );

            if ( doDumpPaintTime ) {
                qDebug() << "Painting diagram" << i << "took" << stopWatch.elapsed() << "milliseconds";
            }
        }

    }
    d->bPaintIsRunning = false;
}


void CartesianCoordinatePlane::slotLayoutChanged ( AbstractDiagram* )
{
    layoutDiagrams();
}

QRectF CartesianCoordinatePlane::getRawDataBoundingRectFromDiagrams() const
{
    // determine unit of the rectangles of all involved diagrams:
    qreal minX = 0;
    qreal maxX = 0;
    qreal minY = 0;
    qreal maxY = 0;
    bool bStarting = true;
    Q_FOREACH( const AbstractDiagram* diagram, diagrams() )
    {
        QPair<QPointF, QPointF> dataBoundariesPair = diagram->dataBoundaries();
        //qDebug() << "CartesianCoordinatePlane::getRawDataBoundingRectFromDiagrams()\ngets diagram->dataBoundaries: " << dataBoundariesPair.first << dataBoundariesPair.second;
        if ( bStarting || dataBoundariesPair.first.x()  < minX ) minX = dataBoundariesPair.first.x();
        if ( bStarting || dataBoundariesPair.first.y()  < minY ) minY = dataBoundariesPair.first.y();
        if ( bStarting || dataBoundariesPair.second.x() > maxX ) maxX = dataBoundariesPair.second.x();
        if ( bStarting || dataBoundariesPair.second.y() > maxY ) maxY = dataBoundariesPair.second.y();
        bStarting = false;
    }
    //qDebug() << "CartesianCoordinatePlane::getRawDataBoundingRectFromDiagrams()\nreturns data boundaries: " << QRectF( QPointF(minX, minY), QSizeF(maxX - minX, maxY - minY) );
    QRectF dataBoundingRect;
    dataBoundingRect.setBottomLeft( QPointF( minX, minY ) );
    dataBoundingRect.setTopRight( QPointF( maxX, maxY ) );
    return dataBoundingRect;
}


QRectF CartesianCoordinatePlane::adjustedToMaxEmptyInnerPercentage(
        const QRectF& r, unsigned int percentX, unsigned int percentY ) const
{
    QRectF ret = r;
    if ( ( axesCalcModeX() != Logarithmic || r.left() < 0.0 ) && percentX > 0 && percentX != 100 ) {
        const bool isPositive = r.left() >= 0;
        if ( ( r.right() >= 0 ) == isPositive ) {
            qreal upperBound = qMax( r.left(), r.right() );
            qreal lowerBound = qMin( r.left(), r.right() );
            qreal innerBound = isPositive ? lowerBound : upperBound;
            qreal outerBound = isPositive ? upperBound : lowerBound;
            if ( innerBound / outerBound * 100 <= percentX && d->xAxisStartAtZero ) {
                if ( isPositive ) {
                    ret.setLeft( 0.0 );
                } else {
                    ret.setRight( 0.0 );
                }
            }
        }
    }
    // ### this doesn't seem to take into account that Qt's y coordinate is inverted
    if ( ( axesCalcModeY() != Logarithmic || r.bottom() < 0.0 ) && percentY > 0 && percentY != 100 ) {
        const bool isPositive = r.bottom() >= 0;
        if ( ( r.top() >= 0 ) == isPositive ) {
            qreal upperBound = qMax( r.top(), r.bottom() );
            qreal lowerBound = qMin( r.top(), r.bottom() );
            const qreal innerBound = isPositive ? lowerBound : upperBound;
            const qreal outerBound = isPositive ? upperBound : lowerBound;
            if ( innerBound / outerBound * 100 <= percentY ) {
                if ( isPositive ) {
                    ret.setBottom( 0.0 );
                } else {
                    ret.setTop( 0.0 );
                }
            }
        }
    }
    return ret;
}


QRectF CartesianCoordinatePlane::calculateRawDataBoundingRect() const
{
    // are manually set ranges to be applied?
    const bool bAutoAdjustHorizontalRange = d->autoAdjustHorizontalRangeToData < 100;
    const bool bAutoAdjustVerticalRange = d->autoAdjustVerticalRangeToData < 100;

    const bool bHardHorizontalRange = d->horizontalMin != d->horizontalMax && !bAutoAdjustHorizontalRange;
    const bool bHardVerticalRange = d->verticalMin != d->verticalMax && !bAutoAdjustVerticalRange;
    QRectF dataBoundingRect;

    // if custom boundaries are set on the plane, use them
    if ( bHardHorizontalRange && bHardVerticalRange ) {
        dataBoundingRect.setLeft( d->horizontalMin );
        dataBoundingRect.setRight( d->horizontalMax );
        dataBoundingRect.setBottom( d->verticalMin );
        dataBoundingRect.setTop( d->verticalMax );
    } else {
        // determine unit of the rectangles of all involved diagrams:
        dataBoundingRect = getRawDataBoundingRectFromDiagrams();
        if ( bHardHorizontalRange ) {
            dataBoundingRect.setLeft( d->horizontalMin );
            dataBoundingRect.setRight( d->horizontalMax );
        }
        if ( bHardVerticalRange ) {
            dataBoundingRect.setBottom( d->verticalMin );
            dataBoundingRect.setTop( d->verticalMax );
        }
    }
    // recalculate the bounds, if automatic adjusting of ranges is desired AND
    //                         both bounds are at the same side of the zero line
    dataBoundingRect = adjustedToMaxEmptyInnerPercentage(
            dataBoundingRect, d->autoAdjustHorizontalRangeToData, d->autoAdjustVerticalRangeToData );
    if ( bAutoAdjustHorizontalRange ) {
        const_cast<CartesianCoordinatePlane*>( this )->d->horizontalMin = dataBoundingRect.left();
        const_cast<CartesianCoordinatePlane*>( this )->d->horizontalMax = dataBoundingRect.right();
    }
    if ( bAutoAdjustVerticalRange ) {
        const_cast<CartesianCoordinatePlane*>( this )->d->verticalMin = dataBoundingRect.bottom();
        const_cast<CartesianCoordinatePlane*>( this )->d->verticalMax = dataBoundingRect.top();
    }
    // qDebug() << Q_FUNC_INFO << dataBoundingRect;
    return dataBoundingRect;
}


DataDimensionsList CartesianCoordinatePlane::getDataDimensionsList() const
{
    const AbstractCartesianDiagram* dgr = diagrams().isEmpty() ? 0 :
                   qobject_cast< const AbstractCartesianDiagram* >( diagrams().first() );
    if ( dgr && dgr->referenceDiagram() ) {
        dgr = dgr->referenceDiagram();
    }
    const BarDiagram *barDiagram = qobject_cast< const BarDiagram* >( dgr );
    const StockDiagram *stockDiagram = qobject_cast< const StockDiagram* >( dgr );

    // note:
    // It does make sense to retrieve the orientation from the first diagram. This is because
    // a coordinate plane can either be for horizontal *or* for vertical diagrams. Both at the
    // same time won't work, and thus the orientation for all diagrams is the same as for the first one.
    const Qt::Orientation diagramOrientation = barDiagram != 0 ? barDiagram->orientation() : Qt::Vertical;
    const bool diagramIsVertical = diagramOrientation == Qt::Vertical;

    DataDimensionsList l;
    if ( dgr ) {
        const QRectF r( calculateRawDataBoundingRect() );
        // We do not access d->gridAttributesHorizontal/Vertical here, but we use the getter function,
        // to get the global attrs, if no special ones have been set for the given orientation.
        const GridAttributes gaH( gridAttributes( Qt::Horizontal ) );
        const GridAttributes gaV( gridAttributes( Qt::Vertical ) );
        // append the first dimension: for Abscissa axes
        l.append(
            DataDimension(
                r.left(), r.right(),
                diagramIsVertical ? ( !stockDiagram && dgr->datasetDimension() > 1 ) : true,
                axesCalcModeX(),
                gaH.gridGranularitySequence(),
                gaH.gridStepWidth(),
                gaH.gridSubStepWidth() ) );
        // append the second dimension: for Ordinate axes
        l.append(
            DataDimension(
                r.bottom(), r.top(),
                diagramIsVertical ? true : ( dgr->datasetDimension() > 1 ),
                axesCalcModeY(),
                gaV.gridGranularitySequence(),
                gaV.gridStepWidth(),
                gaV.gridSubStepWidth() ) );
    } else {
        l.append( DataDimension() ); // This gets us the default 1..0 / 1..0 grid
        l.append( DataDimension() ); // shown, if there is no diagram on this plane.
    }
    return l;
}

QRectF CartesianCoordinatePlane::drawingArea() const
{
    // the rectangle the diagrams cover in the *plane*:
    // We reserve 1px on each side for antialiased drawing, and respect the way QPainter calculates
    // the width of a painted rect (the size is the rectangle size plus the pen width). The latter
    // accounts for another pixel that we subtract from height and width.
    // This way, most clipping for regular pens should be avoided. When pens with a width larger
    // than 1 are used, this may not be sufficient.
    return QRectF( areaGeometry() ).adjusted( 1.0, 1.0, -2.0, -2.0 );
}


QRectF CartesianCoordinatePlane::logicalArea() const
{
    if ( d->dimensions.isEmpty() )
        return QRectF();

    const DataDimension dimX = d->dimensions.first();
    const DataDimension dimY = d->dimensions.last();
    const QPointF pt( qMin( dimX.start, dimX.end ), qMax( dimY.start, dimY.end ) );
    const QSizeF siz( qAbs( dimX.distance() ), -qAbs( dimY.distance() ) );
    const QRectF dataBoundingRect( pt, siz );

    // determine logical top left, taking the "reverse" options into account
    const QPointF topLeft( d->reverseHorizontalPlane ? dataBoundingRect.right() : dataBoundingRect.left(),
                           d->reverseVerticalPlane ? dataBoundingRect.bottom() : dataBoundingRect.top() );

    const qreal width  = dataBoundingRect.width()  * ( d->reverseHorizontalPlane ? -1.0 : 1.0 );
    const qreal height = dataBoundingRect.height() * ( d->reverseVerticalPlane   ? -1.0 : 1.0 );

    return QRectF( topLeft, QSizeF( width, height ) );
}

QRectF CartesianCoordinatePlane::diagramArea() const
{
    const QRectF logArea( logicalArea() );
    QPointF physicalTopLeft = d->coordinateTransformation.translate( logArea.topLeft() );
    QPointF physicalBottomRight = d->coordinateTransformation.translate( logArea.bottomRight() );

    return QRectF( physicalTopLeft, physicalBottomRight ).normalized();
}

QRectF CartesianCoordinatePlane::visibleDiagramArea() const
{
    return diagramArea().intersected( drawingArea() );
}

void CartesianCoordinatePlane::layoutDiagrams()
{
    d->dimensions = gridDimensionsList();
    Q_ASSERT_X ( d->dimensions.count() == 2, "CartesianCoordinatePlane::layoutDiagrams",
                 "Error: gridDimensionsList() did not return exactly two dimensions." );

    // physical area of the plane
    const QRectF physicalArea( drawingArea() );
    // .. in contrast to the logical area
    const QRectF logArea( logicalArea() );

    // TODO: isometric scaling for zooming?

    // the plane area might have changed, so the zoom values might also be changed
    handleFixedDataCoordinateSpaceRelation( physicalArea );

    d->coordinateTransformation.updateTransform( logArea, physicalArea );

    update();
}

void CartesianCoordinatePlane::setFixedDataCoordinateSpaceRelation( bool fixed )
{
    d->fixedDataCoordinateSpaceRelation = fixed;
    d->fixedDataCoordinateSpaceRelationPinnedSize = QSize();
    handleFixedDataCoordinateSpaceRelation( drawingArea() );
}

bool CartesianCoordinatePlane::hasFixedDataCoordinateSpaceRelation() const
{
    return d->fixedDataCoordinateSpaceRelation;
}

void CartesianCoordinatePlane::setXAxisStartAtZero(bool fixedStart)
{
    if (d->xAxisStartAtZero == fixedStart)
        return;

    d->xAxisStartAtZero = fixedStart;
}

bool CartesianCoordinatePlane::xAxisStartAtZero() const
{
    return d->xAxisStartAtZero;
}

void CartesianCoordinatePlane::handleFixedDataCoordinateSpaceRelation( const QRectF& geometry )
{
    if ( !d->fixedDataCoordinateSpaceRelation ) {
        return;
    }
    // is the new geometry ok?
    if ( !geometry.isValid() ) {
        return;
    }

    // note that the pinned size can be invalid even after setting it, if geometry wasn't valid.
    // this is relevant for the cooperation between this method, setFixedDataCoordinateSpaceRelation(),
    // and handleFixedDataCoordinateSpaceRelation().
    if ( !d->fixedDataCoordinateSpaceRelationPinnedSize.isValid() ) {
        d->fixedDataCoordinateSpaceRelationPinnedSize = geometry.size();
        d->fixedDataCoordinateSpaceRelationPinnedZoom = ZoomParameters( zoomFactorX(), zoomFactorY(), zoomCenter() );
        return;
    }

    // if the plane size was changed, change zoom factors to keep the diagram size constant
    if ( d->fixedDataCoordinateSpaceRelationPinnedSize != geometry.size() ) {
        const qreal widthScaling = d->fixedDataCoordinateSpaceRelationPinnedSize.width() / geometry.width();
        const qreal heightScaling = d->fixedDataCoordinateSpaceRelationPinnedSize.height() / geometry.height();

        const qreal newZoomX = d->fixedDataCoordinateSpaceRelationPinnedZoom.xFactor * widthScaling;
        const qreal newZoomY = d->fixedDataCoordinateSpaceRelationPinnedZoom.yFactor * heightScaling;

        const QPointF newCenter = QPointF( d->fixedDataCoordinateSpaceRelationPinnedZoom.xCenter / widthScaling,
                                           d->fixedDataCoordinateSpaceRelationPinnedZoom.yCenter / heightScaling );
        // Use these internal methods to avoid sending the propertiesChanged signal more than once
        bool changed = false;
        if ( doneSetZoomFactorY( newZoomY ) )
            changed = true;
        if ( doneSetZoomFactorX( newZoomX ) )
            changed = true;
        if ( doneSetZoomCenter( newCenter ) )
            changed = true;
        if ( changed )
            emit propertiesChanged();
    }
}

const QPointF CartesianCoordinatePlane::translate( const QPointF& diagramPoint ) const
{
    // Note: We do not test if the point lays inside of the data area,
    //       but we just apply the transformation calculations to the point.
    //       This allows for basic calculations done by the user, see e.g.
    //       the file  examples/Lines/BubbleChart/mainwindow.cpp
    return d->coordinateTransformation.translate( diagramPoint );
}

const QPointF CartesianCoordinatePlane::translateBack( const QPointF& screenPoint ) const
{
    return d->coordinateTransformation.translateBack( screenPoint );
}

void CartesianCoordinatePlane::setIsometricScaling ( bool isOn )
{
    if ( d->isometricScaling != isOn ) {
        d->isometricScaling = isOn;
        layoutDiagrams();
        emit propertiesChanged();
    }
}

bool CartesianCoordinatePlane::doesIsometricScaling () const
{
    return d->isometricScaling;
}

bool CartesianCoordinatePlane::doneSetZoomFactorX( qreal factor )
{
    if ( d->coordinateTransformation.zoom.xFactor == factor ) {
        return false;
    }
    d->coordinateTransformation.zoom.xFactor = factor;
    if ( d->autoAdjustGridToZoom ) {
        d->grid->setNeedRecalculate();
    }
    return true;
}

bool CartesianCoordinatePlane::doneSetZoomFactorY( qreal factor )
{
    if ( d->coordinateTransformation.zoom.yFactor == factor ) {
        return false;
    }
    d->coordinateTransformation.zoom.yFactor = factor;
    if ( d->autoAdjustGridToZoom ) {
        d->grid->setNeedRecalculate();
    }
    return true;
}

bool CartesianCoordinatePlane::doneSetZoomCenter( const QPointF& point )
{
    if ( d->coordinateTransformation.zoom.center() == point ) {
        return false;
    }
    d->coordinateTransformation.zoom.setCenter( point );
    if ( d->autoAdjustGridToZoom ) {
        d->grid->setNeedRecalculate();
    }
    return true;
}

void CartesianCoordinatePlane::setZoomFactors( qreal factorX, qreal factorY )
{
    if ( doneSetZoomFactorX( factorX ) || doneSetZoomFactorY( factorY ) ) {
        d->coordinateTransformation.updateTransform( logicalArea(), drawingArea() );
        emit propertiesChanged();
    }
}

void CartesianCoordinatePlane::setZoomFactorX( qreal factor )
{
    if ( doneSetZoomFactorX( factor ) ) {
        d->coordinateTransformation.updateTransform( logicalArea(), drawingArea() );
        emit propertiesChanged();
    }
}

void CartesianCoordinatePlane::setZoomFactorY( qreal factor )
{
    if ( doneSetZoomFactorY( factor ) ) {
        d->coordinateTransformation.updateTransform( logicalArea(), drawingArea() );
        emit propertiesChanged();
    }
}

void CartesianCoordinatePlane::setZoomCenter( const QPointF& point )
{
    if ( doneSetZoomCenter( point ) ) {
        d->coordinateTransformation.updateTransform( logicalArea(), drawingArea() );
        emit propertiesChanged();
    }
}

QPointF CartesianCoordinatePlane::zoomCenter() const
{
    return d->coordinateTransformation.zoom.center();
}

qreal CartesianCoordinatePlane::zoomFactorX() const
{
    return d->coordinateTransformation.zoom.xFactor;
}

qreal CartesianCoordinatePlane::zoomFactorY() const
{
    return d->coordinateTransformation.zoom.yFactor;
}


CartesianCoordinatePlane::AxesCalcMode CartesianCoordinatePlane::axesCalcModeY() const
{
    return d->coordinateTransformation.axesCalcModeY;
}

CartesianCoordinatePlane::AxesCalcMode CartesianCoordinatePlane::axesCalcModeX() const
{
    return d->coordinateTransformation.axesCalcModeX;
}

void CartesianCoordinatePlane::setAxesCalcModes( AxesCalcMode mode )
{
    if ( d->coordinateTransformation.axesCalcModeY != mode ||
        d->coordinateTransformation.axesCalcModeX != mode ) {
        d->coordinateTransformation.axesCalcModeY = mode;
        d->coordinateTransformation.axesCalcModeX = mode;
        emit propertiesChanged();
        emit viewportCoordinateSystemChanged();
        Q_FOREACH( AbstractDiagram* diag, diagrams() )
        slotLayoutChanged( diag );
    }
}

void CartesianCoordinatePlane::setAxesCalcModeY( AxesCalcMode mode )
{
    if ( d->coordinateTransformation.axesCalcModeY != mode ) {
        d->coordinateTransformation.axesCalcModeY = mode;
        emit propertiesChanged();
        setGridNeedsRecalculate();
        emit viewportCoordinateSystemChanged();
    }
}

void CartesianCoordinatePlane::setAxesCalcModeX( AxesCalcMode mode )
{
    if ( d->coordinateTransformation.axesCalcModeX != mode ) {
        d->coordinateTransformation.axesCalcModeX = mode;
        emit propertiesChanged();
        emit viewportCoordinateSystemChanged();
    }
}

void CartesianCoordinatePlane::setHorizontalRange( const QPair< qreal, qreal > & range )
{
    if ( d->horizontalMin != range.first || d->horizontalMax != range.second ) {
        d->autoAdjustHorizontalRangeToData = 100;
        d->horizontalMin = range.first;
        d->horizontalMax = range.second;
        layoutDiagrams();
        emit propertiesChanged();
        emit boundariesChanged();
    }
}

void CartesianCoordinatePlane::setVerticalRange( const QPair< qreal, qreal > & range )
{

    if ( d->verticalMin != range.first || d->verticalMax != range.second ) {
        d->autoAdjustVerticalRangeToData = 100;
        d->verticalMin = range.first;
        d->verticalMax = range.second;
        layoutDiagrams();
        emit propertiesChanged();
        emit boundariesChanged();
    }
}

QPair< qreal, qreal > CartesianCoordinatePlane::horizontalRange( ) const
{
    QRectF visibleRange = visibleDataRange();
    return QPair<qreal, qreal>( visibleRange.left(), visibleRange.right() );
}

QPair< qreal, qreal > CartesianCoordinatePlane::verticalRange( ) const
{
    QRectF visibleRange = visibleDataRange();
    return QPair<qreal, qreal>( visibleRange.bottom(), visibleRange.top() );
}

void CartesianCoordinatePlane::adjustRangesToData()
{
    const QRectF dataBoundingRect( getRawDataBoundingRectFromDiagrams() );
    d->horizontalMin = dataBoundingRect.left();
    d->horizontalMax = dataBoundingRect.right();
    d->verticalMin = dataBoundingRect.top();
    d->verticalMax = dataBoundingRect.bottom();
    layoutDiagrams();
    emit propertiesChanged();
}

void CartesianCoordinatePlane::adjustHorizontalRangeToData()
{
    const QRectF dataBoundingRect( getRawDataBoundingRectFromDiagrams() );
    d->horizontalMin = dataBoundingRect.left();
    d->horizontalMax = dataBoundingRect.right();
    layoutDiagrams();
    emit propertiesChanged();
}

void CartesianCoordinatePlane::adjustVerticalRangeToData()
{
    const QRectF dataBoundingRect( getRawDataBoundingRectFromDiagrams() );
    d->verticalMin = dataBoundingRect.bottom();
    d->verticalMax = dataBoundingRect.top();
    layoutDiagrams();
    emit propertiesChanged();
}

void CartesianCoordinatePlane::setAutoAdjustHorizontalRangeToData( unsigned int percentEmpty )
{
    if ( d->autoAdjustHorizontalRangeToData != percentEmpty )
    {
        d->autoAdjustHorizontalRangeToData = percentEmpty;
        d->horizontalMin = 0.0;
        d->horizontalMax = 0.0;
        layoutDiagrams();
        emit propertiesChanged();
    }
}

void CartesianCoordinatePlane::setAutoAdjustVerticalRangeToData( unsigned int percentEmpty )
{
    if ( d->autoAdjustVerticalRangeToData != percentEmpty )
    {
        d->autoAdjustVerticalRangeToData = percentEmpty;
        d->verticalMin = 0.0;
        d->verticalMax = 0.0;
        layoutDiagrams();
        emit propertiesChanged();
    }
}

unsigned int CartesianCoordinatePlane::autoAdjustHorizontalRangeToData() const
{
    return d->autoAdjustHorizontalRangeToData;
}

unsigned int CartesianCoordinatePlane::autoAdjustVerticalRangeToData() const
{
    return d->autoAdjustVerticalRangeToData;
}

void CartesianCoordinatePlane::setGridAttributes(
    Qt::Orientation orientation,
    const GridAttributes& a )
{
    if ( orientation == Qt::Horizontal )
        d->gridAttributesHorizontal = a;
    else
        d->gridAttributesVertical = a;
    setHasOwnGridAttributes( orientation, true );
    update();
    emit propertiesChanged();
}

void CartesianCoordinatePlane::resetGridAttributes(
    Qt::Orientation orientation )
{
    setHasOwnGridAttributes( orientation, false );
    update();
}

const GridAttributes CartesianCoordinatePlane::gridAttributes(
    Qt::Orientation orientation ) const
{
    if ( hasOwnGridAttributes( orientation ) ) {
        if ( orientation == Qt::Horizontal )
            return d->gridAttributesHorizontal;
        else
            return d->gridAttributesVertical;
    } else {
        return globalGridAttributes();
    }
}

void CartesianCoordinatePlane::setHasOwnGridAttributes(
    Qt::Orientation orientation, bool on )
{
    if ( orientation == Qt::Horizontal )
        d->hasOwnGridAttributesHorizontal = on;
    else
        d->hasOwnGridAttributesVertical = on;
    emit propertiesChanged();
}

bool CartesianCoordinatePlane::hasOwnGridAttributes(
    Qt::Orientation orientation ) const
{
    return
        ( orientation == Qt::Horizontal )
        ? d->hasOwnGridAttributesHorizontal
        : d->hasOwnGridAttributesVertical;
}

void CartesianCoordinatePlane::setAutoAdjustGridToZoom( bool autoAdjust )
{
    if ( d->autoAdjustGridToZoom != autoAdjust ) {
        d->autoAdjustGridToZoom = autoAdjust;
        d->grid->setNeedRecalculate();
        emit propertiesChanged();
    }
}

#if QT_VERSION < 0x040400 || defined(Q_COMPILER_MANGLES_RETURN_TYPE)
const
#endif
bool CartesianCoordinatePlane::autoAdjustGridToZoom() const
{
    return d->autoAdjustGridToZoom;
}

AbstractCoordinatePlane* CartesianCoordinatePlane::sharedAxisMasterPlane( QPainter* painter )
{
    CartesianCoordinatePlane* plane = this;
    AbstractCartesianDiagram* diag = dynamic_cast< AbstractCartesianDiagram* >( plane->diagram() );
    const CartesianAxis* sharedAxis = 0;
    if ( diag != 0 )
    {
        const CartesianAxisList axes = diag->axes();
        KDAB_FOREACH( const CartesianAxis* a, axes )
        {
            CartesianCoordinatePlane* p = const_cast< CartesianCoordinatePlane* >(
                                              dynamic_cast< const CartesianCoordinatePlane* >( a->coordinatePlane() ) );
            if ( p != 0 && p != this )
            {
                plane = p;
                sharedAxis = a;
            }
        }
    }

    if ( plane == this || painter == 0 )
        return plane;

    const QPointF zero = QPointF( 0, 0 );
    const QPointF tenX = QPointF( 10, 0 );
    const QPointF tenY = QPointF( 0, 10 );


    if ( sharedAxis->isOrdinate() )
    {
        painter->translate( translate( zero ).x(), 0.0 );
        const qreal factor = (translate( tenX ) - translate( zero ) ).x() / ( plane->translate( tenX ) - plane->translate( zero ) ).x();
        painter->scale( factor, 1.0 );
        painter->translate( -plane->translate( zero ).x(), 0.0 );
    }
    if ( sharedAxis->isAbscissa() )
    {
        painter->translate( 0.0, translate( zero ).y() );
        const qreal factor = (translate( tenY ) - translate( zero ) ).y() / ( plane->translate( tenY ) - plane->translate( zero ) ).y();
        painter->scale( 1.0, factor );
        painter->translate( 0.0, -plane->translate( zero ).y() );
    }


    return plane;
}

void CartesianCoordinatePlane::setHorizontalRangeReversed( bool reverse )
{
    if ( d->reverseHorizontalPlane == reverse )
        return;

    d->reverseHorizontalPlane = reverse;
    layoutDiagrams();
    emit propertiesChanged();
}

bool CartesianCoordinatePlane::isHorizontalRangeReversed() const
{
    return d->reverseHorizontalPlane;
}

void CartesianCoordinatePlane::setVerticalRangeReversed( bool reverse )
{
    if ( d->reverseVerticalPlane == reverse )
        return;

    d->reverseVerticalPlane = reverse;
    layoutDiagrams();
    emit propertiesChanged();
}

bool CartesianCoordinatePlane::isVerticalRangeReversed() const
{
    return d->reverseVerticalPlane;
}

QRectF CartesianCoordinatePlane::visibleDataRange() const
{
    QRectF result;

    const QRectF drawArea = drawingArea();

    result.setTopLeft( translateBack( drawArea.topLeft() ) );
    result.setBottomRight( translateBack( drawArea.bottomRight() ) );

    return result;
}

void CartesianCoordinatePlane::setGeometry( const QRect& rectangle )
{
    if ( rectangle == geometry() ) {
        return;
    }

    d->geometry = rectangle;
    if ( d->isometricScaling ) {
        const int hfw = heightForWidth( rectangle.width() );
        // same scaling for x and y means a fixed aspect ratio, which is enforced here
        // always shrink the too large dimension
        if ( hfw < rectangle.height() ) {
            d->geometry.setHeight( hfw );
        } else {
            d->geometry.setWidth( qRound( qreal( rectangle.width() ) *
                                          qreal( rectangle.height() ) / qreal( hfw ) ) );
        }
    }

    AbstractCoordinatePlane::setGeometry( d->geometry );

    Q_FOREACH( AbstractDiagram* diagram, diagrams() ) {
        diagram->resize( d->geometry.size() );
    }
}

Qt::Orientations CartesianCoordinatePlane::expandingDirections() const
{
    // not completely sure why this is required for isometric scaling...
    return d->isometricScaling ? Qt::Horizontal : ( Qt::Horizontal | Qt::Vertical );
}

bool CartesianCoordinatePlane::hasHeightForWidth() const
{
    return d->isometricScaling;
}

int CartesianCoordinatePlane::heightForWidth( int w ) const
{
    // ### using anything for dataRect that depends on geometry will close a feedback loop which
    //     prevents the geometry from stabilizing. specifically, visibleDataRange() depends on
    //     drawingArea(), and no good will come out of using it here.
    QRectF dataRect = logicalArea();
    return qRound( qreal( w ) * qAbs( qreal( dataRect.height() ) / qreal( dataRect.width() ) ) );
}

QSize CartesianCoordinatePlane::sizeHint() const
{
    QSize sh = AbstractCoordinatePlane::sizeHint();
    if ( d->isometricScaling ) {
        // not sure why the next line doesn't cause an infinite loop, but it improves initial size allocation
        sh = d->geometry.size();
        sh.setHeight( heightForWidth( sh.width() ) );
    }
    return sh;
}
