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

#ifndef MASSIF_FILTEREDDATATREEMODEL_H
#define MASSIF_FILTEREDDATATREEMODEL_H

#include <QSortFilterProxyModel>


namespace Massif {

class DataTreeModel;

/**
 * Filter a DataTreeModel and show any branch that has a node
 * that matches the needle.
 */
class FilteredDataTreeModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit FilteredDataTreeModel(DataTreeModel* source);

public slots:
    void setFilter(const QString& needle);

protected:
    virtual bool filterAcceptsColumn(int source_column, const QModelIndex& source_parent) const;
    virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;

private:
    QString m_needle;
};

}

#endif // MASSIF_FILTEREDDATATREEMODEL_H
