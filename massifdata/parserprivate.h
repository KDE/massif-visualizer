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

#ifndef MASSIF_PARSERPRIVATE_H
#define MASSIF_PARSERPRIVATE_H

#include <QtCore/QByteArray>

class QIODevice;

namespace Massif {

class FileData;
class SnapshotItem;
class TreeLeafItem;

class ParserPrivate
{
public:
    ParserPrivate(QIODevice* file, FileData* data);
    ~ParserPrivate();

    enum Error {
        NoError, ///< file could be parsed properly
        Invalid ///< the file was invalid and could not be parsed
    };
    /**
     * @return Whether an Error occurred or not.
     * @see errorLine()
     */
    Error error() const;
    /**
     * @return the line in which an error occurred or -1 if none.
     */
    int errorLine() const;
    /**
     * @return the line which could not be parsed.
     */
    QByteArray errorLineString() const;

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
    TreeLeafItem* parseheapTreeLeafInternal(const QByteArray& line, int depth);

    QIODevice* m_file;
    FileData* m_data;

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
    QByteArray m_errorLineString;

    Error m_error;

    /// current snapshot that is parsed.
    SnapshotItem* m_snapshot;
};

}

#endif // MASSIF_PARSERPRIVATE_H
