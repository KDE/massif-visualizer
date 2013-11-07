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

#ifndef KDGANTTGRAPHICSVIEW_H
#define KDGANTTGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QPrinter>

#include "kdganttglobal.h"

QT_BEGIN_NAMESPACE
class QModelIndex;
class QAbstractItemModel;
class QAbstractProxyModel;
class QItemSelectionModel;
QT_END_NAMESPACE

namespace KDGantt {
    class AbstractRowController;
    class AbstractGrid;
    class GraphicsItem;
    class ConstraintModel;
    class ItemDelegate;

    class KDGANTT_EXPORT GraphicsView : public QGraphicsView {
        Q_OBJECT
        KDGANTT_DECLARE_PRIVATE_BASE_POLYMORPHIC(GraphicsView)

        Q_PROPERTY( bool readOnly READ isReadOnly WRITE setReadOnly )

        Q_PRIVATE_SLOT( d, void slotGridChanged() )
        Q_PRIVATE_SLOT( d, void slotHorizontalScrollValueChanged( int ) )
        Q_PRIVATE_SLOT( d, void slotHeaderContextMenuRequested( const QPoint& ) )
        /* slots for QAbstractItemModel signals */
        Q_PRIVATE_SLOT( d, void slotColumnsInserted( const QModelIndex& parent,  int start, int end ) )
        Q_PRIVATE_SLOT( d, void slotColumnsRemoved( const QModelIndex& parent,  int start, int end ) )
        Q_PRIVATE_SLOT( d, void slotDataChanged( const QModelIndex& topLeft, const QModelIndex& bottomRight ) )
        Q_PRIVATE_SLOT( d, void slotLayoutChanged() )
        Q_PRIVATE_SLOT( d, void slotModelReset() )
        Q_PRIVATE_SLOT( d, void slotRowsInserted( const QModelIndex& parent,  int start, int end ) )
        Q_PRIVATE_SLOT( d, void slotRowsAboutToBeRemoved( const QModelIndex& parent,  int start, int end ) )
        Q_PRIVATE_SLOT( d, void slotRowsRemoved( const QModelIndex& parent,  int start, int end ) )

        Q_PRIVATE_SLOT( d, void slotItemClicked( const QModelIndex& idx ) )
        Q_PRIVATE_SLOT( d, void slotItemDoubleClicked( const QModelIndex& idx ) )
    public:

        explicit GraphicsView( QWidget* parent=0 );
        virtual ~GraphicsView();

        QAbstractItemModel* model() const;
        QAbstractProxyModel* summaryHandlingModel() const;
        ConstraintModel* constraintModel() const;
        QModelIndex rootIndex() const;
        QItemSelectionModel* selectionModel() const;
        AbstractRowController* rowController() const;
        AbstractGrid* grid() const;
        ItemDelegate* itemDelegate() const;

        bool isReadOnly() const;

        void setHeaderContextMenuPolicy( Qt::ContextMenuPolicy );
        Qt::ContextMenuPolicy headerContextMenuPolicy() const;

        QModelIndex indexAt( const QPoint& pos ) const;

        virtual void addConstraint( const QModelIndex& from,
                                    const QModelIndex& to,
                                    Qt::KeyboardModifiers modifiers );

        void clearItems();
        void updateRow( const QModelIndex& );
        void updateScene();

    public Q_SLOTS:
        void updateSceneRect();

    public:
        void deleteSubtree( const QModelIndex& );

        void print( QPrinter* printer, bool drawRowLabels = true, bool drawColumnLabels = true );
        void print( QPrinter* printer, qreal start, qreal end, bool drawRowLabels = true, bool drawColumnLabels = true );
        void print( QPainter* painter, const QRectF& target = QRectF(), bool drawRowLabels = true, bool drawColumnLabels = true );
        void print( QPainter* painter, qreal start, qreal end,
                    const QRectF& target = QRectF(), bool drawRowLabels = true, bool drawColumnLabels = true );

    public Q_SLOTS:
        void setModel( QAbstractItemModel* );
        void setSummaryHandlingModel( QAbstractProxyModel* model );
        void setConstraintModel( ConstraintModel* );
        void setRootIndex( const QModelIndex& );
        void setSelectionModel( QItemSelectionModel* );
        void setRowController( AbstractRowController* );
        void setGrid( AbstractGrid* );
        void setItemDelegate( ItemDelegate* delegate );
        void setReadOnly( bool );

    Q_SIGNALS:
        void activated( const QModelIndex & index );
        void clicked( const QModelIndex & index );
        void qrealClicked( const QModelIndex & index );
        void entered( const QModelIndex & index );
        void pressed( const QModelIndex & index );
        void headerContextMenuRequested( const QPoint& pt );

    protected:
        /*reimp*/void resizeEvent( QResizeEvent* );
    private:
        friend class View;

        GraphicsItem* createItem( ItemType type ) const;
    };
}

#endif /* KDGANTTGRAPHICSVIEW_H */

