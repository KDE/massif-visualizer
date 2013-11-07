/****************************************************************************
** Copyright (C) 2001-2013 Klaralvdalens Datakonsult AB.  All rights reserved.
**
** This file is part of the KD Chart library.
**
** Licensees holding valid commercial KD Chart licenses may use this file in
** accordance with the KD Chart Commercial License Agreement provided with
** the Software.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 and version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.GPL.txt included.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** Contact info@kdab.com if any conditions of this licensing are not
** clear to you.
**
**********************************************************************/

#ifndef KDGANTTSTYLEOPTIONGANTTITEM_H
#define KDGANTTSTYLEOPTIONGANTTITEM_H

#include "kdganttglobal.h"

#include <QStyleOptionViewItem>
#include <QRectF>
#include <QDebug>

namespace KDGantt {
    class AbstractGrid;
    class KDGANTT_EXPORT StyleOptionGanttItem : public QStyleOptionViewItem {
    public:
        enum Position { Left, Right, Center, Hidden };

        StyleOptionGanttItem();
        StyleOptionGanttItem( const StyleOptionGanttItem& other );
        StyleOptionGanttItem& operator=( const StyleOptionGanttItem& other );

        QRectF boundingRect;
        QRectF itemRect;
        Position displayPosition;
        AbstractGrid* grid;
        QString text;
    };
}

#ifndef QT_NO_DEBUG_STREAM

QDebug operator<<( QDebug dbg, KDGantt::StyleOptionGanttItem::Position p);
QDebug operator<<( QDebug dbg, const KDGantt::StyleOptionGanttItem& s );

#endif /* QT_NO_DEBUG_STREAM */


#endif /* KDGANTTSTYLEOPTIONGANTTITEM_H */

