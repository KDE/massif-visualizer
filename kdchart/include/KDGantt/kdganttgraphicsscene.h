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

#ifndef KDGANTTGRAPHICSSCENE_H
#define KDGANTTGRAPHICSSCENE_H

#include <QDateTime>
#include <QList>
#include <QGraphicsScene>
#include <QModelIndex>

#include "kdganttglobal.h"

QT_BEGIN_NAMESPACE
class QAbstractProxyModel;
class QItemSelectionModel;
class QPrinter;
QT_END_NAMESPACE

namespace KDGantt {
    class AbstractGrid;
    class AbstractRowController;
    class GraphicsItem;
    class Constraint;
    class ConstraintModel;
    class ConstraintGraphicsItem;
    class ItemDelegate;

    class KDCHART_EXPORT GraphicsScene : public QGraphicsScene {
        Q_OBJECT
        KDGANTT_DECLARE_PRIVATE_BASE_POLYMORPHIC( GraphicsScene )
    public:
        explicit GraphicsScene( QObject* parent=0 );
        virtual ~GraphicsScene();

        //qreal dateTimeToSceneX( const QDateTime& dt ) const;
        //QDateTime sceneXtoDateTime( qreal x ) const;

        static QModelIndex mainIndex( const QModelIndex& idx );
        static QModelIndex dataIndex( const QModelIndex& idx );

        QAbstractItemModel* model() const;
        QAbstractProxyModel* summaryHandlingModel() const;
        QModelIndex rootIndex() const;
        ConstraintModel* constraintModel() const;
        QItemSelectionModel* selectionModel() const;

        void insertItem( const QPersistentModelIndex&, GraphicsItem* );
        void removeItem( const QModelIndex& );
        using QGraphicsScene::removeItem;
        GraphicsItem* findItem( const QModelIndex& ) const;
        GraphicsItem* findItem( const QPersistentModelIndex& ) const;

        void updateItems();
        void clearItems();
        void deleteSubtree( const QModelIndex& );

        ConstraintGraphicsItem* findConstraintItem( const Constraint& ) const;
        QList<ConstraintGraphicsItem*> findConstraintItems( const QModelIndex& idx ) const;
        void clearConstraintItems();

        void setItemDelegate( ItemDelegate* );
        ItemDelegate* itemDelegate() const;

        void setRowController( AbstractRowController* rc );
        AbstractRowController* rowController() const;

        void setGrid( AbstractGrid* grid );
        AbstractGrid* grid() const;

        bool isReadOnly() const;

        void updateRow( const QModelIndex& idx );
        GraphicsItem* createItem( ItemType type ) const;

        /* used by GraphicsItem */
        void itemEntered( const QModelIndex& );
        void itemPressed( const QModelIndex& );
        void itemClicked( const QModelIndex& );
        void itemDoubleClicked( const QModelIndex& );
        void setDragSource( GraphicsItem* item );
        GraphicsItem* dragSource() const;

        /* Printing */
        void print( QPrinter* printer, bool drawRowLabels = true, bool drawColumnLabels = true );
        void print( QPrinter* printer, qreal start, qreal end, bool drawRowLabels = true, bool drawColumnLabels = true );
        void print( QPainter* painter, const QRectF& target = QRectF(), bool drawRowLabels=true, bool drawColumnLabels = true );
        void print( QPainter* painter, qreal start, qreal end, const QRectF& target = QRectF(), bool drawRowLabels=true, bool drawColumnLabels = true );

    Q_SIGNALS:
        void gridChanged();

        void clicked( const QModelIndex & index );
        void qrealClicked( const QModelIndex & index );
        void entered( const QModelIndex & index );
        void pressed( const QModelIndex & index );

    protected:
        /*reimp*/ void helpEvent( QGraphicsSceneHelpEvent *helpEvent );
        /*reimp*/ void drawBackground( QPainter* painter, const QRectF& rect );
        /*reimp*/ void drawForeground( QPainter* painter, const QRectF& rect );

    public Q_SLOTS:
        void setModel( QAbstractItemModel* );
        void setSummaryHandlingModel( QAbstractProxyModel* );
        void setConstraintModel( ConstraintModel* );
        void setRootIndex( const QModelIndex& idx );
        void setSelectionModel( QItemSelectionModel* selectionmodel );
        void setReadOnly( bool );

    private Q_SLOTS:
        /* slots for ConstraintModel */
        void slotConstraintAdded( const KDGantt::Constraint& );
        void slotConstraintRemoved( const KDGantt::Constraint& );
        void slotGridChanged();
    private:
        void doPrint( QPainter* painter, const QRectF& targetRect,
                      qreal start, qreal end,
                      QPrinter* printer, bool drawRowLabels, bool drawColumnLabels );
    };
}

#endif /* KDGANTTGRAPHICSSCENE_H */
