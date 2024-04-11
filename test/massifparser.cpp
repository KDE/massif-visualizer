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


#include <KCompressionDevice>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QDebug>

#include "massifdata/parser.h"
#include "massifdata/filedata.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    if (app.arguments().size() != 2) {
        qWarning() << "usage: massifparser MASSIF_FILE";
        return 1;
    }

    const QString file = app.arguments().at(1);
    KCompressionDevice device(file);
    if (!device.open(QIODevice::ReadOnly)) {
        qWarning() << "could not open file:" << file;
        return 2;
    }

    qDebug() << "parsing file:" << file;

    QElapsedTimer t;
    t.start();

    Massif::Parser parser;
    QScopedPointer<Massif::FileData> data(parser.parse(&device));
    if (!data) {
        qWarning() << "failed to parse file:" << file;
        qWarning() << parser.errorLineString() << "in line" << parser.errorLine();
        return 3;
    }

    qDebug() << "finished parsing in" << t.elapsed() << "ms";

    return 0;
}
