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

#ifndef KDGANTTCONSTRAINTPROXY_H
#define KDGANTTCONSTRAINTPROXY_H

#include "kdganttglobal.h"

#include <QPointer>

QT_BEGIN_NAMESPACE
class QAbstractProxyModel;
QT_END_NAMESPACE

namespace KDGantt {
    class Constraint;
    class ConstraintModel;

    class ConstraintProxy : public QObject {
        Q_OBJECT
    public:
        explicit ConstraintProxy( QObject* parent = 0 );
        virtual ~ConstraintProxy();

        void setSourceModel( ConstraintModel* src );
        void setDestinationModel( ConstraintModel* dest );
        void setProxyModel( QAbstractProxyModel* proxy );

        ConstraintModel* sourceModel() const;
        ConstraintModel* destinationModel() const;
        QAbstractProxyModel* proxyModel() const;


    private Q_SLOTS:

        void slotSourceConstraintAdded( const KDGantt::Constraint& );
        void slotSourceConstraintRemoved( const KDGantt::Constraint& );

        void slotDestinationConstraintAdded( const KDGantt::Constraint& );
        void slotDestinationConstraintRemoved( const KDGantt::Constraint& );

        void slotLayoutChanged();

    private:
        void copyFromSource();

        QPointer<QAbstractProxyModel> m_proxy;
        QPointer<ConstraintModel> m_source;
        QPointer<ConstraintModel> m_destination;
    };
}

#endif /* KDGANTTCONSTRAINTPROXY_H */

