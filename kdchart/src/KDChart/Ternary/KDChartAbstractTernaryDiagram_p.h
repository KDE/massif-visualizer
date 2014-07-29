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

#ifndef KDCHARTABSTRACTTERNARYDIAGRAM_P_H
#define KDCHARTABSTRACTTERNARYDIAGRAM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the KD Chart API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "KDChartAbstractTernaryDiagram.h"

#include "KDChartTernaryCoordinatePlane.h"
#include <KDChartAbstractDiagram_p.h>
#include <KDChartAbstractThreeDAttributes.h>
#include <KDChartGridAttributes.h>
#include "KDChartPainterSaver_p.h"

#include "ReverseMapper.h"
#include "ChartGraphicsItem.h"
#include <KDABLibFakes>

namespace KDChart {

/**
 * \internal
 */
    class AbstractTernaryDiagram::Private : public AbstractDiagram::Private
    {
        friend class AbstractTernaryDiagram;
    public:
        Private();
        ~Private() {}

        Private( const Private& rhs ) :
            AbstractDiagram::Private( rhs ),
        // Do not copy axes and reference diagrams.
        axesList(),
        referenceDiagram( 0 )
        {
        }

        TernaryAxisList axesList;

        AbstractTernaryDiagram* referenceDiagram;
        QPointF referenceDiagramOffset;

        void drawPoint( QPainter* p, int row, int column,
                        const QPointF& widgetLocation )
        {
            // Q_ASSERT( false ); // unused, to be removed
            static const qreal Diameter = 5.0;
            static const qreal Radius = Diameter / 2.0;
            QRectF ellipseRect( widgetLocation - QPointF( Radius, Radius ),
                                QSizeF( Diameter, Diameter ) );
            p->drawEllipse( ellipseRect );

            reverseMapper.addRect( row, column, ellipseRect );
        }

        virtual void paint( PaintContext* paintContext )
        {
            paintContext->painter()->setRenderHint( QPainter::Antialiasing,
                                                    antiAliasing );
            if ( !axesList.isEmpty() ) {

                Q_FOREACH( TernaryAxis* axis, axesList ) {
                    PainterSaver s( paintContext->painter() );
                    axis->paintCtx( paintContext );
                }
            }
        }
    };

    KDCHART_IMPL_DERIVED_DIAGRAM( AbstractTernaryDiagram, AbstractDiagram, TernaryCoordinatePlane )
}

#endif /* KDCHARTABSTRACTTERNARYDIAGRAM_P_H */

