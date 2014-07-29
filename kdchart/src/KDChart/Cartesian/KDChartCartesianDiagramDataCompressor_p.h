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

#ifndef KDCHARTCARTESIANDIAGRAMDATACOMPRESSOR_H
#define KDCHARTCARTESIANDIAGRAMDATACOMPRESSOR_H

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

#include <limits>

#include <QPair>
#include <QVector>
#include <QObject>
#include <QPointer>
#include <QModelIndex>

#include "KDChartDataValueAttributes.h"
#include "KDChartModelDataCache_p.h"

#include "kdchart_export.h"

class CartesianDiagramDataCompressorTests;
QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE

namespace KDChart {

    class AbstractDiagram;

    // - transparently compress table model data if the diagram widget
    // size does not allow to display all data points in an acceptable way
    // - the class acts much like a proxy model, but is not
    // implemented as one, to avoid performance penalties by QVariant
    // conversions
    // - a wanted side effect is that the compressor will deliver
    // more precise values for more precise media, like paper
    // (a) this is absolutely strictly seriously private API of KDChart
    // (b) if possible, this class is going to be templatized for
    // different diagram types

    // KDCHART_EXPORT is needed as long there's a test using
    // this class directly
    class KDCHART_EXPORT CartesianDiagramDataCompressor : public QObject
    {
        Q_OBJECT
        friend class ::CartesianDiagramDataCompressorTests;

    public:
        class DataPoint {
        public:
            DataPoint()
                : key( std::numeric_limits< qreal >::quiet_NaN() ),
                  value( std::numeric_limits< qreal >::quiet_NaN() ),
                  hidden( false )
                  {}
            qreal key;
            qreal value;
            bool hidden;
            QModelIndex index;
        };
        typedef QVector<DataPoint> DataPointVector;
        class CachePosition {
        public:
            CachePosition()
                : row( -1 ),
                  column( -1 )
                  {}
            CachePosition( int row, int column )
                : row( row ),
                  column( column )
                  {}
            int row;
            int column;

            bool operator==( const CachePosition& rhs ) const
            {
                return row == rhs.row &&
                       column == rhs.column;
            }
            bool operator<( const CachePosition& rhs ) const
            {
                // This function is used to topologically sort all cache positions.

                // Think of them as entries in a matrix or table:
                // An entry comes before another entry if it is either above the other
                // entry, or in the same row and to the left of the other entry.
                return row < rhs.row || ( row == rhs.row && column < rhs.column );
            }
        };

        typedef QMap< QModelIndex, DataValueAttributes > AggregatedDataValueAttributes;
        typedef QMap< CartesianDiagramDataCompressor::CachePosition, AggregatedDataValueAttributes > DataValueAttributesCache;

        enum ApproximationMode {
            // do not approximate, interpolate by averaging all
            // datapoints for a pixel
            Precise,
            // approximate by averaging out over prime number distances
            SamplingSeven
        };

        explicit CartesianDiagramDataCompressor( QObject* parent = 0 );

        // input: model, chart resolution, approximation mode
        void setModel( QAbstractItemModel* );
        void setRootIndex( const QModelIndex& root );
        void setResolution( int x, int y );
        void recalcResolution();
        void setApproximationMode( ApproximationMode mode );
        void setDatasetDimension( int dimension );

        // output: resulting model resolution, data points
        // FIXME (Mirko) rather stupid naming, Mirko!
        int modelDataColumns() const;
        int modelDataRows() const;
        const DataPoint& data( const CachePosition& ) const;

        QPair< QPointF, QPointF > dataBoundaries() const;

        AggregatedDataValueAttributes aggregatedAttrs(
                const AbstractDiagram* diagram,
                const QModelIndex & index,
                const CachePosition& position ) const;

    private Q_SLOTS:
        void slotRowsAboutToBeInserted( const QModelIndex&, int, int );
        void slotRowsInserted( const QModelIndex&, int, int );
        void slotRowsAboutToBeRemoved( const QModelIndex&, int, int );
        void slotRowsRemoved( const QModelIndex&, int, int );

        void slotColumnsAboutToBeInserted( const QModelIndex&, int, int );
        void slotColumnsInserted( const QModelIndex&, int, int );
        void slotColumnsAboutToBeRemoved( const QModelIndex&, int, int );
        void slotColumnsRemoved( const QModelIndex&, int, int );

        void slotModelHeaderDataChanged( Qt::Orientation, int, int );
        void slotModelDataChanged( const QModelIndex&, const QModelIndex& );
        void slotModelLayoutChanged();
        // FIXME resolution changes and root index changes should all
        // be catchable with this method:
        void slotDiagramLayoutChanged( AbstractDiagram* );

        // geometry has changed
        void rebuildCache();
        // reset all cached values, without changing the cache geometry
        void clearCache();

    private:
        // private version of setResolution() that does *not* call rebuildCache()
        bool setResolutionInternal( int x, int y );
        // forget cached data at the position
        void invalidate( const CachePosition& );
        // check if position is inside the dataset's index range
        bool mapsToModelIndex( const CachePosition& ) const;

        CachePosition mapToCache( const QModelIndex& ) const;
        CachePosition mapToCache( int row, int column ) const;
        // Note: returns only valid model indices
        QModelIndexList mapToModel( const CachePosition& ) const;
        qreal indexesPerPixel() const;

        // common logic for slot{Rows,Columns}[AboutToBe]{Inserted,Removed}
        bool prepareDataChange( const QModelIndex& parent,
                                bool isRows, /* columns otherwise */
                                int* start, int* end);

        // retrieve data from the model, put it into the cache
        void retrieveModelData( const CachePosition& ) const;
        // check if a data point is in the cache:
        bool isCached( const CachePosition& ) const;
        // set sample step width according to settings:
        void calculateSampleStepWidth();


        QPointer<QAbstractItemModel> m_model;
        QModelIndex m_rootIndex;

        ApproximationMode m_mode;
        int m_xResolution;
        int m_yResolution;
        unsigned int m_sampleStep;

        mutable QVector<DataPointVector> m_data; // one per dataset
        ModelDataCache< qreal, Qt::DisplayRole > m_modelCache;
        mutable DataValueAttributesCache m_dataValueAttributesCache;
        int m_datasetDimension;
    };
}

#endif
