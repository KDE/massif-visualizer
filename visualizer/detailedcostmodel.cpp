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
                    if (node->label().contains(QString("all below massif's threshold"), Qt::CaseSensitive)) {
                        continue;
                    }
                    if (!m_columns.values().contains(node->label())) {
                        m_columns[node->cost()] = node->label();
                    } else {
                        unsigned int cost = m_columns.key(node->label());
                        m_columns.remove(cost);
                        cost += node->cost();
                        m_columns[cost] = node->label();
                    }
                    nodes << node;
                }
                m_rows << snapshot;
                m_nodes[snapshot] = nodes;
            }
        }
        // limit number of colums
        const int maxColumns = 10;
        if ( m_columns.size() > maxColumns ) {
            QMap< unsigned int, QString >::iterator it = m_columns.begin();
            for ( int i = 0; i < m_columns.size() - maxColumns; ++i ) {
                it = m_columns.erase(it);
            }
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

    if ( role != Qt::DisplayRole ) {
        return QVariant();
    }

    SnapshotItem* snapshot = m_rows[index.row()];
    if (index.column() % 2 == 0) {
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
            return 0;
        } else {
            return double(node->cost());
        }
    }
}

QVariant DetailedCostModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section % 2 == 0) {
        QString label = m_columns.values().at(section / 2);
        // only show name without memory address or location
        int startPos = label.indexOf(':');
        if (startPos == -1) {
            return label;
        }
        if (label.indexOf("???") != -1) {
            return label.mid(startPos + 1);
        }
        int endPos = label.lastIndexOf('(');
        if (endPos == -1) {
            return label;
        }
        return label.mid(startPos + 1, endPos - startPos - 2);
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

QList< QString > Massif::DetailedCostModel::labels() const
{
    return m_columns.values();
}
