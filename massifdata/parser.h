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

#ifndef MASSIF_PARSER_H
#define MASSIF_PARSER_H


#include <QObject>
#include <QString>
#include <QStringList>

class QIODevice;

namespace Massif {

class FileData;

/**
 * This class parses a Massif output file and stores it's information.
 */
class Parser : public QObject
{
    Q_OBJECT

public:
    Parser();
    ~Parser() override;

    /**
     * Parse @p file and return a FileData structure representing the data.
     *
     * @p customAllocators list of wildcard patterns used to find custom allocators
     * @p shouldStop if supplied, this is checked periodically. If the atomic
     *               evaluates to true, parser stops and returns 0.
     *
     * @return Data or null if file could not be parsed.
     *
     * @note The caller has to delete the data afterwards.
     */
    FileData* parse(QIODevice* file,
                    const QStringList& customAllocators = QStringList(),
                    QAtomicInt* shouldStop = 0);

    /**
     * Returns the number of the line which could not be parsed or -1 if no error occurred.
     */
    int errorLine() const;
    /**
     * Returns the line which could not be parsed.
     */
    QByteArray errorLineString() const;

    void setProgress(int value);

Q_SIGNALS:
    void progress(int value);

private:
    int m_errorLine;
    QByteArray m_errorLineString;
};

}

#endif // MASSIF_PARSER_H
