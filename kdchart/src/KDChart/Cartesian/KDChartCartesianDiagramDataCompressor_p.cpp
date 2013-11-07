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

#include "KDChartCartesianDiagramDataCompressor_p.h"

#include <QtDebug>
#include <QAbstractItemModel>

#include "KDChartAbstractCartesianDiagram.h"

#include <KDABLibFakes>

using namespace KDChart;
using namespace std;

CartesianDiagramDataCompressor::CartesianDiagramDataCompressor( QObject* parent )
    : QObject( parent )
    , m_mode( Precise )
    , m_xResolution( 0 )
    , m_yResolution( 0 )
    , m_sampleStep( 0 )
    , m_datasetDimension( 1 )
{
    calculateSampleStepWidth();
    m_data.resize( 0 );
}

static bool contains( const CartesianDiagramDataCompressor::AggregatedDataValueAttributes& aggregated,
                      const DataValueAttributes& attributes )
{
    CartesianDiagramDataCompressor::AggregatedDataValueAttributes::const_iterator it = aggregated.constBegin();
    for ( ; it != aggregated.constEnd(); ++it ) {
        if ( it.value() == attributes ) {
            return true;
        }
    }
    return false;
}

CartesianDiagramDataCompressor::AggregatedDataValueAttributes CartesianDiagramDataCompressor::aggregatedAttrs(
        const AbstractDiagram* diagram,
        const QModelIndex & index,
        const CachePosition& position ) const
{
    // return cached attrs, if any
    DataValueAttributesCache::const_iterator i = m_dataValueAttributesCache.constFind( position );
    if ( i != m_dataValueAttributesCache.constEnd() ) {
        return i.value();
    }

    // aggregate attributes from all indices in the same CachePosition as index
    CartesianDiagramDataCompressor::AggregatedDataValueAttributes aggregated;
    KDAB_FOREACH( const QModelIndex& neighborIndex, mapToModel( position ) ) {
        DataValueAttributes attrs = diagram->dataValueAttributes( neighborIndex );
        // only store visible and unique attributes
        if ( !attrs.isVisible() ) {
            continue;
        }
        if ( !contains( aggregated, attrs ) ) {
            aggregated[ neighborIndex ] = attrs;
        }
    }
    // if none of the attributes had the visible flag set, we just take the one set for the index
    // to avoid returning an empty list (### why not return an empty list?)
    if ( aggregated.isEmpty() ) {
        aggregated[index] = diagram->dataValueAttributes( index );
    }

    m_dataValueAttributesCache[position] = aggregated;
    return aggregated;
}

bool CartesianDiagramDataCompressor::prepareDataChange( const QModelIndex& parent, bool isRows,
                                                        int* start, int* end )
{
    if ( parent != m_rootIndex ) {
        return false;
    }
    Q_ASSERT( *start <= *end );

    CachePosition startPos = isRows ? mapToCache( *start, 0  ) : mapToCache( 0, *start );
    CachePosition endPos = isRows ? mapToCache( *end, 0  ) : mapToCache( 0, *end );

    static const CachePosition nullPosition;
    if ( startPos == nullPosition ) {
        rebuildCache();
        startPos = isRows ? mapToCache( *start, 0  ) : mapToCache( 0, *start );
        endPos = isRows ? mapToCache( *end, 0  ) : mapToCache( 0, *end );
        // The start position still isn't valid,
        // means that no resolution was set yet or we're about to add the first rows
        if ( startPos == nullPosition ) {
            return false;
        }
    }

    *start = isRows ? startPos.row : startPos.column;
    *end = isRows ? endPos.row : endPos.column;
    return true;
}

void CartesianDiagramDataCompressor::slotRowsAboutToBeInserted( const QModelIndex& parent, int start, int end )
{
    if ( !prepareDataChange( parent, true, &start, &end ) ) {
        return;
    }
    for ( int i = 0; i < m_data.size(); ++i )
    {
        Q_ASSERT( start >= 0 && start <= m_data[ i ].size() );
        m_data[ i ].insert( start, end - start + 1, DataPoint() );
    }
}

void CartesianDiagramDataCompressor::slotRowsInserted( const QModelIndex& parent, int start, int end )
{
    if ( !prepareDataChange( parent, true, &start, &end ) ) {
        return;
    }
    for ( int i = 0; i < m_data.size(); ++i )
    {
        for ( int j = start; j < m_data[i].size(); ++j ) {
            retrieveModelData( CachePosition( j, i ) );
        }
    }
}

void CartesianDiagramDataCompressor::slotColumnsAboutToBeInserted( const QModelIndex& parent, int start, int end )
{
    if ( !prepareDataChange( parent, false, &start, &end ) ) {
        return;
    }
    const int rowCount = qMin( m_model ? m_model->rowCount( m_rootIndex ) : 0, m_xResolution );
    Q_ASSERT( start >= 0 && start <= m_data.size() );
    m_data.insert( start, end - start + 1, QVector< DataPoint >( rowCount ) );
}

void CartesianDiagramDataCompressor::slotColumnsInserted( const QModelIndex& parent, int start, int end )
{
    if ( !prepareDataChange( parent, false, &start, &end ) ) {
        return;
    }
    for ( int i = start; i < m_data.size(); ++i )
    {
        for (int j = 0; j < m_data[i].size(); ++j ) {
            retrieveModelData( CachePosition( j, i ) );
        }
    }
}

void CartesianDiagramDataCompressor::slotRowsAboutToBeRemoved( const QModelIndex& parent, int start, int end )
{
    if ( !prepareDataChange( parent, true, &start, &end ) ) {
        return;
    }
    for ( int i = 0; i < m_data.size(); ++i ) {
        m_data[ i ].remove( start, end - start + 1 );
    }
}

void CartesianDiagramDataCompressor::slotRowsRemoved( const QModelIndex& parent, int start, int end )
{
    if ( parent != m_rootIndex )
        return;
    Q_ASSERT( start <= end );
    Q_UNUSED( end )

    CachePosition startPos = mapToCache( start, 0 );
    static const CachePosition nullPosition;
    if ( startPos == nullPosition ) {
        // Since we should already have rebuilt the cache, it won't help to rebuild it again.
        // Do not Q_ASSERT() though, since the resolution might simply not be set or we might now have 0 rows
        return;
    }

    for ( int i = 0; i < m_data.size(); ++i ) {
        for (int j = startPos.row; j < m_data[i].size(); ++j ) {
            retrieveModelData( CachePosition( j, i ) );
        }
    }
}

void CartesianDiagramDataCompressor::slotColumnsAboutToBeRemoved( const QModelIndex& parent, int start, int end )
{
    if ( !prepareDataChange( parent, false, &start, &end ) ) {
        return;
    }
    m_data.remove( start, end - start + 1 );
}

void CartesianDiagramDataCompressor::slotColumnsRemoved( const QModelIndex& parent, int start, int end )
{
    if ( parent != m_rootIndex )
        return;
    Q_ASSERT( start <= end );
    Q_UNUSED( end );

    const CachePosition startPos = mapToCache( 0, start );

    static const CachePosition nullPosition;
    if ( startPos == nullPosition ) {
        // Since we should already have rebuilt the cache, it won't help to rebuild it again.
        // Do not Q_ASSERT() though, since the resolution might simply not be set or we might now have 0 columns
        return;
    }

    for ( int i = startPos.column; i < m_data.size(); ++i ) {
        for ( int j = 0; j < m_data[i].size(); ++j ) {
            retrieveModelData( CachePosition( j, i ) );
        }
    }
}

void CartesianDiagramDataCompressor::slotModelHeaderDataChanged( Qt::Orientation orientation, int first, int last )
{
    if ( orientation != Qt::Vertical )
        return;

    if ( m_model->rowCount( m_rootIndex ) > 0 ) {
        const QModelIndex firstRow = m_model->index( 0, first, m_rootIndex ); // checked
        const QModelIndex lastRow = m_model->index( m_model->rowCount( m_rootIndex ) - 1, last, m_rootIndex ); // checked

        slotModelDataChanged( firstRow, lastRow );
    }
}

void CartesianDiagramDataCompressor::slotModelDataChanged(
    const QModelIndex& topLeftIndex,
    const QModelIndex& bottomRightIndex )
{
    if ( topLeftIndex.parent() != m_rootIndex )
        return;
    Q_ASSERT( topLeftIndex.parent() == bottomRightIndex.parent() );
    Q_ASSERT( topLeftIndex.row() <= bottomRightIndex.row() );
    Q_ASSERT( topLeftIndex.column() <= bottomRightIndex.column() );
    CachePosition topleft = mapToCache( topLeftIndex );
    CachePosition bottomright = mapToCache( bottomRightIndex );
    for ( int row = topleft.row; row <= bottomright.row; ++row )
        for ( int column = topleft.column; column <= bottomright.column; ++column )
            invalidate( CachePosition( row, column ) );
}

void CartesianDiagramDataCompressor::slotModelLayoutChanged()
{
    rebuildCache();
    calculateSampleStepWidth();
}

void CartesianDiagramDataCompressor::slotDiagramLayoutChanged( AbstractDiagram* diagramBase )
{
    AbstractCartesianDiagram* diagram = qobject_cast< AbstractCartesianDiagram* >( diagramBase );
    Q_ASSERT( diagram );
    if ( diagram->datasetDimension() != m_datasetDimension ) {
        setDatasetDimension( diagram->datasetDimension() );
    }
}

int CartesianDiagramDataCompressor::modelDataColumns() const
{
    Q_ASSERT( m_datasetDimension != 0 );
    // only operational if there is a model and a resolution
    if ( m_model ) {
        const int columns = m_model->columnCount( m_rootIndex ) / m_datasetDimension;

//        if ( columns != m_data.size() )
//        {
//            rebuildCache();
//        }

        Q_ASSERT( columns == m_data.size() );
        return columns;
    } else {
        return 0;
    }
}

int CartesianDiagramDataCompressor::modelDataRows() const
{
    // only operational if there is a model, columns, and a resolution
    if ( m_model && m_model->columnCount( m_rootIndex ) > 0 && m_xResolution > 0 ) {
        return m_data.isEmpty() ? 0 : m_data.first().size();
    } else {
        return 0;
    }
}

void CartesianDiagramDataCompressor::setModel( QAbstractItemModel* model )
{
    if ( m_model != 0 && m_model != model ) {
        disconnect( m_model, SIGNAL( headerDataChanged( Qt::Orientation, int, int ) ),
                 this, SLOT( slotModelHeaderDataChanged( Qt::Orientation, int, int ) ) );
        disconnect( m_model, SIGNAL( dataChanged( QModelIndex, QModelIndex ) ),
                 this, SLOT( slotModelDataChanged( QModelIndex, QModelIndex ) ) );
        disconnect( m_model, SIGNAL( layoutChanged() ),
                 this, SLOT( slotModelLayoutChanged() ) );
        disconnect( m_model, SIGNAL( rowsAboutToBeInserted( QModelIndex, int, int ) ),
                 this, SLOT( slotRowsAboutToBeInserted( QModelIndex, int, int ) ) );
        disconnect( m_model, SIGNAL( rowsInserted( QModelIndex, int, int ) ),
                 this, SLOT( slotRowsInserted( QModelIndex, int, int ) ) );
        disconnect( m_model, SIGNAL( rowsAboutToBeRemoved( QModelIndex, int, int ) ),
                 this, SLOT( slotRowsAboutToBeRemoved( QModelIndex, int, int ) ) );
        disconnect( m_model, SIGNAL( rowsRemoved( QModelIndex, int, int ) ),
                 this, SLOT( slotRowsRemoved( QModelIndex, int, int ) ) );
        disconnect( m_model, SIGNAL( columnsAboutToBeInserted( QModelIndex, int, int ) ),
                 this, SLOT( slotColumnsAboutToBeInserted( QModelIndex, int, int ) ) );
        disconnect( m_model, SIGNAL( columnsInserted( QModelIndex, int, int ) ),
                 this, SLOT( slotColumnsInserted( QModelIndex, int, int ) ) );
        disconnect( m_model, SIGNAL( columnsRemoved( QModelIndex, int, int ) ),
                 this, SLOT( slotColumnsRemoved( QModelIndex, int, int ) ) );
        disconnect( m_model, SIGNAL( columnsAboutToBeRemoved( QModelIndex, int, int ) ),
                 this, SLOT( slotColumnsAboutToBeRemoved( QModelIndex, int, int ) ) );
        disconnect( m_model, SIGNAL( modelReset() ),
                    this, SLOT( rebuildCache() ) );
        m_model = 0;
    }

    m_modelCache.setModel( model );

    if ( model != 0 ) {
        m_model = model;
        connect( m_model, SIGNAL( headerDataChanged( Qt::Orientation, int, int ) ),
                 SLOT( slotModelHeaderDataChanged( Qt::Orientation, int, int ) ) );
        connect( m_model, SIGNAL( dataChanged( QModelIndex, QModelIndex ) ),
                 SLOT( slotModelDataChanged( QModelIndex, QModelIndex ) ) );
        connect( m_model, SIGNAL( layoutChanged() ),
                 SLOT( slotModelLayoutChanged() ) );
        connect( m_model, SIGNAL( rowsAboutToBeInserted( QModelIndex, int, int ) ),
                 SLOT( slotRowsAboutToBeInserted( QModelIndex, int, int ) ) );
        connect( m_model, SIGNAL( rowsInserted( QModelIndex, int, int ) ),
                 SLOT( slotRowsInserted( QModelIndex, int, int ) ) );
        connect( m_model, SIGNAL( rowsAboutToBeRemoved( QModelIndex, int, int ) ),
                 SLOT( slotRowsAboutToBeRemoved( QModelIndex, int, int ) ) );
        connect( m_model, SIGNAL( rowsRemoved( QModelIndex, int, int ) ),
                 SLOT( slotRowsRemoved( QModelIndex, int, int ) ) );
        connect( m_model, SIGNAL( columnsAboutToBeInserted( QModelIndex, int, int ) ),
                 SLOT( slotColumnsAboutToBeInserted( QModelIndex, int, int ) ) );
        connect( m_model, SIGNAL( columnsInserted( QModelIndex, int, int ) ),
                 SLOT( slotColumnsInserted( QModelIndex, int, int ) ) );
        connect( m_model, SIGNAL( columnsRemoved( QModelIndex, int, int ) ),
                 SLOT( slotColumnsRemoved( QModelIndex, int, int ) ) );
        connect( m_model, SIGNAL( columnsAboutToBeRemoved( QModelIndex, int, int ) ),
                 SLOT( slotColumnsAboutToBeRemoved( QModelIndex, int, int ) ) );
        connect( m_model, SIGNAL( modelReset() ),
                    this, SLOT( rebuildCache() ) );
    }
    rebuildCache();
    calculateSampleStepWidth();
}

void CartesianDiagramDataCompressor::setRootIndex( const QModelIndex& root )
{
    if ( m_rootIndex != root ) {
        Q_ASSERT( root.model() == m_model || !root.isValid() );
        m_rootIndex = root;
        m_modelCache.setRootIndex( root );
        rebuildCache();
        calculateSampleStepWidth();
    }
}

void CartesianDiagramDataCompressor::recalcResolution()
{
    setResolution( m_xResolution, m_yResolution );
}

void CartesianDiagramDataCompressor::setResolution( int x, int y )
{
    if ( setResolutionInternal( x, y ) ) {
        rebuildCache();
        calculateSampleStepWidth();
    }
}

bool CartesianDiagramDataCompressor::setResolutionInternal( int x, int y )
{
    const int oldXRes = m_xResolution;
    const int oldYRes = m_yResolution;

    if ( m_datasetDimension != 1 ) {
        // just ignore the X resolution in that case
        m_xResolution = m_model ? m_model->rowCount( m_rootIndex ) : 0;
    } else {
        m_xResolution = qMax( 0, x );
    }
    m_yResolution = qMax( 0, y );

    return m_xResolution != oldXRes || m_yResolution != oldYRes;
}

void CartesianDiagramDataCompressor::clearCache()
{
    for ( int column = 0; column < m_data.size(); ++column )
        m_data[column].fill( DataPoint() );
}

void CartesianDiagramDataCompressor::rebuildCache()
{
    Q_ASSERT( m_datasetDimension != 0 );

    m_data.clear();
    setResolutionInternal( m_xResolution, m_yResolution );
    const int columnDivisor = m_datasetDimension != 2 ? 1 : m_datasetDimension;
    const int columnCount = m_model ? m_model->columnCount( m_rootIndex ) / columnDivisor : 0;
    const int rowCount = qMin( m_model ? m_model->rowCount( m_rootIndex ) : 0, m_xResolution );
    m_data.resize( columnCount );
    for ( int i = 0; i < columnCount; ++i ) {
        m_data[i].resize( rowCount );
    }
    // also empty the attrs cache
    m_dataValueAttributesCache.clear();
}

const CartesianDiagramDataCompressor::DataPoint& CartesianDiagramDataCompressor::data( const CachePosition& position ) const
{
    static DataPoint nullDataPoint;
    if ( ! mapsToModelIndex( position ) ) {
        return nullDataPoint;
    }
    if ( ! isCached( position ) ) {
        retrieveModelData( position );
    }
    return m_data[ position.column ][ position.row ];
}

QPair< QPointF, QPointF > CartesianDiagramDataCompressor::dataBoundaries() const
{
    const int colCount = modelDataColumns();
    qreal xMin = std::numeric_limits< qreal >::quiet_NaN();
    qreal xMax = std::numeric_limits< qreal >::quiet_NaN();
    qreal yMin = std::numeric_limits< qreal >::quiet_NaN();
    qreal yMax = std::numeric_limits< qreal >::quiet_NaN();

    for ( int column = 0; column < colCount; ++column )
    {
        const DataPointVector& data = m_data[ column ];
        int row = 0;
        for ( DataPointVector::const_iterator it = data.begin(); it != data.end(); ++it, ++row )
        {
            const DataPoint& p = *it;
            if ( !p.index.isValid() )
                retrieveModelData( CachePosition( row, column ) );

            const qreal valueX = ISNAN( p.key ) ? 0.0 : p.key;
            const qreal valueY = ISNAN( p.value ) ? 0.0 : p.value;
            if ( ISNAN( xMin ) )
            {
                xMin = valueX;
                xMax = valueX;
                yMin = valueY;
                yMax = valueY;
            }
            else
            {
                xMin = qMin( xMin, valueX );
                xMax = qMax( xMax, valueX );
                yMin = qMin( yMin, valueY );
                yMax = qMax( yMax, valueY );
            }
        }
    }

    // NOTE: calculateDataBoundaries must return the *real* data boundaries!
    //       i.e. we may NOT fake yMin to be qMin( 0.0, yMin )
    //       (khz, 2008-01-24)
    const QPointF bottomLeft( xMin, yMin );
    const QPointF topRight( xMax, yMax );
    return qMakePair( bottomLeft, topRight );
}

void CartesianDiagramDataCompressor::retrieveModelData( const CachePosition& position ) const
{
    Q_ASSERT( mapsToModelIndex( position ) );
    DataPoint result;
    result.hidden = true;

    switch ( m_mode ) {
    case Precise:
    {
        const QModelIndexList indexes = mapToModel( position );

        if ( m_datasetDimension == 2 ) {
            Q_ASSERT( indexes.count() == 2 );
            const QModelIndex& xIndex = indexes.at( 0 );
            result.index = xIndex;
            result.key = m_modelCache.data( xIndex );
            result.value = m_modelCache.data( indexes.at( 1 ) );
        } else {
            if ( indexes.isEmpty() ) {
                break;
            }
            result.value = std::numeric_limits< qreal >::quiet_NaN();
            result.key = 0.0;
            Q_FOREACH( const QModelIndex& index, indexes ) {
                const qreal value = m_modelCache.data( index );
                if ( !ISNAN( value ) ) {
                    result.value = ISNAN( result.value ) ? value : result.value + value;
                }
                result.key += index.row();
            }
            result.index = indexes.at( 0 );
            result.key /= indexes.size();
            result.value /= indexes.size();
        }

        Q_FOREACH( const QModelIndex& index, indexes ) {
            // the DataPoint point is visible if any of the underlying, aggregated points is visible
            if ( m_model->data( index, DataHiddenRole ).value<bool>() == false ) {
                result.hidden = false;
            }
        }
        break;
    }
    case SamplingSeven:
        break;
    }

    m_data[ position.column ][ position.row ] = result;
    Q_ASSERT( isCached( position ) );
}

CartesianDiagramDataCompressor::CachePosition CartesianDiagramDataCompressor::mapToCache(
        const QModelIndex& index ) const
{
    Q_ASSERT( m_datasetDimension != 0 );

    static const CachePosition nullPosition;
    if ( !index.isValid() ) {
        return nullPosition;
    }
    return mapToCache( index.row(), index.column() );
}

CartesianDiagramDataCompressor::CachePosition CartesianDiagramDataCompressor::mapToCache(
        int row, int column ) const
{
    Q_ASSERT( m_datasetDimension != 0 );

    if ( m_data.size() == 0 || m_data[ 0 ].size() == 0 ) {
        return mapToCache( QModelIndex() );
    }
    // assumption: indexes per column == 1
    if ( indexesPerPixel() == 0 ) {
        return mapToCache( QModelIndex() );
    }
    return CachePosition( int( row / indexesPerPixel() ), column / m_datasetDimension );
}

QModelIndexList CartesianDiagramDataCompressor::mapToModel( const CachePosition& position ) const
{
    QModelIndexList indexes;
    if ( !mapsToModelIndex( position ) ) {
        return indexes;
    }

    if ( m_datasetDimension == 2 ) {
        indexes << m_model->index( position.row, position.column * 2, m_rootIndex ); // checked
        indexes << m_model->index( position.row, position.column * 2 + 1, m_rootIndex ); // checked
    } else {
        // assumption: indexes per column == 1 (not true e.g. for stock diagrams with often three or
        // four dimensions: High-Low-Close or Open-High-Low-Close)
        Q_ASSERT( position.column < m_model->columnCount( m_rootIndex ) );
        const qreal ipp = indexesPerPixel();
        const int baseRow = floor( position.row * ipp );
        // the following line needs to work for the last row(s), too...
        const int endRow = floor( ( position.row + 1 ) * ipp );
        for ( int row = baseRow; row < endRow; ++row ) {
            Q_ASSERT( row < m_model->rowCount( m_rootIndex ) );
            const QModelIndex index = m_model->index( row, position.column, m_rootIndex );
            if ( index.isValid() ) {
                indexes << index;
            }
        }
    }
    return indexes;
}

qreal CartesianDiagramDataCompressor::indexesPerPixel() const
{
    if ( !m_model || m_data.size() == 0 || m_data[ 0 ].size() == 0 )  {
        return 0;
    }
    return qreal( m_model->rowCount( m_rootIndex ) ) / qreal( m_data[ 0 ].size() );
}

bool CartesianDiagramDataCompressor::mapsToModelIndex( const CachePosition& position ) const
{
    return m_model && m_data.size() > 0 && m_data[ 0 ].size() > 0 &&
           position.column >= 0 && position.column < m_data.size() &&
           position.row >=0 && position.row < m_data[ 0 ].size();
}

void CartesianDiagramDataCompressor::invalidate( const CachePosition& position )
{
    if ( mapsToModelIndex( position ) ) {
        m_data[ position.column ][ position.row ] = DataPoint();
        // Also invalidate the data value attributes at "position".
        // Otherwise the user overwrites the attributes without us noticing
        // it because we keep reading what's in the cache.
        m_dataValueAttributesCache.remove( position );
    }
}

bool CartesianDiagramDataCompressor::isCached( const CachePosition& position ) const
{
    Q_ASSERT( mapsToModelIndex( position ) );
    const DataPoint& p = m_data[ position.column ][ position.row ];
    return p.index.isValid();
}

void CartesianDiagramDataCompressor::calculateSampleStepWidth()
{
    if ( m_mode == Precise ) {
        m_sampleStep = 1;
        return;
    }

    static unsigned int SomePrimes[] = {
        2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47,
        53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101,
        151, 211, 313, 401, 503, 607, 701, 811, 911, 1009,
        10037, 12911, 16001, 20011, 50021,
        100003, 137867, 199999, 500009, 707753, 1000003, 0
    }; // ... after that, having a model at all becomes impractical

    // we want at least 17 samples per data point, using a prime step width
    const qreal WantedSamples = 17;
    if ( WantedSamples > indexesPerPixel() ) {
        m_sampleStep = 1;
    } else {
        int i;
        for ( i = 0; SomePrimes[i] != 0; ++i ) {
            if ( WantedSamples * SomePrimes[i+1] > indexesPerPixel() ) {
                break;
            }
        }
        m_sampleStep = SomePrimes[i];
        if ( SomePrimes[i] == 0 ) {
            m_sampleStep = SomePrimes[i-1];
        } else {
            m_sampleStep = SomePrimes[i];
        }
    }
}

void CartesianDiagramDataCompressor::setDatasetDimension( int dimension )
{
    if ( dimension != m_datasetDimension ) {
        m_datasetDimension = dimension;
        rebuildCache();
        calculateSampleStepWidth();
    }
}
