/*
   This file is part of Massif Visualizer

   Copyright 2010 Milian Wolff <mail@milianw.de>

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

#include "util.h"

#include <KGlobal>
#include <KLocale>
#include <KDebug>

namespace Massif {

QString prettyCost(unsigned long cost)
{
    return KGlobal::locale()->formatByteSize(cost);
}

QString prettyLabel(const QString& label)
{
    int colonPos = label.indexOf(": ");
    if (colonPos == -1) {
        return label;
    } else {
        return label.mid(colonPos + 2);
    }
}

QString functionInLabel(const QString& label)
{
    QString ret = prettyLabel(label);
    int pos = ret.indexOf(") (");
    if (pos != -1) {
        ret.resize(pos + 1);
    }
    return ret;
}

bool isBelowThreshold(const QString& label)
{
    return label.indexOf("all below massif's threshold") != -1;
}

}
