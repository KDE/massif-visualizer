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

#ifndef MASSIF_FILEDATA_H
#define MASSIF_FILEDATA_H

#include "massifdata_export.h"

#include <QtCore/QObject>

namespace Massif {

class SnapshotItem;

/**
 * This structure holds all information that can be extracted from a massif output file.
 */
class MASSIFDATA_EXPORT FileData : public QObject
{
    Q_OBJECT
public:
    FileData(QObject* parent = 0);
    virtual ~FileData();

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

    /**
     * Adds @p snapshot to this dataset and takes ownership.
     */
    void addSnapshot(SnapshotItem* snapshot);
    /**
     * @return List of all snapshots that make up this dataset.
     */
    QList<SnapshotItem*> snapshots() const;

    /**
     * Marks @p snapshot as peak of this dataset.
     * The snapshot should already been added to the dataset.
     */
    void setPeak(SnapshotItem* snapshot);
    /**
     * @return The peak snapshot in this dataset.
     */
    SnapshotItem* peak() const;

private:
    QString m_cmd;
    QString m_description;
    QString m_timeUnit;
    QList<SnapshotItem*> m_snapshots;
    SnapshotItem* m_peak;
};

}

#endif // MASSIF_FILEDATA_H
