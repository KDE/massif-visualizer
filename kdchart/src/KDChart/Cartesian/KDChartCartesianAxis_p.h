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

#ifndef KDCHARTCARTESIANAXIS_P_H
#define KDCHARTCARTESIANAXIS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the KD Chart API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "KDChartCartesianAxis.h"
#include "KDChartAbstractCartesianDiagram.h"
#include "KDChartAbstractAxis_p.h"

#include <KDABLibFakes>

#include <limits>

namespace KDChart {

/**
  * \internal
  */
class CartesianAxis::Private : public AbstractAxis::Private
{
    friend class CartesianAxis;

public:
    Private( AbstractCartesianDiagram* diagram, CartesianAxis* axis )
        : AbstractAxis::Private( diagram, axis )
        , useDefaultTextAttributes( true )
        , cachedHeaderLabels( QStringList() )
        , cachedLabelHeight( 0.0 )
        , cachedFontHeight( 0 )
        , axisTitleSpace( 1.0 )
        , axisSize(1.0)
    {}
    ~Private() {}

    CartesianAxis* axis() const { return static_cast<CartesianAxis *>( mAxis ); }
    void drawTitleText( QPainter*, CartesianCoordinatePlane* plane, const QRect& areaGeoRect ) const;
    const TextAttributes titleTextAttributesWithAdjustedRotation() const;
    QSize calculateMaximumSize() const;
    QString customizedLabelText( const QString& text, Qt::Orientation orientation, qreal value ) const;
    bool isVertical() const;

private:
    friend class TickIterator;
    QString titleText;
    TextAttributes titleTextAttributes;
    bool useDefaultTextAttributes;
    Position position;
    QRect geometry;
    int customTickLength;
    QMap< qreal, QString > annotations;
    QList< qreal > customTicksPositions;
    mutable QStringList cachedHeaderLabels;
    mutable qreal cachedLabelHeight;
    mutable qreal cachedLabelWidth;
    mutable int cachedFontHeight;
    mutable int cachedFontWidth;
    mutable QSize cachedMaximumSize;
    qreal axisTitleSpace;
    qreal axisSize;
};

inline CartesianAxis::CartesianAxis( Private * p, AbstractDiagram* diagram )
    : AbstractAxis( p, diagram )
{
    init();
}

inline CartesianAxis::Private * CartesianAxis::d_func()
{ return static_cast<Private*>( AbstractAxis::d_func() ); }
inline const CartesianAxis::Private * CartesianAxis::d_func() const
{ return static_cast<const Private*>( AbstractAxis::d_func() ); }


class XySwitch
{
public:
    explicit XySwitch( bool _isY ) : isY( _isY ) {}

    // for rvalues
    template< class T >
    T operator()( T x, T y ) const { return isY ? y : x; }

    // lvalues
    template< class T >
    T& lvalue( T& x, T& y ) const { return isY ? y : x; }

    bool isY;
};

class TickIterator
{
public:
    enum TickType {
        NoTick = 0,
        MajorTick,
        MajorTickHeaderDataLabel,
        MajorTickManualShort,
        MajorTickManualLong,
        MinorTick,
        CustomTick
    };
    TickIterator( CartesianAxis *a, CartesianCoordinatePlane* plane, uint majorThinningFactor,
                  bool omitLastTick /* sorry about that */ );
    TickIterator( bool isY, const DataDimension& dimension, bool hasMajorTicks, bool hasMinorTicks,
                  CartesianCoordinatePlane* plane, uint majorThinningFactor );

    qreal position() const { return m_position; }
    QString text() const { return m_text; }
    TickType type() const { return m_type; }
    bool hasShorterLabels() const { return m_axis && !m_axis->labels().isEmpty() &&
                                    m_axis->shortLabels().count() == m_axis->labels().count(); }
    bool isAtEnd() const { return m_position == std::numeric_limits< qreal >::infinity(); }
    void operator++();

    bool areAlmostEqual( qreal r1, qreal r2 ) const;

private:
    // code shared by the two constructors
    void init( bool isY, bool hasMajorTicks, bool hasMinorTicks, CartesianCoordinatePlane* plane );

    bool isHigherPrecedence( qreal importantLabelValue, qreal unimportantLabelValue ) const;
    void computeMajorTickLabel( int decimalPlaces );

    // these are generally set once in the constructor
    CartesianAxis* m_axis;
    DataDimension m_dimension; // upper and lower bounds
    int m_decimalPlaces; // for numeric labels
    bool m_isLogarithmic;
    QMap< qreal, QString > m_annotations;
    QMap< qreal, QString > m_dataHeaderLabels;
    QList< qreal > m_customTicks;
    QStringList m_manualLabelTexts;
    uint m_majorThinningFactor;
    uint m_majorLabelCount;

    // these generally change in operator++(), i.e. from one label to the next
    int m_customTickIndex;
    int m_manualLabelIndex;
    TickType m_type;
    qreal m_position;
    qreal m_customTick;
    qreal m_majorTick;
    qreal m_minorTick;
    QString m_text;
};

}

#endif
