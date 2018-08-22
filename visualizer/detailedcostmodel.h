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

#ifndef MASSIF_DETAILEDCOSTMODEL_H
#define MASSIF_DETAILEDCOSTMODEL_H

#include <QAbstractTableModel>
#include <QVector>

#include "modelitem.h"

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
    explicit DetailedCostModel(QObject* parent = 0);
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
     * Sets the maximum number of datasets in this model to @p count.
     */
    void setMaximumDatasetCount(int count);

    int maximumDatasetCount() const;

    typedef QMap<QModelIndex, const TreeLeafItem*> Peaks;
    /**
     * @return List of peaks with their heap tree leaf items.
     */
    Peaks peaks() const;

    /**
     * @return ModelItem for given index. At maximum one of the pointers in the pair will be valid.
     */
    ModelItem itemForIndex(const QModelIndex& idx) const;

    /**
     * @return Index for given item. Only one of the pointers in the pair should be valid.
     */
    QModelIndex indexForItem(const ModelItem& item) const;

    /**
     * @return Index for given snapshot, or invalid if it's not a detailed snapshot.
     */
    QModelIndex indexForSnapshot(const SnapshotItem* snapshot) const;

    /**
     * @return Index for given TreeLeafItem, or invalid if it's not covered by this model.
     */
    QModelIndex indexForTreeLeaf(const TreeLeafItem* node) const;

    /**
     * Select @p index, which changes the graphical representation of its data.
     */
    void setSelection(const QModelIndex& index);

    /**
     * Hide function @p func.
     */
    void hideFunction(const TreeLeafItem* node);

    /**
     * Hide all functions except for @p func.
     */
    void hideOtherFunctions(const TreeLeafItem* node);

private:
    const FileData* m_data;
    // columns => label
    QVector<QByteArray> m_columns;
    // only to sort snapshots by number
    QVector<const SnapshotItem*> m_rows;
    typedef QHash<const SnapshotItem*, QVector<const TreeLeafItem*> > Nodes;
    // snapshot item => cost intensive nodes
    Nodes m_nodes;
    // peaks: Label => TreeLeafItem,Snapshot
    QHash<QByteArray, ModelItem> m_peaks;
    // selected item
    QModelIndex m_selection;
    int m_maxDatasetCount;
};

}

#endif // MASSIF_DETAILEDCOSTMODEL_H
