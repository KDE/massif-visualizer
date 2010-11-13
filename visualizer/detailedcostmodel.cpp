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

#include "detailedcostmodel.h"

#include <QtCore/QDebug>

#include "massifdata/filedata.h"
#include "massifdata/snapshotitem.h"
#include "massifdata/treeleafitem.h"

#include "KDChartGlobal"
#include "KDChartDataValueAttributes"
#include "KDChartLineAttributes"

#include "util.h"

#include <QtGui/QColor>
#include <QtGui/QPen>
#include <QtGui/QBrush>

#include <QtCore/QMultiMap>
#include <QtCore/qalgorithms.h>

#include <KLocalizedString>

using namespace Massif;

DetailedCostModel::DetailedCostModel(QObject* parent)
    : QAbstractTableModel(parent), m_data(0), m_maxDatasetCount(10)
{
}

DetailedCostModel::~DetailedCostModel()
{
}

void DetailedCostModel::setSource(const FileData* data)
{
    if (m_data) {
        beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
        m_data = 0;
        m_columns.clear();
        m_rows.clear();
        m_nodes.clear();
        m_peaks.clear();
        endRemoveRows();
    }
    if (data) {
        // get top cost points:
        // we traverse the detailed heap trees until the first fork
        QMultiMap<int, QString> sortColumnMap;
        foreach (SnapshotItem* snapshot, data->snapshots()) {
            if (snapshot->heapTree()) {
                QList<TreeLeafItem*> nodes;
                foreach (TreeLeafItem* node, snapshot->heapTree()->children()) {
                    if (isBelowThreshold(node->label())) {
                        continue;
                    }
                    // find interesting node, i.e. until first fork
                    TreeLeafItem* firstNode = node;
                    while (node->children().size() == 1 && node->children().first()->cost() == node->cost()) {
                        node = node->children().first();
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
                        unsigned int cost = sortColumnMap.key(node->label());
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
        m_columns = sortColumnMap.values();
        QAlgorithmsPrivate::qReverse(m_columns.begin(), m_columns.end());

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

    if ( role == KDChart::LineAttributesRole ) {
        static KDChart::LineAttributes attributes;
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

    if (role == KDChart::DatasetBrushRole || role == KDChart::DatasetPenRole) {
        QColor c = QColor::fromHsv(double(index.column() + 1) / columnCount() * 255, 255, 255);
        if (role == KDChart::DatasetBrushRole) {
            return QBrush(c);
        } else {
            return QPen(c);
        }
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
                while (it != m_data->snapshots().constEnd() && (*it)->memHeap() < m_data->peak()->memHeap() * 0.001) {
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

    SnapshotItem* snapshot;
    if (role == Qt::ToolTipRole) {
        // hack: the ToolTip will only be queried by KDChart and that one uses the
        // left index, but we want it to query the right one
        Q_ASSERT(index.row() < m_rows.size());
        snapshot = m_rows.at(index.row());
    } else {
        snapshot = m_rows.at(index.row() - 1);
    }

    if (index.column() % 2 == 0 && role != Qt::ToolTipRole) {
        return snapshot->time();
    } else {
        TreeLeafItem* node = 0;
        const QString needle = m_columns.at(index.column() / 2);
        foreach(TreeLeafItem* n, m_nodes[snapshot]) {
            if (n->label() == needle) {
                node = n;
                break;
            }
        }
        if (role == Qt::ToolTipRole) {
            return i18n("cost of %1, i.e. %2% of snapshot #%3\n%4",
                        prettyCost(node ? node->cost() : 0),
                        // yeah nice how I round to two decimals, right? :D
                        double(int(double(node ? node->cost() : 0)/snapshot->memHeap()*10000))/100,
                        snapshot->number(),
                        needle);
        } else {
            return double(node ? node->cost() : 0);
        }
    }
}

QVariant DetailedCostModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section % 2 == 0) {
        // only show name without memory address or location
        QString label = prettyLabel(m_columns.at(section / 2));
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

QMap< QModelIndex, TreeLeafItem* > DetailedCostModel::peaks() const
{
    QMap< QModelIndex, TreeLeafItem* > peaks;
    QMap< QString, QPair<TreeLeafItem*,SnapshotItem*> >::const_iterator it = m_peaks.constBegin();
    while (it != m_peaks.end()) {
        int row = m_rows.indexOf(it->second);
        Q_ASSERT(row >= 0);
        int column = m_columns.indexOf(it->first->label());
        if (column >= m_maxDatasetCount) {
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

QModelIndex DetailedCostModel::indexForSnapshot(SnapshotItem* snapshot) const
{
    int row = m_rows.indexOf(snapshot);
    if (row == -1) {
        return QModelIndex();
    }
    return index(row + 1, 0);
}

QModelIndex DetailedCostModel::indexForTreeLeaf(TreeLeafItem* node) const
{
    int column = m_columns.indexOf(node->label());
    if (column == -1 || column >= m_maxDatasetCount) {
        return QModelIndex();
    }
    QMap< SnapshotItem*, QList< TreeLeafItem* > >::const_iterator it = m_nodes.constBegin();
    while (it != m_nodes.constEnd()) {
        if (it->contains(node)) {
            return index(m_rows.indexOf(it.key()), column * 2);
        }
        ++it;
    }
    return QModelIndex();
}

QPair< TreeLeafItem*, SnapshotItem* > DetailedCostModel::itemForIndex(const QModelIndex& idx) const
{
    if (!idx.isValid() || idx.parent().isValid() || idx.row() > rowCount() || idx.column() > columnCount()) {
        return QPair< TreeLeafItem*, SnapshotItem* >(0, 0);
    }
    if (idx.row() == 0) {
        return QPair< TreeLeafItem*, SnapshotItem* >(0, 0);
    }
    const QString needle = m_columns.at(idx.column() / 2);
    for (int i = 1; i < 3 && idx.row() - i >= 0; ++i) {
        SnapshotItem* snapshot = m_rows.at(idx.row() - i);
        foreach(TreeLeafItem* n, m_nodes[snapshot]) {
            if (n->label() == needle) {
                return QPair< TreeLeafItem*, SnapshotItem* >(n, 0);
            }
        }
    }
    return QPair< TreeLeafItem*, SnapshotItem* >(0, 0);
}

QModelIndex DetailedCostModel::indexForItem(const QPair< TreeLeafItem*, SnapshotItem* >& item) const
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
