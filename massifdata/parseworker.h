/*
   This file is part of Massif Visualizer

   Copyright 2012 Milian Wolff <mail@milianw.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MASSIFDATA_PARSETHREAD_H
#define MASSIFDATA_PARSETHREAD_H

#include <QThread>
#include <QStringList>

#include <KUrl>

namespace Massif {

class FileData;

class ParseWorker : public QObject
{
    Q_OBJECT

public:
    explicit ParseWorker(QObject* parent = 0);

    void stop();

public slots:
    void parse(const KUrl& url, const QStringList& allocators);

signals:
    /**
     * Emitted once a file was properly parsed.
     */
    void finished(const KUrl& url, Massif::FileData* data);

    /**
     * Emitted if a file could not be parsed.
     */
    void error(const QString& error, const QString& message);

    void progressRange(int min, int max);
    void progress(int value);

private:
    QAtomicInt m_shouldStop;
};

}

#endif // MASSIFDATA_PARSETHREAD_H
