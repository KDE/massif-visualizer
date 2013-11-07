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

#include "KDChartLayoutItems.h"
#include "KDTextDocument.h"
#include "KDChartAbstractArea.h"
#include "KDChartAbstractDiagram.h"
#include "KDChartBackgroundAttributes.h"
#include "KDChartFrameAttributes.h"
#include "KDChartPaintContext.h"
#include "KDChartPainterSaver_p.h"
#include "KDChartPrintingParameters.h"
#include <QTextCursor>
#include <QTextBlockFormat>
#include <QTextDocumentFragment>
#include <QAbstractTextDocumentLayout>
#include <QLayout>
#include <QPainter>
#include <QDebug>
#include <QCoreApplication>
#include <QApplication>
#include <QStringList>
#include <QStyle>

#include <KDABLibFakes>

#include <math.h>


//#define DEBUG_ITEMS_PAINT

/**
    Inform the item about its widget: This enables the item,
    to trigger that widget's update, whenever the size of the item's
    contents has changed.

    Thus, you need to call setParentWidget on every item, that
    has a non-fixed size.
  */
void KDChart::AbstractLayoutItem::setParentWidget( QWidget* widget )
{
    mParent = widget;
}

void KDChart::AbstractLayoutItem::paintAll( QPainter& painter )
{
    paint( &painter );
}

/**
  * Default impl: Paint the complete item using its layouted position and size.
  */
void KDChart::AbstractLayoutItem::paintCtx( PaintContext* context )
{
    if ( context )
        paint( context->painter() );
}

/**
    Report changed size hint: ask the parent widget to recalculate the layout.
  */
void KDChart::AbstractLayoutItem::sizeHintChanged() const
{
    // This is exactly like what QWidget::updateGeometry does.
//  qDebug("KDChart::AbstractLayoutItem::sizeHintChanged() called");
    if ( mParent ) {
        if ( mParent->layout() )
            mParent->layout()->invalidate();
        else
            QApplication::postEvent( mParent, new QEvent( QEvent::LayoutRequest ) );
    }
}

KDChart::TextBubbleLayoutItem::TextBubbleLayoutItem( const QString& text,
                                         const KDChart::TextAttributes& attributes,
                                         const QObject* area,
                                         KDChartEnums::MeasureOrientation orientation,
                                         Qt::Alignment alignment )
    : AbstractLayoutItem( alignment ),
      m_text( new TextLayoutItem( text, attributes, area, orientation, alignment ) )
{
}

KDChart::TextBubbleLayoutItem::TextBubbleLayoutItem()
    : AbstractLayoutItem( Qt::AlignLeft ),
      m_text( new TextLayoutItem() )
{
}

KDChart::TextBubbleLayoutItem::~TextBubbleLayoutItem()
{
    delete m_text;
}

void KDChart::TextBubbleLayoutItem::setAutoReferenceArea( const QObject* area )
{
    m_text->setAutoReferenceArea( area );
}

const QObject* KDChart::TextBubbleLayoutItem::autoReferenceArea() const
{
    return m_text->autoReferenceArea();
}

void KDChart::TextBubbleLayoutItem::setText( const QString& text )
{
    m_text->setText( text );
}

QString KDChart::TextBubbleLayoutItem::text() const
{
    return m_text->text();
}

void KDChart::TextBubbleLayoutItem::setTextAttributes( const TextAttributes& a )
{
    m_text->setTextAttributes( a );
}

KDChart::TextAttributes KDChart::TextBubbleLayoutItem::textAttributes() const
{
    return m_text->textAttributes();
}

bool KDChart::TextBubbleLayoutItem::isEmpty() const
{
    return m_text->isEmpty();
}

Qt::Orientations KDChart::TextBubbleLayoutItem::expandingDirections() const
{
    return m_text->expandingDirections();
}

QSize KDChart::TextBubbleLayoutItem::maximumSize() const
{
    const int border = borderWidth();
    return m_text->maximumSize() + QSize( 2 * border, 2 * border );
}

QSize KDChart::TextBubbleLayoutItem::minimumSize() const
{
    const int border = borderWidth();
    return m_text->minimumSize() + QSize( 2 * border, 2 * border );
}

QSize KDChart::TextBubbleLayoutItem::sizeHint() const
{
    const int border = borderWidth();
    return m_text->sizeHint() + QSize( 2 * border, 2 * border );
}

void KDChart::TextBubbleLayoutItem::setGeometry( const QRect& r )
{
    const int border = borderWidth();
    m_text->setGeometry( r.adjusted( border, border, -border, -border ) );
}

QRect KDChart::TextBubbleLayoutItem::geometry() const
{
    const int border = borderWidth();
    return m_text->geometry().adjusted( -border, -border, border, border );
}

void KDChart::TextBubbleLayoutItem::paint( QPainter* painter )
{
    const QPen oldPen = painter->pen();
    const QBrush oldBrush = painter->brush();
    painter->setPen( Qt::black );
    painter->setBrush( QColor( 255, 255, 220 ) );
    painter->drawRoundRect( geometry(), 10 );
    painter->setPen( oldPen );
    painter->setBrush( oldBrush );
    m_text->paint( painter );
}

int KDChart::TextBubbleLayoutItem::borderWidth() const
{
    return 1;
}

KDChart::TextLayoutItem::TextLayoutItem( const QString& text,
                                         const KDChart::TextAttributes& attributes,
                                         const QObject* area,
                                         KDChartEnums::MeasureOrientation orientation,
                                         Qt::Alignment alignment )
    : AbstractLayoutItem( alignment )
    , mText( text )
    , mTextAlignment( alignment )
    , mAttributes( attributes )
    , mAutoReferenceArea( area )
    , mAutoReferenceOrientation( orientation )
    , cachedSizeHint() // default this to invalid to force just-in-time calculation before first use of sizeHint()
    , cachedFontSize( 0.0 )
    , cachedFont( mAttributes.font() )
{
}

KDChart::TextLayoutItem::TextLayoutItem()
    : AbstractLayoutItem( Qt::AlignLeft )
    , mText()
    , mTextAlignment( Qt::AlignLeft )
    , mAttributes()
    , mAutoReferenceArea( 0 )
    , mAutoReferenceOrientation( KDChartEnums::MeasureOrientationHorizontal )
    , cachedSizeHint() // default this to invalid to force just-in-time calculation before first use of sizeHint()
    , cachedFontSize( 0.0 )
    , cachedFont( mAttributes.font() )
{

}

void KDChart::TextLayoutItem::setAutoReferenceArea( const QObject* area )
{
    mAutoReferenceArea = area;
    cachedSizeHint = QSize();
    sizeHint();
}

const QObject* KDChart::TextLayoutItem::autoReferenceArea() const
{
    return mAutoReferenceArea;
}

void KDChart::TextLayoutItem::setText(const QString & text)
{
    mText = text;
    cachedSizeHint = QSize();
    sizeHint();
    if ( mParent )
        mParent->update();
}

QString KDChart::TextLayoutItem::text() const
{
    return mText;
}

void KDChart::TextLayoutItem::setTextAlignment( Qt::Alignment alignment)
{
    if ( mTextAlignment == alignment )
        return;
    mTextAlignment = alignment;
    if ( mParent )
        mParent->update();
}

Qt::Alignment KDChart::TextLayoutItem::textAlignment() const
{
    return mTextAlignment;
}

/**
  \brief Use this to specify the text attributes to be used for this item.

  \sa textAttributes
*/
void KDChart::TextLayoutItem::setTextAttributes( const TextAttributes &a )
{
    mAttributes = a;
    cachedFont = a.font();
    cachedSizeHint = QSize(); // invalidate size hint
    sizeHint();
    if ( mParent )
        mParent->update();
}

/**
  Returns the text attributes to be used for this item.

  \sa setTextAttributes
*/
KDChart::TextAttributes KDChart::TextLayoutItem::textAttributes() const
{
    return mAttributes;
}


Qt::Orientations KDChart::TextLayoutItem::expandingDirections() const
{
    return 0; // Grow neither vertically nor horizontally
}

QRect KDChart::TextLayoutItem::geometry() const
{
    return mRect;
}

bool KDChart::TextLayoutItem::isEmpty() const
{
    return false; // never empty, otherwise the layout item would not exist
}

QSize KDChart::TextLayoutItem::maximumSize() const
{
    return sizeHint(); // PENDING(kalle) Review, quite inflexible
}

QSize KDChart::TextLayoutItem::minimumSize() const
{
    return sizeHint(); // PENDING(kalle) Review, quite inflexible
}

void KDChart::TextLayoutItem::setGeometry( const QRect& r )
{
    mRect = r;
}

// returns the bounding box of rect rotated around its center
QRectF rotatedRect( const QRectF& rect, qreal rotation )
{
    QTransform t;
    QPointF center = rect.center();
    t.translate( center.x(), center.y() );
    t.rotate( rotation );
    t.translate( -center.x(), -center.y() );
    return t.mapRect( rect );
}

qreal KDChart::TextLayoutItem::fitFontSizeToGeometry() const
{
    QFont f = realFont();
    const qreal origResult = f.pointSizeF();
    qreal result = origResult;
    const qreal minSize = mAttributes.minimalFontSize().value();
    const QSize mySize = geometry().size();
    if ( mySize.isNull() ) {
        return result;
    }

    QFontMetrics fm( f );
    while ( true ) {
        const QSizeF textSize = rotatedRect( fm.boundingRect( mText ), mAttributes.rotation() ).normalized().size();

        if ( textSize.height() <= mySize.height() && textSize.width() <= mySize.width() ) {
            return result;
        }

        result -= 0.5;
        if ( minSize > 0 && result < minSize ) {
            return result + 0.5;
        } else if ( result <= 0.0 ) {
            return origResult;
        }
        f.setPointSizeF( result );
        fm = QFontMetrics( f );
    }
}

qreal KDChart::TextLayoutItem::realFontSize() const
{
    return mAttributes.calculatedFontSize( mAutoReferenceArea, mAutoReferenceOrientation );
}

bool KDChart::TextLayoutItem::maybeUpdateRealFont() const
{
    const qreal fntSiz = realFontSize();
    const bool doUpdate = !cachedSizeHint.isValid() || cachedFontSize != fntSiz;

    if ( doUpdate && fntSiz > 0.0 ) {
        cachedFontSize = fntSiz;
        cachedFont.setPointSizeF( fntSiz );
    }
    return doUpdate; // "didUpdate" by now
}

QFont KDChart::TextLayoutItem::realFont() const
{
    maybeUpdateRealFont();
    return cachedFont;
}

QPolygon KDChart::TextLayoutItem::boundingPolygon() const
{
    // should probably call sizeHint() here, but that one is expensive (see TODO there)
    return mCachedBoundingPolygon;
}

bool KDChart::TextLayoutItem::intersects( const TextLayoutItem& other, const QPointF& myPos, const QPointF& otherPos ) const
{
    return intersects( other, myPos.toPoint(), otherPos.toPoint() );
}

bool KDChart::TextLayoutItem::intersects( const TextLayoutItem& other, const QPoint& myPos, const QPoint& otherPos ) const
{
    if ( mAttributes.rotation() != other.mAttributes.rotation() )
    {
        // that's the code for the common case: the rotation angles don't need to match here
        QPolygon myPolygon = boundingPolygon().translated( myPos );
        QPolygon otherPolygon = other.boundingPolygon().translated( otherPos );

        // create regions out of it
        QRegion myRegion( myPolygon );
        QRegion otherRegion( otherPolygon );

        // now the question - do they intersect or not?
        return ! myRegion.intersected( otherRegion ).isEmpty();

    } else {
        // the rotation angles match so we can use a faster algorithm
        const qreal angle = DEGTORAD( mAttributes.rotation() );
        // both sizes
        const QSizeF mySize( unrotatedSizeHint() );
        const QSizeF otherSize( other.unrotatedSizeHint() );

        // that's myP1 relative to myPos
        QPointF myP1( mySize.height() * sin( angle ), 0.0 );
        // that's otherP1 to myPos
        QPointF otherP1 = QPointF( otherSize.height() * sin( angle ), 0.0 ) + otherPos - myPos;

        // now rotate both points the negative angle around myPos
        myP1 = QPointF( myP1.x() * cos( -angle ), myP1.x() * sin( -angle ) );
        qreal r = sqrt( otherP1.x() * otherP1.x() + otherP1.y() * otherP1.y() );
        if ( myP1.x() == otherP1.x() ) { // vertical
            otherP1 = QPointF( r * sin( -angle ), r * cos( -angle ) );
        } else if ( myP1.y() == otherP1.y() ) { // horizontal
            otherP1 = QPointF( r * cos( -angle ), r * sin( -angle ) );
        }

        return QRectF( myP1, mySize ).intersects( QRectF( otherP1, otherSize ) );
    }
}

QSize KDChart::TextLayoutItem::sizeHint() const
{
    // ### we only really need to recalculate the size hint when mAttributes.rotation has *changed*
    if ( maybeUpdateRealFont() || mAttributes.rotation() || !cachedSizeHint.isValid() ) {
        const QSize newSizeHint( calcSizeHint( cachedFont ) );
        Q_ASSERT( newSizeHint.isValid() );
        if ( newSizeHint != cachedSizeHint ) {
            cachedSizeHint = newSizeHint;
            sizeHintChanged();
        }
    }
    return cachedSizeHint;
}

QSize KDChart::TextLayoutItem::sizeHintUnrotated() const
{
    maybeUpdateRealFont(); // make sure the cached font is up to date
    return unrotatedSizeHint( cachedFont );
}


// PENDING(kalle) Support auto shrink


QSize KDChart::TextLayoutItem::unrotatedTextSize( QFont fnt ) const
{
    if ( fnt == QFont() ) {
        fnt = realFont(); // this is the cached font in most cases
    }

    const QFontMetricsF fm( fnt, GlobalMeasureScaling::paintDevice() );
    QRect veryLarge( 0, 0, 100000, 100000 );
    // this overload of boundingRect() interprets \n as line breaks, not as regular characters.
    return fm.boundingRect( veryLarge, Qt::AlignLeft | Qt::AlignTop, mText ).size().toSize();
}

int KDChart::TextLayoutItem::marginWidth() const
{
    return marginWidth( unrotatedTextSize() );
}

int KDChart::TextLayoutItem::marginWidth( const QSize& textSize ) const
{
    return qMin ( QApplication::style()->pixelMetric( QStyle::PM_ButtonMargin, 0, 0 ),
                  // decrease frame size if the text is small
                  textSize.height() * 2 / 3 );
}

QSize KDChart::TextLayoutItem::unrotatedSizeHint( const QFont& fnt ) const
{
    QSize ret = unrotatedTextSize( fnt );
    const int margin = marginWidth( ret );
    ret += QSize( margin, margin );
    return ret;
}

QSize KDChart::TextLayoutItem::calcSizeHint( const QFont& font ) const
{
    const QSize size = unrotatedSizeHint( font );
    QPoint topLeft( -size.width() * 0.5, -size.height() * 0.5 );
    if ( !mAttributes.rotation() ) {
        mCachedBoundingPolygon.resize( 4 );
        // using the same winding order as returned by QPolygon QTransform::mapToPolygon(const QRect&),
        // which is: 0-1: top edge, 1-2: right edge, 2-3: bottom edge, 3-0: left edge (of input rect)
        mCachedBoundingPolygon[ 0 ] = topLeft;
        mCachedBoundingPolygon[ 1 ] = topLeft + QPoint( size.width(), 0 ); // top right
        mCachedBoundingPolygon[ 2 ] = topLeft + QPoint( size.width(), size.height() ); // bottom right
        mCachedBoundingPolygon[ 3 ] = topLeft + QPoint( 0, size.height() ); // bottom left
        return size;
    }

    const QRect rect( topLeft, size );
    QTransform t;
    t.rotate( mAttributes.rotation() );
    mCachedBoundingPolygon = t.mapToPolygon( rect );

    return mCachedBoundingPolygon.boundingRect().size();
}

void KDChart::TextLayoutItem::paint( QPainter* painter )
{
    if ( !mRect.isValid() ) {
        return;
    }
    const PainterSaver painterSaver( painter );
    QFont f = realFont();
    if ( mAttributes.autoShrink() ) {
        f.setPointSizeF( fitFontSizeToGeometry() );
    }
    painter->setFont( f );

    QSize innerSize = unrotatedTextSize();
    QRectF rect = QRectF( QPointF( 0, 0 ), innerSize );
    rect.translate( -rect.center() );
    painter->translate( mRect.center() );
    painter->rotate( mAttributes.rotation() );
#ifdef DEBUG_ITEMS_PAINT
    painter->setPen( Qt::red );
    painter->drawRect( rect );
#endif

    painter->setPen( PrintingParameters::scalePen( mAttributes.pen() ) );
    QTextDocument* document = mAttributes.textDocument();
    if ( document ) {
        document->setPageSize( rect.size() );
        document->setHtml( mText );
        QAbstractTextDocumentLayout::PaintContext paintcontext;
        // ### this doesn't work for rotated painting because clip does not translate the painting
        // TODO translate the painting either using a QTransform or one of QPainter's transform stages
        paintcontext.clip = rect;
        document->documentLayout()->draw( painter, paintcontext );
    } else {
        painter->drawText( rect, mTextAlignment, mText );
    }
}

KDChart::HorizontalLineLayoutItem::HorizontalLineLayoutItem()
    : AbstractLayoutItem( Qt::AlignCenter )
{
}

Qt::Orientations KDChart::HorizontalLineLayoutItem::expandingDirections() const
{
    return Qt::Vertical|Qt::Horizontal; // Grow both vertically, and horizontally
}

QRect KDChart::HorizontalLineLayoutItem::geometry() const
{
    return mRect;
}

bool KDChart::HorizontalLineLayoutItem::isEmpty() const
{
    return false; // never empty, otherwise the layout item would not exist
}

QSize KDChart::HorizontalLineLayoutItem::maximumSize() const
{
    return QSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
}

QSize KDChart::HorizontalLineLayoutItem::minimumSize() const
{
    return QSize( 0, 0 );
}

void KDChart::HorizontalLineLayoutItem::setGeometry( const QRect& r )
{
    mRect = r;
}

QSize KDChart::HorizontalLineLayoutItem::sizeHint() const
{
    return QSize( -1, 3 ); // see qframe.cpp
}


void KDChart::HorizontalLineLayoutItem::paint( QPainter* painter )
{
    if ( !mRect.isValid() )
        return;

    painter->drawLine( QPointF( mRect.left(), mRect.center().y() ),
                       QPointF( mRect.right(), mRect.center().y() ) );
}


KDChart::VerticalLineLayoutItem::VerticalLineLayoutItem()
    : AbstractLayoutItem( Qt::AlignCenter )
{
}

Qt::Orientations KDChart::VerticalLineLayoutItem::expandingDirections() const
{
    return Qt::Vertical|Qt::Vertical; // Grow both vertically, and horizontally
}

QRect KDChart::VerticalLineLayoutItem::geometry() const
{
    return mRect;
}

bool KDChart::VerticalLineLayoutItem::isEmpty() const
{
    return false; // never empty, otherwise the layout item would not exist
}

QSize KDChart::VerticalLineLayoutItem::maximumSize() const
{
    return QSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
}

QSize KDChart::VerticalLineLayoutItem::minimumSize() const
{
    return QSize( 0, 0 );
}

void KDChart::VerticalLineLayoutItem::setGeometry( const QRect& r )
{
    mRect = r;
}

QSize KDChart::VerticalLineLayoutItem::sizeHint() const
{
    return QSize( 3, -1 ); // see qframe.cpp
}


void KDChart::VerticalLineLayoutItem::paint( QPainter* painter )
{
    if ( !mRect.isValid() )
        return;

    painter->drawLine( QPointF( mRect.center().x(), mRect.top() ),
                       QPointF( mRect.center().x(), mRect.bottom() ) );
}



KDChart::MarkerLayoutItem::MarkerLayoutItem( KDChart::AbstractDiagram* diagram,
                                             const MarkerAttributes& marker,
                                             const QBrush& brush, const QPen& pen,
                                             Qt::Alignment alignment )
    : AbstractLayoutItem( alignment )
    , mDiagram( diagram )
    , mMarker( marker )
    , mBrush( brush )
    , mPen( pen )
{
}

Qt::Orientations KDChart::MarkerLayoutItem::expandingDirections() const
{
    return 0; // Grow neither vertically nor horizontally
}

QRect KDChart::MarkerLayoutItem::geometry() const
{
    return mRect;
}

bool KDChart::MarkerLayoutItem::isEmpty() const
{
    return false; // never empty, otherwise the layout item would not exist
}

QSize KDChart::MarkerLayoutItem::maximumSize() const
{
    return sizeHint(); // PENDING(kalle) Review, quite inflexible
}

QSize KDChart::MarkerLayoutItem::minimumSize() const
{
    return sizeHint(); // PENDING(kalle) Review, quite inflexible
}

void KDChart::MarkerLayoutItem::setGeometry( const QRect& r )
{
    mRect = r;
}

QSize KDChart::MarkerLayoutItem::sizeHint() const
{
    //qDebug() << "KDChart::MarkerLayoutItem::sizeHint() returns:"<<mMarker.markerSize().toSize();
    return mMarker.markerSize().toSize();
}

void KDChart::MarkerLayoutItem::paint( QPainter* painter )
{
    paintIntoRect( painter, mRect, mDiagram, mMarker, mBrush, mPen );
}

void KDChart::MarkerLayoutItem::paintIntoRect(
        QPainter* painter,
        const QRect& rect,
        AbstractDiagram* diagram,
        const MarkerAttributes& marker,
        const QBrush& brush,
        const QPen& pen )
{
    if ( ! rect.isValid() )
        return;

    // The layout management may assign a larger rect than what we
    // wanted. We need to adjust the position.
    const QSize siz = marker.markerSize().toSize();
    QPointF pos = rect.topLeft();
    pos += QPointF( static_cast<qreal>(( rect.width()  - siz.width()) / 2.0 ),
                    static_cast<qreal>(( rect.height() - siz.height()) / 2.0 ) );

#ifdef DEBUG_ITEMS_PAINT
    QPointF oldPos = pos;
#endif

// And finally, drawMarker() assumes the position to be the center
    // of the marker, adjust again.
    pos += QPointF( static_cast<qreal>( siz.width() ) / 2.0,
                    static_cast<qreal>( siz.height() )/ 2.0 );

    diagram->paintMarker( painter, marker, brush, pen, pos.toPoint(), siz );

#ifdef DEBUG_ITEMS_PAINT
    const QPen oldPen( painter->pen() );
    painter->setPen( Qt::red );
    painter->drawRect( QRect(oldPos.toPoint(), siz) );
    painter->setPen( oldPen );
#endif
}


KDChart::LineLayoutItem::LineLayoutItem( KDChart::AbstractDiagram* diagram,
                                         int length,
                                         const QPen& pen,
                                         Qt::Alignment legendLineSymbolAlignment,
                                         Qt::Alignment alignment )
    : AbstractLayoutItem( alignment )
    , mDiagram( diagram )
    , mLength( length )
    , mPen( pen )
    , mLegendLineSymbolAlignment(legendLineSymbolAlignment)
{
    //have a mini pen width
    if ( pen.width() < 2 )
        mPen.setWidth( 2 );
}

Qt::Orientations KDChart::LineLayoutItem::expandingDirections() const
{
    return 0; // Grow neither vertically nor horizontally
}

QRect KDChart::LineLayoutItem::geometry() const
{
    return mRect;
}

bool KDChart::LineLayoutItem::isEmpty() const
{
    return false; // never empty, otherwise the layout item would not exist
}

QSize KDChart::LineLayoutItem::maximumSize() const
{
    return sizeHint(); // PENDING(kalle) Review, quite inflexible
}

QSize KDChart::LineLayoutItem::minimumSize() const
{
    return sizeHint(); // PENDING(kalle) Review, quite inflexible
}

void KDChart::LineLayoutItem::setGeometry( const QRect& r )
{
    mRect = r;
}

QSize KDChart::LineLayoutItem::sizeHint() const
{
    return QSize( mLength, mPen.width()+2 );
}


void KDChart::LineLayoutItem::setLegendLineSymbolAlignment(Qt::Alignment legendLineSymbolAlignment)
{
    if (mLegendLineSymbolAlignment == legendLineSymbolAlignment)
        return;

    mLegendLineSymbolAlignment = legendLineSymbolAlignment;
}

Qt::Alignment KDChart::LineLayoutItem::legendLineSymbolAlignment() const
{
    return mLegendLineSymbolAlignment;
}

void KDChart::LineLayoutItem::paint( QPainter* painter )
{
    paintIntoRect( painter, mRect, mPen, mLegendLineSymbolAlignment );
}

void KDChart::LineLayoutItem::paintIntoRect(
        QPainter* painter,
        const QRect& rect,
        const QPen& pen,
        Qt::Alignment lineAlignment)
{
    if ( ! rect.isValid() )
        return;

    const QPen oldPen = painter->pen();
    painter->setPen( PrintingParameters::scalePen( pen ) );
    qreal y = 0;
    if (lineAlignment == Qt::AlignTop)
        y = rect.top();
    else if (lineAlignment == Qt::AlignBottom)
        y = rect.bottom();
    else
        y = rect.center().y();

    painter->drawLine( QPointF( rect.left(), y ),
                       QPointF( rect.right(), y ) );
    painter->setPen( oldPen );
}


KDChart::LineWithMarkerLayoutItem::LineWithMarkerLayoutItem(
        KDChart::AbstractDiagram* diagram,
        int lineLength,
        const QPen& linePen,
        int markerOffs,
        const MarkerAttributes& marker,
        const QBrush& markerBrush,
        const QPen& markerPen,
        Qt::Alignment alignment )
    : AbstractLayoutItem( alignment )
    , mDiagram( diagram )
    , mLineLength( lineLength )
    , mLinePen( linePen )
    , mMarkerOffs( markerOffs )
    , mMarker( marker )
    , mMarkerBrush( markerBrush )
    , mMarkerPen( markerPen )
{
}

Qt::Orientations KDChart::LineWithMarkerLayoutItem::expandingDirections() const
{
    return 0; // Grow neither vertically nor horizontally
}

QRect KDChart::LineWithMarkerLayoutItem::geometry() const
{
    return mRect;
}

bool KDChart::LineWithMarkerLayoutItem::isEmpty() const
{
    return false; // never empty, otherwise the layout item would not exist
}

QSize KDChart::LineWithMarkerLayoutItem::maximumSize() const
{
    return sizeHint(); // PENDING(kalle) Review, quite inflexible
}

QSize KDChart::LineWithMarkerLayoutItem::minimumSize() const
{
    return sizeHint(); // PENDING(kalle) Review, quite inflexible
}

void KDChart::LineWithMarkerLayoutItem::setGeometry( const QRect& r )
{
    mRect = r;
}

QSize KDChart::LineWithMarkerLayoutItem::sizeHint() const
{
    const QSize sizeM = mMarker.markerSize().toSize();
    const QSize sizeL = QSize( mLineLength, mLinePen.width()+2 );
    return QSize( qMax(sizeM.width(),  sizeL.width()),
                  qMax(sizeM.height(), sizeL.height()) );
}

void KDChart::LineWithMarkerLayoutItem::paint( QPainter* painter )
{
    // paint the line over the full width, into the vertical middle of the rect
    LineLayoutItem::paintIntoRect( painter, mRect, mLinePen, Qt::AlignCenter );

    // paint the marker with the given offset from the left side of the line
    const QRect r(
            QPoint( mRect.x()+mMarkerOffs, mRect.y() ),
            QSize( mMarker.markerSize().toSize().width(), mRect.height() ) );
    MarkerLayoutItem::paintIntoRect(
            painter, r, mDiagram, mMarker, mMarkerBrush, mMarkerPen );
}

KDChart::AutoSpacerLayoutItem::AutoSpacerLayoutItem(
        bool layoutIsAtTopPosition, QHBoxLayout *rightLeftLayout,
        bool layoutIsAtLeftPosition, QVBoxLayout *topBottomLayout )
    : AbstractLayoutItem( Qt::AlignCenter )
    , mLayoutIsAtTopPosition( layoutIsAtTopPosition )
    , mRightLeftLayout( rightLeftLayout )
    , mLayoutIsAtLeftPosition( layoutIsAtLeftPosition )
    , mTopBottomLayout( topBottomLayout )
{
}

Qt::Orientations KDChart::AutoSpacerLayoutItem::expandingDirections() const
{
    return 0; // Grow neither vertically nor horizontally
}

QRect KDChart::AutoSpacerLayoutItem::geometry() const
{
    return mRect;
}

bool KDChart::AutoSpacerLayoutItem::isEmpty() const
{
    return true; // never empty, otherwise the layout item would not exist
}

QSize KDChart::AutoSpacerLayoutItem::maximumSize() const
{
    return sizeHint();
}

QSize KDChart::AutoSpacerLayoutItem::minimumSize() const
{
    return sizeHint();
}

void KDChart::AutoSpacerLayoutItem::setGeometry( const QRect& r )
{
    mRect = r;
}


static void updateCommonBrush( QBrush& commonBrush, bool& bStart, const KDChart::AbstractArea& area )
{
    const KDChart::BackgroundAttributes ba( area.backgroundAttributes() );
    const bool hasSimpleBrush = (
            ! area.frameAttributes().isVisible() &&
            ba.isVisible() &&
            ba.pixmapMode() == KDChart::BackgroundAttributes::BackgroundPixmapModeNone &&
            ba.brush().gradient() == 0 );
    if ( bStart ) {
        bStart = false;
        commonBrush = hasSimpleBrush ? ba.brush() : QBrush();
    } else {
        if ( ! hasSimpleBrush || ba.brush() != commonBrush )
        {
            commonBrush = QBrush();
        }
    }
}

QSize KDChart::AutoSpacerLayoutItem::sizeHint() const
{
    QBrush commonBrush;
    bool bStart=true;
    // calculate the maximal overlap of the top/bottom axes:
    int topBottomOverlap = 0;
    if ( mTopBottomLayout ) {
        for (int i = 0; i < mTopBottomLayout->count(); ++i) {
            AbstractArea* area = dynamic_cast<AbstractArea*>(mTopBottomLayout->itemAt(i));
            if ( area ) {
                //qDebug() << "AutoSpacerLayoutItem testing" << area;
                topBottomOverlap = qMax( topBottomOverlap,
                                         mLayoutIsAtLeftPosition ? area->rightOverlap()
                                                                 : area->leftOverlap() );
                updateCommonBrush( commonBrush, bStart, *area );
            }
        }
    }
    // calculate the maximal overlap of the left/right axes:
    int leftRightOverlap = 0;
    if ( mRightLeftLayout ) {
        for (int i = 0; i < mRightLeftLayout->count(); ++i) {
            AbstractArea* area = dynamic_cast<AbstractArea*>(mRightLeftLayout->itemAt(i));
            if ( area ) {
                //qDebug() << "AutoSpacerLayoutItem testing" << area;
                leftRightOverlap = qMax( leftRightOverlap,
                                         mLayoutIsAtTopPosition ? area->bottomOverlap()
                                                                : area->topOverlap() );
                updateCommonBrush( commonBrush, bStart, *area );
            }
        }
    }
    if ( topBottomOverlap > 0 && leftRightOverlap > 0 )
        mCommonBrush = commonBrush;
    else
        mCommonBrush = QBrush();
    mCachedSize = QSize( topBottomOverlap, leftRightOverlap );
    //qDebug() << mCachedSize;
    return mCachedSize;
}


void KDChart::AutoSpacerLayoutItem::paint( QPainter* painter )
{
    if ( mParentLayout && mRect.isValid() && mCachedSize.isValid() &&
        mCommonBrush.style() != Qt::NoBrush )
    {
        QPoint p1( mRect.topLeft() );
        QPoint p2( mRect.bottomRight() );
        if ( mLayoutIsAtLeftPosition )
            p1.rx() += mCachedSize.width() - mParentLayout->spacing();
        else
            p2.rx() -= mCachedSize.width() - mParentLayout->spacing();
        if ( mLayoutIsAtTopPosition ) {
            p1.ry() += mCachedSize.height() - mParentLayout->spacing() - 1;
            p2.ry() -= 1;
        } else
            p2.ry() -= mCachedSize.height() - mParentLayout->spacing() - 1;
        //qDebug() << mLayoutIsAtTopPosition << mLayoutIsAtLeftPosition;
        //qDebug() << mRect;
        //qDebug() << mParentLayout->margin();
        //qDebug() << QRect( p1, p2 );
        const QPoint oldBrushOrigin( painter->brushOrigin() );
        const QBrush oldBrush( painter->brush() );
        const QPen   oldPen( painter->pen() );
        const QPointF newTopLeft( painter->deviceMatrix().map( p1 ) );
        painter->setBrushOrigin( newTopLeft );
        painter->setBrush( mCommonBrush );
        painter->setPen( Qt::NoPen );
        painter->drawRect( QRect( p1, p2 ) );
        painter->setBrushOrigin( oldBrushOrigin );
        painter->setBrush( oldBrush );
        painter->setPen( oldPen );
    }
    // debug code:
#if 0
    //qDebug() << "KDChart::AutoSpacerLayoutItem::paint()";
    if ( !mRect.isValid() )
        return;

    painter->drawRect( mRect );
    painter->drawLine( QPointF( mRect.topLeft(), mRect.bottomRight() ) );
    painter->drawLine( QPointF( mRect.topRight(), mRect.bottomLeft() ) );
#endif
}
