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

#include <KLocalizedString>
#include <KTemporaryFile>

#include <KDebug>

using namespace Massif;

DotGraphGenerator::DotGraphGenerator(const SnapshotItem* m_snapshot, QObject* parent)
    : QThread(parent), m_snapshot(m_snapshot), m_canceled(false)
{
}

DotGraphGenerator::~DotGraphGenerator()
{
    kDebug() << "closing generator, file will get removed";
}

void DotGraphGenerator::cancel()
{
    m_canceled = true;
}

void DotGraphGenerator::run()
{
    if (!m_file.open()) {
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
    if (m_snapshot->heapTree()) {
        foreach (TreeLeafItem* node, m_snapshot->heapTree()->children()) {
            if (m_canceled) {
                return;
            }

            const QString label = i18n("%1\\ncost: %2", node->label(), prettyCost(node->cost()));
            const QString id = QUuid::createUuid().toString();
            out << '"' << id << "\" [shape=box,label=\"" << label << "\"];\n";
            nodeToDot(node, out, id);
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
    const QString id = QUuid::createUuid().toString();
    out << '"' << id << "\" [label=\"" << i18n("%1\\ncost: %2", node->label(), prettyCost(node->cost())) << "\"];\n";
    out << '"' << parent << "\" -> \"" << id << "\";\n";
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
