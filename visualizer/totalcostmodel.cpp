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

#include "totalcostmodel.h"

#include "massifdata/filedata.h"
#include "massifdata/snapshotitem.h"
#include "massifdata/treeleafitem.h"
#include "massifdata/util.h"

#include "KChartGlobal"
#include "KChartLineAttributes"

#include <QPen>

#include <KLocalizedString>

using namespace Massif;

TotalCostModel::TotalCostModel(QObject* parent): QAbstractTableModel(parent), m_data(nullptr)
{
}

TotalCostModel::~TotalCostModel()
{
}

void TotalCostModel::setSource(const FileData* data)
{
    if (m_data) {
        beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
        m_data = nullptr;
        endRemoveRows();
    }
    if (data) {
        beginInsertRows(QModelIndex(), 0, data->snapshots().size() - 1);
        m_data = data;
        endInsertRows();
    }
}

QModelIndex TotalCostModel::peak() const
{
    Q_ASSERT(m_data);
    if (!m_data->peak()) {
        return QModelIndex();
    }
    return index(m_data->snapshots().indexOf(m_data->peak()), 0);
}

QVariant TotalCostModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_ASSERT(orientation != Qt::Horizontal || section < columnCount());
    if (section == 0 && orientation == Qt::Horizontal) {
        if ( role == KChart::DatasetPenRole ) {
            return QPen(Qt::red);
        } else if ( role == KChart::DatasetBrushRole ) {
            return QBrush(Qt::red);
        } else if ( role == Qt::DisplayRole ) {
            return i18n("Total Memory Heap Consumption");
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

QVariant TotalCostModel::data(const QModelIndex& index, int role) const
{
    // FIXME kdchart queries (-1, -1) for empty models
    if ( index.row() == -1 || index.column() == -1 ) {
//         qWarning() << "TotalCostModel::data: FIXME fix kdchart views to not query model data for invalid indices!";
        return QVariant();
    }

    Q_ASSERT(index.row() >= 0 && index.row() < rowCount(index.parent()));
    Q_ASSERT(index.column() >= 0 && index.column() < columnCount(index.parent()));
    Q_ASSERT(m_data);
    Q_ASSERT(!index.parent().isValid());

    if ( role == KChart::LineAttributesRole ) {
        static KChart::LineAttributes attributes;
        attributes.setDisplayArea(true);
        if (index == m_selection) {
            attributes.setTransparency(255);
        } else {
            attributes.setTransparency(50);
        }
        return QVariant::fromValue(attributes);
    }
    if ( role == KChart::DatasetPenRole ) {
        return QPen(Qt::red);
    } else if ( role == KChart::DatasetBrushRole ) {
        return QBrush(Qt::red);
    }

    if ( role != Qt::DisplayRole && role != Qt::ToolTipRole ) {
        return QVariant();
    }

    if ( role == Qt::ToolTipRole ) {
        // hack: the ToolTip will only be queried by KDChart and that one uses the
        // left index, but we want it to query the right one. but we also need to
        // take the very last data set into account to prevent an overflow there
        const SnapshotItem* snapshot = m_data->snapshots().at(qMin(index.row() + 1, m_data->snapshots().size() - 1));
        return i18n("Snapshot #%1:\n"
                    "Heap cost of %2\n"
                    "Extra heap cost of %3\n"
                    "Stack cost of %4",
                    snapshot->number(), prettyCost(snapshot->memHeap()), prettyCost(snapshot->memHeapExtra()),
                    prettyCost(snapshot->memStacks()));
    }

    const SnapshotItem* snapshot = m_data->snapshots().at(index.row());
    if (index.column() == 0) {
        return snapshot->time();
    } else {
        Q_ASSERT(index.column() == 1);
        return snapshot->cost();
    }
}

int TotalCostModel::columnCount(const QModelIndex&) const
{
    return 2;
}

int TotalCostModel::rowCount(const QModelIndex& parent) const
{
    if (!m_data) {
        return 0;
    }

    if (parent.isValid()) {
        return 0;
    } else {
        // snapshot item
        return m_data->snapshots().count();
    }
}

QModelIndex TotalCostModel::indexForSnapshot(const SnapshotItem* snapshot) const
{
    int row = m_data->snapshots().indexOf(const_cast<SnapshotItem*>(snapshot));
    if (row == -1) {
        return QModelIndex();
    }
    return index(row, 0);
}

QModelIndex TotalCostModel::indexForTreeLeaf(const TreeLeafItem* node) const
{
    Q_UNUSED(node)
    return QModelIndex();
}

ModelItem TotalCostModel::itemForIndex(const QModelIndex& idx) const
{
    if (!idx.isValid() || idx.parent().isValid() || idx.row() > rowCount() || idx.column() > columnCount()) {
        return ModelItem(nullptr, nullptr);
    }
    const SnapshotItem* snapshot = m_data->snapshots().at(idx.row());
    return ModelItem(nullptr, snapshot);
}

QModelIndex TotalCostModel::indexForItem(const ModelItem& item) const
{
    if ((!item.first && !item.second) || item.first) {
        return QModelIndex();
    }
    return indexForSnapshot(item.second);
}

void TotalCostModel::setSelection(const QModelIndex& index)
{
    m_selection = index;
}
