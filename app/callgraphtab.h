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

#ifndef CALLGRAPHTAB_H
#define CALLGRAPHTAB_H

#include "documenttabinterface.h"

#include <visualizer/modelitem.h>

class KAction;

namespace KGraphViewer {
class KGraphViewerInterface;
}

namespace Massif {
class DotGraphGenerator;
}

namespace KParts {
class ReadOnlyPart;
}

class CallGraphTab : public DocumentTabInterface
{
    Q_OBJECT

public:
    CallGraphTab(const Massif::FileData* data, KParts::ReadOnlyPart* graphViewerPart,
                 KXMLGUIClient* guiParent, QWidget* parent = 0);
    ~CallGraphTab();

    void showDotGraph(const Massif::ModelItem& item);

    virtual void settingsChanged();

    virtual void selectModelItem(const Massif::ModelItem& item);
    virtual void setVisible(bool visible);

public slots:
    void showDotGraph();

private slots:
    void slotGraphLoaded();
    void zoomIn();
    void zoomOut();
    void focusExpensiveGraphNode();

private:
    void setupActions();

private:
    KParts::ReadOnlyPart* m_graphViewerPart;
    KGraphViewer::KGraphViewerInterface* m_graphViewer;
    QScopedPointer<Massif::DotGraphGenerator> m_dotGenerator;
    Massif::ModelItem m_lastDotItem;
    Massif::ModelItem m_nextDotItem;

    KAction* m_zoomIn;
    KAction* m_zoomOut;
    KAction* m_focusExpensive;
};

#endif // CALLGRAPHTAB_H
