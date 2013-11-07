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

#include "KDChartAbstractDiagram_p.h"

#include "KDChartBarDiagram.h"
#include "KDChartFrameAttributes.h"
#include "KDChartPainterSaver_p.h"

#include <QAbstractTextDocumentLayout>
#include <QTextBlock>
#include <QApplication>

#include <KDABLibFakes>


using namespace KDChart;

LabelPaintInfo::LabelPaintInfo()
{
}

LabelPaintInfo::LabelPaintInfo( const QModelIndex& _index, const DataValueAttributes& _attrs,
                                const QPainterPath& _labelArea, const QPointF& _markerPos,
                                bool _isValuePositive, const QString& _value )
    : index( _index )
    , attrs( _attrs )
    , labelArea( _labelArea )
    , markerPos( _markerPos )
    , isValuePositive( _isValuePositive )
    , value( _value )
{
}

LabelPaintInfo::LabelPaintInfo( const LabelPaintInfo& other )
    : index( other.index )
    , attrs( other.attrs )
    , labelArea( other.labelArea )
    , markerPos( other.markerPos )
    , isValuePositive( other.isValuePositive )
    , value( other.value )
{
}

AbstractDiagram::Private::Private()
  : diagram( 0 )
  , doDumpPaintTime( false )
  , plane( 0 )
  , attributesModel( new PrivateAttributesModel(0,0) )
  , allowOverlappingDataValueTexts( false )
  , antiAliasing( true )
  , percent( false )
  , datasetDimension( 1 )
  , databoundariesDirty( true )
  , mCachedFontMetrics( QFontMetrics( qApp->font() ) )
{
}

AbstractDiagram::Private::~Private()
{
  if ( attributesModel && qobject_cast<PrivateAttributesModel*>(attributesModel) )
    delete attributesModel;
}

void AbstractDiagram::Private::init()
{
}

void AbstractDiagram::Private::init( AbstractCoordinatePlane* newPlane )
{
    plane = newPlane;
}

bool AbstractDiagram::Private::usesExternalAttributesModel() const
{
    return ( ! attributesModel.isNull() ) &&
           ( ! qobject_cast<PrivateAttributesModel*>(attributesModel) );
}

void AbstractDiagram::Private::setAttributesModel( AttributesModel* amodel )
{
    if ( !attributesModel.isNull() &&
        qobject_cast<PrivateAttributesModel*>(attributesModel) ) {
        delete attributesModel;
    }
    attributesModel = amodel;
}

AbstractDiagram::Private::Private( const AbstractDiagram::Private& rhs ) :
    diagram( 0 ),
    doDumpPaintTime( rhs.doDumpPaintTime ),
    // Do not copy the plane
    plane( 0 ),
    attributesModelRootIndex( QModelIndex() ),
    attributesModel( rhs.attributesModel ),
    allowOverlappingDataValueTexts( rhs.allowOverlappingDataValueTexts ),
    antiAliasing( rhs.antiAliasing ),
    percent( rhs.percent ),
    datasetDimension( rhs.datasetDimension ),
    mCachedFontMetrics( rhs.cachedFontMetrics() )
{
    attributesModel = new PrivateAttributesModel( 0, 0);
    attributesModel->initFrom( rhs.attributesModel );
}

// FIXME: Optimize if necessary
qreal AbstractDiagram::Private::calcPercentValue( const QModelIndex & index ) const
{
    qreal sum = 0.0;
    for ( int col = 0; col < attributesModel->columnCount( QModelIndex() ); col++ )
        sum += attributesModel->data( attributesModel->index( index.row(), col, QModelIndex() ) ).toReal(); // checked
    if ( sum == 0.0 )
        return 0.0;
    return attributesModel->data( attributesModel->mapFromSource( index ) ).toReal() / sum * 100.0;
}

void AbstractDiagram::Private::addLabel(
    LabelPaintCache* cache,
    const QModelIndex& index,
    const CartesianDiagramDataCompressor::CachePosition* position,
    const PositionPoints& points,
    const Position& autoPositionPositive, const Position& autoPositionNegative,
    const qreal value, qreal favoriteAngle /* = 0.0 */ )
{
    CartesianDiagramDataCompressor::AggregatedDataValueAttributes allAttrs(
        aggregatedAttrs( index, position ) );

    QMap<QModelIndex, DataValueAttributes>::const_iterator it;
    for ( it = allAttrs.constBegin(); it != allAttrs.constEnd(); ++it ) {
        DataValueAttributes dva = it.value();
        if ( !dva.isVisible() ) {
            continue;
        }

        const bool isPositive = ( value >= 0.0 );

        RelativePosition relPos( dva.position( isPositive ) );
        relPos.setReferencePoints( points );
        if ( relPos.referencePosition().isUnknown() ) {
            relPos.setReferencePosition( isPositive ? autoPositionPositive : autoPositionNegative );
        }

        // Rotate the label position (not the label itself) if the diagram is rotated so that the defaults still work
        if ( isTransposed() ) {
            KDChartEnums::PositionValue posValue = relPos.referencePosition().value();
            if ( posValue >= KDChartEnums::PositionNorthWest && posValue <= KDChartEnums::PositionWest ) {
                // rotate 90 degrees clockwise
                posValue = static_cast< KDChartEnums::PositionValue >( posValue + 2 );
                if ( posValue > KDChartEnums::PositionWest ) {
                    // wraparound
                    posValue = static_cast< KDChartEnums::PositionValue >( posValue -
                                ( KDChartEnums::PositionWest - KDChartEnums::PositionNorthWest ) );
                }
                relPos.setReferencePosition( Position( posValue ) );
            }
        }

        const QPointF referencePoint = relPos.referencePoint();
        if ( !diagram->coordinatePlane()->isVisiblePoint( referencePoint ) ) {
            continue;
        }

        const qreal fontHeight = cachedFontMetrics( dva.textAttributes().
                calculatedFont( plane, KDChartEnums::MeasureOrientationMinimum ), diagram )->height();

        // Note: When printing data value texts and padding's Measure is using automatic reference area
        //       detection, the font height is used as reference size for both horizontal and vertical
        //       padding.
        QSizeF relativeMeasureSize( fontHeight, fontHeight );

        if ( !dva.textAttributes().hasRotation() ) {
            TextAttributes ta = dva.textAttributes();
            ta.setRotation( favoriteAngle );
            dva.setTextAttributes( ta );
        }

        // get the size of the label text using a subset of the information going into the final layout
        const QString text = formatDataValueText( dva, index, value );
        QTextDocument doc;
        doc.setDocumentMargin( 0 );
        if ( Qt::mightBeRichText( text ) ) {
            doc.setHtml( text );
        } else {
            doc.setPlainText( text );
        }
        const QFont calculatedFont( dva.textAttributes()
                                    .calculatedFont( plane, KDChartEnums::MeasureOrientationMinimum ) );
        doc.setDefaultFont( calculatedFont );

        const QRectF plainRect = doc.documentLayout()->frameBoundingRect( doc.rootFrame() );

        /**
        * A few hints on how the positioning of the text frame is done:
        *
        * Let's assume we have a bar chart, a text for a positive value
        * to be drawn, and "North" as attrs.positivePosition().
        *
        * The reference point (pos) is then set to the top center point
        * of a bar. The offset now depends on the alignment:
        *
        *    Top: text is centered horizontally to the bar, bottom of
        *         text frame starts at top of bar
        *
        *    Bottom: text is centered horizontally to the bar, top of
        *            text frame starts at top of bar
        *
        *    Center: text is centered horizontally to the bar, center
        *            line of text frame is same as top of bar
        *
        *    TopLeft: right edge of text frame is horizontal center of
        *             bar, bottom of text frame is top of bar.
        *
        *    ...
        *
        * Positive and negative value labels are treated equally, "North"
        * also refers to the top of a negative bar, and *not* to the bottom.
        *
        *
        * "NorthEast" likewise refers to the top right edge of the bar,
        * "NorthWest" to the top left edge of the bar, and so on.
        *
        * In other words, attrs.positivePosition() always refers to a
        * position of the *bar*, and relPos.alignment() always refers
        * to an alignment of the text frame relative to this position.
        */

        QTransform transform;
        {
            // move to the general area where the label should be
            QPointF calcPoint = relPos.calculatedPoint( relativeMeasureSize );
            transform.translate( calcPoint.x(), calcPoint.y() );
            // align the text rect; find out by how many half-widths / half-heights to move.
            int dx = -1;
            if ( relPos.alignment() & Qt::AlignLeft ) {
                dx -= 1;
            } else if ( relPos.alignment() & Qt::AlignRight ) {
                 dx += 1;
            }

            int dy = -1;
            if ( relPos.alignment() & Qt::AlignTop ) {
                dy -= 1;
            } else if ( relPos.alignment() & Qt::AlignBottom ) {
                dy += 1;
            }
            transform.translate( qreal( dx ) * plainRect.width() * 0.5,
                                 qreal( dy ) * plainRect.height() * 0.5 );

            // rotate the text rect around its center
            transform.translate( plainRect.center().x(), plainRect.center().y() );
            int rotation = dva.textAttributes().rotation();
            if ( !isPositive && dva.mirrorNegativeValueTextRotation() ) {
                rotation *= -1;
            }
            transform.rotate( rotation );
            transform.translate( -plainRect.center().x(), -plainRect.center().y() );
        }

        QPainterPath labelArea;
        labelArea.addPolygon( transform.mapToPolygon( plainRect.toRect() ) );
        labelArea.closeSubpath();

        // store the label geometry and auxiliary data
        cache->paintReplay.append( LabelPaintInfo( it.key(), dva, labelArea,
                                                   referencePoint, value >= 0.0, text ) );
    }
}

const QFontMetrics* AbstractDiagram::Private::cachedFontMetrics( const QFont& font,
                                                                 const QPaintDevice* paintDevice) const
{
    if ( ( font != mCachedFont ) || ( paintDevice != mCachedPaintDevice ) ) {
        mCachedFontMetrics = QFontMetrics( font, const_cast<QPaintDevice *>( paintDevice ) );
        // TODO what about setting mCachedFont and mCachedPaintDevice?
    }
    return &mCachedFontMetrics;
}

const QFontMetrics AbstractDiagram::Private::cachedFontMetrics() const
{
    return mCachedFontMetrics;
}

QString AbstractDiagram::Private::formatNumber( qreal value, int decimalDigits ) const
{
    const int digits = qMax(decimalDigits, 0);
    const qreal roundingEpsilon = pow( 0.1, digits ) * ( value >= 0.0 ? 0.5 : -0.5 );
    QString asString = QString::number( value + roundingEpsilon, 'f' );
    const int decimalPos = asString.indexOf( QLatin1Char( '.' ) );
    if ( decimalPos < 0 ) {
        return asString;
    }

    int last = qMin( decimalPos + digits, asString.length() - 1 );
    // remove trailing zeros (and maybe decimal dot)
    while ( last > decimalPos && asString[ last ] == QLatin1Char( '0' ) ) {
        last--;
    }
    if ( last == decimalPos ) {
         last--;
    }
    asString.chop( asString.length() - last - 1 );
    return asString;
}

void AbstractDiagram::Private::forgetAlreadyPaintedDataValues()
{
    alreadyDrawnDataValueTexts.clear();
    prevPaintedDataValueText.clear();
}

void AbstractDiagram::Private::paintDataValueTextsAndMarkers(
    PaintContext* ctx,
    const LabelPaintCache &cache,
    bool paintMarkers,
    bool justCalculateRect /* = false */,
    QRectF* cumulatedBoundingRect /* = 0 */ )
{
    if ( justCalculateRect && !cumulatedBoundingRect ) {
        qWarning() << Q_FUNC_INFO << "Neither painting nor finding the bounding rect, what are we doing?";
    }

    const PainterSaver painterSaver( ctx->painter() );
    ctx->painter()->setClipping( false );

    if ( paintMarkers && !justCalculateRect ) {
        KDAB_FOREACH ( const LabelPaintInfo& info, cache.paintReplay ) {
            diagram->paintMarker( ctx->painter(), info.index, info.markerPos );
        }
    }

    TextAttributes ta;
    {
        Measure m( 18.0, KDChartEnums::MeasureCalculationModeRelative,
                   KDChartEnums::MeasureOrientationMinimum );
        m.setReferenceArea( ctx->coordinatePlane() );
        ta.setFontSize( m );
        m.setAbsoluteValue( 6.0 );
        ta.setMinimalFontSize( m );
    }

    forgetAlreadyPaintedDataValues();

    KDAB_FOREACH ( const LabelPaintInfo& info, cache.paintReplay ) {
        const QPointF pos = info.labelArea.elementAt( 0 );
        paintDataValueText( ctx->painter(), info.attrs, pos, info.isValuePositive,
                            info.value, justCalculateRect, cumulatedBoundingRect );

        const QString comment = info.index.data( KDChart::CommentRole ).toString();
        if ( comment.isEmpty() ) {
            continue;
        }
        TextBubbleLayoutItem item( comment, ta, ctx->coordinatePlane()->parent(),
                                   KDChartEnums::MeasureOrientationMinimum,
                                   Qt::AlignHCenter | Qt::AlignVCenter );
        const QRect rect( pos.toPoint(), item.sizeHint() );

        if (cumulatedBoundingRect) {
            (*cumulatedBoundingRect) |= rect;
        }
        if ( !justCalculateRect ) {
            item.setGeometry( rect );
            item.paint( ctx->painter() );
        }
    }
}

QString AbstractDiagram::Private::formatDataValueText( const DataValueAttributes &dva,
                                                       const QModelIndex& index, qreal value ) const
{
    if ( !dva.isVisible() ) {
        return QString();
    }
    if ( dva.usePercentage() ) {
        value = calcPercentValue( index );
    }

    QString ret;
    if ( dva.dataLabel().isNull() ) {
        ret = formatNumber( value, dva.decimalDigits() );
    } else {
        ret = dva.dataLabel();
    }

    ret.prepend( dva.prefix() );
    ret.append( dva.suffix() );

    return ret;
}

void AbstractDiagram::Private::paintDataValueText(
    QPainter* painter,
    const QModelIndex& index,
    const QPointF& pos,
    qreal value,
    bool justCalculateRect /* = false */,
    QRectF* cumulatedBoundingRect /* = 0 */ )
{
    const DataValueAttributes dva( diagram->dataValueAttributes( index ) );
    const QString text = formatDataValueText( dva, index, value );
    paintDataValueText( painter, dva, pos, value >= 0.0, text,
                        justCalculateRect, cumulatedBoundingRect );
}

void AbstractDiagram::Private::paintDataValueText(
    QPainter* painter,
    const DataValueAttributes& attrs,
    const QPointF& pos,
    bool valueIsPositive,
    const QString& text,
    bool justCalculateRect /* = false */,
    QRectF* cumulatedBoundingRect /* = 0 */ )
{
    if ( !attrs.isVisible() ) {
        return;
    }

    const TextAttributes ta( attrs.textAttributes() );
    if ( !ta.isVisible() || ( !attrs.showRepetitiveDataLabels() && prevPaintedDataValueText == text ) ) {
        return;
    }
    prevPaintedDataValueText = text;

    QTextDocument doc;
    doc.setDocumentMargin( 0.0 );
    if ( Qt::mightBeRichText( text ) ) {
        doc.setHtml( text );
    } else {
        doc.setPlainText( text );
    }

    const QFont calculatedFont( ta.calculatedFont( plane, KDChartEnums::MeasureOrientationMinimum ) );

    const PainterSaver painterSaver( painter );
    painter->setPen( PrintingParameters::scalePen( ta.pen() ) );

    doc.setDefaultFont( calculatedFont );
    QAbstractTextDocumentLayout::PaintContext context;
    context.palette = diagram->palette();
    context.palette.setColor( QPalette::Text, ta.pen().color() );

    QAbstractTextDocumentLayout* const layout = doc.documentLayout();
    layout->setPaintDevice( painter->device() );

    painter->translate( pos.x(), pos.y() );
    int rotation = ta.rotation();
    if ( !valueIsPositive && attrs.mirrorNegativeValueTextRotation() ) {
        rotation *= -1;
    }
    painter->rotate( rotation );

    // do overlap detection "as seen by the painter"
    QTransform transform = painter->worldTransform();

    bool drawIt = true;
    // note: This flag can be set differently for every label text!
    // In theory a user could e.g. have some small red text on one of the
    // values that she wants to have written in any case - so we just
    // do not test if such texts would cover some of the others.
    if ( !attrs.showOverlappingDataLabels() ) {
        const QRectF br( layout->frameBoundingRect( doc.rootFrame() ) );
        QPolygon pr = transform.mapToPolygon( br.toRect() );
        // Using QPainterPath allows us to use intersects() (which has many early-exits)
        // instead of QPolygon::intersected (which calculates a slow and precise intersection polygon)
        QPainterPath path;
        path.addPolygon( pr );

        // iterate backwards because recently added items are more likely to overlap, so we spend
        // less time checking irrelevant items when there is overlap
        for ( int i = alreadyDrawnDataValueTexts.count() - 1; i >= 0; i-- ) {
            if ( alreadyDrawnDataValueTexts.at( i ).intersects( path ) ) {
                // qDebug() << "not painting this label due to overlap";
                drawIt = false;
                break;
            }
        }
        if ( drawIt ) {
            alreadyDrawnDataValueTexts << path;
        }
    }

    if ( drawIt ) {
        QRectF rect = layout->frameBoundingRect( doc.rootFrame() );
        if ( cumulatedBoundingRect ) {
            (*cumulatedBoundingRect) |= transform.mapRect( rect );
        }
        if ( !justCalculateRect ) {
            bool paintBack = false;
            BackgroundAttributes back( attrs.backgroundAttributes() );
            if ( back.isVisible() ) {
                paintBack = true;
                painter->setBrush( back.brush() );
            } else {
                painter->setBrush( QBrush() );
            }

            qreal radius = 0.0;
            FrameAttributes frame( attrs.frameAttributes() );
            if ( frame.isVisible() ) {
                paintBack = true;
                painter->setPen( frame.pen() );
                radius = frame.cornerRadius();
            }

            if ( paintBack ) {
                QRectF borderRect( QPointF( 0, 0 ), rect.size() );
                painter->drawRoundedRect( borderRect, radius, radius );
            }
            layout->draw( painter, context );
        }
    }
}

QModelIndex AbstractDiagram::Private::indexAt( const QPoint& point ) const
{
    QModelIndexList l = indexesAt( point );
    qSort( l );
    if ( !l.isEmpty() )
        return l.first();
    else
        return QModelIndex();
}

QModelIndexList AbstractDiagram::Private::indexesAt( const QPoint& point ) const
{
    return reverseMapper.indexesAt( point ); // which could be empty
}

QModelIndexList AbstractDiagram::Private::indexesIn( const QRect& rect ) const
{
    return reverseMapper.indexesIn( rect );
}

CartesianDiagramDataCompressor::AggregatedDataValueAttributes AbstractDiagram::Private::aggregatedAttrs(
    const QModelIndex& index,
    const CartesianDiagramDataCompressor::CachePosition* position ) const
{
    Q_UNUSED( position ); // used by cartesian diagrams only
    CartesianDiagramDataCompressor::AggregatedDataValueAttributes allAttrs;
    allAttrs[index] = diagram->dataValueAttributes( index );
    return allAttrs;
}

void AbstractDiagram::Private::setDatasetAttrs( int dataset, const QVariant& data, int role )
{
    // To store attributes for a dataset, we use the first column
    // that's associated with it. (i.e., with a dataset dimension
    // of two, the column of the keys). In most cases however, there's
    // only one data dimension, and thus also only one column per data set.
    int column = dataset * datasetDimension;

    // For DataHiddenRole, also store the flag in the other data points that belong to this data set,
    // otherwise it's impossible to hide data points in a plotter diagram because there will always
    // be one model index that belongs to this data point that is not hidden.
    // For more details on how hiding works, see the data compressor.
    // Also see KDCH-503 for which this is a workaround.
    int columnSpan = role == DataHiddenRole ? datasetDimension : 1;

    for ( int i = 0; i < columnSpan; i++ ) {
        attributesModel->setHeaderData( column + i, Qt::Horizontal, data, role );
    }
}

QVariant AbstractDiagram::Private::datasetAttrs( int dataset, int role ) const
{
    // See setDataSetAttrs for explanation of column
    int column = dataset * datasetDimension;
    return attributesModel->headerData( column, Qt::Horizontal, role );
}

void AbstractDiagram::Private::resetDatasetAttrs( int dataset, int role )
{
    // See setDataSetAttrs for explanation of column
    int column = dataset * datasetDimension;
    attributesModel->resetHeaderData( column, Qt::Horizontal, role );
}

bool AbstractDiagram::Private::isTransposed() const
{
     // Determine the diagram that specifies the orientation.
     // That diagram is the reference diagram, if it exists, or otherwise the diagram itself.
     // Note: In KDChart 2.3 or earlier, only a bar diagram can be transposed.
     const AbstractCartesianDiagram* refDiagram = qobject_cast< const AbstractCartesianDiagram * >( diagram );
     if ( !refDiagram ) {
         return false;
     }
     if ( refDiagram->referenceDiagram() ) {
         refDiagram = refDiagram->referenceDiagram();
     }
     const BarDiagram* barDiagram = qobject_cast< const BarDiagram* >( refDiagram );
     if ( !barDiagram ) {
         return false;
     }
     return barDiagram->orientation() == Qt::Horizontal;
}

LineAttributesInfo::LineAttributesInfo()
{
}

LineAttributesInfo::LineAttributesInfo( const QModelIndex& _index, const QPointF& _value, const QPointF& _nextValue )
    : index( _index )
    , value ( _value )
    , nextValue ( _nextValue )
{
}
