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

#ifndef MASSIFDATA_UTIL_H
#define MASSIFDATA_UTIL_H

#include <QString>

#include "massifdata_export.h"

namespace Massif {

class TreeLeafItem;
class SnapshotItem;

/**
 * Returns a prettified cost string.
 */
MASSIFDATA_EXPORT QString prettyCost(quint64 cost);

/**
 * Prepares a tree node's label for the UI.
 * So far, only the Mem-Adress will get stripped.
 */
MASSIFDATA_EXPORT QByteArray prettyLabel(const QByteArray& label);

/**
 * Extracts the function name from the @p label
 */
MASSIFDATA_EXPORT QByteArray functionInLabel(const QByteArray& label);

/**
 * Checks whether this label denotes a tree node
 * with aggregated items below massif's threshold.
 */
MASSIFDATA_EXPORT bool isBelowThreshold(const QByteArray& label);

/**
 * Formats a label with richtext for showing in tooltips e.g.
 */
MASSIFDATA_EXPORT QString tooltipForTreeLeaf(Massif::TreeLeafItem* node, Massif::SnapshotItem* snapshot, const QByteArray& label);

}

#endif // MASSIFDATA_UTIL_H
