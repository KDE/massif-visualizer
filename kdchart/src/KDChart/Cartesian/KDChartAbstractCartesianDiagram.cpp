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

#include "KDChartAbstractCartesianDiagram.h"
#include "KDChartAbstractCartesianDiagram_p.h"

#include <KDABLibFakes>


using namespace KDChart;

AbstractCartesianDiagram::Private::Private()
    : referenceDiagram( 0 )
{
}

AbstractCartesianDiagram::Private::~Private()
{
}

bool AbstractCartesianDiagram::compare( const AbstractCartesianDiagram* other ) const
{
    if ( other == this ) return true;
    if ( ! other ) {
        return false;
    }
    return  // compare the base class
            ( static_cast<const AbstractDiagram*>(this)->compare( other ) ) &&
            // compare own properties
            (referenceDiagram() == other->referenceDiagram()) &&
            ((! referenceDiagram()) || (referenceDiagramOffset() == other->referenceDiagramOffset()));
}


#define d d_func()

AbstractCartesianDiagram::AbstractCartesianDiagram ( QWidget* parent, CartesianCoordinatePlane* plane )
    : AbstractDiagram ( new Private(), parent, plane )
{
    init();
}

KDChart::AbstractCartesianDiagram::~AbstractCartesianDiagram()
{
    Q_FOREACH( CartesianAxis* axis, d->axesList ) {
        axis->deleteObserver( this );
    }
    d->axesList.clear();
}

void AbstractCartesianDiagram::init()
{
    d->compressor.setModel( attributesModel() );
    connect( this, SIGNAL( layoutChanged( AbstractDiagram* ) ),
             &( d->compressor ), SLOT( slotDiagramLayoutChanged( AbstractDiagram* ) ) );
    if ( d->plane )
    {
        const bool res = connect( d->plane, SIGNAL( viewportCoordinateSystemChanged() ),
                                  this, SIGNAL( viewportCoordinateSystemChanged() ) );
        Q_UNUSED( res )
        Q_ASSERT( res );
    }
}

void AbstractCartesianDiagram::addAxis( CartesianAxis *axis )
{
    if ( !d->axesList.contains( axis ) ) {
        d->axesList.append( axis );
        axis->createObserver( this );
        layoutPlanes();
    }
}

void AbstractCartesianDiagram::takeAxis( CartesianAxis *axis )
{
    const int idx = d->axesList.indexOf( axis );
    if ( idx != -1 )
        d->axesList.takeAt( idx );
    axis->deleteObserver( this );
    axis->setParentWidget( 0 );
    layoutPlanes();
}

KDChart::CartesianAxisList AbstractCartesianDiagram::axes( ) const
{
    return d->axesList;
}

void KDChart::AbstractCartesianDiagram::layoutPlanes()
{
    AbstractCoordinatePlane* plane = coordinatePlane();
    if ( plane ) {
        plane->layoutPlanes();
    }
}

void KDChart::AbstractCartesianDiagram::setCoordinatePlane( AbstractCoordinatePlane* plane )
{
    if ( coordinatePlane() ) {
        disconnect( attributesModel(), SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ),
                    coordinatePlane(), SLOT( relayout() ) );
        disconnect( attributesModel(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ),
                    coordinatePlane(), SLOT( relayout() ) );
        disconnect( attributesModel(), SIGNAL( columnsRemoved( const QModelIndex&, int, int ) ),
                    coordinatePlane(), SLOT( relayout() ) );
        disconnect( attributesModel(), SIGNAL( columnsInserted( const QModelIndex&, int, int ) ),
                    coordinatePlane(), SLOT( relayout() ) );
        disconnect( coordinatePlane() );
    }
    
    AbstractDiagram::setCoordinatePlane(plane);
    if ( plane ) {
        // Readjust the layout when the dataset count changes
        connect( attributesModel(), SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ),
                 plane, SLOT( relayout() ), Qt::QueuedConnection );
        connect( attributesModel(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ),
                 plane, SLOT( relayout() ), Qt::QueuedConnection );
        connect( attributesModel(), SIGNAL( columnsRemoved( const QModelIndex&, int, int ) ),
                 plane, SLOT( relayout() ), Qt::QueuedConnection );
        connect( attributesModel(), SIGNAL( columnsInserted( const QModelIndex&, int, int ) ),
                 plane, SLOT( relayout() ), Qt::QueuedConnection );
        Q_ASSERT( plane );
        bool con = connect( plane, SIGNAL( viewportCoordinateSystemChanged() ), this, SIGNAL( viewportCoordinateSystemChanged() ) );
        Q_ASSERT( con );
        con = connect( plane, SIGNAL( viewportCoordinateSystemChanged() ), this, SLOT( update() ) );
        Q_ASSERT( con );
        Q_UNUSED( con );
    }
}

void AbstractCartesianDiagram::setReferenceDiagram( AbstractCartesianDiagram* diagram, const QPointF& offset )
{
    d->referenceDiagram = diagram;
    d->referenceDiagramOffset = offset;
}

AbstractCartesianDiagram* AbstractCartesianDiagram::referenceDiagram() const
{
    return d->referenceDiagram;
}

QPointF AbstractCartesianDiagram::referenceDiagramOffset() const
{
    return d->referenceDiagramOffset;
}

void AbstractCartesianDiagram::setRootIndex( const QModelIndex& index )
{
    AbstractDiagram::setRootIndex( index );
    d->compressor.setRootIndex( attributesModel()->mapFromSource( index ) );
}

void AbstractCartesianDiagram::setModel( QAbstractItemModel* model )
{
    AbstractDiagram::setModel( model );
    d->compressor.setModel( attributesModel() );
}

void AbstractCartesianDiagram::setAttributesModel( AttributesModel* model )
{
    AbstractDiagram::setAttributesModel( model );
    d->compressor.setModel( attributesModel() );
}
