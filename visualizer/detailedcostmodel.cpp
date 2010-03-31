/*
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

#include <KLocalizedString>

using namespace Massif;

DetailedCostModel::DetailedCostModel(QObject* parent)
    : QAbstractTableModel(parent), m_data(0)
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
        foreach (SnapshotItem* snapshot, data->snapshots()) {
            if (snapshot->heapTree()) {
                QList<TreeLeafItem*> nodes;
                foreach (TreeLeafItem* node, snapshot->heapTree()->children()) {
                    while (node->children().size() == 1) {
                        node = node->children().first();
                    }
                    if (isBelowThreshold(node->label())) {
                        continue;
                    }
                    if (!m_columns.values().contains(node->label())) {
                        m_columns.insert(node->cost(), node->label());
                        m_peaks[node->label()] = qMakePair(node, snapshot);
                    } else {
                        unsigned int cost = m_columns.key(node->label());
                        m_columns.remove(cost, node->label());
                        cost = node->cost();
                        m_columns.insert(cost, node->label());
                        if (m_peaks[node->label()].first->cost() < node->cost()) {
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
        // limit number of colums
        const int maxColumns = 15;
        if ( m_columns.size() > maxColumns ) {
            QMultiMap< unsigned int, QString >::iterator it = m_columns.begin();
            for ( int i = 0, c = m_columns.size() - maxColumns; i < c; ++i ) {
                m_peaks.remove(it.value());
                it = m_columns.erase(it);
            }
            Q_ASSERT(m_peaks.size() == m_columns.size());
            Q_ASSERT(m_columns.size() == maxColumns);
        }

        if (m_rows.isEmpty()) {
            return;
        }
        beginInsertRows(QModelIndex(), 0, m_rows.size() - 1);
        m_data = data;
        endInsertRows();
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
        if (index.column() == m_selection.column()) {
            attributes.setTransparency(255);
        } else {
            attributes.setTransparency(127);
        }
        return QVariant::fromValue(attributes);
    }

    if (role == KDChart::DatasetBrushRole || role == KDChart::DatasetPenRole) {
        QColor c = QColor::fromHsv(255 - ((double(index.column()/2) + 1) / m_columns.size()) * 255, 255, 255);
        if (role == KDChart::DatasetBrushRole) {
            return QBrush(c);
        } else {
            return QPen(c);
        }
    }

    if ( role != Qt::DisplayRole && role != Qt::ToolTipRole ) {
        return QVariant();
    }

    SnapshotItem* snapshot = m_rows[index.row()];
    if (index.column() % 2 == 0 && role != Qt::ToolTipRole) {
        return snapshot->time();
    } else {
        TreeLeafItem* node = 0;
        const QString needle = m_columns.values().at(index.column() / 2);
        foreach(TreeLeafItem* n, m_nodes[snapshot]) {
            if (n->label() == needle) {
                node = n;
                break;
            }
        }
        if (!node) {
            if (role == Qt::ToolTipRole) {
                return QVariant();
            } else {
                return 0;
            }
        } else {
            if (role == Qt::ToolTipRole) {
                return i18n("cost of %1, i.e. %2% of snapshot #%3\n%4",
                            prettyCost(node->cost()),
                            // yeah nice how I round to two decimals, right? :D
                            double(int(double(node->cost())/snapshot->memHeap()*10000))/100,
                            snapshot->number(),
                            node->label());
            } else {
                return double(node->cost());
            }
        }
    }
}

QVariant DetailedCostModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section % 2 == 0) {
        // only show name without memory address or location
        QString label = prettyLabel(m_columns.values().at(section / 2));
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
    return m_columns.size() * 2;
}

int DetailedCostModel::rowCount(const QModelIndex& parent) const
{
    if (!m_data) {
        return 0;
    }

    if (parent.isValid()) {
        return 0;
    } else {
        return m_rows.count();
    }
}

QMap< QModelIndex, TreeLeafItem* > DetailedCostModel::peaks() const
{
    QMap< QModelIndex, TreeLeafItem* > peaks;
    QMap< QString, QPair<TreeLeafItem*,SnapshotItem*> >::const_iterator it = m_peaks.constBegin();
    while (it != m_peaks.end()) {
        int row = m_rows.indexOf(it->second);
        Q_ASSERT(row >= 0);
        int column = m_columns.values().indexOf(it->first->label());
        Q_ASSERT(column >= 0);
        peaks[index(row, column*2)] = it->first;
        ++it;
    }
    return peaks;
}

QModelIndex DetailedCostModel::indexForSnapshot(SnapshotItem* snapshot) const
{
    int row = m_rows.indexOf(snapshot);
    if (row == -1) {
        return QModelIndex();
    }
    return index(row, 0);
}

QModelIndex DetailedCostModel::indexForTreeLeaf(TreeLeafItem* node) const
{
    int column = m_columns.values().indexOf(node->label());
    if (column == -1) {
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
    SnapshotItem* snapshot = m_rows.at(idx.row());
    TreeLeafItem* node = 0;
    const QString needle = m_columns.values().at(idx.column() / 2);
    foreach(TreeLeafItem* n, m_nodes[snapshot]) {
        if (n->label() == needle) {
            node = n;
            break;
        }
    }
    return QPair< TreeLeafItem*, SnapshotItem* >(node, 0);
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
