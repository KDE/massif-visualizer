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

#include "KDChartDataValueAttributes.h"

#include <QVariant>
#include <QDebug>
#include "KDChartRelativePosition.h"
#include "KDChartPosition.h"
#include <KDChartTextAttributes.h>
#include <KDChartFrameAttributes.h>
#include <KDChartBackgroundAttributes.h>
#include <KDChartMarkerAttributes.h>

#include <KDABLibFakes>

// FIXME till
#define KDCHART_DATA_VALUE_AUTO_DIGITS 4


#define d d_func()

using namespace KDChart;

class DataValueAttributes::Private
{
    friend class DataValueAttributes;
public:
    Private();
private:
    TextAttributes textAttributes;
    FrameAttributes frameAttributes;
    BackgroundAttributes backgroundAttributes;
    MarkerAttributes markerAttributes;
    QString prefix;
    QString suffix;
    QString dataLabel;
    RelativePosition negativeRelPos;
    RelativePosition positiveRelPos;
    qint16 decimalDigits;
    qint16 powerOfTenDivisor;
    bool visible : 1;
    bool showInfinite : 1;
    bool showRepetitiveDataLabels : 1;
    bool showOverlappingDataLabels : 1;
    bool usePercentage : 1;
    bool mirrorNegativeValueTextRotation : 1;
};

DataValueAttributes::Private::Private() :
    decimalDigits( KDCHART_DATA_VALUE_AUTO_DIGITS ),
    powerOfTenDivisor( 0 ),
    visible( false ),
    showInfinite( true )
{
    Measure me( 20.0, KDChartEnums::MeasureCalculationModeAuto, KDChartEnums::MeasureOrientationAuto );
    textAttributes.setFontSize( me );
    me.setValue( 8.0 );
    me.setCalculationMode( KDChartEnums::MeasureCalculationModeAbsolute );
    textAttributes.setMinimalFontSize( me );
    textAttributes.setRotation( -45 );

    // we set the Position to unknown: so the diagrams can take their own decisions
    positiveRelPos.setReferencePosition( Position::Unknown );
    negativeRelPos.setReferencePosition( Position::Unknown );

    positiveRelPos.setAlignment( Qt::AlignTop | Qt::AlignRight );
    negativeRelPos.setAlignment( Qt::AlignBottom | Qt::AlignRight );

    showRepetitiveDataLabels = false;
    showOverlappingDataLabels = false;

    usePercentage = false;
    mirrorNegativeValueTextRotation = false;
}


DataValueAttributes::DataValueAttributes()
    : _d( new Private() )
{
}

DataValueAttributes::DataValueAttributes( const DataValueAttributes& r )
    : _d( new Private( *r.d ) )
{
}

DataValueAttributes & DataValueAttributes::operator=( const DataValueAttributes& r )
{
    if ( this == &r )
        return *this;

    *d = *r.d;

    return *this;
}

DataValueAttributes::~DataValueAttributes()
{
    delete _d; _d = 0;
}


bool DataValueAttributes::operator==( const DataValueAttributes& r ) const
{
    return  isVisible() == r.isVisible() &&
            textAttributes() == r.textAttributes() &&
            frameAttributes() == r.frameAttributes() &&
            backgroundAttributes() == r.backgroundAttributes() &&
            markerAttributes() == r.markerAttributes() &&
            decimalDigits() == r.decimalDigits() &&
            prefix() == r.prefix() &&
            suffix() == r.suffix() &&
            dataLabel() == r.dataLabel() &&
            powerOfTenDivisor() == r.powerOfTenDivisor() &&
            showInfinite() == r.showInfinite() &&
            negativePosition() == r.negativePosition() &&
            positivePosition() == r.positivePosition() &&
            showRepetitiveDataLabels() == r.showRepetitiveDataLabels() &&
            showOverlappingDataLabels() == r.showOverlappingDataLabels() &&
            usePercentage() == r.usePercentage() &&
            mirrorNegativeValueTextRotation() == r.mirrorNegativeValueTextRotation();
}

/*static*/
const DataValueAttributes& DataValueAttributes::defaultAttributes()
{
    static const DataValueAttributes theDefaultDataValueAttributes;
    return theDefaultDataValueAttributes;
}

/*static*/
const QVariant& DataValueAttributes::defaultAttributesAsVariant()
{
    static const QVariant theDefaultDataValueAttributesVariant = qVariantFromValue(defaultAttributes());
    return theDefaultDataValueAttributesVariant;
}


void DataValueAttributes::setVisible( bool visible )
{
    d->visible = visible;
}

bool DataValueAttributes::isVisible() const
{
    return d->visible;
}

void DataValueAttributes::setTextAttributes( const TextAttributes &a )
{
    d->textAttributes = a;
}

TextAttributes DataValueAttributes::textAttributes() const
{
    return d->textAttributes;
}

void DataValueAttributes::setFrameAttributes( const FrameAttributes &a )
{
    d->frameAttributes = a;
}

FrameAttributes DataValueAttributes::frameAttributes() const
{
    return d->frameAttributes;
}

void DataValueAttributes::setBackgroundAttributes( const BackgroundAttributes &a )
{
    d->backgroundAttributes = a;
}

BackgroundAttributes DataValueAttributes::backgroundAttributes() const
{
    return d->backgroundAttributes;
}

void DataValueAttributes::setMarkerAttributes( const MarkerAttributes &a )
{
    d->markerAttributes = a;
}

MarkerAttributes DataValueAttributes::markerAttributes() const
{
    return d->markerAttributes;
}

void DataValueAttributes::setMirrorNegativeValueTextRotation( bool enable )
{
    d->mirrorNegativeValueTextRotation = enable;
}

bool DataValueAttributes::mirrorNegativeValueTextRotation() const
{
    return d->mirrorNegativeValueTextRotation;
}

void DataValueAttributes::setUsePercentage( bool enable )
{
    d->usePercentage = enable;
}

bool DataValueAttributes::usePercentage() const
{
    return d->usePercentage;
}

void DataValueAttributes::setDecimalDigits( int digits )
{
    d->decimalDigits = digits;
}

int DataValueAttributes::decimalDigits() const
{
    return d->decimalDigits;
}

void DataValueAttributes::setPrefix( const QString prefixString )
{
    d->prefix = prefixString;
}

QString DataValueAttributes::prefix() const
{
    return d->prefix;
}

void DataValueAttributes::setSuffix( const QString suffixString )
{
    d->suffix  = suffixString;
}

QString DataValueAttributes::suffix() const
{
    return d->suffix;
}

void DataValueAttributes::setDataLabel( const QString label )
{
    d->dataLabel =  label;
}

QString DataValueAttributes::dataLabel() const
{
    return d->dataLabel;
}

bool DataValueAttributes::showRepetitiveDataLabels() const
{
    return d->showRepetitiveDataLabels;
}

void DataValueAttributes::setShowRepetitiveDataLabels( bool showRepetitiveDataLabels )
{
    d->showRepetitiveDataLabels = showRepetitiveDataLabels;
}

bool DataValueAttributes::showOverlappingDataLabels() const
{
    return d->showOverlappingDataLabels;
}

void DataValueAttributes::setShowOverlappingDataLabels( bool showOverlappingDataLabels )
{
    d->showOverlappingDataLabels = showOverlappingDataLabels;
}

void DataValueAttributes::setPowerOfTenDivisor( int powerOfTenDivisor )
{
    d->powerOfTenDivisor = powerOfTenDivisor;
}

int DataValueAttributes::powerOfTenDivisor() const
{
    return d->powerOfTenDivisor;
}

void DataValueAttributes::setShowInfinite( bool infinite )
{
    d->showInfinite = infinite;
}

bool DataValueAttributes::showInfinite() const
{
    return d->showInfinite;
}

void DataValueAttributes::setNegativePosition( const RelativePosition& relPosition )
{
    d->negativeRelPos = relPosition;
}

const RelativePosition DataValueAttributes::negativePosition() const
{
    return d->negativeRelPos;
}

void DataValueAttributes::setPositivePosition( const RelativePosition& relPosition )
{
    d->positiveRelPos = relPosition;
}

const RelativePosition DataValueAttributes::positivePosition() const
{
    return d->positiveRelPos;
}

#if !defined(QT_NO_DEBUG_STREAM)
QDebug operator<<(QDebug dbg, const KDChart::DataValueAttributes& val )
{
    dbg << "RelativePosition DataValueAttributes("
	<< "visible="<<val.isVisible()
	<< "textattributes="<<val.textAttributes()
	<< "frameattributes="<<val.frameAttributes()
	<< "backgroundattributes="<<val.backgroundAttributes()
	<< "decimaldigits="<<val.decimalDigits()
	<< "poweroftendivisor="<<val.powerOfTenDivisor()
	<< "showinfinite="<<val.showInfinite()
	<< "negativerelativeposition="<<val.negativePosition()
	<< "positiverelativeposition="<<val.positivePosition()
    << "showRepetitiveDataLabels="<<val.showRepetitiveDataLabels()
    << "showOverlappingDataLabels="<<val.showOverlappingDataLabels()
    <<")";
    return dbg;
}
#endif /* QT_NO_DEBUG_STREAM */
