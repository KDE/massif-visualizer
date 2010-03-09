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

#include "datamodel.h"
#include "snapshotitem.h"

#include <QtCore/QIODevice>
#include <QtCore/QTextStream>

#include <QtCore/QDebug>

using namespace Massif;

Massif::ParserPrivate::ParserPrivate(QIODevice* file, Massif::DataModel* model)
    : m_file(file), m_model(model), m_nextLine(FileDesc), m_currentLine(0), m_error(NoError), m_snapshot(0)
{
    QByteArray line;
    QByteArray buffer;

    const int bufsize = 1024;

    buffer.resize(bufsize);
    while (!file->atEnd()) {
        line = m_file->readLine();
        // remove trailing \n
        line.chop(1);
        qDebug() << m_currentLine << line;
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
            m_error = Invalid;
            break;
        }
        ++m_currentLine;
    }
    if (!file->atEnd()) {
        m_error = Invalid;
    }
}

Massif::ParserPrivate::~ParserPrivate()
{
}

Massif::ParserPrivate::Error Massif::ParserPrivate::error() const
{
    return m_error;
}

int Massif::ParserPrivate::errorLine() const
{
    if (m_error == Invalid) {
        return m_currentLine;
    } else {
        return -1;
    }
}

//BEGIN Parser Functions
void ParserPrivate::parseFileDesc(const QByteArray& line)
{
    // desc: ...
    if (!line.startsWith("desc: ")) {
        m_error = Invalid;
        return;
    }
    m_model->setDescription(line.mid(6));
    m_nextLine = FileCmd;
}

void ParserPrivate::parseFileCmd(const QByteArray& line)
{
    // cmd: ...
    if (!line.startsWith("cmd: ")) {
        m_error = Invalid;
        return;
    }
    m_model->setCmd(line.mid(5));
    m_nextLine = FileTimeUnit;
}

void ParserPrivate::parseFileTimeUnit(const QByteArray& line)
{
    // time_unit: ...
    if (!line.startsWith("time_unit: ")) {
        m_error = Invalid;
        return;
    }
    m_model->setTimeUnit(line.mid(11));
    m_nextLine = Snapshot;
}

void ParserPrivate::parseSnapshot(const QByteArray& line)
{
    if (line != "#-----------") {
        m_error = Invalid;
        return;
    }
    // snapshot=N
    QByteArray nextLine = m_file->readLine(1024);
    ++m_currentLine;
    if (!nextLine.startsWith("snapshot=")) {
        m_error = Invalid;
        return;
    }
    nextLine.chop(1);
    QString i(nextLine.mid(9));
    bool ok;
    uint number = i.toUInt(&ok);
    if (!ok) {
        m_error = Invalid;
        return;
    }
    nextLine = m_file->readLine(1024);
    ++m_currentLine;
    if (nextLine != "#-----------\n") {
        m_error = Invalid;
        return;
    }

    m_snapshot = new SnapshotItem;
    m_model->addSnapshot(m_snapshot);
    m_snapshot->setNumber(number);
    m_nextLine = SnapshotTime;
}

void ParserPrivate::parseSnapshotTime(const QByteArray& line)
{

}

void ParserPrivate::parseSnapshotMemHeap(const QByteArray& line)
{

}

void ParserPrivate::parseSnapshotMemHeapExtra(const QByteArray& line)
{

}

void ParserPrivate::parseSnapshotMemStacks(const QByteArray& line)
{

}

void ParserPrivate::parseSnapshotHeapTree(const QByteArray& line)
{

}

void ParserPrivate::parseHeapTreeLeaf(const QByteArray& line)
{

}

//END Parser Functions
