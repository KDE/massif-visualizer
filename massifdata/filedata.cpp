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

#include "filedata.h"
#include "snapshotitem.h"

using namespace Massif;

FileData::FileData(QObject* parent) : QObject(parent), m_peak(0)
{
}

FileData::~FileData()
{
    qDeleteAll(m_snapshots);
}

void FileData::setCmd(const QString& cmd)
{
    m_cmd = cmd;
}

QString FileData::cmd() const
{
    return m_cmd;
}

void FileData::setDescription(const QString& description)
{
    m_description = description;
}

QString FileData::description() const
{
    return m_description;
}

void FileData::setTimeUnit(const QString& unit)
{
    m_timeUnit = unit;
}

QString FileData::timeUnit() const
{
    return m_timeUnit;
}

void FileData::addSnapshot(SnapshotItem* snapshot)
{
    m_snapshots << snapshot;
}

QVector< SnapshotItem* > FileData::snapshots() const
{
    return m_snapshots;
}

void FileData::setPeak(SnapshotItem* snapshot)
{
    Q_ASSERT(m_snapshots.contains(snapshot));
    m_peak = snapshot;
}

SnapshotItem* FileData::peak() const
{
    return m_peak;
}
