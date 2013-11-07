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

#ifndef KDCHARTPLOTTER_P_H
#define KDCHARTPLOTTER_P_H

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

#include "KDChartPlotter.h"

#include <QPainterPath>

#include "KDChartThreeDLineAttributes.h"
#include "KDChartAbstractCartesianDiagram_p.h"
#include "KDChartCartesianDiagramDataCompressor_p.h"
#include "KDChartPlotterDiagramCompressor.h"

#include <KDABLibFakes>


namespace KDChart {

    class PaintContext;

/**
 * \internal
 */
    class Plotter::Private : public QObject, public AbstractCartesianDiagram::Private
    {
        Q_OBJECT
        friend class Plotter;
        friend class PlotterType;

    public:
        Private();
        Private( const Private& rhs );
        ~Private();

        void setCompressorResolution(
            const QSizeF& size,
            const AbstractCoordinatePlane* plane );

        PlotterType* implementor; // the current type
        PlotterType* normalPlotter;
        PlotterType* percentPlotter;
        PlotterDiagramCompressor plotterCompressor;
        Plotter::CompressionMode useCompression;
        qreal mergeRadiusPercentage;
    protected:
        void init();
    public Q_SLOTS:
        void changedProperties();
    };

    KDCHART_IMPL_DERIVED_DIAGRAM( Plotter, AbstractCartesianDiagram, CartesianCoordinatePlane )

    class Plotter::PlotterType
    {
    public:
        explicit PlotterType( Plotter* d )
            : m_private( d->d_func() )
        {
            m_private->init();
        }
        virtual ~PlotterType() {}
        virtual Plotter::PlotType type() const = 0;
        virtual const QPair<QPointF,  QPointF> calculateDataBoundaries() const = 0;
        virtual void paint( PaintContext* ctx ) = 0;
        Plotter* diagram() const;

        Plotter::CompressionMode useCompression() const;
        void setUseCompression( Plotter::CompressionMode value );
        PlotterDiagramCompressor& plotterCompressor() const;

        Plotter::Private* plotterPrivate() const { return m_private; }

    protected:
        // make some elements of m_private available to derived classes:
        AttributesModel* attributesModel() const;
        QModelIndex attributesModelRootIndex() const;
        ReverseMapper& reverseMapper();
        CartesianDiagramDataCompressor& compressor() const;

        int datasetDimension() const;

        Plotter::Private* m_private;
        // TODO: do we need them or not? (currently unused, but maybe there are supposed to be several
        //       compressors
        PlotterDiagramCompressor m_plotterCompressor;
        Plotter::CompressionMode m_useCompression;
    };
}

#endif /* KDCHARTLINEDIAGRAM_P_H */
