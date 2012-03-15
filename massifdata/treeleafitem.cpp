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

#include "treeleafitem.h"

using namespace Massif;

TreeLeafItem::TreeLeafItem()
    : m_cost(0), m_parent(0)
{
}

TreeLeafItem::~TreeLeafItem()
{
    qDeleteAll(m_children);
}

void TreeLeafItem::setLabel(const QByteArray& label)
{
    m_label = label;
}

QByteArray TreeLeafItem::label() const
{
    return m_label;
}

void TreeLeafItem::setCost(quint64 bytes)
{
    m_cost = bytes;
}

quint64 TreeLeafItem::cost() const
{
    return m_cost;
}

void TreeLeafItem::addChild(TreeLeafItem* leaf)
{
    leaf->m_parent = this;
    m_children << leaf;
}

void TreeLeafItem::setChildren(const QList< TreeLeafItem* >& leafs)
{
    m_children = leafs;
}

QList< TreeLeafItem* > TreeLeafItem::children() const
{
    return m_children;
}

TreeLeafItem* TreeLeafItem::parent() const
{
    return m_parent;
}
