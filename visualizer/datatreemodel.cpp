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

#include "datatreemodel.h"

#include <QtCore/QDebug>

#include "massifdata/filedata.h"
#include "massifdata/snapshotitem.h"
#include "massifdata/treeleafitem.h"

#include <KLocalizedString>

#include <QtGui/QBrush>

#include "util.h"

using namespace Massif;

DataTreeModel::DataTreeModel(QObject* parent)
    : QAbstractItemModel(parent), m_data(0)
{
}

DataTreeModel::~DataTreeModel()
{
}

void DataTreeModel::setSource(const FileData* data)
{
    if (m_data) {
        beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
        m_data = 0;
        m_nodeToRow.clear();
        m_heapRootToSnapshot.clear();
        endRemoveRows();
    }
    if (data) {
        beginInsertRows(QModelIndex(), 0, data->snapshots().size() - 1);
        m_data = data;
        int row = 0;
        foreach(SnapshotItem* item, m_data->snapshots()) {
            if (item->heapTree()) {
                mapNodeToRow(item->heapTree(), row++);
                m_heapRootToSnapshot[item->heapTree()] = item;
            } else {
                row++;
            }
        }
        endInsertRows();
    }
}

void DataTreeModel::mapNodeToRow(TreeLeafItem* node, const int row)
{
    m_nodeToRow[node] = row;
    int r = 0;
    foreach(TreeLeafItem* child, node->children()) {
        mapNodeToRow(child, r++);
    }
}

QVariant DataTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == 0) {
            return i18n("Cost");
        } else {
            return i18n("Location");
        }
    }

    return QVariant();
}

QVariant DataTreeModel::data(const QModelIndex& index, int role) const
{
    // FIXME kdchart queries (-1, -1) for empty models
    if ( index.row() == -1 || index.column() == -1 ) {
//         qWarning() << "DataTreeModel::data: FIXME fix kdchart views to not query model data for invalid indices!";
        return QVariant();
    }

    Q_ASSERT(index.row() >= 0 && index.row() < rowCount(index.parent()));
    Q_ASSERT(index.column() >= 0 && index.column() < columnCount(index.parent()));
    Q_ASSERT(m_data);


    // custom background for peak snapshot
    if ( role == Qt::BackgroundRole ) {
//         double maxValue = 1;
        const double maxValue = m_data->peak()->memHeap();
        double currentValue = 0;
        if ( !index.parent().isValid() && m_data->peak() ) {
//             maxValue = m_data->peak()->memHeap();
            SnapshotItem* snapshot = m_data->snapshots().at(index.row());
            currentValue = snapshot->memHeap();
        } else if (index.parent().isValid()) {
            Q_ASSERT(index.internalPointer());
            TreeLeafItem* node = static_cast<TreeLeafItem*>(index.internalPointer());
            currentValue = node->cost();
            Q_ASSERT(node->parent());
            /*
            TreeLeafItem* parent = node->parent();
            while (parent->parent()) {
                parent = parent->parent();
            }
            maxValue = parent->cost();
            // normalize
            maxValue -= parent->children().last()->cost();
            currentValue -= parent->children().last()->cost();
            */
        }
        if (currentValue > 0) {
            const double ratio = (currentValue / maxValue);
            QColor c = QColor::fromHsv(120 - ratio * 120, 255, 255, (-((ratio-1) * (ratio-1))) * 120 + 120);
//             QColor c = QColor::fromHsv(120 - ratio * 120, 255, 255, 120);
            return QBrush(c);
        } else {
            return QVariant();
        }
    }

    if ( role != Qt::DisplayRole && role != Qt::ToolTipRole ) {
        return QVariant();
    }

    if (!index.parent().isValid()) {
        SnapshotItem* snapshot = m_data->snapshots().at(index.row());
        if (role == Qt::ToolTipRole) {
            if (snapshot == m_data->peak()) {
                return i18n("Peak snapshot: heap cost of %1", prettyCost(snapshot->memHeap()));
            } else {
                return i18n("Snapshot #%1: heap cost of %2", snapshot->number(), prettyCost(snapshot->memHeap()));
            }
        }
        if (index.column() == 0) {
            return prettyCost(snapshot->memHeap());
        } else {
            if (snapshot == m_data->peak()) {
                return i18n("Snapshot #%1 (peak)", snapshot->number());
            } else {
                return i18n("Snapshot #%1", snapshot->number());
            }
        }
    } else {
        Q_ASSERT(index.internalPointer());
        TreeLeafItem* item = static_cast<TreeLeafItem*>(index.internalPointer());
        if (role == Qt::ToolTipRole) {
            return tooltipForTreeLeaf(item, snapshotForTreeLeaf(item), item->label());
        }
        if (index.column() == 0) {
            return prettyCost(item->cost());
        } else {
            return prettyLabel(item->label());
        }
    }
    return QVariant();
}

int DataTreeModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        // heap tree item: Cost | Location
        return 2;
    } else {
        // snapshot: Cost | Time
        // TODO: see whether we can have more here
        return 2;
    }
}

int DataTreeModel::rowCount(const QModelIndex& parent) const
{
    if (!m_data || (parent.isValid() && parent.column() != 0)) {
        return 0;
    }

    if (parent.isValid()) {
        if (!parent.internalPointer()) {
            // snapshot without detailed heaptree
            return 0;
        }
        return static_cast<TreeLeafItem*>(parent.internalPointer())->children().size();
    } else {
        return m_data->snapshots().size();
    }
}

QModelIndex DataTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (row < 0 || row >= rowCount(parent) || column < 0 || column >= columnCount(parent)) {
        // invalid
        return QModelIndex();
    }

    if (parent.isValid()) {
        if (parent.column() == 0) {
            Q_ASSERT(parent.internalPointer());
            // parent is a tree leaf item
            return createIndex(row, column, static_cast<void*>(static_cast<TreeLeafItem*>(parent.internalPointer())->children().at(row)));
        } else {
            return QModelIndex();
        }
    } else {
        return createIndex(row, column, static_cast<void*>(m_data->snapshots().at(row)->heapTree()));
    }
}

QModelIndex DataTreeModel::parent(const QModelIndex& child) const
{
    if (child.internalPointer()) {
        TreeLeafItem* item = static_cast<TreeLeafItem*>(child.internalPointer());
        if (item->parent()) {
            Q_ASSERT(m_nodeToRow.contains(item));
            // somewhere in the detailed heap tree
            return createIndex(m_nodeToRow[item->parent()], 0, static_cast<void*>(item->parent()));
        } else {
            // snapshot item with heap tree
            return QModelIndex();
        }
    } else {
        // snapshot item without detailed heap tree
        return QModelIndex();
    }
}

QModelIndex DataTreeModel::indexForSnapshot(SnapshotItem* snapshot) const
{
    int idx = m_data->snapshots().indexOf(snapshot);
    if ( idx == -1 ) {
        return QModelIndex();
    }
    return index(idx, 0);
}

QModelIndex DataTreeModel::indexForTreeLeaf(TreeLeafItem* node) const
{
    if (!m_data || !m_nodeToRow.contains(node)) {
        return QModelIndex();
    }
    return createIndex(m_nodeToRow[node], 0, static_cast<void*>(node));
}

QPair< TreeLeafItem*, SnapshotItem* > DataTreeModel::itemForIndex(const QModelIndex& idx) const
{
    if (!m_data || !idx.isValid() || idx.row() >= m_data->snapshots().count()) {
        return QPair< TreeLeafItem*, SnapshotItem* >(0, 0);
    }
    if (idx.parent().isValid()) {
        Q_ASSERT(idx.internalPointer());
        return QPair< TreeLeafItem*, SnapshotItem* >(static_cast<TreeLeafItem*>(idx.internalPointer()), 0);
    } else {
        return QPair< TreeLeafItem*, SnapshotItem* >(0, m_data->snapshots().at(idx.row()));
    }
}

QModelIndex DataTreeModel::indexForItem(const QPair< TreeLeafItem*, SnapshotItem* >& item) const
{
    if (!item.first && !item.second) {
        return QModelIndex();
    }
    Q_ASSERT((item.first && !item.second) || (!item.first && item.second));
    if (item.first) {
        return indexForTreeLeaf(item.first);
    } else {
        return indexForSnapshot(item.second);
    }
}

SnapshotItem* DataTreeModel::snapshotForTreeLeaf(TreeLeafItem* node) const
{
    while (node->parent()) {
        node = node->parent();
    }
    return m_heapRootToSnapshot.value(node);
}
