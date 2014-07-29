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

#ifndef KDCHARTCHART_P_H
#define KDCHARTCHART_P_H

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

#include <QObject>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "KDChartChart.h"
#include "KDChartAbstractArea.h"
#include "KDChartTextArea.h"
#include "KDChartFrameAttributes.h"
#include "KDChartBackgroundAttributes.h"
#include "KDChartLayoutItems.h"

#include <KDABLibFakes>


namespace KDChart {

class AbstractAreaWidget;
class CartesianAxis;

/*
  struct PlaneInfo can't be declared inside Chart::Private, otherwise MSVC.net says:
  qhash.h(195) : error C2248: 'KDChart::Chart::Private' : cannot access protected class declared in class 'KDChart::Chart'
  KDChartChart_p.h(58) : see declaration of 'KDChart::Chart::Private'
  KDChartChart.h(61) : see declaration of 'KDChart::Chart'
  KDChartChart.cpp(262) : see reference to class template instantiation 'QHash<Key,T>' being compiled, with
            Key=KDChart::AbstractCoordinatePlane *,
            T=KDChart::Chart::Private::PlaneInfo
*/
/**
 * \internal
 */
struct PlaneInfo {
    PlaneInfo()
        : referencePlane( 0 ),
          horizontalOffset( 1 ),
          verticalOffset( 1 ),
          gridLayout( 0 ),
          topAxesLayout( 0 ),
          bottomAxesLayout( 0 ),
          leftAxesLayout( 0 ),
          rightAxesLayout( 0 )
    {}
    AbstractCoordinatePlane *referencePlane;
    int horizontalOffset;
    int verticalOffset;
    QGridLayout* gridLayout;
    QVBoxLayout* topAxesLayout;
    QVBoxLayout* bottomAxesLayout;
    QHBoxLayout* leftAxesLayout;
    QHBoxLayout* rightAxesLayout;
};

struct LayoutGraphNode
{
    LayoutGraphNode()
        : diagramPlane( 0 )
        , leftSuccesor( 0 )
        , bottomSuccesor( 0 )
        , sharedSuccesor( 0 )
        , gridLayout( 0 )
        , topAxesLayout( false )
        , bottomAxesLayout( false )
        , leftAxesLayout( false )
        , rightAxesLayout( false )
        , priority( -1 )
    {}
    AbstractCoordinatePlane* diagramPlane;
    LayoutGraphNode* leftSuccesor;
    LayoutGraphNode* bottomSuccesor;
    LayoutGraphNode* sharedSuccesor;
    QGridLayout* gridLayout;
    bool topAxesLayout;
    bool bottomAxesLayout;
    bool leftAxesLayout;
    bool rightAxesLayout;
    int priority;
    bool operator<( const LayoutGraphNode &other ) const
    {
        return priority < other.priority;
    }
};


/**
 * \internal
 */
class Chart::Private : public QObject
{
    Q_OBJECT
    public:
        Chart* chart;

        enum AxisType { Abscissa, Ordinate };
        bool useNewLayoutSystem;
        CoordinatePlaneList coordinatePlanes;
        HeaderFooterList headerFooters;
        LegendList legends;

        QHBoxLayout* layout;
        QVBoxLayout* vLayout;
        QBoxLayout*  planesLayout;
        QGridLayout* gridPlaneLayout;
        QGridLayout* headerLayout;
        QGridLayout* footerLayout;
        QGridLayout* dataAndLegendLayout;
        QSpacerItem* leftOuterSpacer;
        QSpacerItem* rightOuterSpacer;
        QSpacerItem* topOuterSpacer;
        QSpacerItem* bottomOuterSpacer;

        QVBoxLayout* innerHdFtLayouts[2][3][3];

        QVector<KDChart::TextArea*> textLayoutItems;
        QVector<KDChart::AbstractLayoutItem*> planeLayoutItems;
        QVector<KDChart::Legend*> legendLayoutItems;

        QSize overrideSize;
        bool isFloatingLegendsLayoutDirty;
        bool isPlanesLayoutDirty;

        // since we do not want to derive Chart from AbstractAreaBase, we store the attributes
        // here and call two static painting methods to draw the background and frame.
        KDChart::FrameAttributes frameAttributes;
        KDChart::BackgroundAttributes backgroundAttributes;

        // ### wrong names, "leading" means inter-line distance of text. spacing? margin?
        int globalLeadingLeft, globalLeadingRight, globalLeadingTop, globalLeadingBottom;

        QList< AbstractCoordinatePlane* > mouseClickedPlanes;

        Qt::LayoutDirection layoutDirection;

        Private( Chart* );

        virtual ~Private();

        void createLayouts();
        void updateDirtyLayouts();
        void reapplyInternalLayouts(); // TODO: see if this can be merged with updateDirtyLayouts()
        void paintAll( QPainter* painter );

        struct AxisInfo {
            AxisInfo()
                :plane(0)
            {}
            AbstractCoordinatePlane *plane;
        };
        QHash<AbstractCoordinatePlane*, PlaneInfo> buildPlaneLayoutInfos();
        QVector< LayoutGraphNode* > buildPlaneLayoutGraph();

    public Q_SLOTS:
        void slotLayoutPlanes();
        void slotResizePlanes();
        void slotLegendPositionChanged( AbstractAreaWidget* legend );
        void slotHeaderFooterPositionChanged( HeaderFooter* hf );
        void slotUnregisterDestroyedLegend( Legend * legend );
        void slotUnregisterDestroyedHeaderFooter( HeaderFooter* headerFooter );
        void slotUnregisterDestroyedPlane( AbstractCoordinatePlane* plane );
};

}

#endif
