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

#include "snapshotitem.h"
#include "treeleafitem.h"

using namespace Massif;

SnapshotItem::SnapshotItem()
    : m_number(0), m_time(0), m_memHeap(0), m_memHeapExtra(0), m_memStacks(0), m_heapTree(0)
{
}

SnapshotItem::~SnapshotItem()
{
    delete m_heapTree;
}

void SnapshotItem::setNumber(const unsigned int num)
{
    m_number = num;
}

unsigned int SnapshotItem::number() const
{
    return m_number;
}

void SnapshotItem::setTime(const double time)
{
    m_time = time;
}

double SnapshotItem::time() const
{
    return m_time;
}

void SnapshotItem::setMemHeap(const unsigned long bytes)
{
    m_memHeap = bytes;
}

unsigned long SnapshotItem::memHeap() const
{
    return m_memHeap;
}

void SnapshotItem::setMemHeapExtra(const unsigned long bytes)
{
    m_memHeapExtra = bytes;
}

unsigned long SnapshotItem::memHeapExtra() const
{
    return m_memHeapExtra;
}

void SnapshotItem::setMemStacks(const unsigned int bytes)
{
    m_memStacks = bytes;
}

unsigned int SnapshotItem::memStacks() const
{
    return m_memStacks;
}

void SnapshotItem::setHeapTree(TreeLeafItem* root)
{
    m_heapTree = root;
}

TreeLeafItem* SnapshotItem::heapTree() const
{
    return m_heapTree;
}
