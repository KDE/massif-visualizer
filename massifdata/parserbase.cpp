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

#include "parserbase.h"

#include <QStringList>

using namespace Massif;

ParserBase::ParserBase(Parser* parser, QIODevice* file, FileData* data,
                       const QStringList& customAllocators, QAtomicInt* shouldStop)
: m_parser(parser)
, m_file(file)
, m_data(data)
, m_shouldStop(shouldStop)
, m_errorType(NoError)
, m_errorLine(-1)
{
    m_allocators.reserve(customAllocators.size());
    foreach(const QString& allocator, customAllocators) {
        m_allocators << QRegExp(allocator, Qt::CaseSensitive, QRegExp::Wildcard);
    }
}

ParserBase::~ParserBase()
{

}

void ParserBase::setError(ParserBase::Error type, int line, const QByteArray& lineString)
{
    m_errorType = type;
    m_errorLine = line;
    m_errorLineString = lineString;
}

ParserBase::Error ParserBase::error() const
{
    return m_errorType;
}

int ParserBase::errorLine() const
{
    return m_errorLine;
}

QByteArray ParserBase::errorLineString() const
{
    return m_errorLineString;
}
