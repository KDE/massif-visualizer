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

#include "filtereddatatreemodel.h"
#include "datatreemodel.h"

#include <QDebug>

using namespace Massif;

FilteredDataTreeModel::FilteredDataTreeModel(DataTreeModel* parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setSourceModel(parent);
}

void FilteredDataTreeModel::setFilter(const QString& needle)
{
    m_needle = needle;
    invalidate();
}

bool FilteredDataTreeModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    const QModelIndex& dataIdx = sourceModel()->index(source_row, 0, source_parent);
    Q_ASSERT(dataIdx.isValid());
    if (sourceModel()->data(dataIdx).toString().contains(m_needle, Qt::CaseInsensitive)) {
        return true;
    } else {
        const int rows = sourceModel()->rowCount(dataIdx);
        for ( int i = 0; i < rows; ++i ) {
            if ( filterAcceptsRow(i, dataIdx) ) {
                return true;
            }
        }
        return false;
    }
}

bool FilteredDataTreeModel::filterAcceptsColumn(int, const QModelIndex&) const
{
    return true;
}

void FilteredDataTreeModel::setSourceModel(QAbstractItemModel* sourceModel)
{
    QSortFilterProxyModel::setSourceModel(sourceModel);
}

#include "filtereddatatreemodel.moc"
