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

#ifndef MASSIF_DOTGRAPHGENERATOR_H
#define MASSIF_DOTGRAPHGENERATOR_H

#include <QTextStream>
#include <QThread>
#include <QTemporaryFile>

#include "visualizer_export.h"

namespace Massif {

class SnapshotItem;
class TreeLeafItem;
struct GraphNode;

class VISUALIZER_EXPORT DotGraphGenerator : public QThread
{
    Q_OBJECT
public:
    /**
     * Generates a Dot graph file representing @p snapshot
     * and writes it to a temporary file.
     */
    DotGraphGenerator(const SnapshotItem* snapshot, const QString& timeUnit, QObject* parent = 0);
    /**
     * Generates a Dot graph file representing @p node
     * and writes it to a temporary file.
     */
    DotGraphGenerator(const TreeLeafItem* node, const QString& timeUnit, QObject* parent = 0);
    ~DotGraphGenerator();

    /**
     * Stops generating the Dot graph file and deletes the temp file.
     */
    void cancel();

    virtual void run();
    /**
     * @return A path to the generated Dot graph file. Path might be empty if errors occurred.
     */
    QString outputFile() const;
    /**
     * @return The GraphViz node ID for the most cost-intensive tree leaf item.
     */
    QString mostCostIntensiveGraphvizId() const;

private:
    void nodeToDot(GraphNode* node, QTextStream& out, const QString& parentId = QString(), quint64 cost = 0);
    const SnapshotItem* m_snapshot;
    const TreeLeafItem* m_node;
    QTemporaryFile m_file;
    bool m_canceled;
    quint64 m_maxCost;
    QString m_timeUnit;
    QString m_costlyGraphvizId;
    quint64 m_highestCost;
};

}

#endif // MASSIF_DOTGRAPHGENERATOR_H
