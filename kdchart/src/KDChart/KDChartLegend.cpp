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

#include "KDChartLegend.h"
#include "KDChartLegend_p.h"
#include <KDChartTextAttributes.h>
#include <KDChartMarkerAttributes.h>
#include <KDChartPalette.h>
#include <KDChartAbstractDiagram.h>
#include "KDTextDocument.h"
#include <KDChartDiagramObserver.h>
#include "KDChartLayoutItems.h"

#include <QFont>
#include <QGridLayout>
#include <QPainter>
#include <QTextTableCell>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextDocumentFragment>
#include <QTimer>
#include <QAbstractTextDocumentLayout>
#include <QtDebug>
#include <QLabel>

#include <KDABLibFakes>

using namespace KDChart;

Legend::Private::Private() :
    referenceArea(0),
    position( Position::East ),
    alignment( Qt::AlignCenter ),
    textAlignment( Qt::AlignCenter ),
    relativePosition( RelativePosition() ),
    orientation( Qt::Vertical ),
    order( Qt::AscendingOrder ),
    showLines( false ),
    texts(),
    textAttributes(),
    titleText( QObject::tr( "Legend" ) ),
    titleTextAttributes(),
    spacing( 1 ),
    useAutomaticMarkerSize( true ),
    legendStyle( MarkersOnly )
    //needRebuild( true )
{
    // By default we specify a simple, hard point as the 'relative' position's ref. point,
    // since we can not be sure that there will be any parent specified for the legend.
    relativePosition.setReferencePoints( PositionPoints( QPointF( 0.0, 0.0 ) ) );
    relativePosition.setReferencePosition( Position::NorthWest );
    relativePosition.setAlignment( Qt::AlignTop | Qt::AlignLeft );
    relativePosition.setHorizontalPadding( KDChart::Measure( 4.0, KDChartEnums::MeasureCalculationModeAbsolute ) );
    relativePosition.setVerticalPadding( KDChart::Measure( 4.0, KDChartEnums::MeasureCalculationModeAbsolute ) );
}

Legend::Private::~Private()
{
    // this bloc left empty intentionally
}


#define d d_func()


Legend::Legend( QWidget* parent ) :
    AbstractAreaWidget( new Private(), parent )
{
    d->referenceArea = parent;
    init();
}

Legend::Legend( KDChart::AbstractDiagram* diagram, QWidget* parent ) :
    AbstractAreaWidget( new Private(), parent )
{
    d->referenceArea = parent;
    init();
    setDiagram( diagram );
}

Legend::~Legend()
{
    emit destroyedLegend( this );
}

void Legend::init()
{
    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

    d->layout = new QGridLayout( this );
    d->layout->setMargin( 2 );
    d->layout->setSpacing( d->spacing );
    //setLayout( d->layout );

    const Measure normalFontSizeTitle( 12, KDChartEnums::MeasureCalculationModeAbsolute );
    const Measure normalFontSizeLabels( 10, KDChartEnums::MeasureCalculationModeAbsolute );
    const Measure minimalFontSize( 4, KDChartEnums::MeasureCalculationModeAbsolute );

    TextAttributes textAttrs;
    textAttrs.setPen( QPen( Qt::black ) );
    textAttrs.setFont( QFont( QLatin1String( "helvetica" ), 10, QFont::Normal, false ) );
    textAttrs.setFontSize( normalFontSizeLabels );
    textAttrs.setMinimalFontSize( minimalFontSize );
    setTextAttributes( textAttrs );

    TextAttributes titleTextAttrs;
    titleTextAttrs.setPen( QPen( Qt::black ) );
    titleTextAttrs.setFont( QFont( QLatin1String( "helvetica" ), 12, QFont::Bold, false ) );
    titleTextAttrs.setFontSize( normalFontSizeTitle );
    titleTextAttrs.setMinimalFontSize( minimalFontSize );
    setTitleTextAttributes( titleTextAttrs );

    FrameAttributes frameAttrs;
    frameAttrs.setVisible( true );
    frameAttrs.setPen( QPen( Qt::black ) );
    frameAttrs.setPadding( 1 );
    setFrameAttributes( frameAttrs );

    d->position = Position::NorthEast;
    d->alignment = Qt::AlignCenter;
}


QSize Legend::minimumSizeHint() const
{
    return sizeHint();
}

//#define DEBUG_LEGEND_PAINT

QSize Legend::sizeHint() const
{
#ifdef DEBUG_LEGEND_PAINT
    qDebug()  << "Legend::sizeHint() started";
#endif
    Q_FOREACH( KDChart::AbstractLayoutItem* layoutItem, d->layoutItems ) {
        layoutItem->sizeHint();
    }
    return AbstractAreaWidget::sizeHint();
}

void Legend::needSizeHint()
{
    // Re-build the Legend's content, if it has not been build yet,
    // or if the Legend's geometry has changed, resp.
    buildLegend();
}

void Legend::resizeLayout( const QSize& size )
{
#ifdef DEBUG_LEGEND_PAINT
    qDebug() << "Legend::resizeLayout started";
#endif
    if ( d->layout ) {
        d->layout->setGeometry( QRect(QPoint(0,0), size) );
        activateTheLayout();
    }
#ifdef DEBUG_LEGEND_PAINT
    qDebug() << "Legend::resizeLayout done";
#endif
}

void Legend::activateTheLayout()
{
    if ( d->layout && d->layout->parent() )
        d->layout->activate();
}


void Legend::setLegendStyle( LegendStyle style )
{
    if ( d->legendStyle == style ) return;
    d->legendStyle = style;
    setNeedRebuild();
}

Legend::LegendStyle Legend::legendStyle() const
{
    return d->legendStyle;
}

/**
  * Creates an exact copy of this legend.
  */
Legend* Legend::clone() const
{
    Legend* legend = new Legend( new Private( *d ), 0 );
    legend->setTextAttributes( textAttributes() );
    legend->setTitleTextAttributes( titleTextAttributes() );
    legend->setFrameAttributes( frameAttributes() );
    legend->setUseAutomaticMarkerSize( useAutomaticMarkerSize() );
    legend->setPosition( position() );
    legend->setAlignment( alignment() );
    legend->setTextAlignment( textAlignment() );
    legend->setLegendStyle( legendStyle() );
    return legend;
}


bool Legend::compare( const Legend* other ) const
{
    if ( other == this ) return true;
    if ( !other ) {
        return false;
    }

    return  ( static_cast<const AbstractAreaBase*>(this)->compare( other ) ) &&
            (isVisible()              == other->isVisible()) &&
            (position()               == other->position()) &&
            (alignment()              == other->alignment())&&
            (textAlignment()          == other->textAlignment())&&
            (floatingPosition()       == other->floatingPosition()) &&
            (orientation()            == other->orientation())&&
            (showLines()              == other->showLines())&&
            (texts()                  == other->texts())&&
            (brushes()                == other->brushes())&&
            (pens()                   == other->pens())&&
            (markerAttributes()       == other->markerAttributes())&&
            (useAutomaticMarkerSize() == other->useAutomaticMarkerSize()) &&
            (textAttributes()         == other->textAttributes()) &&
            (titleText()              == other->titleText())&&
            (titleTextAttributes()    == other->titleTextAttributes()) &&
            (spacing()                == other->spacing()) &&
            (legendStyle()            == other->legendStyle());
}


void Legend::paint( QPainter* painter )
{
#ifdef DEBUG_LEGEND_PAINT
    qDebug() << "entering Legend::paint( QPainter* painter )";
#endif
    if ( ! diagram() ) {
        return;
    }

    // re-calculate/adjust the Legend's internal layout and contents, if needed:
    //buildLegend();

    // PENDING(kalle) Support palette

    Q_FOREACH( KDChart::AbstractLayoutItem* layoutItem, d->layoutItems ) {
        layoutItem->paint( painter );
    }
#ifdef DEBUG_LEGEND_PAINT
    qDebug() << "leaving Legend::paint( QPainter* painter )";
#endif
}


uint Legend::datasetCount() const
{
    int modelLabelsCount = 0;
    KDAB_FOREACH ( DiagramObserver* observer, d->observers ) {
        AbstractDiagram* diagram = observer->diagram();
        Q_ASSERT( diagram->datasetLabels().count() == diagram->datasetBrushes().count() );
        modelLabelsCount += diagram->datasetLabels().count();
    }
    return modelLabelsCount;
}


void Legend::setReferenceArea( const QWidget* area )
{
    if ( area == d->referenceArea ) return;
    d->referenceArea = area;
    setNeedRebuild();
}

const QWidget* Legend::referenceArea() const
{
    return d->referenceArea ? d->referenceArea : qobject_cast< const QWidget* >( parent() );
}


AbstractDiagram* Legend::diagram() const
{
    if ( d->observers.isEmpty() )
        return 0;
    return d->observers.first()->diagram();
}

DiagramList Legend::diagrams() const
{
    DiagramList list;
    for (int i = 0; i < d->observers.size(); ++i)
        list << d->observers.at(i)->diagram();
    return list;
}

ConstDiagramList Legend::constDiagrams() const
{
    ConstDiagramList list;
    for (int i = 0; i < d->observers.size(); ++i)
        list << d->observers.at(i)->diagram();
    return list;
}

void Legend::addDiagram( AbstractDiagram* newDiagram )
{
    if ( newDiagram )
    {
        DiagramObserver* observer = new DiagramObserver( newDiagram, this );

        DiagramObserver* oldObs = d->findObserverForDiagram( newDiagram );
        if ( oldObs ) {
            delete oldObs;
            d->observers[ d->observers.indexOf( oldObs ) ] = observer;
        } else {
            d->observers.append( observer );
        }
        connect( observer, SIGNAL( diagramAboutToBeDestroyed(AbstractDiagram*) ),
                           SLOT( resetDiagram(AbstractDiagram*) ));
        connect( observer, SIGNAL( diagramDataChanged(AbstractDiagram*) ),
                 SLOT( setNeedRebuild() ));
        connect( observer, SIGNAL( diagramDataHidden(AbstractDiagram*) ),
                 SLOT( setNeedRebuild() ));
        connect( observer, SIGNAL( diagramAttributesChanged(AbstractDiagram*) ),
                        SLOT( setNeedRebuild() ));
        setNeedRebuild();
    }
}

void Legend::removeDiagram( AbstractDiagram* oldDiagram )
{
    int datasetBrushOffset = 0;
    QList<AbstractDiagram*> diagrams = this->diagrams();
    for (int i =0; i <diagrams.count(); i++)
    {
        if (diagrams.at(i) == oldDiagram)
        {
            for ( int i = 0; i < oldDiagram->datasetBrushes().count(); i++ ) {
                d->brushes.remove(datasetBrushOffset + i);
                d->texts.remove(datasetBrushOffset + i);
            }
            for ( int i = 0; i < oldDiagram->datasetPens().count(); i++ ) {
                d->pens.remove(datasetBrushOffset + i);
            }
            break;
        }
        datasetBrushOffset += diagrams.at(i)->datasetBrushes().count();
    }

    if ( oldDiagram ) {
        DiagramObserver* oldObs = d->findObserverForDiagram( oldDiagram );
        if ( oldObs ) {
            delete oldObs;
            d->observers.removeAt( d->observers.indexOf( oldObs ) );
        }
        setNeedRebuild();
    }
}

void Legend::removeDiagrams()
{
    // removeDiagram() may change the d->observers list. So, build up the list of
    // diagrams to remove first and then remove them one by one.
    QList<AbstractDiagram*> diagrams;
    for (int i = 0; i < d->observers.size(); ++i)
        diagrams.append( d->observers.at(i)->diagram() );
    for (int i = 0; i < diagrams.count(); ++i)
        removeDiagram( diagrams[i] );
}

void Legend::replaceDiagram( AbstractDiagram* newDiagram,
                             AbstractDiagram* oldDiagram )
{
    KDChart::AbstractDiagram* old = oldDiagram;
    if ( ! d->observers.isEmpty() && ! old ) {
        old = d->observers.first()->diagram();
        if ( ! old )
            d->observers.removeFirst(); // first entry had a 0 diagram
    }
    if ( old )
        removeDiagram( old );
    if ( newDiagram )
        addDiagram( newDiagram );
}

uint Legend::dataSetOffset(KDChart::AbstractDiagram* diagram)
{
    uint offset = 0;

    for (int i = 0; i < d->observers.count(); ++i)
    {
        if (d->observers.at(i)->diagram() == diagram)
            return offset;

        KDChart::AbstractDiagram* diagram = d->observers.at(i)->diagram();
        if (!diagram->model())
            continue;

        offset = offset + diagram->model()->columnCount();
    }

    return offset;
}

void Legend::setDiagram( KDChart::AbstractDiagram* newDiagram )
{
    replaceDiagram( newDiagram );
}

void Legend::resetDiagram( AbstractDiagram* oldDiagram )
{
    removeDiagram( oldDiagram );
}

void Legend::setVisible( bool visible )
{
    // do NOT bail out if visible != isVisible(), because the return value of isVisible() also depends
    // on the visibility of the parent.
    QWidget::setVisible( visible );
    emitPositionChanged();
}

void Legend::setNeedRebuild()
{
    buildLegend();
    sizeHint();
}

void Legend::setPosition( Position position )
{
    if ( d->position == position )
        return;
    d->position = position;
    emitPositionChanged();
}

void Legend::emitPositionChanged()
{
    emit positionChanged( this );
    emit propertiesChanged();
}


Position Legend::position() const
{
    return d->position;
}

void Legend::setAlignment( Qt::Alignment alignment )
{
    if ( d->alignment == alignment )
        return;
    d->alignment = alignment;
    emitPositionChanged();
}

Qt::Alignment Legend::alignment() const
{
    return d->alignment;
}

void Legend::setTextAlignment( Qt::Alignment alignment )
{
    if ( d->textAlignment == alignment )
        return;
    d->textAlignment = alignment;
    emitPositionChanged();
}

Qt::Alignment Legend::textAlignment() const
{
    return d->textAlignment;
}

void Legend::setLegendSymbolAlignment(Qt::Alignment alignment)
{
    if (d->legendLineSymbolAlignment == alignment)
        return;

    d->legendLineSymbolAlignment = alignment;
    emitPositionChanged();
}

Qt::Alignment Legend::legendSymbolAlignment() const
{
    return d->legendLineSymbolAlignment ;
}

void Legend::setFloatingPosition( const RelativePosition& relativePosition )
{
    d->position = Position::Floating;
    if ( d->relativePosition != relativePosition ) {
        d->relativePosition  = relativePosition;
        emitPositionChanged();
    }
}

const RelativePosition Legend::floatingPosition() const
{
    return d->relativePosition;
}

void Legend::setOrientation( Qt::Orientation orientation )
{
    if ( d->orientation == orientation ) return;
    d->orientation = orientation;
    setNeedRebuild();
    emitPositionChanged();
}

Qt::Orientation Legend::orientation() const
{
    return d->orientation;
}

void Legend::setSortOrder( Qt::SortOrder order )
{
    if ( d->order == order )
        return;
    d->order = order;
    setNeedRebuild();
    emitPositionChanged();
}

Qt::SortOrder Legend::sortOrder() const
{
    return d->order;
}

void Legend::setShowLines( bool legendShowLines )
{
    if ( d->showLines == legendShowLines ) return;
    d->showLines = legendShowLines;
    setNeedRebuild();
    emitPositionChanged();
}

bool Legend::showLines() const
{
    return d->showLines;
}

void Legend::setUseAutomaticMarkerSize( bool useAutomaticMarkerSize )
{
    d->useAutomaticMarkerSize = useAutomaticMarkerSize;
    setNeedRebuild();
    emitPositionChanged();
}

bool Legend::useAutomaticMarkerSize() const
{
    return d->useAutomaticMarkerSize;
}

/**
    \brief Removes all legend texts that might have been set by setText.

    This resets the Legend to default behaviour: Texts are created automatically.
*/
void Legend::resetTexts()
{
    if ( ! d->texts.count() ) return;
    d->texts.clear();
    setNeedRebuild();
}

void Legend::setText( uint dataset, const QString& text )
{
    if ( d->texts[ dataset ] == text ) return;
    d->texts[ dataset ] = text;
    setNeedRebuild();
}

QString Legend::text( uint dataset ) const
{
    if ( d->texts.find( dataset ) != d->texts.end() ) {
        return d->texts[ dataset ];
    } else {
        return d->modelLabels[ dataset ];
    }
}

const QMap<uint,QString> Legend::texts() const
{
    return d->texts;
}

void Legend::setColor( uint dataset, const QColor& color )
{
    if ( d->brushes[ dataset ] != color ) {
        d->brushes[ dataset ] = color;
        setNeedRebuild();
        update();
    }
}

void Legend::setBrush( uint dataset, const QBrush& brush )
{
    if ( d->brushes[ dataset ] != brush ) {
        d->brushes[ dataset ] = brush;
        setNeedRebuild();
        update();
    }
}

QBrush Legend::brush( uint dataset ) const
{
    if ( d->brushes.contains( dataset ) ) {
        return d->brushes[ dataset ];
    } else {
        return d->modelBrushes[ dataset ];
    }
}

const QMap<uint,QBrush> Legend::brushes() const
{
    return d->brushes;
}


void Legend::setBrushesFromDiagram( KDChart::AbstractDiagram* diagram )
{
    bool bChangesDone = false;
    QList<QBrush> datasetBrushes = diagram->datasetBrushes();
    for ( int i = 0; i < datasetBrushes.count(); i++ ) {
        if ( d->brushes[ i ] != datasetBrushes[ i ] ) {
            d->brushes[ i ]  = datasetBrushes[ i ];
            bChangesDone = true;
        }
    }
    if ( bChangesDone ) {
        setNeedRebuild();
        update();
    }
}


void Legend::setPen( uint dataset, const QPen& pen )
{
    if ( d->pens[dataset] == pen ) return;
    d->pens[dataset] = pen;
    setNeedRebuild();
    update();
}

QPen Legend::pen( uint dataset ) const
{
    if ( d->pens.find( dataset ) != d->pens.end() )
        return d->pens[dataset];
    else
        return d->modelPens[ dataset ];
}

const QMap<uint,QPen> Legend::pens() const
{
    return d->pens;
}


void Legend::setMarkerAttributes( uint dataset, const MarkerAttributes& markerAttributes )
{
    if ( d->markerAttributes[dataset] == markerAttributes ) return;
    d->markerAttributes[ dataset ] = markerAttributes;
    setNeedRebuild();
    update();
}

MarkerAttributes Legend::markerAttributes( uint dataset ) const
{
    if ( d->markerAttributes.find( dataset ) != d->markerAttributes.end() )
        return d->markerAttributes[ dataset ];
    else if ( static_cast<uint>( d->modelMarkers.count() ) > dataset )
        return d->modelMarkers[ dataset ];
    return MarkerAttributes();
}

const QMap<uint, MarkerAttributes> Legend::markerAttributes() const
{
    return d->markerAttributes;
}


void Legend::setTextAttributes( const TextAttributes &a )
{
    if ( d->textAttributes == a ) return;
    d->textAttributes = a;
    setNeedRebuild();
}

TextAttributes Legend::textAttributes() const
{
    return d->textAttributes;
}

void Legend::setTitleText( const QString& text )
{
    if ( d->titleText == text ) return;
    d->titleText = text;
    setNeedRebuild();
}

QString Legend::titleText() const
{
    return d->titleText;
}

void Legend::setTitleTextAttributes( const TextAttributes &a )
{
    if ( d->titleTextAttributes == a ) return;
    d->titleTextAttributes = a;
    setNeedRebuild();
}

TextAttributes Legend::titleTextAttributes() const
{
    return d->titleTextAttributes;
}

void Legend::forceRebuild()
{
#ifdef DEBUG_LEGEND_PAINT
    qDebug() << "entering Legend::forceRebuild()";
#endif
    //setSpacing(d->layout->spacing());
    buildLegend();
#ifdef DEBUG_LEGEND_PAINT
    qDebug() << "leaving Legend::forceRebuild()";
#endif
}

void Legend::setSpacing( uint space )
{
    if ( d->spacing == space && d->layout->spacing() == static_cast<int>(space) ) return;
    d->spacing = space;
    d->layout->setSpacing( space );
    setNeedRebuild();
}

uint Legend::spacing() const
{
    return d->spacing;
}

void Legend::setDefaultColors()
{
    Palette pal = Palette::defaultPalette();
    for ( int i = 0; i < pal.size(); i++ ) {
        setBrush( i, pal.getBrush( i ) );
    }
}

void Legend::setRainbowColors()
{
    Palette pal = Palette::rainbowPalette();
    for ( int i = 0; i < pal.size(); i++ ) {
        setBrush( i, pal.getBrush( i ) );
    }
}

void Legend::setSubduedColors( bool ordered )
{
    Palette pal = Palette::subduedPalette();
    if ( ordered ) {
        for ( int i = 0; i < pal.size(); i++ ) {
            setBrush( i, pal.getBrush( i ) );
        }
    } else {
        static const int s_subduedColorsCount = 18;
        Q_ASSERT( pal.size() >= s_subduedColorsCount );
        static const int order[ s_subduedColorsCount ] = {
            0, 5, 10, 15, 2, 7, 12, 17, 4,
            9, 14, 1, 6, 11, 16, 3, 8, 13
        };
        for ( int i = 0; i < s_subduedColorsCount; i++ ) {
            setBrush( i, pal.getBrush( order[i] ) );
        }
    }
}

void Legend::resizeEvent ( QResizeEvent * event )
{
    Q_UNUSED( event );
#ifdef DEBUG_LEGEND_PAINT
    qDebug() << "Legend::resizeEvent() called";
#endif
    forceRebuild();
    sizeHint();
    QTimer::singleShot(0, this, SLOT(emitPositionChanged()));
}

void Legend::buildLegend()
{
    Q_FOREACH( QLayoutItem* layoutItem, d->layoutItems ) {
        d->layout->removeItem( layoutItem );
    }
    qDeleteAll( d->layoutItems );
    d->layoutItems.clear();

    if ( orientation() == Qt::Vertical ) {
        d->layout->setColumnStretch( 6, 1 );
    } else {
        d->layout->setColumnStretch( 6, 0 );
    }

    d->modelLabels.clear();
    d->modelBrushes.clear();
    d->modelPens.clear();
    d->modelMarkers.clear();
    // retrieve the diagrams' settings for all non-hidden datasets
    for ( int i = 0; i < d->observers.size(); ++i ) {
        const AbstractDiagram* diagram = d->observers.at( i )->diagram();
        if ( diagram ) {
            const QStringList             diagramLabels( diagram->datasetLabels() );
            const QList<QBrush>           diagramBrushes( diagram->datasetBrushes() );
            const QList<QPen>             diagramPens( diagram->datasetPens() );
            const QList<MarkerAttributes> diagramMarkers( diagram->datasetMarkers() );
            const int begin = sortOrder() == Qt::AscendingOrder ? 0 : diagramLabels.count() - 1;
            const int end = sortOrder() == Qt::AscendingOrder ? diagramLabels.count() : -1;
            for ( int dataset = begin; dataset != end; dataset += begin < end ? 1 : -1 ) {
                // only show the label if the diagrams is NOT having the dataset set to hidden
                // and the dataset is not hidden in this legend either
                if ( !diagram->isHidden( dataset ) && !datasetIsHidden( dataset ) ) {
                    d->modelLabels  += diagramLabels[   dataset ];
                    d->modelBrushes += diagramBrushes[  dataset ];
                    d->modelPens    += diagramPens[     dataset ];
                    d->modelMarkers += diagramMarkers[  dataset ];
                }
            }
        }
    }

    Q_ASSERT( d->modelLabels.count() == d->modelBrushes.count() );

    // legend caption
    if ( !titleText().isEmpty() && titleTextAttributes().isVisible() ) {
        // PENDING(kalle) Other properties!
        KDChart::TextLayoutItem* titleItem =
            new KDChart::TextLayoutItem( titleText(), titleTextAttributes(), referenceArea(),
                                         orientation() == Qt::Vertical ? KDChartEnums::MeasureOrientationMinimum
                                                                       : KDChartEnums::MeasureOrientationHorizontal,
                                         d->textAlignment );
        titleItem->setParentWidget( this );

        d->layoutItems << titleItem;
        if ( orientation() == Qt::Vertical )
            d->layout->addItem( titleItem, 0, 0, 1, 5, Qt::AlignCenter );
        else
            d->layout->addItem( titleItem, 0, 0, 1, d->modelLabels.count() ? (d->modelLabels.count()*4) : 1, Qt::AlignCenter );

        // The line between the title and the legend items, if any.
        if ( showLines() && d->modelLabels.count() ) {
            KDChart::HorizontalLineLayoutItem* lineItem = new KDChart::HorizontalLineLayoutItem();
            d->layoutItems << lineItem;
            if ( orientation() == Qt::Vertical ) {
                d->layout->addItem( lineItem, 1, 0, 1, 5, Qt::AlignCenter );
            } else {
                // we have 1+count*4 columns, because we have both, a leading and a trailing spacer
                d->layout->addItem( lineItem, 1, 0, 1, 1+d->modelLabels.count()*4, Qt::AlignCenter );
            }
        }
    }

    const KDChartEnums::MeasureOrientation orient = orientation() == Qt::Vertical ?
                                                    KDChartEnums::MeasureOrientationMinimum :
                                                    KDChartEnums::MeasureOrientationHorizontal;
    const TextAttributes labelAttrs( textAttributes() );
    qreal fontHeight = labelAttrs.calculatedFontSize( referenceArea(), orient );
    const LegendStyle style = legendStyle();
    QFont tmpFont = labelAttrs.font();
    tmpFont.setPointSizeF( fontHeight );

    if ( GlobalMeasureScaling::paintDevice() )
    {
        QFontMetricsF metr( tmpFont, GlobalMeasureScaling::paintDevice() );
        fontHeight = metr.height();
    }
    else
    {
        QFontMetricsF metr( tmpFont );
        fontHeight = metr.height();
    }


    const bool bShowMarkers = (style != LinesOnly);

    QSizeF maxMarkersSize(1.0, 1.0);
    QVector <MarkerAttributes> markerAttrs( d->modelLabels.count() );
    if ( bShowMarkers ) {
        for ( int dataset = 0; dataset < d->modelLabels.count(); ++dataset ) {
            markerAttrs[dataset] = markerAttributes( dataset );
            QSizeF siz;
            if ( useAutomaticMarkerSize() ||
                ! markerAttrs[dataset].markerSize().isValid() )
            {
                siz = QSizeF(fontHeight, fontHeight);
                markerAttrs[dataset].setMarkerSize( siz );
            } else {
                siz = markerAttrs[dataset].markerSize();
            }
            maxMarkersSize =
                    QSizeF(qMax(maxMarkersSize.width(),  siz.width()),
                           qMax(maxMarkersSize.height(), siz.height()));
        }
    }

    // If we show a marker on a line, we paint it after 8 pixels
    // of the line have been painted. This allows to see the line style
    // at the right side of the marker without the line needing to
    // be too long.
    // (having the marker in the middle of the line would require longer lines)
    const int markerOffsOnLine = 8;

    int maxLineLength = 18;
    {
        bool hasComplexPenStyle = false;
        for ( int dataset = 0; dataset < d->modelLabels.count(); ++dataset ) {
            const QPen pn = pen(dataset);
            const Qt::PenStyle ps = pn.style();
            if ( ps != Qt::NoPen ) {
                maxLineLength = qMax( pn.width() * 18, maxLineLength );
                if ( ps != Qt::SolidLine )
                    hasComplexPenStyle = true;
            }
        }
        if ( hasComplexPenStyle && bShowMarkers )
            maxLineLength =
                    maxLineLength + markerOffsOnLine +
                    static_cast<int>(maxMarkersSize.width());
    }

    // Horizontal needs a leading spacer
    if ( orientation() != Qt::Vertical ) {
        d->layout->addItem( new QSpacerItem( spacing(), 1 ), 2, 0 );
    }

    // for all datasets: add (line)marker items and text items to the layout
    for ( int dataset = 0; dataset < d->modelLabels.count(); ++dataset ) {
        KDChart::AbstractLayoutItem* markerLineItem = 0;
        // It is possible to set the marker brush both through the MarkerAttributes,
        // as well as through the dataset brush set in the diagram, whereas the
        // MarkerAttributes are preferred.
        const QBrush markerBrush = markerAttrs[dataset].markerColor().isValid() ?
                                   QBrush(markerAttrs[dataset].markerColor()) : brush( dataset );
        switch ( style ) {
            case( MarkersOnly ):
                markerLineItem = new KDChart::MarkerLayoutItem(
                        diagram(),
                        markerAttrs[dataset],
                        markerBrush,
                        markerAttrs[dataset].pen(),
                        Qt::AlignLeft );
                break;
            case( LinesOnly ):
                markerLineItem = new KDChart::LineLayoutItem(
                        diagram(),
                        maxLineLength,
                        pen( dataset ),
                        d->legendLineSymbolAlignment,
                        Qt::AlignCenter );
                break;
            case( MarkersAndLines ):
                markerLineItem = new KDChart::LineWithMarkerLayoutItem(
                        diagram(),
                        maxLineLength,
                        pen( dataset ),
                        markerOffsOnLine,
                        markerAttrs[dataset],
                        markerBrush,
                        markerAttrs[dataset].pen(),
                        Qt::AlignCenter );
                break;
            default:
                Q_ASSERT( false ); // all styles need to be handled
        }
        if ( markerLineItem ) {
            d->layoutItems << markerLineItem;
            if ( orientation() == Qt::Vertical )
                d->layout->addItem( markerLineItem,
                                    dataset*2+2, // first row is title, second is line
                                    1,
                                    1, 1, Qt::AlignCenter );
            else
                d->layout->addItem( markerLineItem,
                                    2, // all in row two
                                    dataset*4+1 );
        }

        // PENDING(kalle) Other properties!
        KDChart::TextLayoutItem* labelItem =
            new KDChart::TextLayoutItem( text( dataset ),
                labelAttrs,
                referenceArea(), orient,
                d->textAlignment );
        labelItem->setParentWidget( this );

        d->layoutItems << labelItem;
        if ( orientation() == Qt::Vertical )
            d->layout->addItem( labelItem,
                                dataset*2+2, // first row is title, second is line
                                3 );
        else
            d->layout->addItem( labelItem,
                                2, // all in row two
                                dataset*4+2 );

        // horizontal lines (only in vertical mode, and not after the last item)
        if ( orientation() == Qt::Vertical && showLines() && dataset != d->modelLabels.count()-1 ) {
            KDChart::HorizontalLineLayoutItem* lineItem = new KDChart::HorizontalLineLayoutItem();
            d->layoutItems << lineItem;
            d->layout->addItem( lineItem,
                                dataset*2+1+2,
                                0,
                                1, 5, Qt::AlignCenter );
        }

        // vertical lines (only in horizontal mode, and not after the last item)
        if ( orientation() == Qt::Horizontal && showLines() && dataset != d->modelLabels.count()-1 ) {
            KDChart::VerticalLineLayoutItem* lineItem = new KDChart::VerticalLineLayoutItem();
            d->layoutItems << lineItem;
            d->layout->addItem( lineItem,
                                2, // all in row two
                                style == MarkersAndLines ? dataset*4+4 : dataset*4+3 ,
                                1, 1, Qt::AlignCenter );
        }

        // Horizontal needs a spacer
        if ( orientation() != Qt::Vertical ) {
            d->layout->addItem( new QSpacerItem( spacing(), 1 ), 2, dataset * 4 + 4 );
        }
    }

    // vertical line (only in vertical mode)
    if ( orientation() == Qt::Vertical && showLines() && d->modelLabels.count() ) {
        KDChart::VerticalLineLayoutItem* lineItem = new KDChart::VerticalLineLayoutItem();
        d->layoutItems << lineItem;
        d->layout->addItem( lineItem, 2, 2, d->modelLabels.count()*2, 1 );
    }

    // This line is absolutely necessary, otherwise: #2516.
    activateTheLayout();

    emit propertiesChanged();
#ifdef DEBUG_LEGEND_PAINT
    qDebug() << "leaving Legend::buildLegend()";
#endif
}

void Legend::setHiddenDatasets( const QList<uint> hiddenDatasets )
{
    d->hiddenDatasets = hiddenDatasets;
}

const QList<uint> Legend::hiddenDatasets() const
{
    return d->hiddenDatasets;
}

void Legend::setDatasetHidden( uint dataset, bool hidden )
{
    if ( hidden && !d->hiddenDatasets.contains( dataset ) )
        d->hiddenDatasets.append( dataset );
    else if ( !hidden && d->hiddenDatasets.contains( dataset ) )
        d->hiddenDatasets.removeAll( dataset );
}

bool Legend::datasetIsHidden( uint dataset ) const
{
    return d->hiddenDatasets.contains( dataset );
}
