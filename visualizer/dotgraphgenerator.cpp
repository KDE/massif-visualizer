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

#include "dotgraphgenerator.h"

#include "massifdata/filedata.h"
#include "massifdata/snapshotitem.h"
#include "massifdata/treeleafitem.h"

#include "util.h"

#include <QTextStream>
#include <QFile>
#include <QColor>

#include <KLocalizedString>
#include <KTemporaryFile>

#include <KDebug>

namespace Massif {

struct GraphNode {
    const TreeLeafItem* item;
    // incoming calls + cost
    QHash<GraphNode*, unsigned long> children;
    // outgoing calls
    QVector<GraphNode*> parents;
    unsigned long accumulatedCost;
    bool visited;
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
    , m_snapshot(0)
    , m_node(node)
    , m_canceled(false)
    , m_timeUnit(timeUnit)
    , m_highestCost(0)
{
    m_file.open();
}

DotGraphGenerator::~DotGraphGenerator()
{
    kDebug() << "closing generator, file will get removed";
}

void DotGraphGenerator::cancel()
{
    m_canceled = true;
}

QString getLabel(const TreeLeafItem* node)
{
    QString label = prettyLabel(node->label());
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
    return label;
}

QString getColor(unsigned long cost, unsigned long maxCost)
{
    const double ratio = (double(cost) / maxCost);
    return QColor::fromHsv(120 - ratio * 120, (-((ratio-1) * (ratio-1))) * 255 + 255, 255, 255).name();
//     return QColor::fromHsv(120 - ratio * 120, 255, 255).name();
}

GraphNode* buildGraph(const TreeLeafItem* item, QMultiHash<QString, GraphNode*>& knownNodes, GraphNode* parent = 0)
{
    // reuse existing node if possible - but not for below-threshold items!
    GraphNode* node = !isBelowThreshold(item->label()) ? knownNodes.value(item->label(), 0) : 0;
    if (!node) {
        node = new GraphNode;
        knownNodes.insert(item->label(), node);
        node->item = item;
        node->accumulatedCost = 0;
        node->visited = false;
    }

    if (parent && !node->parents.contains(parent)) {
        node->parents << parent;
    }

    node->accumulatedCost += item->cost();

    foreach(TreeLeafItem* child, item->children()) {
        GraphNode* childNode = buildGraph(child, knownNodes, node);
        QMultiHash< GraphNode*, unsigned long >::iterator it = node->children.find(childNode);
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
        kWarning() << "could not create temp file for writing Dot-graph";
        return;
    }

    if (m_canceled) {
        return;
    }

    kDebug() << "creating new dot file in" << m_file.fileName();
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
                                m_snapshot->number(), m_snapshot->time(), prettyCost(m_snapshot->memHeap()),
                                m_timeUnit);
        out << '"' << parentId << "\" [shape=box,label=\"" << label << "\",fillcolor=white];\n";

        m_maxCost = m_snapshot->memHeap();
    } else if (m_node) {
        const TreeLeafItem* topMost = m_node;
        while (topMost->parent()) {
            topMost = topMost->parent();
        }
        m_maxCost = topMost->cost();
    }

    if (m_node) {
        QMultiHash<QString, GraphNode*> nodes;
        GraphNode* root = buildGraph(m_node, nodes);
        m_highestCost = 0;
        nodeToDot(root, out, parentId, 0);
        qDeleteAll(nodes);
    }
    out << "}\n";
    m_file.flush();
}

void DotGraphGenerator::nodeToDot(GraphNode* node, QTextStream& out, const QString& parentId, unsigned long cost)
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
        if (child->accumulatedCost != node->accumulatedCost || node->parents.size() != 1) {
            break;
        }
        if (m_canceled) {
            return;
        }
        node = child;

        label += " | " + prettyLabel(node->item->label());
        wasGrouped = true;
    }

    QString shape;
    if (wasGrouped) {
        label = "{" + label + "}";
        // <...> would be an id, escape it
        label = label.replace('<', "\\<");
        label = label.replace('>', "\\>");
        shape = "record";
    } else {
        shape = "box";
    }

    const QString color = isRoot ? "white" : getColor(node->accumulatedCost, m_maxCost);
    out << '"' << nodeId << "\" [shape=" << shape << ",label=\"" << label << "\",fillcolor=\"" << color << "\"];\n";
    if (!node) {
        return;
    }

    QMultiHash< GraphNode*, unsigned long >::const_iterator it = node->children.constBegin();
    while(it != node->children.constEnd()) {
        if (m_canceled) {
            return;
        }
        nodeToDot(it.key(), out, nodeId, it.value());
        ++it;
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
