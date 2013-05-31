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

#ifndef MASSIF_PARSERBASE_H
#define MASSIF_PARSERBASE_H

#include <QRegExp>
#include <QVector>

class QIODevice;

namespace Massif {

class FileData;
class Parser;

class ParserBase
{
public:
    explicit ParserBase(Parser* parser, QIODevice* file, Massif::FileData* data,
                        const QStringList& customAllocators, QAtomicInt* shouldStop);
    virtual ~ParserBase();

    enum Error {
        NoError, ///< file could be parsed properly
        Invalid, ///< the file was invalid and could not be parsed
        Stopped ///< parser was stopped
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

protected:
    void setError(Error type, int line, const QByteArray& lineString = QByteArray());

    Parser* m_parser;
    QIODevice* m_file;
    FileData* m_data;
    QAtomicInt* m_shouldStop;
    QVector<QRegExp> m_allocators;

private:
    Error m_errorType;
    int m_errorLine;
    QByteArray m_errorLineString;
};

}

#endif // MASSIF_PARSERBASE_H
