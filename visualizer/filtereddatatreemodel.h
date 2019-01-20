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

#ifndef MASSIF_FILTEREDDATATREEMODEL_H
#define MASSIF_FILTEREDDATATREEMODEL_H

#include <QSortFilterProxyModel>
#include <QTimer>

namespace Massif {

class DataTreeModel;

/**
 * Filter class for DataTreeModel
 */
class FilteredDataTreeModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit FilteredDataTreeModel(DataTreeModel* parent);

public Q_SLOTS:
    void setFilter(const QString& needle);

protected:
    /// true for any branch that has an item in it that matches m_needle
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
    /// always true
    bool filterAcceptsColumn(int source_column, const QModelIndex& source_parent) const override;

private Q_SLOTS:
    void timeout();

private:
    /// we don't want that
    void setSourceModel(QAbstractItemModel* sourceModel) override;

    /// search string that should be contained in the data (case insensitively)
    QString m_needle;
    QTimer m_timer;
};

}

#endif // MASSIF_FILTEREDDATATREEMODEL_H
