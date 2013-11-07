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

#include "KDChartPlotter.h"
#include "KDChartPlotter_p.h"

#include "KDChartAbstractGrid.h"
#include "KDChartPainterSaver_p.h"

#include <KDABLibFakes>

#include "KDChartNormalPlotter_p.h"
#include "KDChartPercentPlotter_p.h"

using namespace KDChart;

Plotter::Private::Private()
    : implementor( 0 )
    , normalPlotter( 0 )
    , percentPlotter( 0 )
{
}

Plotter::Private::~Private()
{
    delete normalPlotter;
    delete percentPlotter;
}


#define d d_func()


Plotter::Plotter( QWidget* parent, CartesianCoordinatePlane* plane ) :
    AbstractCartesianDiagram( new Private(), parent, plane )
{
    init();
}

void Plotter::init()
{
    d->diagram = this;
    d->normalPlotter = new NormalPlotter( this );
    d->percentPlotter = new PercentPlotter( this );
    d->implementor = d->normalPlotter;
    QObject* test = d->implementor->plotterPrivate();
    bool connection = connect( this, SIGNAL( boundariesChanged() ), test, SLOT( changedProperties() ) );
    Q_ASSERT( connection );
    Q_UNUSED( connection );

    setDatasetDimensionInternal( 2 );    
}

Plotter::~Plotter()
{
}

/**
  * Creates an exact copy of this diagram.
  */
Plotter* Plotter::clone() const
{
    Plotter* newDiagram = new Plotter( new Private( *d ) );
    newDiagram->setType( type() );
    return newDiagram;
}

bool Plotter::compare( const Plotter* other ) const
{
    if ( other == this )
        return true;
    if ( other == 0 )
        return false;
    return  // compare the base class
            ( static_cast< const AbstractCartesianDiagram* >( this )->compare( other ) ) &&
            // compare own properties
            ( type() == other->type() );
}

void Plotter::setModel( QAbstractItemModel *model )
{
    d->plotterCompressor.setModel( NULL );
    AbstractCartesianDiagram::setModel( model );
    if ( useDataCompression() != Plotter::NONE )
    {
        d->compressor.setModel( NULL );
        if ( attributesModel() != d->plotterCompressor.model() )
        {
            d->plotterCompressor.setModel( attributesModel() );
            connect( &d->plotterCompressor, SIGNAL( boundariesChanged() ), this, SLOT(setDataBoundariesDirty() ) );
            if ( useDataCompression() != Plotter::SLOPE )
            {
                connect( coordinatePlane(), SIGNAL( internal_geometryChanged( QRect,QRect ) ), this, SLOT( setDataBoundariesDirty() ), Qt::QueuedConnection );
                connect( coordinatePlane(), SIGNAL( geometryChanged( QRect,QRect ) ), this, SLOT( setDataBoundariesDirty() ), Qt::QueuedConnection );
                calcMergeRadius();
            }
        }
    }
}

Plotter::CompressionMode Plotter::useDataCompression() const
{    
    return d->implementor->useCompression();
}

void Plotter::setUseDataCompression( Plotter::CompressionMode value )
{
    if ( useDataCompression() != value )
    {
        d->implementor->setUseCompression( value );
        if ( useDataCompression() != Plotter::NONE )
        {
            d->compressor.setModel( NULL );
            if ( attributesModel() != d->plotterCompressor.model() )                
                d->plotterCompressor.setModel( attributesModel() );
        }
    }
}

qreal Plotter::maxSlopeChange() const
{
    return d->plotterCompressor.maxSlopeChange();
}

void Plotter::setMaxSlopeChange( qreal value )
{
    d->plotterCompressor.setMaxSlopeChange( value );
}

qreal Plotter::mergeRadiusPercentage() const
{
    return d->mergeRadiusPercentage;
}

void Plotter::setMergeRadiusPercentage( qreal value )
{
    if ( d->mergeRadiusPercentage != value )
    {
        d->mergeRadiusPercentage = value;
        //d->plotterCompressor.setMergeRadiusPercentage( value );
        //update();
    }
}

/**
  * Sets the plotter's type to \a type
  */
void Plotter::setType( const PlotType type )
{
    if ( d->implementor->type() == type ) {
        return;
    }
    if ( datasetDimension() != 2 )  {
        Q_ASSERT_X ( false, "setType()",
                     "This line chart type can only be used with two-dimensional data." );
        return;
    }
    switch ( type ) {
    case Normal:
        d->implementor = d->normalPlotter;
        break;
    case Percent:
        d->implementor = d->percentPlotter;
        break;
    default:
        Q_ASSERT_X( false, "Plotter::setType", "unknown plotter subtype" );
    }
    bool connection = connect( this, SIGNAL( boundariesChanged() ),
                               d->implementor->plotterPrivate(), SLOT( changedProperties() ) );
    Q_ASSERT( connection );
    Q_UNUSED( connection );

    // d->lineType = type;
    Q_ASSERT( d->implementor->type() == type );

    setDataBoundariesDirty();
    emit layoutChanged( this );
    emit propertiesChanged();
}

/**
  * @return the type of the plotter
  */
Plotter::PlotType Plotter::type() const
{
    return d->implementor->type();
}

/**
  * Sets the global line attributes to \a la
  */
void Plotter::setLineAttributes( const LineAttributes& la )
{
    d->attributesModel->setModelData( qVariantFromValue( la ), LineAttributesRole );
    emit propertiesChanged();
}

/**
  * Sets the line attributes of data set \a column to \a la
  */
void Plotter::setLineAttributes( int column, const LineAttributes& la )
{
    d->setDatasetAttrs( column, qVariantFromValue( la ), LineAttributesRole );
    emit propertiesChanged();
}

/**
  * Resets the line attributes of data set \a column
  */
void Plotter::resetLineAttributes( int column )
{
    d->resetDatasetAttrs( column, LineAttributesRole );
    emit propertiesChanged();
}

/**
  * Sets the line attributes for the model index \a index to \a la
  */
void Plotter::setLineAttributes( const QModelIndex & index, const LineAttributes& la )
{
    d->attributesModel->setData( d->attributesModel->mapFromSource( index ),
                                 qVariantFromValue( la ), LineAttributesRole );
    emit propertiesChanged();
}

/**
 * Remove any explicit line attributes settings that might have been specified before.
 */
void Plotter::resetLineAttributes( const QModelIndex & index )
{
    d->attributesModel->resetData(
            d->attributesModel->mapFromSource(index), LineAttributesRole );
    emit propertiesChanged();
}

/**
  * @return the global line attribute set
  */
LineAttributes Plotter::lineAttributes() const
{
    return d->attributesModel->data( KDChart::LineAttributesRole ).value<LineAttributes>();
}

/**
  * @return the line attribute set of data set \a column
  */
LineAttributes Plotter::lineAttributes( int column ) const
{
    const QVariant attrs( d->datasetAttrs( column, LineAttributesRole ) );
    if ( attrs.isValid() )
        return attrs.value<LineAttributes>();
    return lineAttributes();
}

/**
  * @return the line attribute set of the model index \a index
  */
LineAttributes Plotter::lineAttributes( const QModelIndex& index ) const
{
    return d->attributesModel->data(
        d->attributesModel->mapFromSource( index ), KDChart::LineAttributesRole ).value<LineAttributes>();
}

/**
  * Sets the global 3D line attributes to \a la
  */
void Plotter::setThreeDLineAttributes( const ThreeDLineAttributes& la )
{
    setDataBoundariesDirty();
    d->attributesModel->setModelData( qVariantFromValue( la ), ThreeDLineAttributesRole );
    emit propertiesChanged();
}

/**
  * Sets the 3D line attributes of data set \a column to \a la
  */
void Plotter::setThreeDLineAttributes( int column, const ThreeDLineAttributes& la )
{
    setDataBoundariesDirty();
    d->setDatasetAttrs( column, qVariantFromValue( la ), ThreeDLineAttributesRole );
    emit propertiesChanged();
}

/**
  * Sets the 3D line attributes of model index \a index to \a la
  */
void Plotter::setThreeDLineAttributes( const QModelIndex& index, const ThreeDLineAttributes& la )
{
    setDataBoundariesDirty();
    d->attributesModel->setData( d->attributesModel->mapFromSource( index ), qVariantFromValue( la ),
                                 ThreeDLineAttributesRole );
    emit propertiesChanged();
}

/**
  * @return the global 3D line attributes
  */
ThreeDLineAttributes Plotter::threeDLineAttributes() const
{
    return d->attributesModel->data( KDChart::ThreeDLineAttributesRole ).value<ThreeDLineAttributes>();
}

/**
  * @return the 3D line attributes of data set \a column
  */
ThreeDLineAttributes Plotter::threeDLineAttributes( int column ) const
{
    const QVariant attrs( d->datasetAttrs( column, ThreeDLineAttributesRole ) );
    if ( attrs.isValid() ) {
        return attrs.value<ThreeDLineAttributes>();
    }
    return threeDLineAttributes();
}

/**
  * @return the 3D line attributes of the model index \a index
  */
ThreeDLineAttributes Plotter::threeDLineAttributes( const QModelIndex& index ) const
{
    return d->attributesModel->data(
        d->attributesModel->mapFromSource( index ), KDChart::ThreeDLineAttributesRole ).value<ThreeDLineAttributes>();
}

qreal Plotter::threeDItemDepth( const QModelIndex & index ) const
{
    return threeDLineAttributes( index ).validDepth();
}

qreal Plotter::threeDItemDepth( int column ) const
{
    return threeDLineAttributes( column ).validDepth();
}

/**
  * Sets the value tracker attributes of the model index \a index to \a va
  */
void Plotter::setValueTrackerAttributes( const QModelIndex & index, const ValueTrackerAttributes & va )
{
    d->attributesModel->setData( d->attributesModel->mapFromSource( index ),
                                 qVariantFromValue( va ), KDChart::ValueTrackerAttributesRole );
    emit propertiesChanged();
}

/**
  * Returns the value tracker attributes of the model index \a index
  */
ValueTrackerAttributes Plotter::valueTrackerAttributes( const QModelIndex & index ) const
{
    return d->attributesModel->data(
        d->attributesModel->mapFromSource( index ), KDChart::ValueTrackerAttributesRole ).value<ValueTrackerAttributes>();
}

void Plotter::resizeEvent ( QResizeEvent* )
{
}

const QPair< QPointF, QPointF > Plotter::calculateDataBoundaries() const
{
    if ( !checkInvariants( true ) )
        return QPair< QPointF, QPointF >( QPointF( 0, 0 ), QPointF( 0, 0 ) );

    // note: calculateDataBoundaries() is ignoring the hidden flags.
    //       That's not a bug but a feature: Hiding data does not mean removing them.
    // For totally removing data from KD Chart's view people can use e.g. a proxy model ...

    // calculate boundaries for different line types Normal - Stacked - Percent - Default Normal
    return d->implementor->calculateDataBoundaries();
}


void Plotter::paintEvent ( QPaintEvent*)
{
    QPainter painter ( viewport() );
    PaintContext ctx;
    ctx.setPainter ( &painter );
    ctx.setRectangle ( QRectF ( 0, 0, width(), height() ) );
    paint ( &ctx );
}

void Plotter::paint( PaintContext* ctx )
{
    // note: Not having any data model assigned is no bug
    //       but we can not draw a diagram then either.
    if ( !checkInvariants( true ) ) return;

    AbstractCoordinatePlane* const plane = ctx->coordinatePlane();
    if ( ! plane ) return;
    d->setCompressorResolution( size(), plane );

    if ( !AbstractGrid::isBoundariesValid(dataBoundaries()) ) return;

    const PainterSaver p( ctx->painter() );
    if ( model()->rowCount( rootIndex() ) == 0 || model()->columnCount( rootIndex() ) == 0 )
        return; // nothing to paint for us

    ctx->setCoordinatePlane( plane->sharedAxisMasterPlane( ctx->painter() ) );

    // paint different line types Normal - Stacked - Percent - Default Normal
    d->implementor->paint( ctx );

    ctx->setCoordinatePlane( plane );
}

void Plotter::resize ( const QSizeF& size )
{
    d->setCompressorResolution( size, coordinatePlane() );
    if ( useDataCompression() == Plotter::BOTH || useDataCompression() == Plotter::DISTANCE )
    {
        d->plotterCompressor.cleanCache();
        calcMergeRadius();
    }
    setDataBoundariesDirty();
    QAbstractItemView::resize( size.toSize() );
}

void Plotter::setDataBoundariesDirty()
{
    AbstractCartesianDiagram::setDataBoundariesDirty();
    if ( useDataCompression() == Plotter::DISTANCE || useDataCompression() == Plotter::BOTH )
    {
        calcMergeRadius();
        //d->plotterCompressor.setMergeRadiusPercentage( d->mergeRadiusPercentage );
    }
}

void Plotter::calcMergeRadius()
{
    CartesianCoordinatePlane *plane = dynamic_cast< CartesianCoordinatePlane* >( coordinatePlane() );
    Q_ASSERT( plane );
    //Q_ASSERT( plane->translate( plane->translateBack( plane->visibleDiagramArea().topLeft() ) ) == plane->visibleDiagramArea().topLeft() );
    QRectF range = plane->visibleDataRange();
    //qDebug() << range;
    const qreal radius = std::sqrt( ( range.x() + range.width() ) * ( range.y() +  range.height() ) );
    //qDebug() << radius;
    //qDebug() << radius * d->mergeRadiusPercentage;
    //qDebug() << d->mergeRadiusPercentage;
    d->plotterCompressor.setMergeRadius( radius * d->mergeRadiusPercentage );
}

#if QT_VERSION < 0x040400 || defined(Q_COMPILER_MANGLES_RETURN_TYPE)
const
#endif
int Plotter::numberOfAbscissaSegments () const
{
    return d->attributesModel->rowCount( attributesModelRootIndex() );
}

#if QT_VERSION < 0x040400 || defined(Q_COMPILER_MANGLES_RETURN_TYPE)
const
#endif
int Plotter::numberOfOrdinateSegments () const
{
    return d->attributesModel->columnCount( attributesModelRootIndex() );
}
