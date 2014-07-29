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

#include "KDChartBarDiagram.h"
#include "KDChartBarDiagram_p.h"

#include "KDChartThreeDBarAttributes.h"
#include "KDChartPosition.h"
#include "KDChartAttributesModel.h"
#include "KDChartAbstractGrid.h"
#include "KDChartPainterSaver_p.h"

#include <QPainter>
#include <QDebug>

#include <KDABLibFakes>

#include "KDChartNormalBarDiagram_p.h"
#include "KDChartStackedBarDiagram_p.h"
#include "KDChartPercentBarDiagram_p.h"
#include "KDChartNormalLyingBarDiagram_p.h"
#include "KDChartStackedLyingBarDiagram_p.h"
#include "KDChartPercentLyingBarDiagram_p.h"


using namespace KDChart;

BarDiagram::Private::Private()
    : orientation( Qt::Vertical )
    , implementor( 0 )
    , normalDiagram( 0 )
    , stackedDiagram( 0 )
    , percentDiagram( 0 )
    , normalLyingDiagram( 0 )
    , stackedLyingDiagram( 0 )
    , percentLyingDiagram( 0 )
{
}

BarDiagram::Private::~Private()
{
    delete normalDiagram;
    delete stackedDiagram;
    delete percentDiagram;
    delete normalLyingDiagram;
    delete stackedLyingDiagram;
    delete percentLyingDiagram;
}

void BarDiagram::Private::setOrientationAndType( Qt::Orientation o, BarDiagram::BarType type )
{
    if ( orientation == o && implementor->type() == type ) {
        return;
    }
    BarDiagram *barDia = qobject_cast< BarDiagram * >( diagram );

    orientation = o;

    if ( orientation == Qt::Vertical ) {
        switch ( type ) {
        case Normal:
            implementor = normalDiagram;
            break;
        case Stacked:
            implementor = stackedDiagram;
            break;
        case Percent:
            implementor = percentDiagram;
            break;
        default:
            Q_ASSERT_X( false, "BarDiagram::setType", "unknown diagram subtype" );
        }
    } else {
        switch ( type ) {
        case Normal:
            implementor = normalLyingDiagram;
            break;
        case Stacked:
            implementor = stackedLyingDiagram;
            break;
        case Percent:
            implementor = percentLyingDiagram;
            break;
        default:
            Q_ASSERT_X( false, "BarDiagram::setType", "unknown diagram subtype" );
        }
    }

    Q_ASSERT( implementor->type() == type );

    // AbstractAxis settings - see AbstractDiagram and CartesianAxis
    barDia->setPercentMode( type == BarDiagram::Percent );
    barDia->setDataBoundariesDirty();
    emit barDia->layoutChanged( barDia );
    emit barDia->propertiesChanged();
}

#define d d_func()


BarDiagram::BarDiagram( QWidget* parent, CartesianCoordinatePlane* plane ) :
    AbstractCartesianDiagram( new Private(), parent, plane )
{
    init();
}

void BarDiagram::init()
{
    d->normalDiagram = new NormalBarDiagram( this );
    d->stackedDiagram = new StackedBarDiagram( this );
    d->percentDiagram = new PercentBarDiagram( this );
    d->normalLyingDiagram = new NormalLyingBarDiagram( this );
    d->stackedLyingDiagram = new StackedLyingBarDiagram( this );
    d->percentLyingDiagram = new PercentLyingBarDiagram( this );
    d->implementor = d->normalDiagram;
    d->compressor.setModel( attributesModel() );
}

BarDiagram::~BarDiagram()
{
}

/**
  * Creates an exact copy of this diagram.
  */
BarDiagram * BarDiagram::clone() const
{

    BarDiagram* newDiagram = new BarDiagram( new Private( *d ) );
    newDiagram->setType( type() );
    return newDiagram;
}

bool BarDiagram::compare( const BarDiagram* other ) const
{
    if ( other == this ) return true;
    if ( ! other ) {
        return false;
    }

    return  // compare the base class
            ( static_cast<const AbstractCartesianDiagram*>(this)->compare( other ) ) &&
            // compare own properties
            (type() == other->type());
}

/**
  * Sets the bar diagram's type to \a type
  * \sa BarDiagram::BarType
  */
void BarDiagram::setType( const BarType type )
{
    d->setOrientationAndType( d->orientation, type );
}

/**
  * @return the type of the bar diagram
  */
BarDiagram::BarType BarDiagram::type() const
{
    return d->implementor->type();
}

/**
  * Sets the orientation of the bar diagram
  */
void BarDiagram::setOrientation( Qt::Orientation orientation )
{
    d->setOrientationAndType( orientation, d->implementor->type() );
}

/**
  * @return the orientation of the bar diagram
  */
Qt::Orientation BarDiagram::orientation() const
{
    return d->orientation;
}

/**
  * Sets the global bar attributes to \a ba
  */
void BarDiagram::setBarAttributes( const BarAttributes& ba )
{
    d->attributesModel->setModelData( qVariantFromValue( ba ), BarAttributesRole );
    emit propertiesChanged();
}

/**
  * Sets the bar attributes of data set \a column to \a ba
  */
void BarDiagram::setBarAttributes( int column, const BarAttributes& ba )
{
    d->setDatasetAttrs( column, qVariantFromValue( ba ), BarAttributesRole );
    emit propertiesChanged();
}

/**
  * Sets the line attributes for the model index \a index to \a ba
  */
void BarDiagram::setBarAttributes( const QModelIndex& index, const BarAttributes& ba )
{
    attributesModel()->setData(
        d->attributesModel->mapFromSource( index ),
        qVariantFromValue( ba ),
        BarAttributesRole );
    emit propertiesChanged();
}

/**
  * @return the global bar attribute set
  */
BarAttributes BarDiagram::barAttributes() const
{
    return d->attributesModel->data( KDChart::BarAttributesRole ).value<BarAttributes>();
}

/**
  * @return the bar attribute set of data set \a column
  */
BarAttributes BarDiagram::barAttributes( int column ) const
{
    const QVariant attrs( d->datasetAttrs( column, KDChart::BarAttributesRole ) );
    if ( attrs.isValid() )
        return attrs.value<BarAttributes>();
    return barAttributes();
}

/**
  * @return the bar attribute set of the model index \a index
  */
BarAttributes BarDiagram::barAttributes( const QModelIndex& index ) const
{
    return d->attributesModel->data(
            d->attributesModel->mapFromSource( index ),
            KDChart::BarAttributesRole ).value<BarAttributes>();
}

/**
  * Sets the global 3D bar attributes to \a threeDAttrs
  */
void BarDiagram::setThreeDBarAttributes( const ThreeDBarAttributes& threeDAttrs )
{
    setDataBoundariesDirty();
    d->attributesModel->setModelData( qVariantFromValue( threeDAttrs ), ThreeDBarAttributesRole );
    emit layoutChanged( this );
    emit propertiesChanged();
}

/**
  * Sets the 3D bar attributes of dataset \a column to \a threeDAttrs
  */
void BarDiagram::setThreeDBarAttributes( int column, const ThreeDBarAttributes& threeDAttrs )
{
    setDataBoundariesDirty();
    d->setDatasetAttrs( column,  qVariantFromValue( threeDAttrs ), ThreeDBarAttributesRole );
    //emit layoutChanged( this );
    emit propertiesChanged();
}

/**
  * Sets the 3D line attributes of model index \a index to \a threeDAttrs
  */
void BarDiagram::setThreeDBarAttributes( const QModelIndex& index, const ThreeDBarAttributes& threeDAttrs )
{
    setDataBoundariesDirty();
    d->attributesModel->setData(
        d->attributesModel->mapFromSource(index),
        qVariantFromValue( threeDAttrs ),
        ThreeDBarAttributesRole );
    //emit layoutChanged( this );
    emit propertiesChanged();
}

/**
  * @return the global 3D bar attributes
  */
ThreeDBarAttributes BarDiagram::threeDBarAttributes() const
{
    return d->attributesModel->data( KDChart::ThreeDBarAttributesRole ).value<ThreeDBarAttributes>();
}

/**
  * @return the 3D bar attributes of data set \a column
  */
ThreeDBarAttributes BarDiagram::threeDBarAttributes( int column ) const
{
    const QVariant attrs( d->datasetAttrs( column, KDChart::ThreeDBarAttributesRole ) );
    if ( attrs.isValid() )
        return attrs.value<ThreeDBarAttributes>();
    return threeDBarAttributes();
}

/**
  * @return the 3D bar attributes of the model index \a index
  */
ThreeDBarAttributes BarDiagram::threeDBarAttributes( const QModelIndex& index ) const
{
    return d->attributesModel->data(
            d->attributesModel->mapFromSource(index),
            KDChart::ThreeDBarAttributesRole ).value<ThreeDBarAttributes>();
}

qreal BarDiagram::threeDItemDepth( const QModelIndex& index ) const
{
    return threeDBarAttributes( index ).validDepth();
}

qreal BarDiagram::threeDItemDepth( int column ) const
{
    return threeDBarAttributes( column ).validDepth();
}

void BarDiagram::resizeEvent ( QResizeEvent*)
{

}

const QPair<QPointF, QPointF> BarDiagram::calculateDataBoundaries() const
{
    d->compressor.setResolution( static_cast<int>( this->size().width() * coordinatePlane()->zoomFactorX() ),
                                 static_cast<int>( this->size().height() * coordinatePlane()->zoomFactorY() ) );

    if ( !checkInvariants( true ) ) {
        return QPair< QPointF, QPointF >( QPointF( 0, 0 ), QPointF( 0, 0 ) );
    }

    // note: calculateDataBoundaries() is ignoring the hidden flags.
    // That's not a bug but a feature: Hiding data does not mean removing them.
    // For totally removing data from KD Chart's view people can use e.g. a proxy model
    // calculate boundaries for different line types Normal - Stacked - Percent - Default Normal
    return d->implementor->calculateDataBoundaries();
}

void BarDiagram::paintEvent ( QPaintEvent*)
{
    QPainter painter ( viewport() );
    PaintContext ctx;
    ctx.setPainter ( &painter );
    ctx.setRectangle( QRectF ( 0, 0, width(), height() ) );
    paint ( &ctx );
}

void BarDiagram::paint( PaintContext* ctx )
{
    if ( !checkInvariants( true ) ) return;
    if ( !AbstractGrid::isBoundariesValid(dataBoundaries()) ) return;
    const PainterSaver p( ctx->painter() );
    if ( model()->rowCount( rootIndex() ) == 0 || model()->columnCount( rootIndex() ) == 0 )
        return; // nothing to paint for us

    AbstractCoordinatePlane* const plane = ctx->coordinatePlane();
    ctx->setCoordinatePlane( plane->sharedAxisMasterPlane( ctx->painter() ) );

    // This was intended as a fix for KDCH-515, however it caused KDCH-816
    // and the original problem in KDCH-515 had by then been fixed in another way.
    // Bottom line is, this code is wrong because the above call to
    // plane->sharedAxisMasterPlane() performs a translation of the painter, which
    // also translates the clip rect, so if we set the old clip rect again afterwards,
    // we get a wrong clipping.
    // Also, this code is unnecessary because CartesianCoordinatePlane::paint()
    // already sets the clipping properly before calling this method.
    // ctx->painter()->setClipping( true );
    // ctx->painter()->setClipRect( ctx->rectangle() );

    // paint different bar types Normal - Stacked - Percent - Default Normal
    d->implementor->paint( ctx );

    ctx->setCoordinatePlane( plane );
}

void BarDiagram::resize( const QSizeF& size )
{
    d->compressor.setResolution( static_cast< int >( size.width() * coordinatePlane()->zoomFactorX() ),
                                 static_cast< int >( size.height() * coordinatePlane()->zoomFactorY() ) );
    setDataBoundariesDirty();
    QAbstractItemView::resize( size.toSize() );
}

#if QT_VERSION < 0x040400 || defined(Q_COMPILER_MANGLES_RETURN_TYPE)
const
#endif
int BarDiagram::numberOfAbscissaSegments () const
{
    return d->attributesModel->rowCount(attributesModelRootIndex());
}

#if QT_VERSION < 0x040400 || defined(Q_COMPILER_MANGLES_RETURN_TYPE)
const
#endif
int BarDiagram::numberOfOrdinateSegments () const
{
    return d->attributesModel->columnCount(attributesModelRootIndex());
}

//#undef d
