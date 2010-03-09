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

#ifndef MASSIF_DATAMODEL_H
#define MASSIF_DATAMODEL_H

#include <qt4/QtCore/QModelIndex>

namespace Massif {

class SnapshotItem;

/**
 * A model that stores all information one can extract from a Massif output file.
 */
class DataModel : public QAbstractItemModel
{
public:
    DataModel(QObject* parent = 0);
    virtual ~DataModel();

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    /**
     * The top-most items (i.e. with invalid parents) are snapshots which have four columns.
     * Everything below is a detailed entry in the heap tree and has two columns.
     */
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex& child) const;
    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

    virtual Qt::ItemFlags flags(const QModelIndex& index) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    /**
     * Set the @p cmd that was profiled with massif.
     */
    void setCmd(const QString& cmd);
    /**
     * @return Command that was profiled with massif.
     */
    QString cmd() const;

    /**
     * Set a @p description.
     */
    void setDescription(const QString& description);
    /**
     * @return Description for this massif run.
     */
    QString description() const;

    /**
     * Set the @p unit that times are measured in.
     */
    void setTimeUnit(const QString& unit);
    /**
     * @return The unit that times are measured in.
     */
    QString timeUnit() const;

private:
    QString m_cmd;
    QString m_description;
    QString m_timeUnit;
    QList<SnapshotItem*> m_snapshots;
};

}

#endif // MASSIF_DATAMODEL_H
