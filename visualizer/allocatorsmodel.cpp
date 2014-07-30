/*
   This file is part of Massif Visualizer

   Copyright 2014 Milian Wolff <mail@milianw.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "allocatorsmodel.h"

#include "massifdata/snapshotitem.h"
#include "massifdata/filedata.h"
#include "massifdata/treeleafitem.h"

#include <KLocalizedString>

using namespace Massif;

namespace {

ParsedLabel labelForNode(const TreeLeafItem* node)
{
    ParsedLabel label;
    if (!isBelowThreshold(node->label())) {
        label = parseLabel(node->label());
        label.address.clear();
    }
    return label;
}

}

AllocatorsModel::AllocatorsModel(const FileData* data, QObject* parent)
    : QAbstractItemModel(parent)
{
    m_data.reserve(1024);

    QHash<ParsedLabel, int> labelToData;
    foreach (const SnapshotItem* snapshot, data->snapshots()) {
        if (!snapshot->heapTree()) {
            continue;
        }
        foreach (const TreeLeafItem* node, snapshot->heapTree()->children()) {
            const ParsedLabel label = labelForNode(node);

            int idx = labelToData.value(label, -1);
            if (idx == -1) {
                idx = m_data.size();
                labelToData[label] = idx;
                Data data;
                data.label = label;
                data.peak = node;
                m_data << data;
            }

            Data& data = m_data[idx];
            if (data.peak->cost() < node->cost()) {
                data.peak = node;
            }
        }
    }
}

AllocatorsModel::~AllocatorsModel()
{

}

int AllocatorsModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return NUM_COLUMNS;
}

int AllocatorsModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_data.size();
}

QModelIndex AllocatorsModel::index(int row, int column, const QModelIndex& parent) const
{
    if (parent.isValid() || row < 0 || column < 0 || row >= m_data.size() || column >= NUM_COLUMNS) {
        return QModelIndex();
    }
    return createIndex(row, column);
}

QModelIndex AllocatorsModel::parent(const QModelIndex& /*child*/) const
{
    return QModelIndex();
}

QVariant AllocatorsModel::data(const QModelIndex& index, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::ToolTipRole && role != SortRole && role != ItemRole) {
        return QVariant();
    }

    if (!index.isValid() || index.parent().isValid() || index.row() < 0 || index.row() >= m_data.size()
        || index.column() < 0 || index.column() >= NUM_COLUMNS)
    {
        return QVariant();
    }

    const Data& data = m_data.value(index.row());

    if (role == Qt::ToolTipRole) {
        QString tooltip = i18n("<dt>peak cost:</dt><dd>%1</dd>", prettyCost(data.peak->cost()));
        tooltip += formatLabelForTooltip(data.label);
        return finalizeTooltip(tooltip);
    } else if (role == ItemRole) {
        return QVariant::fromValue(ModelItem(data.peak, 0));
    }

    switch (index.column()) {
        case Function:
            if (data.label.function.isEmpty()) {
                return i18n("below threshold");
            }
            return shortenTemplates(data.label.function);
        case Location:
            return data.label.location;
        case Peak:
            if (role == SortRole) {
                return data.peak->cost();
            }
            return prettyCost(data.peak->cost());
    }

    return QVariant();
}

QVariant AllocatorsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal || section < 0 || section >= NUM_COLUMNS) {
        return QVariant();
    }

    switch (section) {
        case Function:
            return i18n("Function");
        case Location:
            return i18n("Location");
        case Peak:
            return i18n("Peak");
    }

    return QVariant();
}

QModelIndex AllocatorsModel::indexForItem(const ModelItem& item) const
{
    if (!item.first) {
        return QModelIndex();
    }

    const ParsedLabel label = labelForNode(item.first);

    for (int i = 0; i < m_data.size(); ++i) {
        if (m_data[i].label == label) {
            return createIndex(i, Function);
        }
    }

    return QModelIndex();
}

void AllocatorsModel::settingsChanged()
{
    if (m_data.isEmpty()) {
        return;
    }
    // update shorten templates
    dataChanged(createIndex(0, Function),
                createIndex(m_data.size() - 1, Function));
}
