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

#include <QtCore/QAbstractTableModel>

namespace Massif {

class FileData;

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

    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;

private:
    const FileData* m_data;
};

}

#endif // MASSIF_TOTALCOSTMODEL_H
