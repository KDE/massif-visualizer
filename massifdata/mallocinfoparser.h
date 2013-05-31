/*
   This file is part of Massif Visualizer

   Copyright 2013 Milian Wolff <mail@milianw.de>

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

#ifndef MASSIF_MALLOCINFOPARSER_H
#define MASSIF_MALLOCINFOPARSER_H

#include "parserbase.h"

namespace Massif {

class SnapshotItem;
class TreeLeafItem;

class MallocInfoParser : public ParserBase
{
public:
    MallocInfoParser(Parser* parser, QIODevice* file, FileData* data,
                     const QStringList& customAllocators, QAtomicInt* shouldStop);
    virtual ~MallocInfoParser();

private:
    int m_currentLine;
    /// current snapshot that is parsed.
    SnapshotItem* m_snapshot;
    /// parent tree leaf item
    TreeLeafItem* m_parentItem;
};

}

#endif // MASSIF_MALLOCINFOPARSER_H
