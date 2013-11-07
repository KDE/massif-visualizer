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

#ifndef KDGANTTCONSTRAINTGRAPHICSITEM_H
#define KDGANTTCONSTRAINTGRAPHICSITEM_H

#include <QGraphicsItem>

#include "kdganttconstraint.h"

namespace KDGantt {
    class GraphicsScene;

    class KDGANTT_EXPORT ConstraintGraphicsItem : public QGraphicsItem {
    public:
        enum { Type = UserType + 43 };

        explicit ConstraintGraphicsItem( const Constraint& c,
                                         QGraphicsItem* parent = 0, GraphicsScene* scene = 0 );
        virtual ~ConstraintGraphicsItem();

        /*reimp*/ int type() const;
        /*reimp (non virtual)*/GraphicsScene* scene() const;

        /*reimp*/ QString ganttToolTip() const;

        /*reimp*/ QRectF boundingRect() const;
        /*reimp*/ void paint( QPainter* painter, const QStyleOptionGraphicsItem* option,
                              QWidget* widget = 0 );

        inline const Constraint& constraint() const { return m_constraint; }
        Constraint proxyConstraint() const;

        void setStart( const QPointF& start );
        inline QPointF start() const { return m_start; }
        void setEnd( const QPointF& end );
        inline QPointF end() const { return m_end; }

        void updateItem( const QPointF& start,const QPointF& end );
    private:
        Constraint m_constraint;
        QPointF m_start;
        QPointF m_end;
    };
}

#endif /* KDGANTTCONSTRAINTGRAPHICSITEM_H */

