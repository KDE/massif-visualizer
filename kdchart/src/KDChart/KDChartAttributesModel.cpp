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

#include "KDChartAttributesModel.h"
#include "KDChartPalette.h"
#include "KDChartGlobal.h"

#include <QDebug>
#include <QPen>
#include <QPointer>

#include <KDChartTextAttributes.h>
#include <KDChartFrameAttributes.h>
#include <KDChartBackgroundAttributes.h>
#include <KDChartDataValueAttributes.h>
#include <KDChartMarkerAttributes.h>
#include <KDChartBarAttributes.h>
#include <KDChartStockBarAttributes.h>
#include <KDChartLineAttributes.h>
#include <KDChartPieAttributes.h>
#include <KDChartAbstractThreeDAttributes.h>
#include <KDChartThreeDBarAttributes.h>
#include <KDChartThreeDLineAttributes.h>
#include <KDChartThreeDPieAttributes.h>
#include <KDChartGridAttributes.h>
#include <KDChartValueTrackerAttributes.h>

#include <KDABLibFakes>


using namespace KDChart;


class AttributesModel::Private
{
public:
    Private();

    QMap< int, QMap< int, QMap< int, QVariant > > > dataMap;
    QMap< int, QMap< int, QVariant > > horizontalHeaderDataMap;
    QMap< int, QMap< int, QVariant > > verticalHeaderDataMap;
    QMap< int, QVariant > modelDataMap;
    QMap< int, QVariant > defaultsMap;
    int dataDimension;
    AttributesModel::PaletteType paletteType;
    Palette palette;
};

AttributesModel::Private::Private()
  : dataDimension( 1 ),
    paletteType( AttributesModel::PaletteTypeDefault ),
    palette( Palette::defaultPalette() )
{
}

#define d d_func()

AttributesModel::AttributesModel( QAbstractItemModel* model, QObject * parent/* = 0 */ )
  : AbstractProxyModel( parent ),
    _d( new Private )
{
    setSourceModel( model );
    setDefaultForRole( KDChart::DataValueLabelAttributesRole,
                       DataValueAttributes::defaultAttributesAsVariant() );
}

AttributesModel::~AttributesModel()
{
    delete _d;
    _d = 0;
}

void AttributesModel::initFrom( const AttributesModel* other )
{
    *d = *other->d;
}

bool AttributesModel::compareHeaderDataMaps( const QMap< int, QMap< int, QVariant > >& mapA,
                                             const QMap< int, QMap< int, QVariant > >& mapB ) const
{
    if ( mapA.count() != mapB.count() ) {
        return false;
    }
    QMap< int, QMap< int, QVariant > >::const_iterator itA = mapA.constBegin();
    QMap< int, QMap< int, QVariant > >::const_iterator itB = mapB.constBegin();
    for ( ; itA != mapA.constEnd(); ++itA, ++itB ) {
        if ( itA->count() != itB->count() ) {
            return false;
        }
        QMap< int, QVariant >::const_iterator it2A = itA->constBegin();
        QMap< int, QVariant >::const_iterator it2B = itB->constBegin();
        for ( ; it2A != itA->constEnd(); ++it2A, ++it2B ) {
            if ( it2A.key() != it2B.key() ) {
                return false;
            }
            if ( !compareAttributes( it2A.key(), it2A.value(), it2B.value() ) ) {
                return false;
            }
        }
    }
    return true;
}

bool AttributesModel::compare( const AttributesModel* other ) const
{
    if ( other == this ) {
        return true;
    }
    if ( !other || d->paletteType != other->d->paletteType ) {
        return false;
    }

    {
        if ( d->dataMap.count() != other->d->dataMap.count() ) {
            return false;
        }
        QMap< int, QMap< int, QMap<int, QVariant > > >::const_iterator itA = d->dataMap.constBegin();
        QMap< int, QMap< int, QMap<int, QVariant > > >::const_iterator itB = other->d->dataMap.constBegin();
        for ( ; itA != d->dataMap.constEnd(); ++itA, ++itB ) {
            if ( itA->count() != itB->count() ) {
                return false;
            }
            QMap< int, QMap< int, QVariant > >::const_iterator it2A = itA->constBegin();
            QMap< int, QMap< int, QVariant > >::const_iterator it2B = itB->constBegin();
            for ( ; it2A != itA->constEnd(); ++it2A, ++it2B ) {
                if ( it2A->count() != it2B->count() ) {
                    return false;
                }
                QMap< int, QVariant >::const_iterator it3A = it2A->constBegin();
                QMap< int, QVariant >::const_iterator it3B = it2B->constBegin();
                for ( ; it3A != it2A->constEnd(); ++it3A, ++it3B ) {
                    if ( it3A.key() != it3B.key() ) {
                        return false;
                    }
                    if ( !compareAttributes( it3A.key(), it3A.value(), it3B.value() ) ) {
                        return false;
                    }
                }
            }
        }
    }

    if ( !compareHeaderDataMaps( d->horizontalHeaderDataMap, other->d->horizontalHeaderDataMap ) ||
         !compareHeaderDataMaps( d->verticalHeaderDataMap, other->d->verticalHeaderDataMap ) ) {
        return false;
    }

    {
        if ( d->modelDataMap.count() != other->d->modelDataMap.count() ) {
            return false;
        }
        QMap< int, QVariant >::const_iterator itA = d->modelDataMap.constBegin();
        QMap< int, QVariant >::const_iterator itB = other->d->modelDataMap.constBegin();
        for ( ; itA != d->modelDataMap.constEnd(); ++itA, ++itB ) {
            if ( itA.key() != itB.key() ) {
                return false;
            }
            if ( !compareAttributes( itA.key(), itA.value(), itB.value() ) ) {
                return false;
            }
        }
    }
    return true;
}

bool AttributesModel::compareAttributes(
        int role, const QVariant& a, const QVariant& b ) const
{
    if ( isKnownAttributesRole( role ) ) {
        switch ( role ) {
            case DataValueLabelAttributesRole:
                return (a.value<DataValueAttributes>() ==
                        b.value<DataValueAttributes>());
            case DatasetBrushRole:
                return (a.value<QBrush>() ==
                        b.value<QBrush>());
            case DatasetPenRole:
                return (a.value<QPen>() ==
                        b.value<QPen>());
            case ThreeDAttributesRole:
                // As of yet there is no ThreeDAttributes class,
                // and the AbstractThreeDAttributes class is pure virtual,
                // so we ignore this role for now.
                // (khz, 04.04.2007)
                /*
                return (qVariantValue<ThreeDAttributes>( a ) ==
                        qVariantValue<ThreeDAttributes>( b ));
                */
                break;
            case LineAttributesRole:
                return (a.value<LineAttributes>() ==
                        b.value<LineAttributes>());
            case ThreeDLineAttributesRole:
                return (a.value<ThreeDLineAttributes>() ==
                        b.value<ThreeDLineAttributes>());
            case BarAttributesRole:
                return (a.value<BarAttributes>() ==
                        b.value<BarAttributes>());
            case StockBarAttributesRole:
                return (a.value<StockBarAttributes>() ==
                        b.value<StockBarAttributes>());
            case ThreeDBarAttributesRole:
                return (a.value<ThreeDBarAttributes>() ==
                        b.value<ThreeDBarAttributes>());
            case PieAttributesRole:
                return (a.value<PieAttributes>() ==
                        b.value<PieAttributes>());
            case ThreeDPieAttributesRole:
                return (a.value<ThreeDPieAttributes>() ==
                        b.value<ThreeDPieAttributes>());
            case ValueTrackerAttributesRole:
                return (a.value<ValueTrackerAttributes>() ==
                        b.value<ValueTrackerAttributes>());
            case DataHiddenRole:
                return (a.value<bool>() ==
                        b.value<bool>());
            default:
                Q_ASSERT( false ); // all of our own roles need to be handled
                break;
        }
    } else {
        return (a == b);
    }
    return true;
}


QVariant AttributesModel::headerData( int section, Qt::Orientation orientation,
                                      int role/* = Qt::DisplayRole */ ) const
{
    if ( sourceModel() ) {
        const QVariant sourceData = sourceModel()->headerData( section, orientation, role );
        if ( sourceData.isValid() ) {
            return sourceData;
        }
    }

    // the source model didn't have data set, let's use our stored values
    const QMap< int, QMap< int, QVariant> >& map = orientation == Qt::Horizontal ?
                                                   d->horizontalHeaderDataMap : d->verticalHeaderDataMap;
    QMap< int, QMap< int, QVariant > >::const_iterator mapIt = map.find( section );
    if ( mapIt != map.constEnd() ) {
        const QMap< int, QVariant >& dataMap = mapIt.value();
        QMap< int, QVariant >::const_iterator dataMapIt = dataMap.find( role );
        if ( dataMapIt != dataMap.constEnd() ) {
            return dataMapIt.value();
        }
    }

    return defaultHeaderData( section, orientation, role );
}


QVariant AttributesModel::defaultHeaderData( int section, Qt::Orientation orientation, int role ) const
{
    // Default values if nothing else matches

    const int dataset = section / d->dataDimension;

    switch ( role ) {
    case Qt::DisplayRole:
        //TODO for KDChart 3.0: return QString::number( dataset + 1 );
        return QLatin1String( orientation == Qt::Vertical ?  "Series " : "Item " ) + QString::number( dataset ) ;
    case KDChart::DatasetBrushRole:
        return d->palette.getBrush( dataset );
    case KDChart::DatasetPenRole:
        // if no per model override was set, use the (possibly default) color set for the brush
        if ( !modelData( role ).isValid() ) {
            QBrush brush = headerData( section, orientation, DatasetBrushRole ).value< QBrush >();
            return QPen( brush.color() );
        }
    default:
        break;
    }

    return QVariant();
}

// Note: Our users NEED this method - even if
//       we do not need it at drawing time!
//       (khz, 2006-07-28)
QVariant AttributesModel::data( int role ) const
{
  if ( isKnownAttributesRole( role ) ) {
      // check if there is something set at global level
      QVariant v = modelData( role );

      // else return the default setting, if any
      if ( !v.isValid() )
          v = defaultsForRole( role );
      return v;
  }
  return QVariant();
}


// Note: Our users NEED this method - even if
//       we do not need it at drawing time!
//       (khz, 2006-07-28)
QVariant AttributesModel::data( int column, int role ) const
{
  if ( isKnownAttributesRole( role ) ) {
      // check if there is something set for the column (dataset)
      QVariant v;
      v = headerData( column, Qt::Horizontal, role );

      // check if there is something set at global level
      if ( !v.isValid() )
          v = data( role ); // includes automatic fallback to default
      return v;
  }
  return QVariant();
}


QVariant AttributesModel::data( const QModelIndex& index, int role ) const
{
    if ( index.isValid() ) {
        Q_ASSERT( index.model() == this );
    }
    if ( !sourceModel() ) {
        return QVariant();
    }

    if ( index.isValid() ) {
        const QVariant sourceData = sourceModel()->data( mapToSource( index ), role );
        if ( sourceData.isValid() ) {
            return sourceData;
        }
    }

    // check if we are storing a value for this role at this cell index
    if ( d->dataMap.contains( index.column() ) ) {
        const QMap< int,  QMap< int, QVariant > >& colDataMap = d->dataMap[ index.column() ];
        if ( colDataMap.contains( index.row() ) ) {
            const QMap< int, QVariant >& dataMap = colDataMap[ index.row() ];
            if ( dataMap.contains( role ) ) {
                const QVariant v = dataMap[ role ];
                if ( v.isValid() ) {
                    return v;
                }
            }
        }
    }
    // check if there is something set for the column (dataset), or at global level
    if ( index.isValid() ) {
        return data( index.column(), role ); // includes automatic fallback to default
    }

    return QVariant();
}


bool AttributesModel::isKnownAttributesRole( int role ) const
{
    switch ( role ) {
        // fallthrough intended
    case DataValueLabelAttributesRole:
    case DatasetBrushRole:
    case DatasetPenRole:
    case ThreeDAttributesRole:
    case LineAttributesRole:
    case ThreeDLineAttributesRole:
    case BarAttributesRole:
    case StockBarAttributesRole:
    case ThreeDBarAttributesRole:
    case PieAttributesRole:
    case ThreeDPieAttributesRole:
    case ValueTrackerAttributesRole:
    case DataHiddenRole:
        return true;
    default:
        return false;
    }
}

QVariant AttributesModel::defaultsForRole( int role ) const
{
    // returns default-constructed QVariant if not found
    return d->defaultsMap.value( role );
}

bool AttributesModel::setData ( const QModelIndex & index, const QVariant & value, int role )
{
    if ( !isKnownAttributesRole( role ) ) {
        return sourceModel()->setData( mapToSource(index), value, role );
    } else {
        QMap< int,  QMap< int, QVariant> > &colDataMap = d->dataMap[ index.column() ];
        QMap< int, QVariant > &dataMap = colDataMap[ index.row() ];
        dataMap.insert( role, value );
        emit attributesChanged( index, index );
        return true;
    }
}

bool AttributesModel::resetData ( const QModelIndex & index, int role )
{
    return setData( index, QVariant(), role );
}

bool AttributesModel::setHeaderData ( int section, Qt::Orientation orientation,
                                      const QVariant & value, int role )
{
    if ( sourceModel() && headerData( section, orientation, role ) == value ) {
        return true;
    }

    if ( !isKnownAttributesRole( role ) ) {
        return sourceModel()->setHeaderData( section, orientation, value, role );
    } else {
        QMap< int,  QMap<int, QVariant > > &sectionDataMap
            = orientation == Qt::Horizontal ? d->horizontalHeaderDataMap : d->verticalHeaderDataMap;

        QMap< int, QVariant > &dataMap = sectionDataMap[ section ];
        dataMap.insert( role, value );
        if ( sourceModel() ) {
            int numRows = rowCount( QModelIndex() );
            int numCols = columnCount( QModelIndex() );
            if ( orientation == Qt::Horizontal && numRows > 0 )
                emit attributesChanged( index( 0, section, QModelIndex() ),
                                        index( numRows - 1, section, QModelIndex() ) );
            else if ( orientation == Qt::Vertical && numCols > 0 )
                emit attributesChanged( index( section, 0, QModelIndex() ),
                                        index( section, numCols - 1, QModelIndex() ) );
            emit headerDataChanged( orientation, section, section );

            // FIXME: This only makes sense for orientation == Qt::Horizontal,
            // but what if orientation == Qt::Vertical?
            if ( section != -1 && numRows > 0 )
                emit dataChanged( index( 0, section, QModelIndex() ),
                                  index( numRows - 1, section, QModelIndex() ) );
        }
        return true;
    }
}

bool AttributesModel::resetHeaderData ( int section, Qt::Orientation orientation, int role )
{
    return setHeaderData ( section, orientation, QVariant(), role );
}

void AttributesModel::setPaletteType( AttributesModel::PaletteType type )
{
    if ( d->paletteType == type ) {
        return;
    }
    d->paletteType = type;
    switch ( type ) {
    case PaletteTypeDefault:
        d->palette = Palette::defaultPalette();
        break;
    case PaletteTypeSubdued:
        d->palette = Palette::subduedPalette();
        break;
    case PaletteTypeRainbow:
        d->palette = Palette::rainbowPalette();
        break;
    default:
        qWarning( "Unknown palette type!" );
    }
}

AttributesModel::PaletteType AttributesModel::paletteType() const
{
    return d->paletteType;
}

bool KDChart::AttributesModel::setModelData( const QVariant value, int role )
{
    d->modelDataMap.insert( role, value );
    int numRows = rowCount( QModelIndex() );
    int numCols = columnCount( QModelIndex() );
    if ( sourceModel() && numRows > 0 && numCols > 0 ) {
        emit attributesChanged( index( 0, 0, QModelIndex() ),
                                index( numRows - 1, numCols - 1, QModelIndex() ) );
        beginResetModel();
	endResetModel();
    }
    return true;
}

QVariant KDChart::AttributesModel::modelData( int role ) const
{
    return d->modelDataMap.value( role, QVariant() );
}

int AttributesModel::rowCount( const QModelIndex& index ) const
{
    if ( sourceModel() ) {
        return sourceModel()->rowCount( mapToSource(index) );
    } else {
        return 0;
    }
}

int AttributesModel::columnCount( const QModelIndex& index ) const
{
    if ( sourceModel() ) {
        return sourceModel()->columnCount( mapToSource(index) );
    } else {
        return 0;
    }
}

void AttributesModel::setSourceModel( QAbstractItemModel* sourceModel )
{
    if ( this->sourceModel() != 0 )
    {
        disconnect( this->sourceModel(), SIGNAL( dataChanged( const QModelIndex&, const QModelIndex&)),
                                   this, SLOT( slotDataChanged( const QModelIndex&, const QModelIndex&)));
        disconnect( this->sourceModel(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ),
                                   this, SLOT( slotRowsInserted( const QModelIndex&, int, int ) ) );
        disconnect( this->sourceModel(), SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ),
                                   this, SLOT( slotRowsRemoved( const QModelIndex&, int, int ) ) );
        disconnect( this->sourceModel(), SIGNAL( rowsAboutToBeInserted( const QModelIndex&, int, int ) ),
                                   this, SLOT( slotRowsAboutToBeInserted( const QModelIndex&, int, int ) ) );
        disconnect( this->sourceModel(), SIGNAL( rowsAboutToBeRemoved( const QModelIndex&, int, int ) ),
                                   this, SLOT( slotRowsAboutToBeRemoved( const QModelIndex&, int, int ) ) );
        disconnect( this->sourceModel(), SIGNAL( columnsInserted( const QModelIndex&, int, int ) ),
                                   this, SLOT( slotColumnsInserted( const QModelIndex&, int, int ) ) );
        disconnect( this->sourceModel(), SIGNAL( columnsRemoved( const QModelIndex&, int, int ) ),
                                   this, SLOT( slotColumnsRemoved( const QModelIndex&, int, int ) ) );
        disconnect( this->sourceModel(), SIGNAL( columnsAboutToBeInserted( const QModelIndex&, int, int ) ),
                                   this, SLOT( slotColumnsAboutToBeInserted( const QModelIndex&, int, int ) ) );
        disconnect( this->sourceModel(), SIGNAL( columnsAboutToBeRemoved( const QModelIndex&, int, int ) ),
                                   this, SLOT( slotColumnsAboutToBeRemoved( const QModelIndex&, int, int ) ) );
        disconnect( this->sourceModel(), SIGNAL( modelReset() ),
                                   this, SIGNAL( modelReset() ) );
        disconnect( this->sourceModel(), SIGNAL( layoutChanged() ),
                                   this, SIGNAL( layoutChanged() ) );
    }
    QAbstractProxyModel::setSourceModel( sourceModel );
    if ( this->sourceModel() != NULL )
    {
        connect( this->sourceModel(), SIGNAL( dataChanged( const QModelIndex&, const QModelIndex&)),
                                this, SLOT( slotDataChanged( const QModelIndex&, const QModelIndex&)));
        connect( this->sourceModel(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ),
                                this, SLOT( slotRowsInserted( const QModelIndex&, int, int ) ) );
        connect( this->sourceModel(), SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ),
                                this, SLOT( slotRowsRemoved( const QModelIndex&, int, int ) ) );
        connect( this->sourceModel(), SIGNAL( rowsAboutToBeInserted( const QModelIndex&, int, int ) ),
                                this, SLOT( slotRowsAboutToBeInserted( const QModelIndex&, int, int ) ) );
        connect( this->sourceModel(), SIGNAL( rowsAboutToBeRemoved( const QModelIndex&, int, int ) ),
                                this, SLOT( slotRowsAboutToBeRemoved( const QModelIndex&, int, int ) ) );
        connect( this->sourceModel(), SIGNAL( columnsInserted( const QModelIndex&, int, int ) ),
                                this, SLOT( slotColumnsInserted( const QModelIndex&, int, int ) ) );
        connect( this->sourceModel(), SIGNAL( columnsRemoved( const QModelIndex&, int, int ) ),
                                this, SLOT( slotColumnsRemoved( const QModelIndex&, int, int ) ) );
        connect( this->sourceModel(), SIGNAL( columnsAboutToBeInserted( const QModelIndex&, int, int ) ),
                                this, SLOT( slotColumnsAboutToBeInserted( const QModelIndex&, int, int ) ) );
        connect( this->sourceModel(), SIGNAL( columnsAboutToBeRemoved( const QModelIndex&, int, int ) ),
                                this, SLOT( slotColumnsAboutToBeRemoved( const QModelIndex&, int, int ) ) );
        connect( this->sourceModel(), SIGNAL( modelReset() ),
                                this, SIGNAL( modelReset() ) );
        connect( this->sourceModel(), SIGNAL( layoutChanged() ),
                                this, SIGNAL( layoutChanged() ) );
    }
}

void AttributesModel::slotRowsAboutToBeInserted( const QModelIndex& parent, int start, int end )
{
    beginInsertRows( mapFromSource( parent ), start, end );
}

void AttributesModel::slotColumnsAboutToBeInserted( const QModelIndex& parent, int start, int end )
{
    beginInsertColumns( mapFromSource( parent ), start, end );
}

void AttributesModel::slotRowsInserted( const QModelIndex& parent, int start, int end )
{
    Q_UNUSED( parent );
    Q_UNUSED( start );
    Q_UNUSED( end );
    endInsertRows();
}

void AttributesModel::slotColumnsInserted( const QModelIndex& parent, int start, int end )
{
    Q_UNUSED( parent );
    Q_UNUSED( start );
    Q_UNUSED( end );
    endInsertColumns();
}

void AttributesModel::slotRowsAboutToBeRemoved( const QModelIndex& parent, int start, int end )
{
    beginRemoveRows( mapFromSource( parent ), start, end );
}

void AttributesModel::slotColumnsAboutToBeRemoved( const QModelIndex& parent, int start, int end )
{
    beginRemoveColumns( mapFromSource( parent ), start, end );
}

void AttributesModel::slotRowsRemoved( const QModelIndex& parent, int start, int end )
{
    Q_UNUSED( parent );
    Q_UNUSED( start );
    Q_UNUSED( end );
    endRemoveRows();
}

void AttributesModel::removeEntriesFromDataMap( int start, int end )
{
    QMap< int, QMap< int, QMap< int, QVariant > > >::iterator it = d->dataMap.find( end );
    // check that the element was found
    if ( it != d->dataMap.end() ) {
        ++it;
        QVector< int > indexesToDel;
        for ( int i = start; i < end && it != d->dataMap.end(); ++i ) {
            d->dataMap[ i ] = it.value();
            indexesToDel << it.key();
            ++it;
        }
        if ( indexesToDel.isEmpty() ) {
            for ( int i = start; i < end; ++i ) {
                indexesToDel << i;
            }
        }
        for ( int i  = 0; i < indexesToDel.count(); ++i ) {
            d->dataMap.remove( indexesToDel[ i ] );
        }
    }
}

void AttributesModel::removeEntriesFromDirectionDataMaps( Qt::Orientation dir, int start, int end )
{
    QMap<int,  QMap<int, QVariant> > &sectionDataMap
        = dir == Qt::Horizontal ? d->horizontalHeaderDataMap : d->verticalHeaderDataMap;
    QMap<int, QMap<int, QVariant> >::iterator it = sectionDataMap.upperBound( end );
    // check that the element was found
    if ( it != sectionDataMap.end() )
    {
        QVector< int > indexesToDel;
        for ( int i = start; i < end && it != sectionDataMap.end(); ++i )
        {
            sectionDataMap[ i ] = it.value();
            indexesToDel << it.key();
            ++it;
        }
        if ( indexesToDel.isEmpty() )
        {
            for ( int i = start; i < end; ++i )
            {
                indexesToDel << i;
            }
        }
        for ( int i  = 0; i < indexesToDel.count(); ++i )
        {
            sectionDataMap.remove( indexesToDel[ i ] );
        }
    }
}

void AttributesModel::slotColumnsRemoved( const QModelIndex& parent, int start, int end )
{
    Q_UNUSED( parent );
    Q_UNUSED( start );
    Q_UNUSED( end );
    Q_ASSERT_X( sourceModel(), "removeColumn", "This should only be triggered if a valid source Model exists!" );
    for ( int i = start; i <= end; ++i ) {
        d->verticalHeaderDataMap.remove( start );
    }
    removeEntriesFromDataMap( start, end );
    removeEntriesFromDirectionDataMaps( Qt::Horizontal, start, end );
    removeEntriesFromDirectionDataMaps( Qt::Vertical, start, end );

    endRemoveColumns();
}

void AttributesModel::slotDataChanged( const QModelIndex& topLeft, const QModelIndex& bottomRight )
{
    emit dataChanged( mapFromSource( topLeft ), mapFromSource( bottomRight ) );
}

void AttributesModel::setDefaultForRole( int role, const QVariant& value )
{
    if ( value.isValid() ) {
        d->defaultsMap.insert( role, value );
    } else {
        // erase the possibily existing value to not let the map grow:
        QMap<int, QVariant>::iterator it = d->defaultsMap.find( role );
        if ( it != d->defaultsMap.end() ) {
            d->defaultsMap.erase( it );
        }
    }

    Q_ASSERT( defaultsForRole( role ).value<KDChart::DataValueAttributes>()  == value.value<KDChart::DataValueAttributes>() );
}

void AttributesModel::setDatasetDimension( int dimension )
{
    //### need to "reformat" or throw away internal data?
    d->dataDimension = dimension;
}

int AttributesModel::datasetDimension() const
{
    return d->dataDimension;
}
