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

#ifndef CARTESIANCOORDINATETRANSFORMATION_H
#define CARTESIANCOORDINATETRANSFORMATION_H

#include <QList>
#include <QRectF>
#include <QPointF>

#include "KDChartZoomParameters.h"

#include <cmath>
#include <limits>

namespace KDChart {

    // FIXME: if this struct is used more often, we need to make it a class
    // with proper accessor methods:

    /**
      * \internal
      */
    struct CoordinateTransformation {

        CoordinateTransformation()
            : axesCalcModeY( CartesianCoordinatePlane::Linear ),
              axesCalcModeX( CartesianCoordinatePlane::Linear ),
              isPositiveX( true ),
              isPositiveY( true )
        {}

        CartesianCoordinatePlane::AxesCalcMode axesCalcModeY;
        CartesianCoordinatePlane::AxesCalcMode axesCalcModeX;

        ZoomParameters zoom;

        QTransform transform;
        QTransform backTransform;
        // a logarithmic scale cannot cross zero, so we have to know which side we are on.
        bool isPositiveX;
        bool isPositiveY;

        qreal logTransform( qreal value, bool isPositiveRange ) const
        {
            if ( isPositiveRange ) {
                return log10( value );
            } else  {
                return -log10( -value );
            }
        }

        qreal logTransformBack( qreal value, bool wasPositive ) const
        {
            if ( wasPositive ) {
                return pow( 10.0, value );
            } else {
                return -pow( 10.0, -value );
            }
        }

        void updateTransform( const QRectF& constDataRect, const QRectF& screenRect )
        {
            QRectF dataRect = constDataRect;
            if ( axesCalcModeX == CartesianCoordinatePlane::Logarithmic ) {
                // the data will be scaled by logTransform() later, so scale its bounds as well
                isPositiveX = dataRect.left() >= 0.0;
                dataRect.setLeft( logTransform( dataRect.left(), isPositiveX ) );
                dataRect.setRight( logTransform( dataRect.right(), isPositiveX ) );
            }
            if ( axesCalcModeY == CartesianCoordinatePlane::Logarithmic ) {
                isPositiveY = dataRect.top() >= 0.0;
                dataRect.setTop( logTransform( dataRect.top(), isPositiveY  ) );
                dataRect.setBottom( logTransform( dataRect.bottom(), isPositiveY ) );
            }

            transform.reset();
            // read the following transformation sequence from bottom to top(!)
            transform.translate( screenRect.left(), screenRect.bottom() );
            transform.scale( screenRect.width(), screenRect.height() );

            // TODO: mirror in case of "reverse" axes?

            // transform into screen space
            transform.translate( 0.5, -0.5 );
            transform.scale( zoom.xFactor, zoom.yFactor );
            transform.translate( -zoom.xCenter, 1.0 - zoom.yCenter );
            // zoom
            transform.scale( 1.0 / dataRect.width(), 1.0 / dataRect.height() );
            transform.translate( -dataRect.left(), -dataRect.bottom() );
            // transform into the unit square

            backTransform = transform.inverted();
        }

        // convert data space point to screen point
        inline QPointF translate( const QPointF& dataPoint ) const
        {
            QPointF data = dataPoint;
            if ( axesCalcModeX == CartesianCoordinatePlane::Logarithmic ) {
                data.setX( logTransform( data.x(), isPositiveX  ) );
            }
            if ( axesCalcModeY == CartesianCoordinatePlane::Logarithmic ) {
                data.setY( logTransform( data.y(), isPositiveY ) );
            }

            return transform.map( data );
        }

        // convert screen point to data space point
        inline const QPointF translateBack( const QPointF& screenPoint ) const
        {
            QPointF ret = backTransform.map( screenPoint );
            if ( axesCalcModeX == CartesianCoordinatePlane::Logarithmic ) {
                ret.setX( logTransformBack( ret.x(), isPositiveX ) );
            }
            if ( axesCalcModeY == CartesianCoordinatePlane::Logarithmic ) {
                ret.setY( logTransformBack( ret.y(), isPositiveY ) );
            }
            return ret;
        }
    };

    typedef QList<CoordinateTransformation> CoordinateTransformationList;

}

#endif
