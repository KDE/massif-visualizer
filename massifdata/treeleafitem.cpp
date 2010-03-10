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

Massif::TreeLeafItem::TreeLeafItem()
    : m_cost(0)
{
}

Massif::TreeLeafItem::~TreeLeafItem()
{
    qDeleteAll(m_children);
}

Massif::TreeLeafItem::setLabel(const QString label)
{
    m_label = label;
}

QString Massif::TreeLeafItem::label() const
{
    return m_label;
}

Massif::TreeLeafItem::setCost(const unsigned int bytes)
{
    m_cost = bytes;
}

unsigned int Massif::TreeLeafItem::cost() const
{
    return m_cost;
}

void Massif::TreeLeafItem::addChild(Massif::TreeLeafItem* leaf)
{
    m_children << leaf;
}

QList< Massif::TreeLeafItem* > Massif::TreeLeafItem::children() const
{
    return m_children;
}
