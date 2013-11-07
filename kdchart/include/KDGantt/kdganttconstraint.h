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

#ifndef KDGANTTCONSTRAINT_H
#define KDGANTTCONSTRAINT_H

#include <QMap>
#include <QModelIndex>
#include <QObject>
#include <QSharedDataPointer>
#include <QVariant>

#include "kdganttglobal.h"
#ifndef QT_NO_DEBUG_STREAM
#include <QDebug>
#endif

namespace KDGantt {
    class KDGANTT_EXPORT Constraint {
        class Private;
    public:
        enum Type
        {
            TypeSoft = 0,
            TypeHard = 1
        };
        enum RelationType
        {
            FinishStart = 0,
            FinishFinish = 1,
            StartStart = 2,
            StartFinish = 3
        };

        enum ConstraintDataRole
        {
            ValidConstraintPen = Qt::UserRole,
            InvalidConstraintPen
        };

        typedef QMap<int, QVariant> DataMap;

        Constraint();
        Constraint( const QModelIndex& idx1,
                    const QModelIndex& idx2,
                    Type type=TypeSoft,
                    RelationType relType=FinishStart,
                    const DataMap& datamap=DataMap() );
        Constraint( const Constraint& other);
        ~Constraint();

        Type type() const;
        RelationType relationType() const;
        QModelIndex startIndex() const;
        QModelIndex endIndex() const;

        void setData( int role, const QVariant& value );
        QVariant data( int role ) const;

        void setDataMap( const QMap< int, QVariant >& datamap );
        QMap< int, QVariant > dataMap() const;

        bool compareIndexes(const Constraint& other) const;

        Constraint& operator=( const Constraint& other );
        bool operator==( const Constraint& other ) const;

        inline bool operator!=( const Constraint& other ) const {
            return !operator==( other );
        }

        uint hash() const;
#ifndef QT_NO_DEBUG_STREAM
        QDebug debug( QDebug dbg) const;
#endif

    private:
        QSharedDataPointer<Private> d;
    };

    inline uint qHash( const Constraint& c ) {return c.hash();}
}

#ifndef QT_NO_DEBUG_STREAM
QDebug KDCHART_EXPORT operator<<( QDebug dbg, const KDGantt::Constraint& c );
#endif /* QT_NO_DEBUG_STREAM */

#endif /* KDGANTTCONSTRAINT_H */

