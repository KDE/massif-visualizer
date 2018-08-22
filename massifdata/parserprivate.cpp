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

#include <QIODevice>

#include <QDebug>

using namespace Massif;

#define VALIDATE(l, x) if (!(x)) { m_errorLineString = l; m_error = Invalid; return; }

#define VALIDATE_RETURN(l, x, y) if (!(x)) { m_errorLineString = l; m_error = Invalid; return y; }

static QByteArray midRef(const QByteArray& line, int offset, int length = -1)
{
    return QByteArray::fromRawData(line.data() + offset, length == -1 ? (line.length() - offset) : length);
}

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
        if (allocator.contains(QLatin1Char('*'))) {
            m_allocators << QRegExp(allocator, Qt::CaseSensitive, QRegExp::Wildcard);
        } else {
            m_plainAllocators << allocator.toLatin1();
        }
    }

    while (!file->atEnd()) {
        if (shouldStop && *shouldStop) {
            m_error = Stopped;
            return;
        }
        if (file->size()) {
            // use pos to determine progress when reading the file, won't work for compressed files
            parser->setProgress(static_cast<int>(double(file->pos() * 100) / file->size()));
        }
        const QByteArray& line = readLine();
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
            qWarning() << "invalid line" << m_currentLine << line;
            m_error = Invalid;
            if (m_errorLineString.isEmpty()) {
                m_errorLineString = line;
            }
            // we use fromRawData, so better detach here
            m_errorLineString.detach();
            break;
        }
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
    VALIDATE(line, line.startsWith("desc: "))

    m_data->setDescription(QString::fromUtf8(line.mid(6)));
    m_nextLine = FileCmd;

    if (!m_file->size()) {
        // for zipped files, parse the desc line for a --max-snapshots parameter
        // and use that number for the progress bar. read the manual to know that
        // this might not be a good measure, but better than nothing.
        QRegExp pattern(QStringLiteral("--max-snapshots=([0-9]+)"), Qt::CaseSensitive, QRegExp::RegExp2);
        if (pattern.indexIn(m_data->description()) != -1) {
            m_expectedSnapshots = pattern.cap(1).toInt();
        }
    }
}

void ParserPrivate::parseFileCmd(const QByteArray& line)
{
    // cmd: ...
    VALIDATE(line, line.startsWith("cmd: "))

    m_data->setCmd(QString::fromUtf8(line.mid(5)));
    m_nextLine = FileTimeUnit;
}

void ParserPrivate::parseFileTimeUnit(const QByteArray& line)
{
    // time_unit: ...
    VALIDATE(line, line.startsWith("time_unit: "))

    m_data->setTimeUnit(QString::fromUtf8(line.mid(11)));
    m_nextLine = Snapshot;
}

void ParserPrivate::parseSnapshot(const QByteArray& line)
{
    VALIDATE(line, line == "#-----------")

    // snapshot=N
    QByteArray nextLine = readLine();
    VALIDATE(nextLine, nextLine.startsWith("snapshot="))
    bool ok;
    uint number = midRef(nextLine, 9).toUInt(&ok);
    VALIDATE(nextLine, ok)
    nextLine = readLine();
    VALIDATE(nextLine, nextLine == "#-----------")

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
    VALIDATE(line, line.startsWith("time="))
    bool ok;
    double time = midRef(line, 5).toDouble(&ok);
    VALIDATE(line, ok)
    m_snapshot->setTime(time);
    m_nextLine = SnapshotMemHeap;
}

void ParserPrivate::parseSnapshotMemHeap(const QByteArray& line)
{
    VALIDATE(line, line.startsWith("mem_heap_B="))
    bool ok;
    quint64 bytes = midRef(line, 11).toULongLong(&ok);
    VALIDATE(line, ok)
    m_snapshot->setMemHeap(bytes);
    m_nextLine = SnapshotMemHeapExtra;
}

void ParserPrivate::parseSnapshotMemHeapExtra(const QByteArray& line)
{
    VALIDATE(line, line.startsWith("mem_heap_extra_B="))
    bool ok;
    quint64 bytes = midRef(line, 17).toULongLong(&ok);
    VALIDATE(line, ok)
    m_snapshot->setMemHeapExtra(bytes);
    m_nextLine = SnapshotMemStacks;
}

void ParserPrivate::parseSnapshotMemStacks(const QByteArray& line)
{
    VALIDATE(line, line.startsWith("mem_stacks_B="))
    bool ok;
    quint64 bytes = midRef(line, 13).toULongLong(&ok);
    VALIDATE(line, ok)
    m_snapshot->setMemStacks(bytes);
    m_nextLine = SnapshotHeapTree;
}

void ParserPrivate::parseSnapshotHeapTree(const QByteArray& line)
{
    VALIDATE(line, line.startsWith("heap_tree="))
    QByteArray valueRef = midRef(line, 10);
    if (valueRef == "empty") {
        m_nextLine = Snapshot;
    } else if (valueRef == "detailed") {
        m_nextLine = HeapTreeLeaf;
    } else if (valueRef == "peak") {
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
        QRegExp matchBT(QStringLiteral("in ([0-9]+) places, all below massif's threshold"),
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
            label.replace(oldPlaces.toUtf8(), QByteArray::number(places));
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
    VALIDATE_RETURN(line, line.length() > depth + 1 && line.at(depth) == 'n', false)
    int colonPos = line.indexOf(':', depth);
    VALIDATE_RETURN(line, colonPos != -1, false)
    bool ok;

    unsigned int children = midRef(line, depth + 1, colonPos - depth - 1).toUInt(&ok);
    VALIDATE_RETURN(line, ok, false)

    int spacePos = line.indexOf(' ', colonPos + 2);
    VALIDATE_RETURN(line, spacePos != -1, false)
    quint64 cost = midRef(line, colonPos + 2, spacePos - colonPos - 2).toULongLong(&ok);
    VALIDATE_RETURN(line, ok, false)

    if (!cost && !children) {
        // ignore these empty entries
        return true;
    }

    const QByteArray label = getLabel(line.mid(spacePos + 1));

    bool isCustomAlloc = false;

    if (depth > 0 && !m_allocators.isEmpty()) {
        const QByteArray func = functionInLabel(label);
        foreach(const QByteArray& allocator, m_plainAllocators) {
            if (func.contains(allocator)) {
                isCustomAlloc = true;
                break;
            }
        }
        if (!isCustomAlloc) {
            const QString funcString = QString::fromLatin1(func);
            foreach(const QRegExp& allocator, m_allocators) {
                if (allocator.indexIn(funcString) != -1) {
                    isCustomAlloc = true;
                    break;
                }
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
        QByteArray nextLine = readLine();
        if (nextLine.isEmpty()) {
            // fail gracefully if the tree is not complete, esp. useful for cases where
            // an app run with massif crashes and massif doesn't finish the full tree dump.
            return true;
        }
        if (!parseheapTreeLeafInternal(nextLine, depth + 1)) {
            return false;
        }
    }

    return true;
}

QByteArray ParserPrivate::readLine()
{
    ++m_currentLine;

    const int read = m_file->readLine(m_lineBuffer, BUF_SIZE);
    if (read == -1) {
        return {};
    }
    if (read == BUF_SIZE - 1 && m_lineBuffer[BUF_SIZE - 2] != '\n') {
        // support for really long lines that don't fit into the buffer
        QByteArray line = QByteArray::fromRawData(m_lineBuffer, read) + m_file->readLine();
        line.chop(1); // remove trailing \n
        return line;
    }
    return QByteArray::fromRawData(m_lineBuffer, read - 1 /* -1 to remove trailing \n */);
}

//END Parser Functions
