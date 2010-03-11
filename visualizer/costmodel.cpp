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

#include "costmodel.h"

#include <QtCore/QDebug>

#include "massifdata/filedata.h"
#include "massifdata/snapshotitem.h"
#include "massifdata/treeleafitem.h"

using namespace Massif;

CostModel::CostModel(QObject* parent): QAbstractTableModel(parent), m_data(0)
{
}

CostModel::~CostModel()
{
}

void CostModel::setSource(const FileData* data)
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

QVariant CostModel::data(const QModelIndex& index, int role) const
{
    qDebug() << "data requested for" << index << "and role" << role;
    // FIXME kdchart queries (-1, -1) for empty models
    if ( index.row() == -1 || index.column() == -1 ) {
        qWarning() << "CostModel::data: FIXME fix kdchart views to not query model data for invalid indices!";
        return QVariant();
    }

    Q_ASSERT ( index.row() >= 0 && index.row() < rowCount(index.parent()) );
    Q_ASSERT ( index.column() >= 0 && index.column() < columnCount(index.parent()) );
    Q_ASSERT ( m_data );

    if ( role != Qt::DisplayRole ) {
        return QVariant();
    }

    if (index.parent().isValid()) {
        // detailed heap tree item
        ///FIXME: implement this properly
        return QVariant();
    } else {
        SnapshotItem* snapshot = m_data->snapshots().at(index.row());
        if (index.column() % 2 == 0) {
            return QVariant::fromValue<unsigned long>(snapshot->time());
        } else if (index.column() == 1) {
            return snapshot->memHeap();
        } else {
            int item = index.column() / 2 - 1;
            if ( !snapshot->heapTree() || snapshot->heapTree()->children().size() <= item ) {
                return 0;
            } else {
                return snapshot->heapTree()->children().at(item)->cost();
            }
        }
    }
    return QVariant();
}

int CostModel::columnCount(const QModelIndex& parent) const
{
    return 2 * 5;
}

int CostModel::rowCount(const QModelIndex& parent) const
{
    if (!m_data) {
        return 0;
    }

    if (parent.isValid()) {
        // detailed heap tree item
        ///FIXME: implement this properly
        return 0;
    } else {
        // snapshot item
        return m_data->snapshots().count();
    }
}
