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

#ifndef KDGANTTITEMDELEGATE_H
#define KDGANTTITEMDELEGATE_H

#include <QItemDelegate>
#include <QBrush>
#include <QPen>
#include <QDebug>

#include "kdganttglobal.h"

namespace KDGantt {
    class StyleOptionGanttItem;
    class Constraint;

    class KDGANTT_EXPORT ItemDelegate : public QItemDelegate {
        Q_OBJECT
        KDGANTT_DECLARE_PRIVATE_BASE_POLYMORPHIC( ItemDelegate )
    public:
        enum InteractionState { State_None = 0,
                                State_Move,
                                State_ExtendLeft,
                                State_ExtendRight,
                                State_DragConstraint
        };

        explicit ItemDelegate( QObject* parent = 0 );
        virtual ~ItemDelegate();

        void setDefaultBrush( ItemType type, const QBrush& brush );
        QBrush defaultBrush( ItemType type ) const;

        void setDefaultPen( ItemType type, const QPen& pen );
        QPen defaultPen( ItemType type ) const;

        virtual Span itemBoundingSpan(const StyleOptionGanttItem& opt, const QModelIndex& idx) const;
        virtual QRectF constraintBoundingRect( const QPointF& start, const QPointF& end, const Constraint &constraint ) const;
        virtual InteractionState interactionStateFor( const QPointF& pos,
						      const StyleOptionGanttItem& opt,
						      const QModelIndex& idx ) const;

        virtual void paintGanttItem( QPainter* p, const StyleOptionGanttItem& opt, const QModelIndex& idx );
        virtual void paintConstraintItem( QPainter* p, const QStyleOptionGraphicsItem& opt,
                                          const QPointF& start, const QPointF& end, const Constraint &constraint );
        
        virtual QString toolTip( const QModelIndex &idx ) const;

    protected:
        void paintFinishStartConstraint( QPainter* p, const QStyleOptionGraphicsItem& opt,
                const QPointF& start, const QPointF& end, const Constraint &constraint );
        QPolygonF finishStartLine( const QPointF& start, const QPointF& end ) const;
        QPolygonF finishStartArrow( const QPointF& start, const QPointF& end ) const;

        void paintFinishFinishConstraint( QPainter* p, const QStyleOptionGraphicsItem& opt,
                const QPointF& start, const QPointF& end, const Constraint &constraint );
        QPolygonF finishFinishLine( const QPointF& start, const QPointF& end ) const;
        QPolygonF finishFinishArrow( const QPointF& start, const QPointF& end ) const;
        
        void paintStartStartConstraint( QPainter* p, const QStyleOptionGraphicsItem& opt,
                const QPointF& start, const QPointF& end, const Constraint &constraint );
        QPolygonF startStartLine( const QPointF& start, const QPointF& end ) const;
        QPolygonF startStartArrow( const QPointF& start, const QPointF& end ) const;
        
        void paintStartFinishConstraint( QPainter* p, const QStyleOptionGraphicsItem& opt,
                const QPointF& start, const QPointF& end, const Constraint &constraint );
        QPolygonF startFinishLine( const QPointF& start, const QPointF& end ) const;
        QPolygonF startFinishArrow( const QPointF& start, const QPointF& end ) const;

        
    };
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<( QDebug dbg, KDGantt::ItemDelegate::InteractionState );
#endif

#endif /* KDGANTTITEMDELEGATE_H */

