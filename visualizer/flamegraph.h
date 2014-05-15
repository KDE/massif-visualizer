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

#ifndef FLAMEGRAPH_H
#define FLAMEGRAPH_H

#include <QWidget>

#include "visualizer_export.h"

class QGraphicsScene;
class QGraphicsView;

namespace Massif {

class FileData;

class VISUALIZER_EXPORT FlameGraph : public QWidget
{
    Q_OBJECT
public:
    FlameGraph(QWidget* parent, Qt::WindowFlags flags = 0);
    ~FlameGraph();

    void setData(const FileData* data);

protected:
    virtual bool eventFilter(QObject* object, QEvent* event);

private:
    QGraphicsScene* m_scene;
    QGraphicsView* m_view;
    const FileData* m_data;
};

}

#endif // FLAMEGRAPH_H
