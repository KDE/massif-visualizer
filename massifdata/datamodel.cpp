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

#include "datamodel.h"
#include "snapshotitem.h"

#include <QtCore/QDebug>

using namespace Massif;

DataModel::DataModel(QObject* parent): QAbstractItemModel(parent), m_peak(0)
{

}

DataModel::~DataModel()
{
    qDeleteAll(m_snapshots);
}

QVariant DataModel::data(const QModelIndex& index, int role) const
{
    // FIXME kdchart queries (-1, -1) for empty models
    if ( index.row() == -1 || index.column() == -1 )
    {
        qWarning() << "DataModel::data: FIXME fix kdchart views to not query model data for invalid indices!";
        return QVariant();
    }

    Q_ASSERT ( index.row() >= 0 && index.row() < rowCount(index.parent()) );
    Q_ASSERT ( index.column() >= 0 && index.column() < columnCount(index.parent()) );

    if ( role != Qt::DisplayRole )
    {
        return QVariant();
    }

    if (index.parent().isValid()) {
        // detailed heap tree item
        ///FIXME: implement this properly
        return QVariant();
    } else {
        SnapshotItem* snapshot = m_snapshots.at(index.row());
        if (index.column() == 0) {
            return snapshot->number();
        } else if (index.column() == 1) {
            return QVariant::fromValue<qulonglong>(snapshot->time());
        } else if (index.column() == 2) {
            return snapshot->memHeap();
        } else if (index.column() == 3) {
            return snapshot->memHeapExtra();
        } else if (index.column() == 4) {
            return snapshot->memStacks();
        }
    }
    return QVariant();
}

int DataModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        // detailed heap tree item
        return 2;
    } else {
        // snapshot item
        return 5;
    }
}

int DataModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        // detailed heap tree item
        ///FIXME: implement this properly
        return 0;
    } else {
        // snapshot item
        return m_snapshots.count();
    }
}

QModelIndex DataModel::parent(const QModelIndex& child) const
{
    if (child.internalId()) {
        // detailed heap tree item
        ///FIXME: implement this properly
        return createIndex(0, 0, 0);
    } else {
        // snapshot items (they have no parent)
        return QModelIndex();
    }
}

QModelIndex DataModel::index(int row, int column, const QModelIndex& parent) const
{
    if (row >= 0 && row < rowCount(parent) && column > 0 && column <= columnCount(parent)) {
        quint32 id = parent.isValid() ? parent.internalId() : 0;
        return createIndex(row, column, id);
    } else {
        return QModelIndex();
    }
}

Qt::ItemFlags DataModel::flags(const QModelIndex&) const
{
    return Qt::ItemIsEnabled;
}

QVariant DataModel::headerData(int, Qt::Orientation, int) const
{
    return QVariant();
}

//BEGIN Massif File Data
void DataModel::setCmd(const QString& cmd)
{
    m_cmd = cmd;
}

QString DataModel::cmd() const
{
    return m_cmd;
}

void DataModel::setDescription(const QString& description)
{
    m_description = description;
}

QString DataModel::description() const
{
    return m_description;
}

void DataModel::setTimeUnit(const QString& unit)
{
    m_timeUnit = unit;
}

QString DataModel::timeUnit() const
{
    return m_timeUnit;
}

void DataModel::addSnapshot(Massif::SnapshotItem* snapshot)
{
    m_snapshots << snapshot;
}

QList< SnapshotItem* > DataModel::snapshots() const
{
    return m_snapshots;
}

void DataModel::setPeak(SnapshotItem* snapshot)
{
    Q_ASSERT(m_snapshots.contains(snapshot));
    m_peak = snapshot;
}

SnapshotItem* DataModel::peak() const
{
    return m_peak;
}


//END Massif File Data

