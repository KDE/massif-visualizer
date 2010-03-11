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

#ifndef MASSIF_DATATREEMODEL_H
#define MASSIF_DATATREEMODEL_H

#include <QtCore/QAbstractItemModel>

namespace Massif {

class FileData;
class TreeLeafItem;
class SnapshotItem;

/**
 * A model that gives a tree representation of the full Massif data. Useful for e.g. ListViews.
 */
class DataTreeModel : public QAbstractItemModel
{
public:
    DataTreeModel(QObject* parent = 0);
    virtual ~DataTreeModel();

    /**
     * That the source data for this model.
     */
    void setSource(const FileData* data);

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex& child) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

private:
    const FileData* m_data;

    void mapNodeToRow(TreeLeafItem* node, const int row);
    QMap<TreeLeafItem*, int> m_nodeToRow;
    QMap<TreeLeafItem*, SnapshotItem*> m_heapRootToSnapshot;
};

}

#endif // MASSIF_DATATREEMODEL_H