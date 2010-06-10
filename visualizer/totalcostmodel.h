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

#ifndef MASSIF_TOTALCOSTMODEL_H
#define MASSIF_TOTALCOSTMODEL_H

#include <QBrush>
#include <QPair>
#include <QtCore/QAbstractTableModel>

namespace Massif {

class FileData;
class TreeLeafItem;
class SnapshotItem;

/**
 * A model that gives a tabular access on the costs in a massif output file.
 */
class TotalCostModel : public QAbstractTableModel
{
public:
    TotalCostModel(QObject* parent = 0);
    virtual ~TotalCostModel();

    /**
     * That the source data for this model.
     */
    void setSource(const FileData* data);

    /**
     * @return Index of the peak dataset.
     */
    QModelIndex peak() const;

    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    /**
     * @return Item for given index. At maximum one of the pointers in the pair will be valid.
     */
    QPair<TreeLeafItem*, SnapshotItem*> itemForIndex(const QModelIndex& idx) const;

    /**
     * @return Index for given item. Only one of the pointers in the pair should be valid.
     */
    QModelIndex indexForItem(const QPair<TreeLeafItem*, SnapshotItem*>& item) const;

    /**
     * @return Index for given snapshot, or invalid if it's not a detailed snapshot.
     */
    QModelIndex indexForSnapshot(SnapshotItem* snapshot) const;

    /**
     * @return Index for given TreeLeafItem, or invalid if it's not covered by this model.
     */
    QModelIndex indexForTreeLeaf(TreeLeafItem* node) const;

    /**
     * Select @p index, which changes the graphical representation of its data.
     */
    void setSelection(const QModelIndex& index);

private:
    const FileData* m_data;
    // selected item
    QModelIndex m_selection;
};

}

#endif // MASSIF_TOTALCOSTMODEL_H
