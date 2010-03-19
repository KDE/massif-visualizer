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

#include "parser.h"

#include "filedata.h"
#include "parserprivate.h"
#include "snapshotitem.h"

#include <QtCore/QIODevice>

#include <QtCore/QDebug>

using namespace Massif;

Parser::Parser()
    : m_errorLine(-1)
{
}

Parser::~Parser()
{
}

FileData* Parser::parse(QIODevice* file)
{
    Q_ASSERT(file->isOpen());
    Q_ASSERT(file->isReadable());

    FileData* data = new FileData;

    ParserPrivate p(file, data);

    if (p.error()) {
        delete data;
        data = 0;
        m_errorLine = p.errorLine();
        m_errorLineString = p.errorLineString();
    } else {
        m_errorLine = -1;
        m_errorLineString.clear();
    }

    // if there is no peak marked, it might be that we have a massif file
    // that got aborted somewhere in the middle. hence lets find it ourself
    if (!data->peak() && !data->snapshots().isEmpty()) {
        foreach ( SnapshotItem* snapshot, data->snapshots() ) {
            if (!snapshot->heapTree()) {
                // peak should have detailed info
                continue;
            }
            if (!data->peak() || snapshot->memHeap() > data->peak()->memHeap()) {
                data->setPeak(snapshot);
            }
        }
    }

    return data;
}

int Parser::errorLine() const
{
    return m_errorLine;
}

QString Parser::errorLineString() const
{
    return m_errorLineString;
}
