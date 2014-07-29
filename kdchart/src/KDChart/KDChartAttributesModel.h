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

#ifndef __KDCHART_ATTRIBUTES_MODEL_H__
#define __KDCHART_ATTRIBUTES_MODEL_H__

#include "KDChartAbstractProxyModel.h"
#include <QMap>
#include <QVariant>

#include "KDChartGlobal.h"

namespace KDChart {

/**
  * @brief A proxy model used for decorating data with attributes.
  *
  *        An AttributesModel forwards data from and to the source model and adds attributes,
  *        data that influences the graphical rendering of the source model data.
  *        The attributes are distinguished from the source model's data by their @p role values.
  *        Therefore this class does not need to, and does not, change the data layout from the
  *        source model's; indexes that refer to the same data have the same row and column
  *        values in both models.
  *        Attribute changes, that is changes to data with the attribute role, via the interface
  *        of this class (including setData()) are stored internally and not forwarded to the source model.
  */
class KDCHART_EXPORT AttributesModel : public AbstractProxyModel
{
    Q_OBJECT
    KDCHART_DECLARE_PRIVATE_BASE_POLYMORPHIC( AttributesModel )
public:
    enum PaletteType {
        PaletteTypeDefault = 0,
        PaletteTypeRainbow = 1,
        PaletteTypeSubdued = 2
    };

    explicit AttributesModel( QAbstractItemModel* model, QObject * parent = 0 );
    ~AttributesModel();

    /** Copies the internal data (maps and palette) of another
     *  AttributesModel* into this one.
     */
    void initFrom( const AttributesModel* other );

    /** Returns true if both, all of the attributes set, and
     * the palette set is equal in both of the AttributeModels.
     */
    bool compare( const AttributesModel* other ) const;

    bool compareAttributes( int role, const QVariant& a, const QVariant& b ) const;

    /* Attributes Model specific API */
    bool setModelData( const QVariant value, int role );
    QVariant modelData( int role ) const;

    /** Returns whether the given role corresponds to one of the known
     * internally used ones. */
    bool isKnownAttributesRole( int role ) const;

    /** Sets the palettetype used by this attributesmodel */
    void setPaletteType( PaletteType type );
    PaletteType paletteType() const;

    /** Returns the data that were specified at global level,
      * or the default data, or QVariant().
      */
    QVariant data(int role) const;

    /** Returns the data that were specified at per column level,
      * or the globally set data, or the default data, or QVariant().
      */
    QVariant data(int column, int role) const;

    /** \reimpl */
    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    /** \reimpl */
    int rowCount(const QModelIndex& ) const;
    /** \reimpl */
    int columnCount(const QModelIndex& ) const;
    /** \reimpl */
    QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const;
    /** \reimpl */
    bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::DisplayRole);
    /** Remove any explicit attributes settings that might have been specified before. */
    bool resetData ( const QModelIndex & index, int role = Qt::DisplayRole);
    /** \reimpl */
    bool setHeaderData ( int section, Qt::Orientation orientation, const QVariant & value,
                         int role = Qt::DisplayRole);
    /** Returns default values for the header data. */
    virtual QVariant defaultHeaderData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    /** Remove any explicit attributes settings that might have been specified before. */
    bool resetHeaderData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole);
    /** \reimpl */
    void setSourceModel ( QAbstractItemModel* sourceModel );

    /** Define the default value for a certain role.
        Passing a default-constructed QVariant is equivalent to removing the default. */
    void setDefaultForRole( int role, const QVariant& value );

    /** Set the dimension of the dataset in the source model. \sa AbstractDiagram::setDatasetDimension */
    void setDatasetDimension( int dimension );
    int datasetDimension() const;

Q_SIGNALS:
    void attributesChanged( const QModelIndex&, const QModelIndex& );

private Q_SLOTS:
    void slotRowsAboutToBeInserted( const QModelIndex& parent, int start, int end );
    void slotColumnsAboutToBeInserted( const QModelIndex& parent, int start, int end );
    void slotRowsInserted( const QModelIndex& parent, int start, int end );
    void slotColumnsInserted( const QModelIndex& parent, int start, int end );

    void slotRowsAboutToBeRemoved( const QModelIndex& parent, int start, int end );
    void slotColumnsAboutToBeRemoved( const QModelIndex& parent, int start, int end );
    void slotRowsRemoved( const QModelIndex& parent, int start, int end );
    void slotColumnsRemoved( const QModelIndex& parent, int start, int end );

    void slotDataChanged( const QModelIndex& topLeft, const QModelIndex& bottomRight );

private:
    // helper
    QVariant defaultsForRole( int role ) const;
    bool compareHeaderDataMaps( const QMap< int, QMap< int, QVariant > >& mapA,
                                const QMap< int, QMap< int, QVariant > >& mapB ) const;

    void removeEntriesFromDataMap( int start, int end );
    void removeEntriesFromDirectionDataMaps( Qt::Orientation dir, int start, int end );
};

}

#endif
