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
#include <QUuid>
#include <QColor>

#include <KLocalizedString>
#include <KTemporaryFile>

#include <KDebug>

using namespace Massif;

DotGraphGenerator::DotGraphGenerator(const SnapshotItem* snapshot, const QString& timeUnit, QObject* parent)
    : QThread(parent), m_snapshot(snapshot), m_node(snapshot->heapTree()), m_canceled(false), m_timeUnit(timeUnit)
{
    m_file.open();
}

DotGraphGenerator::DotGraphGenerator(const TreeLeafItem* node, const QString& timeUnit, QObject* parent)
    : QThread(parent), m_snapshot(0), m_node(node), m_canceled(false), m_timeUnit(timeUnit)
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
    return i18n("%1\\ncost: %2", label, prettyCost(node->cost()));
}

QString getColor(unsigned long cost, unsigned long maxCost)
{
    const double ratio = (double(cost) / maxCost);
    return QColor::fromHsv(120 - ratio * 120, (-((ratio-1) * (ratio-1))) * 255 + 255, 255, 255).name();
//     return QColor::fromHsv(120 - ratio * 120, 255, 255).name();
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
    const QString id = QUuid::createUuid().toString();
    QString label;
    if (m_snapshot) {
        label = i18n("snapshot #%1 (taken at %2%4)\\nheap cost: %3",
                                m_snapshot->number(), m_snapshot->time(), prettyCost(m_snapshot->memHeap()),
                                m_timeUnit);

        m_maxCost = m_snapshot->memHeap();
        if (m_node && m_node->children().isEmpty()) {
            m_costlyGraphvizId = id;
        }
    } else if (m_node) {
        m_costlyGraphvizId = id;
        const TreeLeafItem* topMost = m_node;
        while (topMost->parent()) {
            topMost = topMost->parent();
        }
        m_maxCost = topMost->cost();
        label = getLabel(m_node);
    }
    out << '"' << id << "\" [shape=box,label=\"" << label << "\",fillcolor=white];\n";

    if (m_node) {
        foreach (TreeLeafItem* child, m_node->children()) {
            nodeToDot(child, out, id);
        }
    }
    out << "}\n";
    m_file.flush();
}

void DotGraphGenerator::nodeToDot(TreeLeafItem* node, QTextStream& out, const QString& parent)
{
    if (m_canceled) {
        return;
    }

    const QString id = QUuid::createUuid().toString();
    if (m_costlyGraphvizId.isEmpty()) {
        m_costlyGraphvizId = id;
    }
    if (!parent.isEmpty()) {
        // edge
        out << '"' << id << "\" -> \"" << parent << "\";\n";
    }

    QString label = getLabel(node);
    const QString color = getColor(node->cost(), m_maxCost);
    // group nodes with same cost but different label
    bool wasGrouped = false;
    while (node && node->children().count() == 1 && node->children().first()->cost() == node->cost()) {
        if (m_canceled) {
            return;
        }
        node = node->children().first();

        if (prettyLabel(node->label()) != prettyLabel(node->parent()->label())) {
            label += " | " + prettyLabel(node->label());
            wasGrouped = true;
        }
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

    // node
    out << '"' << id << "\" [shape=" << shape << ",label=\"" << label << "\",fillcolor=\"" << color << "\"];\n";
    if (!node) {
        return;
    }

    foreach (TreeLeafItem* child, node->children()) {
        if (m_canceled) {
            return;
        }
        nodeToDot(child, out, id);
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
