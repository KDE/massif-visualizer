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

#include "totalcostmodel.h"

#include <QtCore/QDebug>

#include "massifdata/filedata.h"
#include "massifdata/snapshotitem.h"
#include "massifdata/treeleafitem.h"

using namespace Massif;

TotalCostModel::TotalCostModel(QObject* parent): QAbstractTableModel(parent), m_data(0)
{
}

TotalCostModel::~TotalCostModel()
{
}

void TotalCostModel::setSource(const FileData* data)
{
    if (m_data) {
        beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
        m_data = 0;
        endRemoveRows();
    }
    if (data) {
        beginInsertRows(QModelIndex(), 0, data->snapshots().size() - 1);
        m_data = data;
        endInsertRows();
    }
}

QVariant TotalCostModel::data(const QModelIndex& index, int role) const
{
    qDebug() << "data requested for" << index << "and role" << role;
    // FIXME kdchart queries (-1, -1) for empty models
    if ( index.row() == -1 || index.column() == -1 ) {
        qWarning() << "TotalCostModel::data: FIXME fix kdchart views to not query model data for invalid indices!";
        return QVariant();
    }

    Q_ASSERT(index.row() >= 0 && index.row() < rowCount(index.parent()));
    Q_ASSERT(index.column() >= 0 && index.column() < columnCount(index.parent()));
    Q_ASSERT(m_data);
    Q_ASSERT(!index.parent().isValid());

    if ( role != Qt::DisplayRole ) {
        return QVariant();
    }

    SnapshotItem* snapshot = m_data->snapshots().at(index.row());
    if (index.column() % 2 == 0) {
        return QVariant::fromValue<unsigned long>(snapshot->time());
    } else {
        Q_ASSERT(index.column() == 1);
        return snapshot->memHeap();
    }
}

int TotalCostModel::columnCount(const QModelIndex& parent) const
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
