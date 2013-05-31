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

#include "parser.h"

#include "filedata.h"
#include "massifparser.h"
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

FileData* Parser::parse(QIODevice* file, const QStringList& customAllocators, QAtomicInt* shouldStop)
{
    Q_ASSERT(file->isOpen());
    Q_ASSERT(file->isReadable());

    QScopedPointer<FileData> data(new FileData);
    QScopedPointer<ParserBase> parser;

    parser.reset(new MassifParser(this, file, data.data(), customAllocators, shouldStop));

    Q_ASSERT(parser);
    if (parser->error()) {
        m_errorLine = parser->errorLine();
        m_errorLineString = parser->errorLineString();
        return 0;
    } else {
        m_errorLine = -1;
        m_errorLineString.clear();
    }

    // when a massif run gets terminated (^C) the snapshot data might be wrong,
    // hence just always ensure we pick the proper peak ourselves
    if (!data->snapshots().isEmpty()) {
        foreach ( SnapshotItem* snapshot, data->snapshots() ) {
            if (!snapshot->heapTree()) {
                // peak should have detailed info
                continue;
            }
            if (!data->peak() || snapshot->cost() > data->peak()->cost()) {
                data->setPeak(snapshot);
            }
        }
        // still not found? pick any other snapshot
        if (!data->peak()) {
            foreach( SnapshotItem* snapshot, data->snapshots() ) {
                if (!data->peak() || snapshot->cost() > data->peak()->cost()) {
                    data->setPeak(snapshot);
                }
            }
        }
    }
    // peak might still be zero if we have no snapshots, should be handled in the UI then

    return data.take();
}

int Parser::errorLine() const
{
    return m_errorLine;
}

QByteArray Parser::errorLineString() const
{
    return m_errorLineString;
}

void Parser::setProgress(int value)
{
    emit progress(value);
}

#include "parser.moc"
