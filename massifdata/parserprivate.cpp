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

#include "parserprivate.h"

#include "filedata.h"
#include "snapshotitem.h"
#include "treeleafitem.h"
#include "util.h"
#include "parser.h"

#include <QtCore/QIODevice>

#include <QtCore/QDebug>

using namespace Massif;

#define VALIDATE(x) if (!(x)) { m_error = Invalid; return; }

#define VALIDATE_RETURN(x, y) if (!(x)) { m_error = Invalid; return y; }

ParserPrivate::ParserPrivate(Parser* parser, QIODevice* file, FileData* data,
                             const QStringList& customAllocators,
                             QAtomicInt* shouldStop)
    : m_parser(parser)
    , m_file(file)
    , m_data(data)
    , m_nextLine(FileDesc)
    , m_currentLine(0)
    , m_error(NoError)
    , m_snapshot(0)
    , m_parentItem(0)
    , m_hadCustomAllocators(false)
    , m_expectedSnapshots(100)
{
    foreach(const QString& allocator, customAllocators) {
        m_allocators << QRegExp(allocator, Qt::CaseSensitive, QRegExp::Wildcard);
    }

    const int BUF_SIZE = 4096;
    char line_buf[BUF_SIZE];

    while (!file->atEnd()) {
        if (shouldStop && *shouldStop) {
            m_error = Stopped;
            return;
        }
        if (file->size()) {
            // use pos to determine progress when reading the file, won't work for compressed files
            parser->setProgress(static_cast<int>(double(file->pos() * 100) / file->size()));
        }
        const int read = m_file->readLine(line_buf, BUF_SIZE);
        const QByteArray& line = QByteArray::fromRawData(line_buf, read - 1 /* -1 to remove trailing \n */);
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
            m_errorLineString.detach();
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

    if (!m_file->size()) {
        // for zipped files, parse the desc line for a --max-snapshots parameter
        // and use that number for the progress bar. read the manual to know that
        // this might not be a good measure, but better than nothing.
        QRegExp pattern("--max-snapshots=([0-9]+)", Qt::CaseSensitive, QRegExp::RegExp2);
        if (pattern.indexIn(m_data->description()) != -1) {
            m_expectedSnapshots = pattern.cap(1).toInt();
        }
    }
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
    QByteArray i(nextLine.mid(9));
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

    if (!m_file->size()) {
        // see above: progress calculation for compressed files
        m_parser->setProgress(number * 100 / m_expectedSnapshots);
    }
}

void ParserPrivate::parseSnapshotTime(const QByteArray& line)
{
    VALIDATE(line.startsWith("time="))
    QByteArray timeStr(line.mid(5));
    bool ok;
    double time = timeStr.toDouble(&ok);
    VALIDATE(ok)
    m_snapshot->setTime(time);
    m_nextLine = SnapshotMemHeap;
}

void ParserPrivate::parseSnapshotMemHeap(const QByteArray& line)
{
    VALIDATE(line.startsWith("mem_heap_B="))
    QByteArray byteStr(line.mid(11));
    bool ok;
    quint64 bytes = byteStr.toULongLong(&ok);
    VALIDATE(ok)
    m_snapshot->setMemHeap(bytes);
    m_nextLine = SnapshotMemHeapExtra;
}

void ParserPrivate::parseSnapshotMemHeapExtra(const QByteArray& line)
{
    VALIDATE(line.startsWith("mem_heap_extra_B="))
    QByteArray byteStr(line.mid(17));
    bool ok;
    quint64 bytes = byteStr.toULongLong(&ok);
    VALIDATE(ok)
    m_snapshot->setMemHeapExtra(bytes);
    m_nextLine = SnapshotMemStacks;
}

void ParserPrivate::parseSnapshotMemStacks(const QByteArray& line)
{
    VALIDATE(line.startsWith("mem_stacks_B="))
    QByteArray byteStr(line.mid(13));
    bool ok;
    quint64 bytes = byteStr.toULongLong(&ok);
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

bool sortLeafsByCost(TreeLeafItem* l, TreeLeafItem* r)
{
    return l->cost() > r->cost();
}

void ParserPrivate::parseHeapTreeLeaf(const QByteArray& line)
{
    parseheapTreeLeafInternal(line, 0);
    m_nextLine = Snapshot;
    // we need to do some post processing if we had custom allocators:
    // - sort by cost
    // - merge "in XYZ places all below threshold"
    if (m_hadCustomAllocators) {
        Q_ASSERT(m_snapshot->heapTree());
        QVector<TreeLeafItem*> newChildren = m_snapshot->heapTree()->children();
        TreeLeafItem* belowThreshold = 0;
        uint places = 0;
        QString oldPlaces;
        ///TODO: is massif translateable?
        QRegExp matchBT("in ([0-9]+) places, all below massif's threshold",
                                            Qt::CaseSensitive, QRegExp::RegExp2);
        QVector<TreeLeafItem*>::iterator it = newChildren.begin();
        while(it != newChildren.end()) {
            TreeLeafItem* child = *it;
            if (matchBT.indexIn(QString::fromLatin1(child->label())) != -1) {
                places += matchBT.cap(1).toUInt();
                if (belowThreshold) {
                    // merge with previously found node
                    belowThreshold->setCost(belowThreshold->cost() + child->cost());
                    delete child;
                    it = newChildren.erase(it);
                    continue;
                } else {
                    belowThreshold = child;
                    oldPlaces = matchBT.cap(1);
                    // no break, see above
                }
            }
            ++it;
        }
        if (belowThreshold) {
            QByteArray label = belowThreshold->label();
            label.replace(oldPlaces, QByteArray::number(places));
            belowThreshold->setLabel(label);
        }
        qSort(newChildren.begin(), newChildren.end(), sortLeafsByCost);
        m_snapshot->heapTree()->setChildren(newChildren);
    }
    m_parentItem = 0;
}

struct SaveAndRestoreItem
{
    SaveAndRestoreItem(TreeLeafItem** item, TreeLeafItem* val)
        : m_item(item)
    {
        m_oldVal = *m_item;
        *m_item = val;
    }
    ~SaveAndRestoreItem()
    {
        *m_item = m_oldVal;
    }
    TreeLeafItem** m_item;
    TreeLeafItem* m_oldVal;
};

QByteArray ParserPrivate::getLabel(const QByteArray& original)
{
    QSet<QByteArray>::const_iterator it = m_labels.constFind(original);
    if (it != m_labels.constEnd()) {
        // reuse known label to leverage implicit sharing
        return *it;
    } else {
        m_labels.insert(original);
        return original;
    }
}


bool ParserPrivate::parseheapTreeLeafInternal(const QByteArray& line, int depth)
{
    VALIDATE_RETURN(line.length() > depth + 1 && line.at(depth) == 'n', false)
    int colonPos = line.indexOf(':', depth);
    VALIDATE_RETURN(colonPos != -1, false)
    bool ok;

    QByteArray tmpStr = QByteArray::fromRawData(line.data() + depth + 1, colonPos - depth - 1);
    unsigned int children = tmpStr.toUInt(&ok);
    VALIDATE_RETURN(ok, false)

    int spacePos = line.indexOf(' ', colonPos + 2);
    VALIDATE_RETURN(spacePos != -1, false)
    tmpStr = QByteArray::fromRawData(line.data() + colonPos + 2, spacePos - colonPos - 2);
    quint64 cost = tmpStr.toULongLong(&ok);
    VALIDATE_RETURN(ok, false)

    if (!cost && !children) {
        // ignore these empty entries
        return true;
    }

    const QByteArray label = getLabel(line.mid(spacePos + 1));

    bool isCustomAlloc = false;

    if (depth > 0 && !m_allocators.isEmpty()) {
        const QByteArray func = functionInLabel(label);
        foreach(const QRegExp& allocator, m_allocators) {
            if (allocator.pattern().contains('*')) {
                if (allocator.indexIn(func) != -1) {
                    isCustomAlloc = true;
                    break;
                }
            } else if (func.contains(allocator.pattern().toLatin1())) {
                isCustomAlloc = true;
                break;
            }
        }
    }

    TreeLeafItem* newParent = 0;
    if (!isCustomAlloc) {
        TreeLeafItem* leaf = new TreeLeafItem;
        leaf->setCost(cost);
        leaf->setLabel(label);

        if (!depth) {
            m_snapshot->setHeapTree(leaf);
        } else {
            Q_ASSERT(m_parentItem);
            m_parentItem->addChild(leaf);
        }

        newParent = leaf;
    } else {
        // the first line/heaptree start must never be a custom allocator, e.g.:
        // n11: 1776070 (heap allocation functions) malloc/new/new[], --alloc-fns, etc.
        Q_ASSERT(depth);
        Q_ASSERT(m_snapshot->heapTree());
        newParent = m_snapshot->heapTree();
        m_hadCustomAllocators = true;
    }

    SaveAndRestoreItem lastParent(&m_parentItem, newParent);

    for (unsigned int i = 0; i < children; ++i) {
        ++m_currentLine;
        QByteArray nextLine = m_file->readLine();
        if (nextLine.isEmpty()) {
            // fail gracefully if the tree is not complete, esp. useful for cases where
            // an app run with massif crashes and massif doesn't finish the full tree dump.
            return true;
        }
        // remove trailing \n
        nextLine.chop(1);
        if (!parseheapTreeLeafInternal(nextLine, depth + 1)) {
            return false;
        }
    }

    return true;
}

//END Parser Functions
