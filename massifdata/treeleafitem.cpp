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

void TreeLeafItem::setLabel(const QString label)
{
    m_label = label;
}

QString TreeLeafItem::label() const
{
    return m_label;
}

void TreeLeafItem::setCost(const unsigned int bytes)
{
    m_cost = bytes;
}

unsigned int TreeLeafItem::cost() const
{
    return m_cost;
}

void TreeLeafItem::addChild(TreeLeafItem* leaf)
{
    leaf->m_parent = this;
    m_children << leaf;
}

QList< TreeLeafItem* > TreeLeafItem::children() const
{
    return m_children;
}

TreeLeafItem* TreeLeafItem::parent() const
{
    return m_parent;
}

