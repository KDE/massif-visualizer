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

#ifndef KDCHARTMEASURE_H
#define KDCHARTMEASURE_H

#include <QDebug>
#include <Qt>
#include <QStack>
#include "KDChartGlobal.h"
#include "KDChartEnums.h"

/** \file KDChartMeasure.h
 *  \brief Declaring the class KDChart::Measure.
 *
 *
 */

QT_BEGIN_NAMESPACE
class QObject;
class QPaintDevice;
QT_END_NAMESPACE

namespace KDChart {

/**
  * \class Measure KDChartMeasure.h KDChartMeasure
  * \brief Measure is used to specify relative and absolute sizes in KDChart, e.g. font sizes.
  *
  */

class KDCHART_EXPORT Measure
{
public:
    Measure();
    /*implicit*/ Measure( qreal value,
                          KDChartEnums::MeasureCalculationMode mode = KDChartEnums::MeasureCalculationModeAuto,
                          KDChartEnums::MeasureOrientation orientation = KDChartEnums::MeasureOrientationAuto );
    Measure( const Measure& );
    Measure &operator= ( const Measure& );

    void setValue( qreal val ) { mValue = val; }
    qreal value() const { return mValue; }

    void setCalculationMode( KDChartEnums::MeasureCalculationMode mode ) { mMode = mode; }
    KDChartEnums::MeasureCalculationMode calculationMode() const { return mMode; }

    /**
      * The reference area must either be derived from AbstractArea
      * or from QWidget, so it can also be derived from AbstractAreaWidget.
      */
    void setRelativeMode( const QObject * area,
                          KDChartEnums::MeasureOrientation orientation )
    {
        mMode = KDChartEnums::MeasureCalculationModeRelative;
        mArea = area;
        mOrientation = orientation;
    }

    /**
     * \brief This is a convenience method for specifying a value,
     *  implicitly setting the calculation mode to MeasureCalculationModeAbsolute.
     *
     * Calling setAbsoluteValue( value ) is the same as calling
\verbatim
    setValue( value );
    setCalculationMode( KDChartEnums::MeasureCalculationModeAbsolute );
\endverbatim
     */
    void setAbsoluteValue( qreal val )
    {
        mMode = KDChartEnums::MeasureCalculationModeAbsolute;
        mValue = val;
    }

    /**
      * The reference area must either be derived from AbstractArea
      * or from QWidget, so it can also be derived from AbstractAreaWidget.
      */
    void setReferenceArea( const QObject * area ) { mArea = area; }
    /**
      * The returned reference area will be derived from AbstractArea
      * or QWidget or both.
      */
    const QObject * referenceArea() const { return mArea; }

    void setReferenceOrientation( KDChartEnums::MeasureOrientation orientation ) { mOrientation = orientation; }
    KDChartEnums::MeasureOrientation referenceOrientation() const { return mOrientation; }

    /**
      * The reference area must either be derived from AbstractArea
      * or from QWidget, so it can also be derived from AbstractAreaWidget.
      */
    qreal calculatedValue( const QObject * autoArea, KDChartEnums::MeasureOrientation autoOrientation ) const;
    qreal calculatedValue( const QSizeF& autoSize, KDChartEnums::MeasureOrientation autoOrientation ) const;
    const QSizeF sizeOfArea( const QObject* area ) const;

    bool operator==( const Measure& ) const;
    bool operator!=( const Measure& other ) const { return !operator==(other); }

private:
    qreal mValue;
    KDChartEnums::MeasureCalculationMode mMode;
    const QObject* mArea;
    KDChartEnums::MeasureOrientation mOrientation;
}; // End of class Measure



/**
 * Auxiliary class used by the KDChart::Measure and KDChart::Chart class.
 *
 * Normally there should be no need to call any of these methods yourself.
 *
 * They are used by KDChart::Chart::paint( QPainter*, const QRect& )
 * to adjust all of the relative Measures according to the target
 * rectangle's size.
 *
 * Default factors are (1.0, 1.0)
 */
class GlobalMeasureScaling
{
public:
    static GlobalMeasureScaling* instance();

    GlobalMeasureScaling();
    virtual ~GlobalMeasureScaling();

public:
    /**
     * Set new factors to be used by all Measure objects from now on.
     * Previous values will be saved on a stack internally.
     */
    static void setFactors(qreal factorX, qreal factorY);

    /**
     * Restore factors to the values before the previous call to
     * setFactors. The current values are popped off a stack internally.
     */
    static void resetFactors();

    /**
     * Return the currently active factors.
     */
    static const QPair< qreal, qreal > currentFactors();

    /**
     * Set the paint device to use for calculating font metrics.
     */
    static void setPaintDevice( QPaintDevice* paintDevice );

    /**
     * Return the paint device to use for calculating font metrics.
     */
    static QPaintDevice* paintDevice();

private:
    QStack< QPair< qreal, qreal > > mFactors;
    QPaintDevice* m_paintDevice;
};

}

#if !defined(QT_NO_DEBUG_STREAM)
KDCHART_EXPORT QDebug operator<<(QDebug, const KDChart::Measure& );
#endif /* QT_NO_DEBUG_STREAM */

#endif // KDCHARTMEASURE_H
