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

#ifndef MASSIF_DETAILEDCOSTMODEL_H
#define MASSIF_DETAILEDCOSTMODEL_H

#include <QtCore/QAbstractTableModel>
#include <QtCore/QMultiMap>
#include <QtCore/QStringList>

namespace Massif {

class FileData;
class SnapshotItem;
class TreeLeafItem;

/**
 * A model that gives a tabular access on the costs in a massif output file.
 */
class DetailedCostModel : public QAbstractTableModel
{
public:
    DetailedCostModel(QObject* parent = 0);
    virtual ~DetailedCostModel();

    /**
     * That the source data for this model.
     */
    void setSource(const FileData* data);

    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    /**
     * @return List of peaks with their heap tree leaf items.
     */
    QMap<QModelIndex, TreeLeafItem*> peaks() const;

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
    // only a map to sort it by total cost
    // total cost => label
    QMultiMap<unsigned int, QString> m_columns;
    // only to sort snapshots by number
    QList<SnapshotItem*> m_rows;
    // snapshot item => cost intensive nodes
    QMap<SnapshotItem*, QList<TreeLeafItem*> > m_nodes;
    // peaks: Label => TreeLeafItem,Snapshot
    QMap<QString, QPair<TreeLeafItem*,SnapshotItem*> > m_peaks;
    // selected item
    QModelIndex m_selection;
};

}

#endif // MASSIF_DETAILEDCOSTMODEL_H
