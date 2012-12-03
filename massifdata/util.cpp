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

#include "util.h"

#include "snapshotitem.h"
#include "treeleafitem.h"

#include <KGlobal>
#include <KLocale>
#include <KConfigGroup>
#include <KDebug>

#include <QTextDocument>

namespace Massif {

QString prettyCost(quint64 cost)
{
    Q_ASSERT(KGlobal::config());
    KConfigGroup conf = KGlobal::config()->group(QLatin1String("Settings"));
    int precision = conf.readEntry(QLatin1String("prettyCostPrecision"), 1);
    return KGlobal::locale()->formatByteSize(cost, precision);
}

void shortenTemplates(QByteArray& function)
{
    Q_ASSERT(KGlobal::config());
    KConfigGroup conf = KGlobal::config()->group(QLatin1String("Settings"));
    if (conf.readEntry(QLatin1String("shortenTemplates"), false)) {
        // remove template arguments between <...>
        int depth = 0;
        int open = 0;
        for (int i = 0; i < function.length(); ++i) {
            if (function.at(i) == '<') {
                if (!depth) {
                    open = i;
                }
                ++depth;
            } else if (function.at(i) == '>') {
                --depth;
                if (!depth) {
                    function.remove(open + 1, i - open - 1);
                    i = open + 1;
                    open = 0;
                }
            }
        }
    }
}

struct ParsedLabel
{
    QByteArray address;
    QByteArray function;
    QByteArray location;
};

ParsedLabel parseLabel(const QByteArray& label)
{
    ParsedLabel ret;
    int functionStart = 0;
    int functionEnd = label.length();
    if (label.startsWith("0x")) {
        int colonPos = label.indexOf(": ");
        if (colonPos != -1) {
            ret.address = label.left(colonPos);
            functionStart = colonPos + 2;
        }
    }
    if (label.endsWith(')')) {
        int locationPos = label.lastIndexOf(" (");
        if (locationPos != -1) {
            ret.location = label.mid(locationPos + 2, label.length() - locationPos - 3);
            functionEnd = locationPos;
        }
    }
    ret.function = label.mid(functionStart, functionEnd - functionStart);
    return ret;
}

QByteArray prettyLabel(const QByteArray& label)
{
    ParsedLabel parsed = parseLabel(label);
    shortenTemplates(parsed.function);

    if (!parsed.location.isEmpty()) {
        return parsed.function + " (" + parsed.location + ")";
    } else {
        return parsed.function;
    }
}

QByteArray functionInLabel(const QByteArray& label)
{
    return parseLabel(label).function;
}

QByteArray addressInLabel(const QByteArray& label)
{
    return parseLabel(label).address;
}

QByteArray locationInLabel(const QByteArray& label)
{
    return parseLabel(label).location;
}

bool isBelowThreshold(const QByteArray& label)
{
    return label.indexOf("all below massif's threshold") != -1;
}

QString formatLabel(const QByteArray& label)
{
    ParsedLabel parsed = parseLabel(label);
    QString ret;
    if (!parsed.function.isEmpty()) {
        ret += i18n("<dt>function:</dt><dd>%1</dd>\n", Qt::escape(parsed.function));
    }
    if (!parsed.location.isEmpty()) {
        ret += i18n("<dt>location:</dt><dd>%1</dd>\n", Qt::escape(parsed.location));
    }
    if (!parsed.address.isEmpty()) {
        ret += i18n("<dt>address:</dt><dd>%1</dd>\n", Qt::escape(parsed.address));
    }
    return ret;
}

QString tooltipForTreeLeaf(TreeLeafItem* node, SnapshotItem* snapshot, const QByteArray& label)
{
    QString tooltip = "<html><head><style>dt{font-weight:bold;} dd {font-family:monospace;}</style></head><body><dl>\n";
    tooltip += i18n("<dt>cost:</dt><dd>%1, i.e. %2% of snapshot #%3</dd>", prettyCost(node ? node->cost() : 0),
                    // yeah nice how I round to two decimals, right? :D
                    double(int(double(node ? node->cost() : 0)/snapshot->cost()*10000))/100, snapshot->number());
    tooltip += formatLabel(label);
    tooltip += "</dl></body></html>";
    return tooltip;
}

}
