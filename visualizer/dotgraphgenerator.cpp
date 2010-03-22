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

#include <QTextStream>

#include "massifdata/filedata.h"
#include "massifdata/snapshotitem.h"
#include "massifdata/treeleafitem.h"

#include <KLocalizedString>
#include <QFile>

#include <KDebug>

#include <KTemporaryFile>

using namespace Massif;

DotGraphGenerator::DotGraphGenerator(const SnapshotItem* m_snapshot, QObject* parent)
    : QThread(parent), m_snapshot(m_snapshot), m_canceled(false)
{

}

DotGraphGenerator::~DotGraphGenerator()
{

}

void DotGraphGenerator::cancel()
{
    m_canceled = true;
}

#define checkCancel() if (m_canceled) { file.close(); file.remove(); return; }

void DotGraphGenerator::run()
{
    KTemporaryFile file;
    file.setAutoRemove(false);
    if (!file.open()) {
        kWarning() << "could not create temp file for writing Dot-graph";
        return;
    }

    checkCancel()

    m_file = file.fileName();
    QTextStream out(&file);
    out << "digraph " << "m_snapshot" << m_snapshot->number() << " {\n";
    const QString label = i18n("m_snapshot #%1 (taken at %2)\\nheap cost: %3 bytes", m_snapshot->number(), m_snapshot->time(), m_snapshot->memHeap());
    const QString id = "s" + QString::number(m_snapshot->number());
    out << id << "[shape=box,label=\"" << label << "\"];\n";
    checkCancel()
    if (m_snapshot->heapTree()) {
        foreach (TreeLeafItem* node, m_snapshot->heapTree()->children()) {
            checkCancel()
            nodeToDot(node, out, id);
        }
    }
    out << "}\n";
    file.close();
}

void DotGraphGenerator::nodeToDot(TreeLeafItem* node, QTextStream& out, const QString& parent)
{
    const QString id = node->label().mid(0, node->label().indexOf(':') - 1);
    out << '"' << id << '"' << " [label=\"" << i18n("%1\\ncost: %2 bytes", node->label(), node->cost()) << "\"];\n";
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
    return m_file;
}
