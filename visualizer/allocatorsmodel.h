/*
   This file is part of Massif Visualizer

   Copyright 2014 Milian Wolff <mail@milianw.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ALLOCATORSMODEL_H
#define ALLOCATORSMODEL_H

#include <QAbstractItemModel>
#include <QVector>

#include <massifdata/util.h>

#include "modelitem.h"
#include "visualizer_export.h"

namespace Massif {

class TreeLeafItem;

class FileData;

class VISUALIZER_EXPORT AllocatorsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit AllocatorsModel(const FileData* data, QObject* parent = 0);
    virtual ~AllocatorsModel();

    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;

    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex& child) const;

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    QModelIndex indexForItem(const Massif::ModelItem& item) const;
    void settingsChanged();

    enum Columns {
        Function = 0,
        Peak,
        Location,
        NUM_COLUMNS,
    };

    enum Roles {
        SortRole = Qt::UserRole + 1,
        ItemRole,
    };
private:
    struct Data
    {
        ParsedLabel label;
        const TreeLeafItem* peak;
    };
    QVector<Data> m_data;
};

}

#endif // ALLOCATORSMODEL_H
