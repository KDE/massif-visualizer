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

#include <QTextDocument>

namespace Massif {

QString prettyCost(quint64 cost)
{
    Q_ASSERT(KGlobal::config());
    KConfigGroup conf = KGlobal::config()->group(QLatin1String("Settings"));
    int precision = conf.readEntry(QLatin1String("prettyCostPrecision"), 1);
    return KGlobal::locale()->formatByteSize(cost, precision);
}

QByteArray shortenTemplates(const QByteArray& identifier)
{
    QByteArray ret = identifier;
    Q_ASSERT(KGlobal::config());
    KConfigGroup conf = KGlobal::config()->group(QLatin1String("Settings"));
    if (conf.readEntry(QLatin1String("shortenTemplates"), false)) {
        // remove template arguments between <...>
        int depth = 0;
        int open = 0;
        for (int i = 0; i < ret.length(); ++i) {
            if (ret.at(i) == '<') {
                if (!depth) {
                    open = i;
                }
                ++depth;
            } else if (ret.at(i) == '>') {
                --depth;
                if (!depth) {
                    ret.remove(open + 1, i - open - 1);
                    i = open + 1;
                    open = 0;
                }
            }
        }
    }
    return ret;
}

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

uint qHash(const ParsedLabel& label)
{
    return qHash(label.function) + 17 * qHash(label.location) + 19 * qHash(label.address);
}

QByteArray prettyLabel(const QByteArray& label)
{
    ParsedLabel parsed = parseLabel(label);
    const QByteArray func = shortenTemplates(parsed.function);

    if (!parsed.location.isEmpty()) {
        return func + " (" + parsed.location + ")";
    } else {
        return func;
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

QString formatLabelForTooltip(const ParsedLabel& parsed)
{
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

QString finalizeTooltip(const QString& contents)
{
    return "<html><head><style>dt{font-weight:bold;} dd {font-family:monospace;}</style></head><body><dl>\n"
        + contents + "</dl></body></html>";
}

QString tooltipForTreeLeaf(const TreeLeafItem* node, const SnapshotItem* snapshot, const QByteArray& label)
{
    QString tooltip = i18n("<dt>cost:</dt><dd>%1, i.e. %2% of snapshot #%3</dd>", prettyCost(node ? node->cost() : 0),
                    // yeah nice how I round to two decimals, right? :D
                    double(int(double(node ? node->cost() : 0)/snapshot->cost()*10000))/100, snapshot->number());
    tooltip += formatLabelForTooltip(parseLabel(label));
    return finalizeTooltip(tooltip);
}

}
