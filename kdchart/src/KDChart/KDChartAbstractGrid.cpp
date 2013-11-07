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

#include "KDChartAbstractGrid.h"
#include "KDChartPaintContext.h"

#include <qglobal.h>

#include <KDABLibFakes>


using namespace KDChart;
using namespace std;


static qreal _trunc( qreal v )
{
    return (( v > 0.0 ) ? floor( v ) : ceil( v ));
}


AbstractGrid::AbstractGrid ()
    : mPlane( 0 )
{
    //this bloc left empty intentionally
}

AbstractGrid::~AbstractGrid()
{
    //this bloc left empty intentionally
}

void AbstractGrid::setNeedRecalculate()
{
    mCachedRawDataDimensions.clear();
}

DataDimensionsList AbstractGrid::updateData( AbstractCoordinatePlane* plane )
{
    if ( plane ) {
        const DataDimensionsList rawDataDimensions( plane->getDataDimensionsList() );
        // ### this could be dangerous becaus calculateGrid() looks at some data we are not checking
        //     for changes here.
        if ( mCachedRawDataDimensions.empty() || ( rawDataDimensions != mCachedRawDataDimensions ) ) {
            mCachedRawDataDimensions = rawDataDimensions;
            mPlane = plane;
            mDataDimensions = calculateGrid( rawDataDimensions );
        }
    }
    return mDataDimensions;
}

bool AbstractGrid::isBoundariesValid(const QRectF& r )
{
    return isBoundariesValid( qMakePair( r.topLeft(), r.bottomRight() ) );
}

bool AbstractGrid::isBoundariesValid(const QPair<QPointF,QPointF>& b )
{
  return isValueValid( b.first.x() )  && isValueValid( b.first.y() ) &&
         isValueValid( b.second.x() ) && isValueValid( b.second.y() );
}

bool AbstractGrid::isBoundariesValid(const DataDimensionsList& l )
{
    for (int i = 0; i < l.size(); ++i)
        if ( ! isValueValid( l.at(i).start ) || ! isValueValid( l.at(i).end ) )
            return false;
    return true;
}

bool AbstractGrid::isValueValid(const qreal& r )
{
  return !(ISNAN(r) || ISINF(r));
}

void AbstractGrid::adjustLowerUpperRange(
        qreal& start, qreal& end,
        qreal stepWidth,
        bool adjustLower, bool adjustUpper )
{
    const qreal startAdjust = ( start >= 0.0 ) ? 0.0 : -1.0;
    const qreal endAdjust   = ( end   >= 0.0 ) ? 1.0 :  0.0;
    if ( adjustLower && !qFuzzyIsNull( fmod( start, stepWidth ) ) )
        start = stepWidth * (_trunc( start / stepWidth ) + startAdjust);
    if ( adjustUpper && !qFuzzyIsNull( fmod( end, stepWidth ) ) )
        end = stepWidth * (_trunc( end / stepWidth ) + endAdjust);
}

const DataDimension AbstractGrid::adjustedLowerUpperRange(
        const DataDimension& dim,
        bool adjustLower, bool adjustUpper )
{
    DataDimension result( dim );
    adjustLowerUpperRange(
            result.start, result.end,
            result.stepWidth,
            adjustLower, adjustUpper );
    return result;
}
