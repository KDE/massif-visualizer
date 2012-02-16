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

#ifndef SNAPSHOTITEM_H
#define SNAPSHOTITEM_H

#include "massifdata_export.h"

namespace Massif
{

class TreeLeafItem;

class MASSIFDATA_EXPORT SnapshotItem
{
public:
    SnapshotItem();
    ~SnapshotItem();

    /**
     * Sets the number of this snapshot.
     */
    void setNumber(uint num);
    /**
     * @return The number of this snapshot.
     */
    uint number() const;

    /**
     * Sets the @p time at which this snapshot was taken.
     * The time can be measured in different formats,
     * @see FileData::timeUnit()
     */
    void setTime(double time);
    /**
     * @return The time at which this snapshot was taken.
     * The time can be measured in different formats,
     * @see FileData::timeUnit()
     */
    double time() const;

    /**
     * Sets the size of the memory heap in bytes.
     */
    void setMemHeap(quint64 bytes);
    /**
     * @return The size of the memory heap in bytes.
     */
    quint64 memHeap() const;

    /**
     * Sets the size of the extra memory heap in bytes.
     */
    void setMemHeapExtra(quint64 bytes);
    /**
     * @return The size of the extra memory heap in bytes.
     */
    quint64 memHeapExtra() const;

    /**
     * Sets the size of the memory stacks in bytes.
     */
    void setMemStacks(quint64 bytes);
    /**
     * @return The size of the memory stacks in bytes.
     */
    quint64 memStacks() const;

    /**
     * Sets @p root as root node of the detailed heap tree of this snapshot.
     */
    void setHeapTree(TreeLeafItem* root);
    /**
     * @return The root node of the detailed heap tree or zero if none is set.
     */
    TreeLeafItem* heapTree() const;

private:
    uint m_number;
    double m_time;
    quint64 m_memHeap;
    quint64  m_memHeapExtra;
    quint64 m_memStacks;
    TreeLeafItem* m_heapTree;
};

}

#endif // SNAPSHOTITEM_H
