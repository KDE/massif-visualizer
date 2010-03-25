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

DotGraphGenerator::DotGraphGenerator(const SnapshotItem* snapshot, QObject* parent)
    : QThread(parent), m_snapshot(snapshot), m_canceled(false)
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

QString getLabel(TreeLeafItem* node)
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

QString getColor(unsigned int cost, unsigned int maxCost)
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
    out << "digraph " << "m_snapshot" << m_snapshot->number() << " {\n";
    if (m_canceled) {
        return;
    }
    if (m_snapshot->heapTree() && !m_snapshot->heapTree()->children().isEmpty()) {
        m_maxCost = m_snapshot->heapTree()->cost();
        foreach (TreeLeafItem* node, m_snapshot->heapTree()->children()) {
            if (m_canceled) {
                return;
            }
            nodeToDot(node, out, QString());
        }
    } else {
        const QString label = i18n("snapshot #%1 (taken at %2)\\nheap cost: %3", m_snapshot->number(), m_snapshot->time(), prettyCost(m_snapshot->memHeap()));
        const QString id = QUuid::createUuid().toString();
        out << '"' << id << "\" [shape=box,label=\"" << label << "\"];\n";
    }
    out << "}\n";
    m_file.flush();
}

void DotGraphGenerator::nodeToDot(TreeLeafItem* node, QTextStream& out, const QString& parent)
{
    if (m_canceled) {
        return;
    }
    QString id;

    if (!parent.isEmpty() && node->parent()->cost() == node->cost() && prettyLabel(node->label()) == prettyLabel(node->parent()->label())) {
        // don't add this node
        id = parent;
    } else {
        id = QUuid::createUuid().toString();
        out << '"' << id << "\" [shape=box,label=\"" << getLabel(node) << "\",fillcolor=\"" << getColor(node->cost(), m_maxCost) << "\"];\n";
        if (!parent.isEmpty()) {
            out << '"' << parent << "\" -> \"" << id << "\";\n";
        }
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
