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

#include "snapshotitem.h"

SnapshotItem::SnapshotItem()
    : m_time(0), m_memHeap(0), m_memHeapExtra(0), m_memStacks(0)
{
}

SnapshotItem::~SnapshotItem()
{
}

void SnapshotItem::setTime(const std::time_t time)
{
    m_time = time;
}

std::time_t SnapshotItem::time() const
{
    return m_time;
}

void SnapshotItem::setMemHeap(const unsigned int bytes)
{
    m_memHeap = bytes;
}

unsigned int SnapshotItem::memHeap() const
{
    return m_memHeap;
}

void SnapshotItem::setMemHeapExtra(const unsigned int bytes)
{
    m_memHeapExtra = bytes;
}

unsigned int SnapshotItem::memHeapExtra() const
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
