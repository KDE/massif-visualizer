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
#include <QList>

#include <massifdata/util.h>

#include "modelitem.h"

namespace Massif {

class TreeLeafItem;

class FileData;

class AllocatorsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit AllocatorsModel(const FileData* data, QObject* parent = nullptr);
    ~AllocatorsModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

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
    QList<Data> m_data;
};

}

#endif // ALLOCATORSMODEL_H
