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
    void setNumber(const unsigned int num);
    /**
     * @return The number of this snapshot.
     */
    unsigned int number() const;

    /**
     * Sets the @p time at which this snapshot was taken.
     * The time can be measured in different formats,
     * @see FileData::timeUnit()
     */
    void setTime(const double time);
    /**
     * @return The time at which this snapshot was taken.
     * The time can be measured in different formats,
     * @see FileData::timeUnit()
     */
    double time() const;

    /**
     * Sets the size of the memory heap in bytes.
     */
    void setMemHeap(const unsigned long bytes);
    /**
     * @return The size of the memory heap in bytes.
     */
    unsigned long memHeap() const;

    /**
     * Sets the size of the extra memory heap in bytes.
     */
    void setMemHeapExtra(const unsigned long bytes);
    /**
     * @return The size of the extra memory heap in bytes.
     */
    unsigned long memHeapExtra() const;

    /**
     * Sets the size of the memory stacks in bytes.
     */
    void setMemStacks(const unsigned int bytes);
    /**
     * @return The size of the memory stacks in bytes.
     */
    unsigned int memStacks() const;

    /**
     * Sets @p root as root node of the detailed heap tree of this snapshot.
     */
    void setHeapTree(TreeLeafItem* root);
    /**
     * @return The root node of the detailed heap tree or zero if none is set.
     */
    TreeLeafItem* heapTree() const;

private:
    unsigned int m_number;
    double m_time;
    unsigned long m_memHeap;
    unsigned long  m_memHeapExtra;
    unsigned int m_memStacks;
    TreeLeafItem* m_heapTree;
};

}

#endif // SNAPSHOTITEM_H
