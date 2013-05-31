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

#include "mallocinfoparser.h"
#include "filedata.h"
#include "snapshotitem.h"
#include "treeleafitem.h"

#include <QXmlStreamReader>
#include <QDebug>
#include <KLocalizedString>

using namespace Massif;

///TODO: make translatable
static const QByteArray KEPT_LABEL("ideally freeable");
static const QByteArray FRAGMENTED_LABEL("fragmented");
static const QByteArray MMAPPED_LABEL("mmapped");

MallocInfoParser::MallocInfoParser(Parser* parser, QIODevice* file, FileData* data,
                                   const QStringList& customAllocators, QAtomicInt* shouldStop)
: ParserBase(parser, file, data, customAllocators, shouldStop)
{
    QXmlStreamReader reader(file);
    SnapshotItem* currentSnapshot = 0;
    TreeLeafItem* currentHeap = 0;
    quint64 firstSnapshotTime = 0;
    while(!reader.atEnd()) {
        QXmlStreamReader::TokenType token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            qDebug() << reader.name();
            const QXmlStreamAttributes attributes = reader.attributes();
            if (reader.name() == QLatin1String("mallocinfo")) {
                data->setCmd(attributes.value(QLatin1String("cmd")).toString());
                data->setDescription(attributes.value(QLatin1String("descr")).toString());
                data->setTimeUnit(QString("ms"));
            } else if (reader.name() == QLatin1String("snapshot")) {
                currentSnapshot = new SnapshotItem;
                currentSnapshot->setHeapTree(new TreeLeafItem);
                currentSnapshot->setNumber(attributes.value(QLatin1String("id")).toUtf8().toUInt());
                quint64 time = attributes.value(QLatin1String("time")).toUtf8().toULongLong();
                if (!firstSnapshotTime) {
                    firstSnapshotTime = time;
                }
                currentSnapshot->setTime(time - firstSnapshotTime);

                data->addSnapshot(currentSnapshot);
            } else if (reader.name() == QLatin1String("heap")) {
                currentHeap = new TreeLeafItem;
                currentHeap->setLabel(QByteArray("heap #") + attributes.value(QLatin1String("nr")).toUtf8());
                currentSnapshot->heapTree()->addChild(currentHeap);
            } else if (reader.name() == QLatin1String("system") && attributes.value(QLatin1String("type")) == QLatin1String("current")) {
                quint64 cost = attributes.value(QLatin1String("size")).toUtf8().toULongLong();
                if (currentHeap) {
                    currentHeap->setCost(cost);
                } else {
                    currentSnapshot->setMemHeap(cost);
                    currentSnapshot->heapTree()->setCost(cost);
                }
            } else if (reader.name() == QLatin1String("mallinfo")) {
                TreeLeafItem* fragmented = new TreeLeafItem;
                fragmented->setLabel(FRAGMENTED_LABEL);
                fragmented->setCost(attributes.value(QLatin1String("used")).toUtf8().toULongLong());
                currentSnapshot->heapTree()->addChild(fragmented);

                TreeLeafItem* kept = new TreeLeafItem;
                kept->setLabel(KEPT_LABEL);
                kept->setCost(attributes.value(QLatin1String("kept")).toUtf8().toULongLong());
                currentSnapshot->heapTree()->addChild(kept);

                /* TODO: include this?
                TreeLeafItem* mmapped = new TreeLeafItem;
                mmapped->setLabel(MMAPPED_LABEL);
                mmapped->setCost(attributes.value(QLatin1String("mmap")).toUtf8().toULongLong());
                currentSnapshot->heapTree()->addChild(mmapped);
                */

                fragmented->setCost(currentSnapshot->cost() - fragmented->cost() - kept->cost());
            }
        } else if (token == QXmlStreamReader::QXmlStreamReader::EndElement) {
            if (reader.name() == QLatin1String("snapshot")) {
                currentSnapshot = 0;
            } else if (reader.name() == QLatin1String("heap")) {
                currentHeap = 0;
            }
        }
    }
    if (reader.hasError()) {
        setError(ParserBase::Invalid, -1, reader.errorString().toUtf8());
    }
}

MallocInfoParser::~MallocInfoParser()
{

}
