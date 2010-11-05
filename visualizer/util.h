/*
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

#ifndef VISUALIZER_UTIL_H
#define VISUALIZER_UTIL_H

#include <QString>

#include "visualizer_export.h"

namespace Massif {

/**
 * Returns a prettified cost string.
 */
VISUALIZER_EXPORT QString prettyCost(unsigned long cost);

/**
 * Prepares a tree node's label for the UI.
 * So far, only the Mem-Adress will get stripped.
 */
VISUALIZER_EXPORT QString prettyLabel(const QString& label);

/**
 * Checks whether this label denotes a tree node
 * with aggregated items below massif's threshold.
 */
VISUALIZER_EXPORT bool isBelowThreshold(const QString& label);

}

#endif // VISUALIZER_UTIL_H
