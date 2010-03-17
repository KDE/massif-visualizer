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

#include "filtereddatatreemodel.h"

#include "datatreemodel.h"

#include <QDebug>

using namespace Massif;

FilteredDataTreeModel::FilteredDataTreeModel(DataTreeModel* source)
    : QSortFilterProxyModel(source)
{
    setSourceModel(source);
}

bool FilteredDataTreeModel::filterAcceptsColumn(int, const QModelIndex&) const
{
    return true;
}

bool FilteredDataTreeModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if (m_needle.isEmpty()) {
//         qDebug() << "empty needle:" << sourceModel()->index(source_row, 1, source_parent) << "is fine";
        return true;
    }
    const QModelIndex& dataIdx = sourceModel()->index(source_row, 1, source_parent);
    if (!dataIdx.isValid()) {
//         qDebug() << "!!!!!!!!!invalid idx";
        return false;
    }
//     int parents = 0;
//     QModelIndex p = source_parent;
//     while (p.isValid()) {
//         p = p.parent();
//         ++parents;
//     }
//     QString indent = QString().fill(' ', parents);

    if (dataIdx.data().toString().contains(m_needle, Qt::CaseInsensitive)) {
//         qDebug() << indent.fill('X') << "match in" << dataIdx << m_needle << "VS" << dataIdx.data();
        return true;
    } else {
        const QModelIndex& newParent = sourceModel()->index(source_row, 0, source_parent);
        const int rows = sourceModel()->rowCount(newParent);
//         qDebug() << indent << "search in children of" << dataIdx.data().toString() << "with" << rows << "rows";
        for (int i = 0; i < rows; ++i) {
            if (filterAcceptsRow(i, newParent)) {
//                 qDebug() << indent << "ÖÖÖÖÖÖÖÖÖÖÖÖÖchild matched";
                return true;
            }
        }
//         qDebug() << indent << "no match in" << dataIdx << dataIdx.data();
        return false;
    }
}

void FilteredDataTreeModel::setFilter(const QString& needle)
{
    m_needle = needle;
    invalidateFilter();
}

#include "filtereddatatreemodel.moc"
