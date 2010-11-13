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

#include "filedata.h"
#include "snapshotitem.h"

#include <QtCore/QDebug>

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

QList< SnapshotItem* > FileData::snapshots() const
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
