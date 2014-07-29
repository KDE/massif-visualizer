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

#include "KDChartPlotterDiagramCompressor.h"

#include "KDChartPlotterDiagramCompressor_p.h"
#include <QtCore/QPointF>

#include <limits>
#include <KDABLibFakes>

using namespace KDChart;

qreal calculateSlope( const PlotterDiagramCompressor::DataPoint &lhs, const PlotterDiagramCompressor::DataPoint & rhs )
{
    return ( rhs.value - lhs.value ) /  ( rhs.key - lhs.key );
}

PlotterDiagramCompressor::Iterator::Iterator( int dataSet, PlotterDiagramCompressor *parent )
    : m_parent( parent )
    , m_index( 0 )
    , m_dataset( dataSet )
    , m_bufferIndex( 0 )
    , m_rebuffer( true )
{
    if ( m_parent )
    {
        if ( parent->rowCount() > m_dataset && parent->rowCount() > 0 )
        {
            m_buffer.append( parent->data( CachePosition( m_index, m_dataset ) ) );
        }
    }
    else
    {
        m_dataset = - 1;
        m_index = - 1;
    }
}

PlotterDiagramCompressor::Iterator::Iterator( int dataSet, PlotterDiagramCompressor *parent, QVector< DataPoint > buffer )
    : m_parent( parent )
    , m_buffer( buffer )
    , m_index( 0 )
    , m_dataset( dataSet )
    , m_bufferIndex( 0 )
    , m_rebuffer( false )
    , m_timeOfCreation( QDateTime::currentDateTime() )
{
    if ( !m_parent )
    {
        m_dataset = -1 ;
        m_index = - 1;
    }
    else
    {
        // buffer needs to be filled
        if ( parent->datasetCount() > m_dataset && parent->rowCount() > 0 && m_buffer.isEmpty() )
        {
            m_buffer.append( parent->data( CachePosition( m_index, m_dataset ) ) );
            m_rebuffer = true;
        }
    }
}

PlotterDiagramCompressor::Iterator::~Iterator()
{
    if ( m_parent )
    {
        if ( m_parent.data()->d->m_timeOfLastInvalidation < m_timeOfCreation )
            m_parent.data()->d->m_bufferlist[ m_dataset ] = m_buffer;
    }
}

bool PlotterDiagramCompressor::Iterator::isValid() const
{
    if ( m_parent == 0 )
        return false;
    return m_dataset >= 0 && m_index >= 0 && m_parent.data()->rowCount() > m_index;
}

//PlotterDiagramCompressor::Iterator& PlotterDiagramCompressor::Iterator::operator++()
//{
//    ++m_index;

//    ++m_bufferIndex;
//    // the version that checks dataBoundaries is seperated here, this is to avoid the runtime cost
//    // of checking everytime the boundaries if thats not necessary
//    if ( m_parent.data()->d->forcedBoundaries( Qt::Vertical ) || m_parent.data()->d->forcedBoundaries( Qt::Vertical ) )
//    {
//        if ( m_bufferIndex >= m_buffer.count()  && m_rebuffer )
//        {
//            if ( m_index < m_parent.data()->rowCount() )
//            {
//                PlotterDiagramCompressor::DataPoint dp = m_parent.data()->data( CachePosition( m_index, m_dataset ) );
//                if ( m_parent.data()->d->inBoundaries( Qt::Vertical, dp ) && m_parent.data()->d->inBoundaries( Qt::Horizontal, dp ) )
//                {
//                    m_buffer.append( dp );
//                }
//                else
//                {
//                    if ( m_index + 1 < m_parent.data()->rowCount() )
//                    {
//                        PlotterDiagramCompressor::DataPoint dp1 = m_parent.data()->data( CachePosition( m_index, m_dataset ) );
//                        if ( m_parent.data()->d->inBoundaries( Qt::Vertical, dp1 ) && m_parent.data()->d->inBoundaries( Qt::Horizontal, dp1 ) )
//                        {
//                            m_buffer.append( dp );
//                        }
//                    }
//                }
//            }
//        }
//        else
//        {
//            if ( m_bufferIndex == m_buffer.count() )
//                m_index = - 1;
//            return *this;
//        }
//        PlotterDiagramCompressor::DataPoint dp;
//        if ( isValid() )
//            dp = m_parent.data()->data( CachePosition( m_index - 1, m_dataset ) );
//        if ( m_parent )
//        {
//            if ( m_index >= m_parent.data()->rowCount() )
//                m_index = -1;
//            else
//            {
//                const qreal mergeRadius = m_parent.data()->d->m_mergeRadius;
//                PlotterDiagramCompressor::DataPoint newdp = m_parent.data()->data( CachePosition( m_index, m_dataset ) );
//                while ( dp.distance( newdp ) <= mergeRadius
//                        || !( m_parent.data()->d->inBoundaries( Qt::Vertical, dp ) || m_parent.data()->d->inBoundaries( Qt::Horizontal, dp ) ) )
//                {
//                    ++m_index;
//                    if ( m_index >= m_parent.data()->rowCount() )
//                    {
//                        m_index = - 1;
//                        break;
//                    }
//                    newdp = m_parent.data()->data( CachePosition( m_index, m_dataset ) );
//                }
//            }
//        }
//    }
//    else
//    {
//        // we have a new point in the buffer
//        if ( m_bufferIndex >= m_buffer.count()  && m_rebuffer )
//        {
//            if ( m_index < m_parent.data()->rowCount() )
//                m_buffer.append( m_parent.data()->data( CachePosition( m_index, m_dataset ) ) );
//        }
//        else
//        {
//            if ( m_bufferIndex == m_buffer.count() )
//                m_index = - 1;
//            return *this;
//        }
//        PlotterDiagramCompressor::DataPoint dp;
//        if ( isValid() )
//            dp = m_parent.data()->data( CachePosition( m_index - 1, m_dataset ) );
//        // make sure we switch to the next point which would be in the buffer
//        if ( m_parent )
//        {
//            PlotterDiagramCompressor *parent = m_parent.data();
//            if ( m_index >= parent->rowCount() )
//                m_index = -1;
//            else
//            {
//                switch ( parent->d->m_mode )
//                {
//                case( PlotterDiagramCompressor::DISTANCE ):
//                    {
//                        const qreal mergeRadius = m_parent.data()->d->m_mergeRadius;
//                        PlotterDiagramCompressor::DataPoint newdp = m_parent.data()->data( CachePosition( m_index, m_dataset ) );
//                        while ( dp.distance( newdp ) <= mergeRadius )
//                        {
//                            ++m_index;
//                            if ( m_index >= m_parent.data()->rowCount() )
//                            {
//                                m_index = - 1;
//                                break;
//                            }
//                            newdp = m_parent.data()->data( CachePosition( m_index, m_dataset ) );
//                        }
//                    }
//                    break;
//                case( PlotterDiagramCompressor::BOTH ):
//                    {
//                        const qreal mergeRadius = m_parent.data()->d->m_mergeRadius;
//                        PlotterDiagramCompressor::DataPoint newdp = m_parent.data()->data( CachePosition( m_index, m_dataset ) );
//                        while ( dp.distance( newdp ) <= mergeRadius )
//                        {
//                            ++m_index;
//                            if ( m_index >= m_parent.data()->rowCount() )
//                            {
//                                m_index = - 1;
//                                break;
//                            }
//                            newdp = m_parent.data()->data( CachePosition( m_index, m_dataset ) );
//                        }
//                    }
//                    break;
//                case ( PlotterDiagramCompressor::SLOPE ):
//                    {
//                        const qreal mergedist = parent->d->m_maxSlopeRadius;
//                        qreal oldSlope = 0;
//                        qreal newSlope = 0;

//                        PlotterDiagramCompressor::DataPoint newdp = m_parent.data()->data( CachePosition( m_index, m_dataset ) );
//                        PlotterDiagramCompressor::DataPoint olddp = PlotterDiagramCompressor::DataPoint();
//                        if ( m_bufferIndex > 1 )
//                        {
//                            oldSlope = calculateSlope( m_buffer[ m_bufferIndex - 2 ], m_buffer[ m_bufferIndex - 1 ] );
//                            newSlope = calculateSlope( m_buffer[ m_bufferIndex - 1 ], newdp );
//                        }
//                        bool first = true;
//                        while ( qAbs( newSlope - oldSlope ) < mergedist )
//                        {
//                            ++m_index;
//                            if ( m_index >= m_parent.data()->rowCount() )
//                            {
//                                m_index = - 1;
//                                break;
//                            }
//                            if ( first )
//                            {
//                                oldSlope = newSlope;
//                                first = false;
//                            }
//                            olddp = newdp;
//                            newdp = m_parent.data()->data( CachePosition( m_index, m_dataset ) );
//                            newSlope = calculateSlope( olddp, newdp );
//                        }
//                    }
//                    break;
//                default:
//                    Q_ASSERT( false );
//                }
//            }
//        }
//    }
//    return *this;
//}

void PlotterDiagramCompressor::Iterator::handleSlopeForward( const DataPoint &dp )
{
    PlotterDiagramCompressor* parent = m_parent.data();
    const qreal mergedist = parent->d->m_maxSlopeRadius;
    qreal oldSlope = 0;
    qreal newSlope = 0;

    PlotterDiagramCompressor::DataPoint newdp = dp;
    PlotterDiagramCompressor::DataPoint olddp = PlotterDiagramCompressor::DataPoint();
    if ( m_bufferIndex > 1 )
    {
        //oldSlope = calculateSlope( m_buffer[ m_bufferIndex - 2 ], m_buffer[ m_bufferIndex - 1 ] );
        //newSlope = calculateSlope( m_buffer[ m_bufferIndex - 1 ], newdp );
        oldSlope = calculateSlope( parent->data( CachePosition( m_index - 2, m_dataset ) ) , parent->data( CachePosition( m_index - 1, m_dataset ) ) );
        newSlope = calculateSlope( parent->data( CachePosition( m_index - 1, m_dataset ) ), newdp );
        qreal accumulatedDist = qAbs( newSlope - oldSlope );
        qreal olddist = accumulatedDist;
        qreal newdist;
        int counter = 0;
        while ( accumulatedDist < mergedist )
        {
            ++m_index;
            if ( m_index >= m_parent.data()->rowCount() )
            {
                m_index = - 1;
                if ( m_buffer.last() != parent->data( CachePosition( parent->rowCount() -1, m_dataset ) ) )
                    m_index = parent->rowCount();
                break;
            }
            oldSlope = newSlope;
            olddp = newdp;
            newdp = parent->data( CachePosition( m_index, m_dataset ) );
            newSlope = calculateSlope( olddp, newdp );
            newdist = qAbs( newSlope - oldSlope );
            if ( olddist == newdist )
            {
                ++counter;
            }
            else
            {
                if ( counter > 10 )
                    break;
            }
            accumulatedDist += newdist;
            olddist = newdist;
        }
        m_buffer.append( newdp );
    }
    else
        m_buffer.append( dp );
}

PlotterDiagramCompressor::Iterator& PlotterDiagramCompressor::Iterator::operator++()
{
    PlotterDiagramCompressor* parent = m_parent.data();
    Q_ASSERT( parent );
    const int count = parent->rowCount();
    //increment the indexes
    ++m_index;
    ++m_bufferIndex;
    //if the index reached the end of the datamodel make this iterator an enditerator
    //and make sure the buffer was not already build, if thats the case its not necessary
    //to rebuild it and it would be hard to extend it as we had to know where m_index was
    if ( m_index >= count || ( !m_rebuffer && m_bufferIndex == m_buffer.count() ) )
    {
        if ( m_bufferIndex == m_buffer.count() )
        {
            if ( m_buffer.last() != parent->data( CachePosition( parent->rowCount() -1, m_dataset ) ) )
                m_index = parent->rowCount();
            else
                m_index = - 1;
            ++m_bufferIndex;
        }
        else
            m_index = -1;
    }
    //if we reached the end of the buffer continue filling the buffer
    if ( m_bufferIndex == m_buffer.count() && m_index >= 0 && m_rebuffer )
    {
        PlotterDiagramCompressor::DataPoint dp = parent->data( CachePosition( m_index, m_dataset ) );
        if ( parent->d->inBoundaries( Qt::Vertical, dp ) && parent->d->inBoundaries( Qt::Horizontal, dp ) )
        {
            if ( parent->d->m_mode == PlotterDiagramCompressor::SLOPE )
                handleSlopeForward( dp );
        }
        else
        {
            m_index = -1;
        }
    }
    return *this;
}

PlotterDiagramCompressor::Iterator PlotterDiagramCompressor::Iterator::operator++( int )
{
    Iterator result = *this;
    ++result;
    return result;
}

PlotterDiagramCompressor::Iterator& PlotterDiagramCompressor::Iterator::operator += ( int value )
{    
    for ( int index = m_index; index + value != m_index; ++( *this ) ) {};
    return *this;
}

PlotterDiagramCompressor::Iterator& PlotterDiagramCompressor::Iterator::operator--()
{    
    --m_index;
    --m_bufferIndex;
    return *this;
}

PlotterDiagramCompressor::Iterator PlotterDiagramCompressor::Iterator::operator--( int )
{
    Iterator result = *this;
    --result;
    return result;
}

PlotterDiagramCompressor::Iterator& PlotterDiagramCompressor::Iterator::operator-=( int value )
{
    m_index -= value;
    return *this;
}

PlotterDiagramCompressor::DataPoint PlotterDiagramCompressor::Iterator::operator*()
{
    if ( !m_parent )
        return PlotterDiagramCompressor::DataPoint();
    Q_ASSERT( m_parent );
    if ( m_index == m_parent.data()->rowCount() )
        return m_parent.data()->data( CachePosition( m_parent.data()->rowCount() - 1 , m_dataset ) );
    return m_buffer[ m_bufferIndex ];
}

bool PlotterDiagramCompressor::Iterator::operator==( const PlotterDiagramCompressor::Iterator &other ) const
{
    return m_parent.data() == other.m_parent.data() && m_index == other.m_index && m_dataset == other.m_dataset;
}

bool PlotterDiagramCompressor::Iterator::operator!=( const PlotterDiagramCompressor::Iterator &other ) const
{
    return ! ( *this == other );
}

void PlotterDiagramCompressor::Iterator::invalidate()
{
    m_dataset = - 1;
}

PlotterDiagramCompressor::Private::Private( PlotterDiagramCompressor *parent )
    : m_parent( parent )
    , m_model( 0 )
    , m_mergeRadius( 0.1 )
    , m_maxSlopeRadius( 0.1 )
    , m_boundary( qMakePair( QPointF( std::numeric_limits<qreal>::quiet_NaN(), std::numeric_limits<qreal>::quiet_NaN() )
                                      , QPointF( std::numeric_limits<qreal>::quiet_NaN(), std::numeric_limits<qreal>::quiet_NaN() ) ) )
    , m_forcedXBoundaries( qMakePair( std::numeric_limits<qreal>::quiet_NaN(), std::numeric_limits<qreal>::quiet_NaN() ) )
    , m_forcedYBoundaries( qMakePair( std::numeric_limits<qreal>::quiet_NaN(), std::numeric_limits<qreal>::quiet_NaN() ) )
    , m_mode( PlotterDiagramCompressor::SLOPE )
{

}

void PlotterDiagramCompressor::Private::setModelToZero()
{
    m_model = 0;
}

inline bool inBoundary( const QPair< qreal, qreal > &bounds, qreal value )
{
    return bounds.first <= value && value <= bounds.second;
}

bool PlotterDiagramCompressor::Private::inBoundaries( Qt::Orientation orient, const PlotterDiagramCompressor::DataPoint &dp ) const
{
    if ( orient == Qt::Vertical && forcedBoundaries( Qt::Vertical ) )
    {
        return inBoundary( m_forcedYBoundaries, dp.value );
    }
    else if ( forcedBoundaries( Qt::Horizontal ) )
    {
        return inBoundary( m_forcedXBoundaries, dp.key );
    }
    return true;
}

//// TODO this is not threadsafe do never try to invoke the painting in a different thread than this
//// method
//void PlotterDiagramCompressor::Private::rowsInserted( const QModelIndex& /*parent*/, int start, int end )
//{

//    if ( m_bufferlist.count() > 0 && !m_bufferlist[ 0 ].isEmpty() && start < m_bufferlist[ 0 ].count() )
//    {
//        calculateDataBoundaries();
//        clearBuffer();
//        return;
//    }
//    // we are handling appends only here, a prepend might be added, insert is expensive if not needed
//    qreal minX = std::numeric_limits< qreal >::max();
//    qreal minY = std::numeric_limits< qreal >::max();
//    qreal maxX = std::numeric_limits< qreal >::min();
//    qreal maxY = std::numeric_limits< qreal >::min();
//    for ( int dataset = 0; dataset < m_bufferlist.size(); ++dataset )
//    {
//        PlotterDiagramCompressor::DataPoint predecessor = m_bufferlist[ dataset ].isEmpty() ? DataPoint() : m_bufferlist[ dataset ].last();

//        qreal oldSlope = 0;
//        qreal newSlope = 0;
//        PlotterDiagramCompressor::DataPoint newdp = m_parent->data( CachePosition( start, dataset ) );
//        PlotterDiagramCompressor::DataPoint olddp = PlotterDiagramCompressor::DataPoint();
//        const int datacount = m_bufferlist[ dataset ].count();
//        if ( m_mode != PlotterDiagramCompressor::DISTANCE && m_bufferlist[ dataset ].count() > 1 )
//        {
//            oldSlope = calculateSlope( m_bufferlist[ dataset ][ datacount - 2 ], m_bufferlist[ dataset ][ datacount - 1 ] );
//            newSlope = calculateSlope( m_bufferlist[ dataset ][ datacount - 1 ], newdp );
//        }
//        bool first = true;
//        for ( int row = start; row <= end; ++row )
//        {
//            PlotterDiagramCompressor::DataPoint curdp = m_parent->data( CachePosition( row, dataset ) );
//            const bool checkcur = inBoundaries( Qt::Vertical, curdp ) && inBoundaries( Qt::Horizontal, curdp );
//            const bool checkpred = inBoundaries( Qt::Vertical, predecessor ) && inBoundaries( Qt::Horizontal, predecessor );
//            const bool check = checkcur || checkpred;
//            switch ( m_mode )
//            {
//            case( PlotterDiagramCompressor::BOTH ):
//                {
//                    if ( predecessor.distance( curdp ) > m_mergeRadius && check )
//                    {
//                        if ( start > m_bufferlist[ dataset ].count() && !m_bufferlist[ dataset ].isEmpty() )
//                        {
//                            m_bufferlist[ dataset ].append( curdp );
//                        }
//                        else if ( !m_bufferlist[ dataset ].isEmpty() )
//                        {
//                            m_bufferlist[ dataset ].insert( row, curdp );
//                        }
//                        predecessor = curdp;
//                        minX = qMin( curdp.key, m_boundary.first.x() );
//                        minY = qMin( curdp.value, m_boundary.first.y() );
//                        maxX = qMax( curdp.key, m_boundary.second.x() );
//                        maxY = qMax( curdp.value, m_boundary.second.y() );
//                    }
//                }
//                break;
//            case ( PlotterDiagramCompressor::DISTANCE ):
//                {
//                    if ( predecessor.distance( curdp ) > m_mergeRadius && check )
//                    {
//                        if ( start > m_bufferlist[ dataset ].count() && !m_bufferlist[ dataset ].isEmpty() )
//                        {
//                            m_bufferlist[ dataset ].append( curdp );
//                        }
//                        else if ( !m_bufferlist[ dataset ].isEmpty() )
//                        {
//                            m_bufferlist[ dataset ].insert( row, curdp );
//                        }
//                        predecessor = curdp;
//                        minX = qMin( curdp.key, m_boundary.first.x() );
//                        minY = qMin( curdp.value, m_boundary.first.y() );
//                        maxX = qMax( curdp.key, m_boundary.second.x() );
//                        maxY = qMax( curdp.value, m_boundary.second.y() );
//                    }
//                }
//                break;
//            case( PlotterDiagramCompressor::SLOPE ):
//                {
//                    if ( check && qAbs( newSlope - oldSlope ) >= m_maxSlopeRadius )
//                    {
//                        if ( start > m_bufferlist[ dataset ].count() && !m_bufferlist[ dataset ].isEmpty() )
//                        {
//                            m_bufferlist[ dataset ].append( curdp );
//                            oldSlope = newSlope;
//                        }
//                        else if ( !m_bufferlist[ dataset ].isEmpty() )
//                        {
//                            m_bufferlist[ dataset ].insert( row, curdp );
//                            oldSlope = newSlope;
//                        }

//                        predecessor = curdp;
//                        minX = qMin( curdp.key, m_boundary.first.x() );
//                        minY = qMin( curdp.value, m_boundary.first.y() );
//                        maxX = qMax( curdp.key, m_boundary.second.x() );
//                        maxY = qMax( curdp.value, m_boundary.second.y() );

//                        if ( first )
//                        {
//                            oldSlope = newSlope;
//                            first = false;
//                        }
//                        olddp = newdp;
//                        newdp = m_parent->data( CachePosition( row, dataset ) );
//                        newSlope = calculateSlope( olddp, newdp );
//                    }
//                }
//                break;
//            }
//        }
//    }
//    setBoundaries( qMakePair( QPointF( minX, minY ), QPointF( maxX, maxY ) ) );
//    emit m_parent->rowCountChanged();
//}
#include <QDebug>
// TODO this is not threadsafe do never try to invoke the painting in a different thread than this
// method
void PlotterDiagramCompressor::Private::rowsInserted( const QModelIndex& /*parent*/, int start, int end )
{

    //Q_ASSERT( std::numeric_limits<qreal>::quiet_NaN() < 5 || std::numeric_limits<qreal>::quiet_NaN() > 5 );
    //Q_ASSERT( 5 == qMin( std::numeric_limits<qreal>::quiet_NaN(),  5.0 ) );
    //Q_ASSERT( 5 == qMax( 5.0, std::numeric_limits<qreal>::quiet_NaN() ) );
    if ( m_bufferlist.count() > 0 && !m_bufferlist[ 0 ].isEmpty() && start < m_bufferlist[ 0 ].count() )
    {
        calculateDataBoundaries();
        clearBuffer();
        return;
    }

    // we are handling appends only here, a prepend might be added, insert is expensive if not needed
    qreal minX = m_boundary.first.x();
    qreal minY = m_boundary.first.y();
    qreal maxX = m_boundary.second.x();
    qreal maxY = m_boundary.second.y();
    for ( int dataset = 0; dataset < m_bufferlist.size(); ++dataset )
    {
        if ( m_mode == PlotterDiagramCompressor::SLOPE )
        {
            PlotterDiagramCompressor::DataPoint predecessor = m_bufferlist[ dataset ].isEmpty() ? DataPoint() : m_bufferlist[ dataset ].last();
            qreal oldSlope = 0;
            qreal newSlope = 0;
            int counter = 0;            

            PlotterDiagramCompressor::DataPoint newdp = m_parent->data( CachePosition( start, dataset ) );
            PlotterDiagramCompressor::DataPoint olddp = PlotterDiagramCompressor::DataPoint();
            if ( start  > 1 )
            {
                oldSlope = calculateSlope( m_parent->data( CachePosition( start - 2, dataset ) ), m_parent->data( CachePosition( start - 1, dataset ) ) );
                olddp = m_parent->data( CachePosition( start - 1, dataset ) );
            }
            else
            {
                m_bufferlist[ dataset ].append( newdp );
                minX = qMin( minX, newdp.key );
                minY = qMin( minY, newdp.value );
                maxX = qMax( newdp.key, maxX );
                maxY = qMax( newdp.value, maxY );
                continue;
            }

            qreal olddist = 0;
            qreal newdist = 0;            
            for ( int row = start; row <= end; ++row )
            {
                PlotterDiagramCompressor::DataPoint curdp = m_parent->data( CachePosition( row, dataset ) );
                newdp = curdp;
                newSlope = calculateSlope( olddp, newdp );
                olddist = newdist;
                newdist = qAbs( newSlope - oldSlope );
                m_accumulatedDistances[ dataset ] += newdist;
                const bool checkcur = inBoundaries( Qt::Vertical, curdp ) && inBoundaries( Qt::Horizontal, curdp );
                const bool checkpred = inBoundaries( Qt::Vertical, predecessor ) && inBoundaries( Qt::Horizontal, predecessor );
                const bool check = checkcur || checkpred;

                if ( m_accumulatedDistances[ dataset ] >= m_maxSlopeRadius && check )
                {
                    if ( start > m_bufferlist[ dataset ].count() && !m_bufferlist[ dataset ].isEmpty() )
                    {
                        m_bufferlist[ dataset ].append( curdp );
                    }
                    else if ( !m_bufferlist[ dataset ].isEmpty() )
                    {
                        m_bufferlist[ dataset ].insert( row, curdp );
                    }
                    predecessor = curdp;
                    m_accumulatedDistances[ dataset ] = 0;
                }
                minX = qMin( minX, curdp.key );
                minY = qMin( minY, curdp.value );
                maxX = qMax( curdp.key, maxX );
                maxY = qMax( curdp.value, maxY );

                oldSlope = newSlope;
                olddp = newdp;
                if ( olddist == newdist )
                {
                    ++counter;
                }
                else
                {
                    if ( counter > 10 )
                    {
                        m_bufferlist[ dataset ].append( curdp );
                        predecessor = curdp;
                        m_accumulatedDistances[ dataset ] = 0;
                    }
                }
            }
            setBoundaries( qMakePair( QPointF( minX, minY ), QPointF( maxX, maxY ) ) );
        }
        else
        {
        PlotterDiagramCompressor::DataPoint predecessor = m_bufferlist[ dataset ].isEmpty() ? DataPoint() : m_bufferlist[ dataset ].last();

        for ( int row = start; row <= end; ++row )
        {
            PlotterDiagramCompressor::DataPoint curdp = m_parent->data( CachePosition( row, dataset ) );
            const bool checkcur = inBoundaries( Qt::Vertical, curdp ) && inBoundaries( Qt::Horizontal, curdp );
            const bool checkpred = inBoundaries( Qt::Vertical, predecessor ) && inBoundaries( Qt::Horizontal, predecessor );
            const bool check = checkcur || checkpred;
            if ( predecessor.distance( curdp ) > m_mergeRadius && check )
            {
                if ( start > m_bufferlist[ dataset ].count() && !m_bufferlist[ dataset ].isEmpty() )
                {
                    m_bufferlist[ dataset ].append( curdp );
                }
                else if ( !m_bufferlist[ dataset ].isEmpty() )
                {
                    m_bufferlist[ dataset ].insert( row, curdp );
                }
                predecessor = curdp;
                qreal minX = qMin( curdp.key, m_boundary.first.x() );
                qreal minY = qMin( curdp.value, m_boundary.first.y() );
                qreal maxX = qMax( curdp.key, m_boundary.second.x() );
                qreal maxY = qMax( curdp.value, m_boundary.second.y() );
                setBoundaries( qMakePair( QPointF( minX, minY ), QPointF( maxX, maxY ) ) );
            }
        }
        }
    }
    emit m_parent->rowCountChanged();
}


void PlotterDiagramCompressor::setCompressionModel( CompressionMode value )
{
    Q_ASSERT( d );
    if ( d->m_mode != value )
    {
        d->m_mode = value;
        d->clearBuffer();
        emit rowCountChanged();
    }
}

void PlotterDiagramCompressor::Private::setBoundaries( const Boundaries & bound )
{
    if ( bound != m_boundary )
    {
        m_boundary = bound;
        emit m_parent->boundariesChanged();
    }
}

void PlotterDiagramCompressor::Private::calculateDataBoundaries()
{
    if ( !forcedBoundaries( Qt::Vertical ) || !forcedBoundaries( Qt::Horizontal ) )
    {
        qreal minX = std::numeric_limits<qreal>::quiet_NaN();
        qreal minY = std::numeric_limits<qreal>::quiet_NaN();
        qreal maxX = std::numeric_limits<qreal>::quiet_NaN();
        qreal maxY = std::numeric_limits<qreal>::quiet_NaN();
        for ( int dataset = 0; dataset < m_parent->datasetCount(); ++dataset )
        {
            for ( int row = 0; row < m_parent->rowCount(); ++ row )
            {
                PlotterDiagramCompressor::DataPoint dp = m_parent->data( CachePosition( row, dataset ) );
                minX = qMin( minX, dp.key );
                minY = qMin( minY, dp.value );
                maxX = qMax( dp.key, maxX );
                maxY = qMax( dp.value, maxY );
                Q_ASSERT( !ISNAN( minX ) );
                Q_ASSERT( !ISNAN( minY ) );
                Q_ASSERT( !ISNAN( maxX ) );
                Q_ASSERT( !ISNAN( maxY ) );
            }
        }
        if ( forcedBoundaries( Qt::Vertical ) )
        {
            minY = m_forcedYBoundaries.first;
            maxY = m_forcedYBoundaries.second;
        }
        if ( forcedBoundaries( Qt::Horizontal ) )
        {
            minX = m_forcedXBoundaries.first;
            maxX = m_forcedXBoundaries.second;
        }
        setBoundaries( qMakePair( QPointF( minX, minY ), QPointF( maxX, maxY ) ) );
    }
}

QModelIndexList PlotterDiagramCompressor::Private::mapToModel( const CachePosition &pos )
{
    QModelIndexList indexes;
    QModelIndex index;
    index = m_model->index( pos.first, pos.second * 2, QModelIndex() );
    Q_ASSERT( index.isValid() );
    indexes << index;
    index = m_model->index( pos.first, pos.second * 2 + 1, QModelIndex() );
    Q_ASSERT( index.isValid() );
    indexes << index;
    return indexes;
}

bool PlotterDiagramCompressor::Private::forcedBoundaries( Qt::Orientation orient ) const
{
    if ( orient == Qt::Vertical )
        return !ISNAN( m_forcedYBoundaries.first ) && !ISNAN( m_forcedYBoundaries.second );
    else
        return !ISNAN( m_forcedXBoundaries.first ) && !ISNAN( m_forcedXBoundaries.second );
}

void PlotterDiagramCompressor::Private::clearBuffer()
{
    //TODO all iterator have to be invalid after this operation
    //TODO make sure there are no regressions, the timeOfLastInvalidation should stop iterators from
    // corrupting the cache
    m_bufferlist.clear();
    m_bufferlist.resize( m_parent->datasetCount() );
    m_accumulatedDistances.clear();
    m_accumulatedDistances.resize( m_parent->datasetCount() );
    m_timeOfLastInvalidation = QDateTime::currentDateTime();
}

PlotterDiagramCompressor::PlotterDiagramCompressor(QObject *parent)
    : QObject(parent)
    , d( new Private( this ) )
{
}

PlotterDiagramCompressor::~PlotterDiagramCompressor()
{
    delete d;
    d = 0;
}

void PlotterDiagramCompressor::setForcedDataBoundaries( const QPair< qreal, qreal > &bounds, Qt::Orientation direction )
{
    if ( direction == Qt::Vertical )
    {
        d->m_forcedYBoundaries = bounds;
    }
    else
    {
        d->m_forcedXBoundaries = bounds;
    }
    d->clearBuffer();
    emit boundariesChanged();
}

QAbstractItemModel* PlotterDiagramCompressor::model() const
{
    Q_ASSERT( d );
    return d->m_model;
}

void PlotterDiagramCompressor::setModel( QAbstractItemModel *model )
{
    Q_ASSERT( d );
    if ( d->m_model )
    {
        d->m_model->disconnect( this );
        d->m_model->disconnect( d );
    }
    d->m_model = model;
    if ( d->m_model)
    {
        d->m_bufferlist.resize( datasetCount() );
        d->m_accumulatedDistances.resize( datasetCount() );
        d->calculateDataBoundaries();
        connect( d->m_model, SIGNAL( rowsInserted ( QModelIndex, int, int ) ), d, SLOT( rowsInserted( QModelIndex, int, int ) ) );
        connect( d->m_model, SIGNAL( modelReset() ), d, SLOT( clearBuffer() ) );
        connect( d->m_model, SIGNAL( destroyed( QObject* ) ), d, SLOT( setModelToZero() ) );
    }
}

PlotterDiagramCompressor::DataPoint PlotterDiagramCompressor::data( const CachePosition& pos ) const
{
    DataPoint point;
    QModelIndexList indexes = d->mapToModel( pos );
    Q_ASSERT( indexes.count() == 2 );
    QVariant yValue = d->m_model->data( indexes.last() );
    QVariant xValue = d->m_model->data( indexes.first() );
    Q_ASSERT( xValue.isValid() );
    Q_ASSERT( yValue.isValid() );
    bool ok = false;
    point.key = xValue.toReal( &ok );
    Q_ASSERT( ok );
    ok = false;
    point.value = yValue.toReal( &ok );
    Q_ASSERT( ok );
    point.index = indexes.first();
    return point;
}

void PlotterDiagramCompressor::setMergeRadius( qreal radius )
{
    if ( d->m_mergeRadius != radius )
    {
        d->m_mergeRadius = radius;
        if ( d->m_mode != PlotterDiagramCompressor::SLOPE )
            emit rowCountChanged();
    }
}

void PlotterDiagramCompressor::setMaxSlopeChange( qreal value )
{
    if ( d->m_maxSlopeRadius != value )
    {
        d->m_maxSlopeRadius = value;
        emit boundariesChanged();
    }
}

qreal PlotterDiagramCompressor::maxSlopeChange() const
{
    return d->m_maxSlopeRadius;
}

void PlotterDiagramCompressor::setMergeRadiusPercentage( qreal radius )
{
    Boundaries bounds = dataBoundaries();
    const qreal width = radius * ( bounds.second.x() - bounds.first.x() );
    const qreal height = radius * ( bounds.second.y() - bounds.first.y() );
    const qreal realRadius = std::sqrt( width * height );
    setMergeRadius( realRadius );
}

int PlotterDiagramCompressor::rowCount() const
{
    return d->m_model ? d->m_model->rowCount() : 0;
}

void PlotterDiagramCompressor::cleanCache()
{
    d->clearBuffer();
}

int PlotterDiagramCompressor::datasetCount() const
{
    if ( d->m_model && d->m_model->columnCount() == 0 )
        return 0;
    return d->m_model ? ( d->m_model->columnCount() + 1 ) / 2  : 0;
}

QPair< QPointF, QPointF > PlotterDiagramCompressor::dataBoundaries() const
{
    Boundaries bounds = d->m_boundary;
    if ( d->forcedBoundaries( Qt::Vertical ) )
    {
        bounds.first.setY( d->m_forcedYBoundaries.first );
        bounds.second.setY( d->m_forcedYBoundaries.second );
    }
    if ( d->forcedBoundaries( Qt::Horizontal ) )
    {
        bounds.first.setX( d->m_forcedXBoundaries.first );
        bounds.second.setX( d->m_forcedXBoundaries.second );
    }
    return bounds;
}

PlotterDiagramCompressor::Iterator PlotterDiagramCompressor::begin( int dataSet )
{
    Q_ASSERT( dataSet >= 0 && dataSet < d->m_bufferlist.count() );
    return Iterator( dataSet, this, d->m_bufferlist[ dataSet ] );
}

PlotterDiagramCompressor::Iterator PlotterDiagramCompressor::end( int dataSet )
{
    Iterator it( dataSet, this );
    it.m_index = -1;
    return it;
}
