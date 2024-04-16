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

#include "detailedcostmodel.h"

#include "massifdata/filedata.h"
#include "massifdata/snapshotitem.h"
#include "massifdata/treeleafitem.h"
#include "massifdata/util.h"

#include "KChartGlobal"
#include "KChartDataValueAttributes"
#include "KChartLineAttributes"

#include <QColor>
#include <QPen>
#include <QBrush>

#include <QMultiMap>
#include <algorithm>

#include <KLocalizedString>

using namespace Massif;

namespace {

QVariant colorForColumn(int column, int columnCount, int role)
{
    QColor c = QColor::fromHsv(double(column + 1) / columnCount * 255, 255, 255);
    if (role == KChart::DatasetBrushRole) {
        return QBrush(c);
    } else {
        return QPen(c);
    }
}

}

DetailedCostModel::DetailedCostModel(QObject* parent)
    : QAbstractTableModel(parent), m_data(nullptr), m_maxDatasetCount(10)
{
}

DetailedCostModel::~DetailedCostModel()
{
}

void DetailedCostModel::setSource(const FileData* data)
{
    if (m_data) {
        beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
        m_data = nullptr;
        m_columns.clear();
        m_rows.clear();
        m_nodes.clear();
        m_peaks.clear();
        endRemoveRows();
    }
    if (data) {
        // get top cost points:
        // we traverse the detailed heap trees until the first fork
        QMultiMap<int, QByteArray> sortColumnMap;
        foreach (const SnapshotItem* snapshot, data->snapshots()) {
            if (snapshot->heapTree()) {
                QList<const TreeLeafItem*> nodes;
                foreach (const TreeLeafItem* node, snapshot->heapTree()->children()) {
                    if (isBelowThreshold(node->label())) {
                        continue;
                    }
                    // find interesting node, i.e. until first fork
                    const TreeLeafItem* firstNode = node;
                    while (node->children().size() == 1 && node->children().at(0)->cost() == node->cost()) {
                        node = node->children().at(0);
                    }
                    if (node->children().isEmpty()) {
                        // when we traverse the tree down until the end (i.e. no forks),
                        // we end up in main() most probably, and that's uninteresting
                        node = firstNode;
                    }
                    if (!sortColumnMap.values().contains(node->label())) {
                        sortColumnMap.insert(node->cost(), node->label());
                        m_peaks[node->label()] = qMakePair(node, snapshot);
                    } else {
                        quint64 cost = sortColumnMap.key(node->label());
                        if (node->cost() > cost) {
                            sortColumnMap.remove(cost, node->label());
                            cost = node->cost();
                            sortColumnMap.insert(cost, node->label());
                            m_peaks[node->label()].first = node;
                            m_peaks[node->label()].second = snapshot;
                        }
                    }
                    nodes << node;
                }
                m_rows << snapshot;
                m_nodes[snapshot] = nodes;
            }
        }
        // ugh, yet another bad usage of QList in Qt's API
        m_columns = sortColumnMap.values().toVector();
        std::reverse(m_columns.begin(), m_columns.end());

        if (m_rows.isEmpty()) {
            return;
        }
        // +1 for the offset (+0 would be m_rows.size() -1)
        beginInsertRows(QModelIndex(), 0, m_rows.size());
        m_data = data;
        endInsertRows();
    }
}

void DetailedCostModel::setMaximumDatasetCount(int count)
{
    Q_ASSERT(count >= 0);
    const int currentCols = qMin(m_columns.size(), m_maxDatasetCount);
    const int newCols = qMin(m_columns.size(), count);
    if (currentCols == newCols) {
        return;
    }
    if (newCols < currentCols) {
        beginRemoveColumns(QModelIndex(), newCols * 2, currentCols * 2 - 1);
    } else {
        beginInsertColumns(QModelIndex(), currentCols * 2, newCols * 2 - 1);
    }
    m_maxDatasetCount = count;
    if (newCols < currentCols) {
        endRemoveColumns();
    } else {
        endInsertColumns();
    }
    Q_ASSERT(columnCount() == newCols * 2);
}

int DetailedCostModel::maximumDatasetCount() const
{
    return m_maxDatasetCount;
}

QVariant DetailedCostModel::data(const QModelIndex& index, int role) const
{
    // FIXME kdchart queries (-1, -1) for empty models
    if ( index.row() == -1 || index.column() == -1 ) {
//         qWarning() << "DetailedCostModel::data: FIXME fix kdchart views to not query model data for invalid indices!";
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
        } else if (index.column() == m_selection.column()) {
            attributes.setTransparency(152);
        } else {
            attributes.setTransparency(127);
        }
        return QVariant::fromValue(attributes);
    }

    if (role == KChart::DatasetBrushRole || role == KChart::DatasetPenRole) {
        return colorForColumn(index.column(), columnCount(), role);
    }

    if ( role != Qt::DisplayRole && role != Qt::ToolTipRole ) {
        return QVariant();
    }

    if (index.row() == 0) {
        if (role == Qt::ToolTipRole) {
            return QVariant();
        } else {
            if (index.column() % 2 == 0) {
                // get x-coordinate of the last snapshot with cost below 0.1% of peak cost
                QList< SnapshotItem* >::const_iterator it = m_data->snapshots().constBegin();
                double time = 0;
                while (it != m_data->snapshots().constEnd() && (*it)->cost() < m_data->peak()->cost() * 0.001) {
                    time = (*it)->time();
                    ++it;
                }
                return time;
            } else {
                // cost to 0
                return 0;
            }
        }
    }

    const SnapshotItem* snapshot;
    if (role == Qt::ToolTipRole) {
        // hack: the ToolTip will only be queried by KChart and that one uses the
        // left index, but we want it to query the right one
        Q_ASSERT(index.row() < m_rows.size());
        snapshot = m_rows.at(index.row());
    } else {
        snapshot = m_rows.at(index.row() - 1);
    }

    if (index.column() % 2 == 0 && role != Qt::ToolTipRole) {
        return snapshot->time();
    } else {
        const TreeLeafItem* node = nullptr;
        const QByteArray needle = m_columns.at(index.column() / 2);
        foreach(const TreeLeafItem* n, m_nodes[snapshot]) {
            if (n->label() == needle) {
                node = n;
                break;
            }
        }
        if (role == Qt::ToolTipRole) {
            return tooltipForTreeLeaf(node, snapshot, needle);
        } else {
            return node ? node->cost() : 0;
        }
    }
}

QVariant DetailedCostModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && section % 2 == 0 && section < columnCount()) {
        if (role == KChart::DatasetBrushRole || role == KChart::DatasetPenRole) {
            return colorForColumn(section, columnCount(), role);
        } else if (role == Qt::DisplayRole ) {
            // only show name without memory address or location
            QByteArray label = prettyLabel(m_columns.at(section / 2));
            if (label.indexOf("???") != -1) {
                return label;
            }
            int endPos = label.lastIndexOf('(');
            if (endPos == -1) {
                return label;
            }
            label = label.mid(0, endPos - 1);
            const int maxLen = 40;
            if (label.length() > maxLen) {
                label.resize(maxLen - 3);
                label.append("...");
            }
            return label;
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

int DetailedCostModel::columnCount(const QModelIndex&) const
{
    return qMin(m_maxDatasetCount, m_columns.size()) * 2;
}

int DetailedCostModel::rowCount(const QModelIndex& parent) const
{
    if (!m_data) {
        return 0;
    }

    if (parent.isValid()) {
        return 0;
    } else {
        // +1 to get a zero row first
        return m_rows.count() + 1;
    }
}

DetailedCostModel::Peaks DetailedCostModel::peaks() const
{
    Peaks peaks;
    QHash<QByteArray, ModelItem>::const_iterator it = m_peaks.constBegin();
    while (it != m_peaks.end()) {
        int row = m_rows.indexOf(it->second);
        Q_ASSERT(row >= 0);
        int column = m_columns.indexOf(it->first->label());
        if (column >= m_maxDatasetCount || column == -1) {
            ++it;
            continue;
        }
        Q_ASSERT(column >= 0);
        peaks[index(row + 1, column*2)] = it->first;
        ++it;
    }
    Q_ASSERT(peaks.size() == qMin(m_maxDatasetCount, m_columns.size()));
    return peaks;
}

QModelIndex DetailedCostModel::indexForSnapshot(const SnapshotItem* snapshot) const
{
    int row = m_rows.indexOf(snapshot);
    if (row == -1) {
        return QModelIndex();
    }
    return index(row + 1, 0);
}

QModelIndex DetailedCostModel::indexForTreeLeaf(const TreeLeafItem* node) const
{
    int column = m_columns.indexOf(node->label());
    if (column == -1 || column >= m_maxDatasetCount) {
        return QModelIndex();
    }
    Nodes::const_iterator it = m_nodes.constBegin();
    while (it != m_nodes.constEnd()) {
        if (it->contains(node)) {
            return index(m_rows.indexOf(it.key()), column * 2);
        }
        ++it;
    }
    return QModelIndex();
}

ModelItem DetailedCostModel::itemForIndex(const QModelIndex& idx) const
{
    if (!idx.isValid() || idx.parent().isValid() || idx.row() > rowCount() || idx.column() > columnCount()) {
        return ModelItem(nullptr, nullptr);
    }
    if (idx.row() == 0) {
        return ModelItem(nullptr, nullptr);
    }
    const QByteArray needle = m_columns.at(idx.column() / 2);
    for (int i = 1; i < 3 && idx.row() - i >= 0; ++i) {
        const SnapshotItem* snapshot = m_rows.at(idx.row() - i);
        foreach(const TreeLeafItem* n, m_nodes[snapshot]) {
            if (n->label() == needle) {
                return ModelItem(n, nullptr);
            }
        }
    }
    return ModelItem(nullptr, nullptr);
}

QModelIndex DetailedCostModel::indexForItem(const ModelItem& item) const
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

void DetailedCostModel::setSelection(const QModelIndex& index)
{
    m_selection = index;
}

void DetailedCostModel::hideFunction(const TreeLeafItem* node)
{
    beginResetModel();
    Nodes::iterator it = m_nodes.begin();
    while (it != m_nodes.end()) {
        QList< const TreeLeafItem* >::iterator it2 = it.value().begin();
        while (it2 != it.value().end()) {
            if ((*it2)->label() == node->label()) {
                it2 = it.value().erase(it2);
            } else {
                ++it2;
            }
        }
        ++it;
    }
    int idx = m_columns.indexOf(node->label());
    if (idx != -1) {
        m_columns.remove(idx);
    }
    endResetModel();
}

void DetailedCostModel::hideOtherFunctions(const TreeLeafItem* node)
{
    beginResetModel();
    m_columns.clear();
    m_columns << node->label();

    Nodes::iterator it = m_nodes.begin();
    const Nodes::iterator end = m_nodes.end();
    while (it != end) {
        QList< const TreeLeafItem* >::iterator it2 = it.value().begin();
        while (it2 != it.value().end()) {
            if ((*it2)->label() != node->label()) {
                it2 = it.value().erase(it2);
            } else {
                ++it2;
            }
        }
        ++it;
    }

    endResetModel();
}
