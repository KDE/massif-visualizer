/*
   This file is part of Massif Visualizer

   Copyright 2014 Milian Wolff <mail@milianw.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "flamegraph.h"

#include <cmath>

#include <QVBoxLayout>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsSimpleTextItem>
#include <QWheelEvent>
#include <QEvent>

#include <QElapsedTimer>
#include <QDebug>

#include <massifdata/filedata.h>
#include <massifdata/snapshotitem.h>
#include <massifdata/treeleafitem.h>
#include <massifdata/util.h>

using namespace Massif;

namespace {

struct Frame {
    quint64 cost;
    QMap<QByteArray, Frame> children;
};

typedef QMap<QByteArray, Frame> StackData;

// TODO: aggregate top-down instead of bottom-up to better resemble
// other flame graphs with the culprits on top instead of on bottom
void aggregateStack(TreeLeafItem* item, StackData* data)
{
    const QByteArray label = isBelowThreshold(item->label()) ? QByteArray() : functionInLabel(item->label());

    Frame& frame = (*data)[label];
    frame.cost = qMax(item->cost(), frame.cost);

    foreach(TreeLeafItem* child, item->children()) {
        aggregateStack(child, &frame.children);
    }
}

QVector<QGraphicsItem*> toGraphicsItems(const StackData& data, const qreal x_0 = 0, const qreal y_0 = 0, const qreal maxWidth = 100.0)
{
    QVector<QGraphicsItem*> ret;
    ret.reserve(data.size());

    double totalCost = 0;
    foreach(const Frame& frame, data) {
        totalCost += frame.cost;
    }

    QMap<QByteArray, Frame>::const_iterator it = data.constBegin();
    qreal x = x_0;
    const qreal h = 20;
    const qreal y = y_0;

    const qreal x_margin = 0;
    const qreal y_margin = 2;

    for (; it != data.constEnd(); ++it) {
        const qreal w = maxWidth * double(it.value().cost) / totalCost;
        QGraphicsRectItem* rect = new QGraphicsRectItem(x, y, w, h);
        QGraphicsSimpleTextItem* text = new QGraphicsSimpleTextItem(it.key(), rect);
        text->moveBy(x, y);
        rect->setBrush(QColor(Qt::white));
        ret += toGraphicsItems(it.value().children, x, y + h + y_margin, w);
        qDebug() << x << y << w << h << it.key() << prettyCost(it.value().cost);
        x += w + x_margin;
        ret << rect;
    }

    return ret;
}

}

FlameGraph::FlameGraph(QWidget* parent, Qt::WindowFlags flags)
    : QWidget(parent, flags)
    , m_scene(new QGraphicsScene(this))
    , m_view(new QGraphicsView(this))
    , m_data(0)
{
    setLayout(new QVBoxLayout);

    m_scene->addText("Hello, world!");

    m_view->setScene(m_scene);
    m_view->installEventFilter(this);

    layout()->addWidget(m_view);
}

FlameGraph::~FlameGraph()
{

}

bool FlameGraph::eventFilter(QObject* object, QEvent* event)
{
    if (object == m_view && event->type() == QEvent::Wheel) {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
//         m_view->scale(wheelEvent->);
        qreal scale = pow(1.1, double(wheelEvent->delta()) / (120.0 * 2.));
        qDebug() << wheelEvent->delta() << scale;
        m_view->scale(scale, scale);

    }
    return QObject::eventFilter(object, event);
}

void FlameGraph::setData(const FileData* data)
{
    m_data = data;
    m_scene->clear();

    if (!data) {
        return;
    }

    qDebug() << "Evaluating flame graph";
    QElapsedTimer t; t.start();

    StackData stackData;
    foreach (SnapshotItem* snapshot, m_data->snapshots()) {
        if (!snapshot->heapTree()) {
            continue;
        }
        aggregateStack(snapshot->heapTree(), &stackData);
    }

    foreach(QGraphicsItem* item, toGraphicsItems(stackData)) {
        m_scene->addItem(item);
    }

    m_view->fitInView( m_scene->itemsBoundingRect(), Qt::IgnoreAspectRatio );
    m_view->scale(5, 5);

    qDebug() << "took me: " << t.elapsed();
}
