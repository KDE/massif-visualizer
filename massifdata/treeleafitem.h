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

#ifndef TREELEAFITEM_H
#define TREELEAFITEM_H

#include <QString>
#include <QList>
#include <QMetaType>

namespace Massif {

class TreeLeafItem
{
public:
    TreeLeafItem();
    ~TreeLeafItem();

    /**
     * Sets the label for this leaf item.
     */
    void setLabel(const QByteArray& label);
    /**
     * @return The label for this leaf item.
     */
    QByteArray label() const;

    /**
     * Sets the cost for this item in bytes.
     */
    void setCost(quint64 bytes);
    /**
     * @return The cost for this item in bytes.
     */
    quint64 cost() const;

    /**
     * Adds @p leaf as child of this item.
     * This item takes ownership.
     */
    void addChild(TreeLeafItem* leaf);

    /**
     * Sets @p leafs as children of this item and takes ownership.
     * No existing children will be deleted and might get leaked
     * if you do not do this yourself.
     */
    void setChildren(const QList<TreeLeafItem*>& leafs);

    /**
     * @return The children of this leaf.
     */
    QList<TreeLeafItem*> children() const;

    /**
     * @return The parent tree leaf item or zero, if this is the root node.
     */
    TreeLeafItem* parent() const;

private:
    QByteArray m_label;
    quint64 m_cost;
    QList<TreeLeafItem*> m_children;

    TreeLeafItem* m_parent;
};

}

Q_DECLARE_METATYPE(Massif::TreeLeafItem*);
Q_DECLARE_METATYPE(const Massif::TreeLeafItem*);

#endif // TREELEAFITEM_H
