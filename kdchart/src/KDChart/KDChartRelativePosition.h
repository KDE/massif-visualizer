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

#ifndef KDCHARTRELATIVEPOSITION_H
#define KDCHARTRELATIVEPOSITION_H

#include <QDebug>
#include <QMetaType>
#include <Qt>
#include <QPointF>
#include <QSizeF>
#include "KDChartGlobal.h"

namespace KDChart {

    class Position;
    class PositionPoints;
    class Measure;

/**
  \class RelativePosition KDChartRelativePosition.h
  \brief Defines relative position information: reference area, position
  in this area (reference position), horizontal / vertical padding, and rotation.

  See detailed description of \a KDChart::Position for an illustration of the
  different possible reference positions.

  Using RelativePosition you can specify the relative parts
  of some position information, and you can specify the absolute parts:
  the reference area, and the position in this area.

  \note To get an absolute position, you have three options:
  \li either you declare both, the relative and the absolute parts,
      using setReferenceArea for the latter,
  \li or you specify a set of points, using setReferencePoints,
  \li or you don't use either, leaving it to KD Chart to find a suitable reference area.
  */
class KDCHART_EXPORT RelativePosition
{
public:
    RelativePosition();
    RelativePosition( const RelativePosition& );

    RelativePosition & operator=( const RelativePosition & other );

    ~RelativePosition();

    /**
     * \brief Set the reference area to be used to find the anchor point.
     *
     * The reference area's type can be either a QWidget subclass or a KDChart::AbstractArea subclass.
     *
     * \note Usage of reference area and reference points is mutually exclusive:
     * Only one can be used, so any previously set reference points are cleared
     * when you call setReferenceArea.
     *
     * Also note: In a few cases KD Chart will ignore your area (or points, resp.) settings!
     * Relative positioning of data value texts is an example: For these
     * the reference area is always taken to be the data area.
     *
     * \sa setReferencePosition, setAlignment, setHorizontalPadding, setVerticalPadding
     */
    void setReferenceArea( QObject* area );
    QObject* referenceArea() const;

    /**
     * \brief Set a set of points from which the anchor point will be selected.
     *
     * \note Usage of reference area and reference points is mutually exclusive:
     * Only one can be used, so any previously set reference area is cleared
     * when you call setReferencePoints.
     *
     * Also note: In a few cases KD Chart will ignore your points (or area, resp.) settings!
     * Relative positioning of data value texts is an example: For these
     * the reference area is always taken to be the data area.
     *
     * \sa setReferenceArea, setReferencePosition, setAlignment, setHorizontalPadding, setVerticalPadding
     */
    void setReferencePoints( const PositionPoints& points );
    const PositionPoints referencePoints() const;

    /**
     * \brief Set the position of the anchor point.
     *
     * The anchor point of a RelativePosition may be one of the pre-defined
     * points of it's reference area - for details see KDChart::Position.
     *
     * See detailed description of \a KDChart::Position for an illustration of the
     * different possible reference positions.
     *
     *  \sa resetReferencePosition, setReferenceArea, setAlignment, setHorizontalPadding, setVerticalPadding, KDChart::Position
     */
    void setReferencePosition( Position position );

    /**
     * \brief Resets the position of the anchor point to the built-in default.
     *
     * If the anchor point of a RelativePosition is reset (or never changed from the
     * default setting) KD Chart will choose an appropriate Position at run-time.
     *
     * e.g. BarDiagrams will use Position::North / Position::South for positive / negative values.
     *
     *  \sa setReferencePosition, setReferenceArea, setAlignment, setHorizontalPadding, setVerticalPadding, KDChart::Position
     */
    void resetReferencePosition();
    Position referencePosition() const;

    /**
     * Set the alignment of the content placed by this RelativePosition.
     *
     * Padding is applied first to obtain the final reference point
     * for the content's alignment
     *
     * \note To print centered content, besides calling setAlignment( Qt::AlignCenter )
     * you might also want to set zero padding to have your text centered more precisely.
     *
     * \sa setReferencePosition, setReferenceArea, setHorizontalPadding, setVerticalPadding
     */
    void setAlignment( Qt::Alignment flags );
    Qt::Alignment alignment() const;

    /**
     * Set the width of the horizontal padding between the anchor point and the content
     * placed by this RelativePosition.
     *
     * \note When printing data value texts this Measure is used to find the alignment
     * point for this text, then alignment() is use to determine how to align the text
     * relative to that point.
     * The font height is used as reference size for both horizontal and vertical padding
     * if the respective padding's Measure is using automatic reference area detection.
     *
     * \sa setVerticalPadding, setReferencePosition, setReferenceArea
     */
    void setHorizontalPadding( const Measure& padding );
    Measure horizontalPadding() const;

    /**
     * Set the height of the vertical padding between the anchor point and the content
     * placed by this RelativePosition.
     *
     * \note When printing data value texts this Measure is used to find the alignment
     * point for this text, then alignment() is use to determine how to align the text
     * relative to that point.
     * The font height is used as reference size for both horizontal and vertical padding
     * if the respective padding's Measure is using automatic reference area detection.
     *
     * \sa setHorizontalPadding, setReferencePosition, setReferenceArea
     */
    void setVerticalPadding( const Measure& padding );
    Measure verticalPadding() const;

    void setRotation( qreal rot );
    qreal rotation() const;

    /**
     * \brief Return the reference point, according to the reference area/position, and ignoring padding.
     *
     * This method is called at drawing time.
     * The returned point is used to test if the label of a data value is to be printed: a label
     * is printed only if its reference point is inside or touching the coordinate plane.
     *
     * If polarDegrees is set, the degree information will be returned that was stored for the
     * respective point. This is used by the PieDiagram class to determine how vertical/horizontal
     * padding settings should affect the position of the data value texts' reference points.
     * \sa calculatedPoint, setReferenceArea, setReferencePosition, setHorizontalPadding, setVerticalPadding
     */
    const QPointF referencePoint(qreal* polarDegrees=0) const;

    /**
     * \brief Calculate a point, accordin to the reference area/position and the padding.
     *
     * This method is called at drawing time: The returned point is used as anchor point.
     * Note that it is the task of the calling code to place the content, taking the alignment
     * property into account. This class does not know the size of the content so it
     * cannot place it.
     *
     * \sa referencePoint, setReferenceArea, setReferencePosition, setHorizontalPadding, setVerticalPadding
     */
    const QPointF calculatedPoint( const QSizeF& autoSize ) const;

    bool operator==( const RelativePosition& ) const;
    bool operator!=( const RelativePosition & other ) const;

private:
    KDCHART_DECLARE_PRIVATE_BASE_VALUE( RelativePosition )
};

inline bool RelativePosition::operator!=( const RelativePosition & other ) const { return !operator==( other ); }
}

#if !defined(QT_NO_DEBUG_STREAM)
KDCHART_EXPORT QDebug operator<<(QDebug, const KDChart::RelativePosition& );
#endif /* QT_NO_DEBUG_STREAM */

KDCHART_DECLARE_SWAP_SPECIALISATION( KDChart::RelativePosition )

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO( KDChart::RelativePosition, Q_MOVABLE_TYPE );
QT_END_NAMESPACE

Q_DECLARE_METATYPE( KDChart::RelativePosition )

#endif // KDCHARTRELATIVEPOSITION_H
