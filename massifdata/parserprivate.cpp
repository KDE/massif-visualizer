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

#include "parserprivate.h"

#include "filedata.h"
#include "snapshotitem.h"
#include "treeleafitem.h"

#include <QtCore/QIODevice>

#include <QtCore/QDebug>

using namespace Massif;

#define VALIDATE(x) if (!(x)) { m_error = Invalid; return; }

#define VALIDATE_RETURN(x, y) if (!(x)) { m_error = Invalid; return y; }

ParserPrivate::ParserPrivate(QIODevice* file, FileData* data)
    : m_file(file), m_data(data), m_nextLine(FileDesc), m_currentLine(0), m_error(NoError), m_snapshot(0)
{
    QByteArray line;
    QByteArray buffer;

    const int bufsize = 1024;

    buffer.resize(bufsize);
    while (!file->atEnd()) {
        line = m_file->readLine();
        // remove trailing \n
        line.chop(1);
        switch (m_nextLine) {
            case FileDesc:
                parseFileDesc(line);
                break;
            case FileCmd:
                parseFileCmd(line);
                break;
            case FileTimeUnit:
                parseFileTimeUnit(line);
                break;
            case Snapshot:
                parseSnapshot(line);
                break;
            case SnapshotHeapTree:
                parseSnapshotHeapTree(line);
                break;
            case SnapshotMemHeap:
                parseSnapshotMemHeap(line);
                break;
            case SnapshotMemHeapExtra:
                parseSnapshotMemHeapExtra(line);
                break;
            case SnapshotTime:
                parseSnapshotTime(line);
                break;
            case SnapshotMemStacks:
                parseSnapshotMemStacks(line);
                break;
            case HeapTreeLeaf:
                parseHeapTreeLeaf(line);
                break;
        }
        if (m_error != NoError) {
            qWarning() << "invalid line" << (m_currentLine + 1) << line;
            m_error = Invalid;
            m_errorLineString = line;
            break;
        }
        ++m_currentLine;
    }
    if (!file->atEnd()) {
        m_error = Invalid;
    }
}

ParserPrivate::~ParserPrivate()
{
}

ParserPrivate::Error ParserPrivate::error() const
{
    return m_error;
}

int ParserPrivate::errorLine() const
{
    if (m_error == Invalid) {
        return m_currentLine;
    } else {
        return -1;
    }
}

QByteArray ParserPrivate::errorLineString() const
{
    return m_errorLineString;
}

//BEGIN Parser Functions
void ParserPrivate::parseFileDesc(const QByteArray& line)
{
    // desc: ...
    VALIDATE(line.startsWith("desc: "))

    m_data->setDescription(line.mid(6));
    m_nextLine = FileCmd;
}

void ParserPrivate::parseFileCmd(const QByteArray& line)
{
    // cmd: ...
    VALIDATE(line.startsWith("cmd: "))

    m_data->setCmd(line.mid(5));
    m_nextLine = FileTimeUnit;
}

void ParserPrivate::parseFileTimeUnit(const QByteArray& line)
{
    // time_unit: ...
    VALIDATE(line.startsWith("time_unit: "))

    m_data->setTimeUnit(line.mid(11));
    m_nextLine = Snapshot;
}

void ParserPrivate::parseSnapshot(const QByteArray& line)
{
    VALIDATE(line == "#-----------")

    // snapshot=N
    QByteArray nextLine = m_file->readLine(1024);
    ++m_currentLine;
    VALIDATE(nextLine.startsWith("snapshot="))
    nextLine.chop(1);
    QString i(nextLine.mid(9));
    bool ok;
    uint number = i.toUInt(&ok);
    VALIDATE(ok)
    nextLine = m_file->readLine(1024);
    ++m_currentLine;
    VALIDATE(nextLine == "#-----------\n")

    m_snapshot = new SnapshotItem;
    m_data->addSnapshot(m_snapshot);
    m_snapshot->setNumber(number);
    m_nextLine = SnapshotTime;
}

void ParserPrivate::parseSnapshotTime(const QByteArray& line)
{
    VALIDATE(line.startsWith("time="))
    QString timeStr(line.mid(5));
    bool ok;
    double time = timeStr.toDouble(&ok);
    VALIDATE(ok)
    m_snapshot->setTime(time);
    m_nextLine = SnapshotMemHeap;
}

void ParserPrivate::parseSnapshotMemHeap(const QByteArray& line)
{
    VALIDATE(line.startsWith("mem_heap_B="))
    QString byteStr(line.mid(11));
    bool ok;
    unsigned long bytes = byteStr.toULong(&ok);
    VALIDATE(ok)
    m_snapshot->setMemHeap(bytes);
    m_nextLine = SnapshotMemHeapExtra;
}

void ParserPrivate::parseSnapshotMemHeapExtra(const QByteArray& line)
{
    VALIDATE(line.startsWith("mem_heap_extra_B="))
    QString byteStr(line.mid(17));
    bool ok;
    unsigned long bytes = byteStr.toULong(&ok);
    VALIDATE(ok)
    m_snapshot->setMemHeapExtra(bytes);
    m_nextLine = SnapshotMemStacks;
}

void ParserPrivate::parseSnapshotMemStacks(const QByteArray& line)
{
    VALIDATE(line.startsWith("mem_stacks_B="))
    QString byteStr(line.mid(13));
    bool ok;
    unsigned int bytes = byteStr.toUInt(&ok);
    VALIDATE(ok)
    m_snapshot->setMemStacks(bytes);
    m_nextLine = SnapshotHeapTree;
}

void ParserPrivate::parseSnapshotHeapTree(const QByteArray& line)
{
    VALIDATE(line.startsWith("heap_tree="))
    QByteArray value = line.mid(10);
    if (value == "empty") {
        m_nextLine = Snapshot;
    } else if (value == "detailed") {
        m_nextLine = HeapTreeLeaf;
    } else if (value == "peak") {
        m_nextLine = HeapTreeLeaf;
        m_data->setPeak(m_snapshot);
    } else {
        m_error = Invalid;
        return;
    }
}

void ParserPrivate::parseHeapTreeLeaf(const QByteArray& line)
{
    m_snapshot->setHeapTree(parseheapTreeLeafInternal(line, 0));
    m_nextLine = Snapshot;
}

TreeLeafItem* ParserPrivate::parseheapTreeLeafInternal(const QByteArray& line, int depth)
{
    VALIDATE_RETURN(line.length() > depth + 1 && line.at(depth) == 'n', 0)
    int colonPos = line.indexOf(':', depth);
    VALIDATE_RETURN(colonPos != -1, 0)
    bool ok;

    QByteArray tmpStr = line.mid(depth + 1, colonPos - depth - 1);
    unsigned int children = tmpStr.toUInt(&ok);
    VALIDATE_RETURN(ok, 0)

    int spacePos = line.indexOf(' ', colonPos + 2);
    VALIDATE_RETURN(spacePos != -1, 0)
    tmpStr = line.mid(colonPos + 2, spacePos - colonPos - 2);
    unsigned long cost = tmpStr.toULong(&ok);
    VALIDATE_RETURN(ok, 0)

    TreeLeafItem* leaf = new TreeLeafItem;
    leaf->setCost(cost);
    leaf->setLabel(line.mid(spacePos + 1));

    for (unsigned int i = 0; i < children; ++i) {
        ++m_currentLine;
        QByteArray nextLine = m_file->readLine();
        if (nextLine.isEmpty()) {
            // fail gracefully if the tree is not complete, esp. useful for cases where
            // an app run with massif crashes and massif doesn't finish the full tree dump.
            return leaf;
        }
        // remove trailing \n
        nextLine.chop(1);
        TreeLeafItem* child = parseheapTreeLeafInternal(nextLine, depth + 1);
        VALIDATE_RETURN(child, leaf)
        if (!child->cost()) {
            delete child;
        } else {
            leaf->addChild(child);
        }
    }

    return leaf;
}

//END Parser Functions
