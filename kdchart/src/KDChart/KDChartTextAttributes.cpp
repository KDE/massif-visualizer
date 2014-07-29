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

#include "KDChartTextAttributes.h"

#include <KDChartCartesianCoordinatePlane.h>
#include <KDChartCartesianCoordinatePlane_p.h>

#include <QFont>
#include <QPen>
#include <qglobal.h>
#include <QApplication>
#include <QSharedPointer>
#include <QTextDocument>
#include <KDABLibFakes>

#define d d_func()

using namespace KDChart;

class TextAttributes::Private
{
    friend class TextAttributes;
public:
     Private();
private:
    bool visible;
    QFont font;
    mutable QFont cachedFont;
    mutable qreal cachedFontSize;
    Measure fontSize;
    Measure minimalFontSize;
    bool autoRotate;
    bool autoShrink;
    bool hasRotation;
    int rotation;
    QPen pen;
    QSharedPointer<QTextDocument> document;
};

TextAttributes::Private::Private()
    : visible( true ),
      font( QApplication::font() ),
      cachedFontSize( -1.0 ),
      autoRotate( false ),
      autoShrink( false ),
      hasRotation( false ),
      rotation( 0 ),
      pen( Qt::black )
{
}

TextAttributes::TextAttributes()
    : _d( new Private() )
{
}

TextAttributes::TextAttributes( const TextAttributes& r )
    : _d( new Private( *r.d ) )
{

}

TextAttributes & TextAttributes::operator=( const TextAttributes& r )
{
    if ( this == &r )
        return *this;

    *d = *r.d;

    return *this;
}

TextAttributes::~TextAttributes()
{
    delete _d; _d = 0;
}


bool TextAttributes::operator==( const TextAttributes& r ) const
{
    // the following works around a bug in gcc 4.3.2
    // causing StyleHint to be set to Zero when copying a QFont
    const QFont myFont( font() );
    QFont r_font( r.font() );
    r_font.setStyleHint( myFont.styleHint(), myFont.styleStrategy() );
    return ( isVisible() == r.isVisible() &&
             myFont == r_font &&
             fontSize() == r.fontSize() &&
             minimalFontSize() == r.minimalFontSize() &&
             autoRotate() == r.autoRotate() &&
             autoShrink() == r.autoShrink() &&
             rotation() == r.rotation() &&
             pen() == r.pen() &&
             textDocument() == r.textDocument() );
}


void TextAttributes::setVisible( bool visible )
{
    d->visible = visible;
}

bool TextAttributes::isVisible() const
{
    return d->visible;
}

void TextAttributes::setFont( const QFont& font )
{
    d->font       = font;
    d->cachedFont = font; // note: we do not set the font's size here, but in calculatedFont()
    d->cachedFontSize = -1.0;
}

QFont TextAttributes::font() const
{
    return d->font;
}

void TextAttributes::setFontSize( const Measure & measure )
{
    d->fontSize = measure;
}

Measure TextAttributes::fontSize() const
{
    return d->fontSize;
}

void TextAttributes::setMinimalFontSize( const Measure & measure )
{
    d->minimalFontSize = measure;
}

Measure TextAttributes::minimalFontSize() const
{
    return d->minimalFontSize;
}

bool TextAttributes::hasAbsoluteFontSize() const
{
    return d->fontSize.calculationMode() == KDChartEnums::MeasureCalculationModeAbsolute
        && d->minimalFontSize.calculationMode() == KDChartEnums::MeasureCalculationModeAbsolute;
}

qreal TextAttributes::calculatedFontSize( const QSizeF &referenceSize,
                                          KDChartEnums::MeasureOrientation autoReferenceOrientation ) const
{
    const qreal normalSize  = fontSize().calculatedValue( referenceSize, autoReferenceOrientation );
    const qreal minimalSize = minimalFontSize().calculatedValue( referenceSize, autoReferenceOrientation );
    return qMax( normalSize, minimalSize );
}

#if QT_VERSION < 0x040400 || defined(Q_COMPILER_MANGLES_RETURN_TYPE)
const
#endif
qreal TextAttributes::calculatedFontSize( const QObject* autoReferenceArea,
                                          KDChartEnums::MeasureOrientation autoReferenceOrientation ) const
{
    const qreal normalSize  = fontSize().calculatedValue( autoReferenceArea, autoReferenceOrientation );
    const qreal minimalSize = minimalFontSize().calculatedValue( autoReferenceArea, autoReferenceOrientation );
    return qMax( normalSize, minimalSize );
}

const QFont TextAttributes::calculatedFont( const QObject* autoReferenceArea,
                                            KDChartEnums::MeasureOrientation autoReferenceOrientation ) const
{
    qreal size = NaN;

    const CartesianCoordinatePlane* plane = qobject_cast< const CartesianCoordinatePlane* >( autoReferenceArea );
    if ( plane  && plane->hasFixedDataCoordinateSpaceRelation() ) {
        // HACK
        // if hasFixedDataCoordinateSpaceRelation, we use a zoom trick to keep the diagram at a constant size
        // even when the plane size changes. calculatedFontSize() usually uses the plane size, not the diagram
        // size, to determine the font size. here we need to give it the diagram size in order to make the font
        // size constant, too. see KDCHDEV-219.
        CartesianCoordinatePlane::Private *priv
            = CartesianCoordinatePlane::Private::get( const_cast< CartesianCoordinatePlane * >( plane ) );
        size = calculatedFontSize( priv->fixedDataCoordinateSpaceRelationPinnedSize,
                                   autoReferenceOrientation );
    } else {
        size = calculatedFontSize( autoReferenceArea, autoReferenceOrientation );
    }

    if ( size > 0.0 && d->cachedFontSize != size ) {
        d->cachedFontSize = size;
        d->cachedFont.setPointSizeF( d->cachedFontSize );
    }

    return d->cachedFont;
}


void TextAttributes::setAutoRotate( bool autoRotate )
{
    d->autoRotate = autoRotate;
}

bool TextAttributes::autoRotate() const
{
    return d->autoRotate;
}

void TextAttributes::setAutoShrink( bool autoShrink )
{
    d->autoShrink = autoShrink;
}

bool TextAttributes::autoShrink() const
{
    return d->autoShrink;
}

void TextAttributes::setRotation( int rotation )
{
    d->hasRotation = true;
    d->rotation = rotation;
}

int TextAttributes::rotation() const
{
    return d->rotation;
}

void TextAttributes::resetRotation()
{
    d->hasRotation = false;
    d->rotation = 0;
}

bool TextAttributes::hasRotation() const
{
    return d->hasRotation;
}

void TextAttributes::setPen( const QPen& pen )
{
    d->pen = pen;
}

QPen TextAttributes::pen() const
{
    return d->pen;
}

QTextDocument* TextAttributes::textDocument() const
{
    return d->document.data();
}

void TextAttributes::setTextDocument(QTextDocument* document)
{
    d->document = QSharedPointer<QTextDocument>(document);
}

#if !defined(QT_NO_DEBUG_STREAM)
QDebug operator<<(QDebug dbg, const KDChart::TextAttributes& ta)
{
    dbg << "KDChart::TextAttributes("
    << "visible=" << ta.isVisible()
    << "font=" << ta.font().toString() /* What? No QDebug for QFont? */
    << "fontsize=" << ta.fontSize()
    << "minimalfontsize=" << ta.minimalFontSize()
    << "autorotate=" << ta.autoRotate()
    << "autoshrink=" << ta.autoShrink()
    << "rotation=" << ta.rotation()
    << "pen=" << ta.pen()
    << ")";
    return dbg;
}
#endif /* QT_NO_DEBUG_STREAM */
