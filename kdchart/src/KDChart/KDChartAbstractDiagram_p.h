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

#ifndef KDCHARTABSTRACTDIAGRAM_P_H
#define KDCHARTABSTRACTDIAGRAM_P_H

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

#include "KDChartAbstractDiagram.h"
#include "KDChartAbstractCoordinatePlane.h"
#include "KDChartDataValueAttributes.h"
#include "KDChartBackgroundAttributes.h"
#include "KDChartRelativePosition.h"
#include "KDChartPosition.h"
#include "KDChartPaintContext.h"
#include "KDChartPrintingParameters.h"
#include "KDChartChart.h"
#include <KDChartCartesianDiagramDataCompressor_p.h>
#include "ReverseMapper.h"

#include <QMap>
#include <QPoint>
#include <QPointer>
#include <QFont>
#include <QFontMetrics>
#include <QPaintDevice>
#include <QModelIndex>


namespace KDChart {
    class LabelPaintInfo {
    public:
        LabelPaintInfo();
        LabelPaintInfo( const QModelIndex& _index, const DataValueAttributes& _attrs,
                        const QPainterPath& _labelArea, const QPointF& _markerPos,
                        bool _isValuePositive, const QString& _value );
        LabelPaintInfo( const LabelPaintInfo& other );
        QModelIndex index;
        DataValueAttributes attrs;
        QPainterPath labelArea;
        QPointF markerPos;
        bool isValuePositive;
        // could (ab)use attrs.dataLabel() instead
        QString value;
    };

    class LabelPaintCache
    {
    public:
        LabelPaintCache() {}
        ~LabelPaintCache()
        {
            clear();
        }

        void clear()
        {
            paintReplay.clear();
        }

        QVector<LabelPaintInfo> paintReplay;
    private:
        LabelPaintCache( LabelPaintCache& other ); // no copies
    };


/**
 * \internal
 */
    class AttributesModel;

    class KDChart::AbstractDiagram::Private
    {
        friend class AbstractDiagram;
    public:
        explicit Private();
        virtual ~Private();

        Private( const Private& rhs );

        void setAttributesModel( AttributesModel* );

        bool usesExternalAttributesModel() const;

        // FIXME: Optimize if necessary
        virtual qreal calcPercentValue( const QModelIndex & index ) const;

        // this should possibly be virtual so it can be overridden
        void addLabel( LabelPaintCache* cache,
                       const QModelIndex& index,
                       const CartesianDiagramDataCompressor::CachePosition* position,
                       const PositionPoints& points, const Position& autoPositionPositive,
                       const Position& autoPositionNegative, const qreal value,
                       qreal favoriteAngle = 0.0 );

        const QFontMetrics* cachedFontMetrics( const QFont& font, const QPaintDevice* paintDevice) const;
        const QFontMetrics cachedFontMetrics() const;

        QString formatNumber( qreal value, int decimalDigits ) const;
        QString formatDataValueText( const DataValueAttributes &dva,
                                     const QModelIndex& index, qreal value ) const;

        void forgetAlreadyPaintedDataValues();

        void paintDataValueTextsAndMarkers( PaintContext* ctx,
                                            const LabelPaintCache& cache,
                                            bool paintMarkers,
                                            bool justCalculateRect=false,
                                            QRectF* cumulatedBoundingRect=0 );

        void paintDataValueText( QPainter* painter,
                                 const QModelIndex& index,
                                 const QPointF& pos,
                                 qreal value,
                                 bool justCalculateRect=false,
                                 QRectF* cumulatedBoundingRect=0 );

        void paintDataValueText( QPainter* painter,
                                 const DataValueAttributes& attrs,
                                 const QPointF& pos,
                                 bool valueIsPositive,
                                 const QString& text,
                                 bool justCalculateRect=false,
                                 QRectF* cumulatedBoundingRect=0 );

        inline int datasetCount() const
        {
            return attributesModel->columnCount( attributesModelRootIndex ) / datasetDimension;
        }

        virtual QModelIndex indexAt( const QPoint& point ) const;

        QModelIndexList indexesAt( const QPoint& point ) const;

        QModelIndexList indexesIn( const QRect& rect ) const;

        virtual CartesianDiagramDataCompressor::AggregatedDataValueAttributes aggregatedAttrs(
                const QModelIndex & index,
                const CartesianDiagramDataCompressor::CachePosition * position ) const;

        /**
         * Sets arbitrary attributes of a data set.
         */
        void setDatasetAttrs( int dataset, const QVariant& data, int role );

        /**
         * Retrieves arbitrary attributes of a data set.
         */
        QVariant datasetAttrs( int dataset, int role ) const;

        /**
         * Resets an attribute of a dataset back to its default.
         */
        void resetDatasetAttrs( int dataset, int role );

        /**
         * Whether the diagram is transposed (X and Y swapped), which has the same effect as rotating
         * the diagram 90° clockwise and then inverting the vertical (then X) coordinate.
         */
        bool isTransposed() const;

        static Private* get( AbstractDiagram *diagram ) { return diagram->_d; }

        AbstractDiagram* diagram;
        ReverseMapper reverseMapper;
        bool doDumpPaintTime; // for use in performance testing code

    protected:
        void init();
        void init( AbstractCoordinatePlane* plane );

        QPointer<AbstractCoordinatePlane> plane;
        mutable QModelIndex attributesModelRootIndex;
        QPointer<AttributesModel> attributesModel;
        bool allowOverlappingDataValueTexts;
        bool antiAliasing;
        bool percent;
        int datasetDimension;
        mutable QPair<QPointF,QPointF> databoundaries;
        mutable bool databoundariesDirty;

        QMap< Qt::Orientation, QString > unitSuffix;
        QMap< Qt::Orientation, QString > unitPrefix;
        QMap< int, QMap< Qt::Orientation, QString > > unitSuffixMap;
        QMap< int, QMap< Qt::Orientation, QString > > unitPrefixMap;
        QList< QPainterPath > alreadyDrawnDataValueTexts;

    private:
        QString prevPaintedDataValueText;
        mutable QFontMetrics mCachedFontMetrics;
        mutable QFont mCachedFont;
        mutable QPaintDevice* mCachedPaintDevice;
    };

    inline AbstractDiagram::AbstractDiagram( Private * p ) : _d( p )
    {
        init();
    }
    inline AbstractDiagram::AbstractDiagram(
        Private * p, QWidget* parent, AbstractCoordinatePlane* plane )
        : QAbstractItemView( parent ), _d( p )
    {
        _d->init( plane );
        init();
    }


    class LineAttributesInfo {
    public:
        LineAttributesInfo();
        LineAttributesInfo( const QModelIndex& _index, const QPointF& _value, const QPointF& _nextValue );

        QModelIndex index;
        QPointF value;
        QPointF nextValue;
    };

    typedef QVector<LineAttributesInfo> LineAttributesInfoList;
    typedef QVectorIterator<LineAttributesInfo> LineAttributesInfoListIterator;

}
#endif /* KDCHARTDIAGRAM_P_H */
