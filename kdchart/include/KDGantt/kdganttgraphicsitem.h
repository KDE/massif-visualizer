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

#ifndef KDGANTTGRAPHICSITEM_H
#define KDGANTTGRAPHICSITEM_H

#include "kdganttglobal.h"
#include "kdganttstyleoptionganttitem.h"

#include <QGraphicsItem>
#include <QDateTime>
#include <QPersistentModelIndex>

QT_BEGIN_NAMESPACE
class QGraphicsLineItem;
QT_END_NAMESPACE

namespace KDGantt {
    class GraphicsScene;
    class ConstraintGraphicsItem;

    /* Can we fit every kind of item into one gfxitem class? */
    class GraphicsItem : public QGraphicsItem {
    public:
        enum { Type = UserType + 42 };

        explicit GraphicsItem( QGraphicsItem* parent = 0, GraphicsScene* scene = 0 );
        explicit GraphicsItem( const QModelIndex& idx, QGraphicsItem* parent = 0, GraphicsScene* scene = 0 );
        virtual ~GraphicsItem();

        /*reimp*/int type() const;
        /*reimp (non-virtual)*/ GraphicsScene* scene() const;

        void updateItem( const Span& rowgeometry, const QPersistentModelIndex& idx );

        //virtual ItemType itemType() const = 0;

        //qreal dateTimeToSceneX( const QDateTime& dt ) const;
        //QDateTime sceneXtoDateTime( qreal x ) const;

        QRectF rect() const { return m_rect; }
        void setRect( const QRectF& r );
        void setBoundingRect( const QRectF& r );

        virtual QString ganttToolTip() const;

        const QPersistentModelIndex& index() const { return m_index; }
        void setIndex( const QPersistentModelIndex& idx );

        bool isEditable() const;
        bool isUpdating() const { return m_isupdating; }

        void addStartConstraint( ConstraintGraphicsItem* );
        void addEndConstraint( ConstraintGraphicsItem* );
        void removeStartConstraint( ConstraintGraphicsItem* );
        void removeEndConstraint( ConstraintGraphicsItem* );
        QList<ConstraintGraphicsItem*> startConstraints() const { return m_startConstraints; }
        QList<ConstraintGraphicsItem*> endConstraints() const { return m_endConstraints; }

        /*reimp*/ QRectF boundingRect() const;
        /*reimp*/ void paint( QPainter* painter, const QStyleOptionGraphicsItem* option,
                              QWidget* widget = 0 );

        /*reimp*/ QVariant itemChange( GraphicsItemChange, const QVariant& value );
    protected:
        /*reimp*/ void focusInEvent( QFocusEvent* event );
        /*reimp*/ void hoverMoveEvent( QGraphicsSceneHoverEvent* );
        /*reimp*/ void hoverLeaveEvent( QGraphicsSceneHoverEvent* );
        /*reimp*/ void mousePressEvent( QGraphicsSceneMouseEvent* );
        /*reimp*/ void mouseReleaseEvent( QGraphicsSceneMouseEvent* );
        /*reimp*/ void mouseDoubleClickEvent( QGraphicsSceneMouseEvent* );
        /*reimp*/ void mouseMoveEvent( QGraphicsSceneMouseEvent* );

    private:
        void init();

        QPointF startConnector( int relationType ) const;
        QPointF endConnector( int relationType ) const;
        void updateConstraintItems();
        StyleOptionGanttItem getStyleOption() const;
        void updateModel();
        void updateItemFromMouse( const QPointF& scenepos );
        void constraintsChanged();

        QRectF m_rect;
        QRectF m_boundingrect;
        QPersistentModelIndex m_index;
        bool m_isupdating;
        int m_istate;
        QPointF m_presspos;
        QPointF m_pressscenepos;
        QGraphicsLineItem* m_dragline;
        GraphicsItem* m_dragtarget;
        QList<ConstraintGraphicsItem*> m_startConstraints;
        QList<ConstraintGraphicsItem*> m_endConstraints;
    };
}

#endif /* KDGANTTGRAPHICSITEM_H */

