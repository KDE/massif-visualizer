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

#ifndef KDGANTTSUMMARYHANDLINGPROXYMODEL_H
#define KDGANTTSUMMARYHANDLINGPROXYMODEL_H

#include "kdganttforwardingproxymodel.h"

namespace KDGantt {
    class KDCHART_EXPORT SummaryHandlingProxyModel : public ForwardingProxyModel {
        Q_OBJECT
        KDGANTT_DECLARE_PRIVATE_BASE_POLYMORPHIC( SummaryHandlingProxyModel )
    public:
        explicit SummaryHandlingProxyModel( QObject* parent=0 );
        virtual ~SummaryHandlingProxyModel();

        /*reimp*/ void setSourceModel( QAbstractItemModel* model );

        /*reimp*/ QVariant data( const QModelIndex& proxyIndex, int role = Qt::DisplayRole) const;
        /*reimp*/ bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );

        /*reimp*/ Qt::ItemFlags flags( const QModelIndex& idx ) const;

    protected:
        /*reimp*/ void sourceModelReset();
        /*reimp*/ void sourceLayoutChanged();
        /*reimp*/ void sourceDataChanged( const QModelIndex& from, const QModelIndex& to );
        /*reimp*/ void sourceColumnsAboutToBeInserted( const QModelIndex& idx, int start, int end );
        /*reimp*/ void sourceColumnsAboutToBeRemoved( const QModelIndex& idx, int start, int end );
        /*reimp*/ void sourceRowsAboutToBeInserted( const QModelIndex& idx, int start, int end );
        /*reimp*/ void sourceRowsAboutToBeRemoved( const QModelIndex&, int start, int end );
    };
}

#endif /* KDGANTTSUMMARYHANDLINGPROXYMODEL_H */

