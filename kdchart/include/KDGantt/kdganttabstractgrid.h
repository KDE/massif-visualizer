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

#ifndef KDGANTTABSTRACTGRID_H
#define KDGANTTABSTRACTGRID_H

#include "kdganttglobal.h"
#include "kdganttconstraint.h"

QT_BEGIN_NAMESPACE
class QPainter;
class QRectF;
class QAbstractItemModel;
class QModelIndex;
QT_END_NAMESPACE

namespace KDGantt {
    class AbstractRowController;
    class Span;

    class KDGANTT_EXPORT AbstractGrid : public QObject {
        Q_OBJECT
        KDGANTT_DECLARE_PRIVATE_BASE_POLYMORPHIC( AbstractGrid )
	friend class GraphicsScene;
    public:

        AbstractGrid(QObject* parent = 0);
        virtual ~AbstractGrid();

        QAbstractItemModel* model() const;
        QModelIndex rootIndex() const;

        virtual Span mapToChart( const QModelIndex& idx ) const = 0;
        virtual bool mapFromChart( const Span& span, const QModelIndex& idx,
                                   const QList<Constraint>& constraints=QList<Constraint>() ) const = 0;
        virtual qreal mapToChart( const QVariant &value ) const;
        virtual QVariant mapFromChart( qreal x ) const;
        bool isSatisfiedConstraint( const Constraint& c ) const;

        virtual void paintGrid( QPainter* painter, const QRectF& sceneRect, const QRectF& exposedRect,
                                AbstractRowController* rowController = 0, QWidget* widget=0 ) = 0;
        virtual void paintHeader( QPainter* painter, const QRectF& headerRect, const QRectF& exposedRect,
                                  qreal offset, QWidget* widget=0 ) = 0;

    public Q_SLOTS:
        /*internal*/ virtual void setModel( QAbstractItemModel* model );
        /*internal*/ virtual void setRootIndex( const QModelIndex& idx );
    Q_SIGNALS:
        void gridChanged();

    protected:
        virtual void drawBackground(QPainter* paint, const QRectF& rect);
        virtual void drawForeground(QPainter* paint, const QRectF& rect);
    };
}

#endif /* KDGANTTABSTRACTGRID_H */
