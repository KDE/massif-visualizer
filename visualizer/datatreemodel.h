/*
   This file is part of Massif Visualizer

   Copyright 2010 Milian Wolff <mail@milianw.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MASSIF_DATATREEMODEL_H
#define MASSIF_DATATREEMODEL_H

#include <QtCore/QAbstractItemModel>

#include "modelitem.h"

#include "visualizer_export.h"

namespace Massif {

class FileData;
class TreeLeafItem;
class SnapshotItem;

/**
 * A model that gives a tree representation of the full Massif data. Useful for e.g. ListViews.
 */
class VISUALIZER_EXPORT DataTreeModel : public QAbstractItemModel
{
public:
    DataTreeModel(QObject* parent = 0);
    virtual ~DataTreeModel();

    /**
     * That the source data for this model.
     */
    void setSource(const FileData* data);

    /**
     * @return Item for given index. At maximum one of the pointers in the pair will be valid.
     */
    ModelItem itemForIndex(const QModelIndex& idx) const;

    /**
     * @return Index for given item. At maximum one of the pointers should be valid in the input pair.
     */
    QModelIndex indexForItem(const ModelItem& item) const;

    /**
     * @return Index for given snapshot, or invalid if it's not a detailed snapshot.
     */
    QModelIndex indexForSnapshot(const SnapshotItem* snapshot) const;

    /**
     * @return Index for given TreeLeafItem, or invalid if it's not covered by this model.
     */
    QModelIndex indexForTreeLeaf(const TreeLeafItem* node) const;

    enum CustomRoles {
        RawLabelRole = Qt::UserRole + 1
    };

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex& child) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    const SnapshotItem* snapshotForTreeLeaf(const TreeLeafItem* node) const;
private:
    const FileData* m_data;

    void mapNodeToRow(const TreeLeafItem* node, const int row);
    QHash<const TreeLeafItem*, int> m_nodeToRow;
    QHash<const TreeLeafItem*, const SnapshotItem*> m_heapRootToSnapshot;
};

}

#endif // MASSIF_DATATREEMODEL_H
