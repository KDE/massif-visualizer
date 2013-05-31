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

#ifndef MASSIF_MASSIFPARSER_H
#define MASSIF_MASSIFPARSER_H

#include "parserbase.h"

#include <QtCore/QByteArray>
#include <QtCore/QStringList>
#include <QSet>

namespace Massif {

class SnapshotItem;
class TreeLeafItem;

class MassifParser : public ParserBase
{
public:
    MassifParser(Parser* parser, QIODevice* file, FileData* data,
                       const QStringList& customAllocators, QAtomicInt* shouldStop);
    virtual ~MassifParser();

private:
    void parseFileDesc(const QByteArray& line);
    void parseFileCmd(const QByteArray& line);
    void parseFileTimeUnit(const QByteArray& line);
    void parseSnapshot(const QByteArray& line);
    void parseSnapshotHeapTree(const QByteArray& line);
    void parseSnapshotMemHeap(const QByteArray& line);
    void parseSnapshotMemHeapExtra(const QByteArray& line);
    void parseSnapshotTime(const QByteArray& line);
    void parseSnapshotMemStacks(const QByteArray& line);
    void parseHeapTreeLeaf(const QByteArray& line);
    bool parseheapTreeLeafInternal(const QByteArray& line, int depth);

    QByteArray getLabel(const QByteArray& original);

    /**
     * Each value in the enum identifies a line in the massif output file.
     */
    enum ExpectData {
        //BEGIN File wide data
        FileDesc, ///< desc: ...
        FileCmd, ///< cmd: ...
        FileTimeUnit, ///< time_unit: ...
        //END
        //BEGIN SnapShot data
        Snapshot, ///< actually three lines: #-----------\nsnapshot=N\n#-----------\n
        SnapshotTime, ///< time=N
        SnapshotMemHeap, ///< mem_heap_B=N
        SnapshotMemHeapExtra, ///< mem_heap_extra_B=N
        SnapshotMemStacks, ///< mem_stacks_B=N
        SnapshotHeapTree, ///< heap_tree=empty | heap_tree=detailed
        //END
        //BEGIN detailed Heap Tree data
        HeapTreeLeaf ///< nX: Y ... , where X gives the number of children and Y represents the heap size in bytes
                     /// the ... can be in one of three formats, here are examples for each of them:
                     /// 1) (heap allocation functions) malloc/new/new[], --alloc-fns, etc.
                     /// 2) in 803 places, all below massif's threshold (01.00%)
                     /// 3) 0x6F675AB: QByteArray::resize(int) (in /usr/lib/libQtCore.so.4.5.2)
        //END
    };
    ExpectData m_nextLine;
    int m_currentLine;
    /// current snapshot that is parsed.
    SnapshotItem* m_snapshot;
    /// parent tree leaf item
    TreeLeafItem* m_parentItem;
    /// set to true if we had custom allocators
    bool m_hadCustomAllocators;

    /// improve memory consumption by re-using known labels
    /// and making use of the implicit sharing of QByteArrays
    QSet<QByteArray> m_labels;

    int m_expectedSnapshots;
};

}

#endif // MASSIF_MASSIFPARSER_H
