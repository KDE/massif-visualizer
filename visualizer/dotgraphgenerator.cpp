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

#include "dotgraphgenerator.h"

#include "massifdata/filedata.h"
#include "massifdata/snapshotitem.h"
#include "massifdata/treeleafitem.h"
#include "massifdata/util.h"

#include <QTextStream>
#include <QFile>
#include <QColor>
#include <QDebug>

#include <KLocalizedString>


namespace Massif {

struct GraphNode {
    const TreeLeafItem* item;
    // incoming calls + cost
    QHash<GraphNode*, quint64> children;
    // outgoing calls
    QVector<GraphNode*> parents;
    quint64 accumulatedCost;
    bool visited;
    quint32 belowThresholdCount;
    quint64 belowThresholdCost;
};

}

Q_DECLARE_TYPEINFO(Massif::GraphNode, Q_MOVABLE_TYPE);

using namespace Massif;

DotGraphGenerator::DotGraphGenerator(const SnapshotItem* snapshot, const QString& timeUnit, QObject* parent)
    : QThread(parent)
    , m_snapshot(snapshot)
    , m_node(snapshot->heapTree())
    , m_canceled(false)
    , m_timeUnit(timeUnit)
    , m_highestCost(0)
{
    m_file.open();
}

DotGraphGenerator::DotGraphGenerator(const TreeLeafItem* node, const QString& timeUnit, QObject* parent)
    : QThread(parent)
    , m_snapshot(nullptr)
    , m_node(node)
    , m_canceled(false)
    , m_timeUnit(timeUnit)
    , m_highestCost(0)
{
    m_file.open();
}

DotGraphGenerator::~DotGraphGenerator()
{
    qDebug() << "closing generator, file will get removed";
}

void DotGraphGenerator::cancel()
{
    m_canceled = true;
}

QString getLabel(const TreeLeafItem* node)
{
    QByteArray label = prettyLabel(node->label());
    const int lineWidth = 40;
    if (label.length() > lineWidth) {
        int lastPos = 0;
        int lastBreak = 0;
        while (true) {
            lastPos = label.indexOf(',', lastPos);
            if (lastPos == -1) {
                break;
            } else if (lastPos - lastBreak > lineWidth) {
                label.insert(lastPos, "\\n\\ \\ ");
                lastPos = lastPos + 4;
                lastBreak = lastPos;
                continue;
            } else {
                lastPos++;
            }
        }
    }
    return QString::fromUtf8(label);
}

QString getColor(quint64 cost, quint64 maxCost)
{
    Q_ASSERT(cost <= maxCost);
    const double ratio = (double(cost) / maxCost);
    Q_ASSERT(ratio <= 1.0);
    return QColor::fromHsv(120 - ratio * 120, (-((ratio-1) * (ratio-1))) * 255 + 255, 255, 255).name();
//     return QColor::fromHsv(120 - ratio * 120, 255, 255).name();
}

GraphNode* buildGraph(const TreeLeafItem* item, QMultiHash<QByteArray, GraphNode*>& knownNodes,
                      quint64& maxCost, GraphNode* parent = nullptr)
{
    // merge below-threshold items
    if (parent && item->children().isEmpty()) {
        static QRegExp matchBT(QStringLiteral("in ([0-9]+) places, all below massif's threshold"),
                                                Qt::CaseSensitive, QRegExp::RegExp2);
        if (matchBT.indexIn(QString::fromLatin1(item->label())) != -1) {
            parent->belowThresholdCost += item->cost();
            parent->belowThresholdCount += matchBT.cap(1).toInt();
        }
        return nullptr;
    }
    GraphNode* node = knownNodes.value(item->label(), nullptr);
    if (!node) {
        node = new GraphNode;
        knownNodes.insert(item->label(), node);
        node->item = item;
        node->accumulatedCost = 0;
        node->visited = false;
        node->belowThresholdCost = 0;
        node->belowThresholdCount = 0;
    }

    if (parent && !node->parents.contains(parent)) {
        node->parents << parent;
    }

    node->accumulatedCost += item->cost();

    if (node->accumulatedCost > maxCost) {
        maxCost = node->accumulatedCost;
    }

    foreach(TreeLeafItem* child, item->children()) {
        GraphNode* childNode = buildGraph(child, knownNodes, maxCost, node);
        if (!childNode) {
            // was below-threshold item
            continue;
        }
        QMultiHash< GraphNode*, quint64 >::iterator it = node->children.find(childNode);
        if (it != node->children.end()) {
            it.value() += child->cost();
        } else {
            node->children.insert(childNode, child->cost());
        }
    }

    return node;
}

void DotGraphGenerator::run()
{
    if (!m_file.isOpen()) {
        qWarning() << "could not create temp file for writing Dot-graph";
        return;
    }

    if (m_canceled) {
        return;
    }

    qDebug() << "creating new dot file in" << m_file.fileName();
    QTextStream out(&m_file);

    out << "digraph callgraph {\n"
           "rankdir = BT;\n";
    if (m_canceled) {
        return;
    }

    QString parentId;
    if (m_snapshot) {
        // also show some info about the selected snapshot
        parentId = QString::number((qint64) m_snapshot);
        const QString label = i18n("snapshot #%1 (taken at %2%4)\\nheap cost: %3",
                                m_snapshot->number(), m_snapshot->time(), prettyCost(m_snapshot->cost()),
                                m_timeUnit);
        out << '"' << parentId << "\" [shape=box,label=\"" << label << "\",fillcolor=white];\n";

        m_maxCost = m_snapshot->cost();
    } else if (m_node) {
        const TreeLeafItem* topMost = m_node;
        while (topMost->parent()) {
            topMost = topMost->parent();
        }
        m_maxCost = topMost->cost();
    }

    if (m_node) {
        QMultiHash<QByteArray, GraphNode*> nodes;
        GraphNode* root = buildGraph(m_node, nodes, m_maxCost);
        m_highestCost = 0;
        nodeToDot(root, out, parentId, 0);
        qDeleteAll(nodes);
    }
    out << "}\n";
    m_file.flush();
}

void DotGraphGenerator::nodeToDot(GraphNode* node, QTextStream& out, const QString& parentId, quint64 cost)
{
    if (m_canceled) {
        return;
    }

    const QString nodeId = QString::number((qint64) node);
    // write edge with annotated cost
    if (!parentId.isEmpty()) {
        // edge
        out << '"' << nodeId << "\" -> \"" << parentId << '"';
        if (cost) {
            out << " [label = \"" << prettyCost(cost) << "\"]";
        }
        out << ";\n";
    }

    if (node->visited) {
        // don't visit children again - the edge is all we need
        return;
    }
    node->visited = true;

    const bool isRoot = m_snapshot && m_snapshot->heapTree() == node->item;

    // first item we find will be the most cost-intensive one
    ///TODO this should take accumulated cost into account!
    if (m_highestCost < node->accumulatedCost && !isRoot) {
        m_costlyGraphvizId = nodeId;
        m_highestCost = node->accumulatedCost;
    }


    QString label = getLabel(node->item);
    // group nodes with same cost but different label
    // but only if they don't have any other outgoing calls, i.e. parents.size() = 1
    bool wasGrouped = false;
    while (node && node->children.count() == 1)
    {
        GraphNode* child = node->children.begin().key();
        if (child->accumulatedCost != node->accumulatedCost || node->parents.size() != 1 || child->belowThresholdCount ) {
            break;
        }
        if (m_canceled) {
            return;
        }
        node = child;

        label += QLatin1String(" | ") + QString::fromUtf8(prettyLabel(node->item->label()));
        wasGrouped = true;
    }

    QString shape;
    if (wasGrouped) {
        label = QLatin1Char('{') + label + QLatin1Char('}');
        // <...> would be an id, escape it
        label = label.replace(QLatin1Char('<'), QLatin1String("\\<"));
        label = label.replace(QLatin1Char('>'), QLatin1String("\\>"));
        shape = QStringLiteral("record");
    } else {
        shape = QStringLiteral("box");
    }

    const QString color = isRoot ? QStringLiteral("white") : getColor(node->accumulatedCost, m_maxCost);
    out << '"' << nodeId << "\" [shape=" << shape << ",label=\"" << label << "\",fillcolor=\"" << color << "\"];\n";
    if (!node) {
        return;
    }

    QMultiHash< GraphNode*, quint64 >::const_iterator it = node->children.constBegin();
    while(it != node->children.constEnd()) {
        if (m_canceled) {
            return;
        }
        nodeToDot(it.key(), out, nodeId, it.value());
        ++it;
    }
    // handle below-threshold
    if (node->belowThresholdCount) {
        // node
        const QString btLabel = i18np("in one place below threshold", "in %1 places, all below threshold", node->belowThresholdCount);
        out << '"' << nodeId << "-bt\" [shape=box,label=\"" << btLabel
            << "\",fillcolor=\"" << getColor(node->belowThresholdCost, m_maxCost) << "\"];\n";
        // edge
        out << '"' << nodeId << "-bt\" -> \"" << nodeId << "\" [label =\"" << prettyCost(node->belowThresholdCost) << "\"];\n";
    }
}

QString DotGraphGenerator::outputFile() const
{
    return m_file.fileName();
}

QString DotGraphGenerator::mostCostIntensiveGraphvizId() const
{
    return m_costlyGraphvizId;
}

#include "moc_dotgraphgenerator.cpp"
