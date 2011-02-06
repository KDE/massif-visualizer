/*
   This file is part of Massif Visualizer

   Copyright 2010 Milian Wolff <mail@milianw.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef MASSIF_DATATREEMODEL_H
#define MASSIF_DATATREEMODEL_H

#include <QBrush>
#include <QPair>
#include <QtCore/QAbstractItemModel>

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
    QPair<TreeLeafItem*, SnapshotItem*> itemForIndex(const QModelIndex& idx) const;

    /**
     * @return Index for given item. At maximum one of the pointers should be valid in the input pair.
     */
    QModelIndex indexForItem(const QPair<TreeLeafItem*, SnapshotItem*>& item) const;

    /**
     * @return Index for given snapshot, or invalid if it's not a detailed snapshot.
     */
    QModelIndex indexForSnapshot(SnapshotItem* snapshot) const;

    /**
     * @return Index for given TreeLeafItem, or invalid if it's not covered by this model.
     */
    QModelIndex indexForTreeLeaf(TreeLeafItem* node) const;

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex& child) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    SnapshotItem* snapshotForTreeLeaf(TreeLeafItem* node) const;
private:
    const FileData* m_data;

    void mapNodeToRow(TreeLeafItem* node, const int row);
    QMap<TreeLeafItem*, int> m_nodeToRow;
    QMap<TreeLeafItem*, SnapshotItem*> m_heapRootToSnapshot;
};

}

#endif // MASSIF_DATATREEMODEL_H
