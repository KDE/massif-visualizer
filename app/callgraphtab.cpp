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

#include "callgraphtab.h"

#include "visualizer/dotgraphgenerator.h"
#include "massifdata/filedata.h"

#include <QVBoxLayout>
#include <QAction>
#include <QDebug>

#include <KParts/ReadOnlyPart>
#include <KStandardAction>
#include <KActionCollection>
#include <KLocalizedString>

#include <kgraphviewer_interface.h>

using namespace Massif;

CallGraphTab::CallGraphTab(const FileData* data, KParts::ReadOnlyPart* graphViewerPart,
                           KXMLGUIClient* guiParent, QWidget* parent)
    : DocumentTabInterface(data, guiParent, parent)
    , m_graphViewerPart(graphViewerPart)
    , m_graphViewer(qobject_cast<KGraphViewer::KGraphViewerInterface*>(m_graphViewerPart))
    , m_dotGenerator(0)
    , m_zoomIn(0)
    , m_zoomOut(0)
    , m_focusExpensive(0)
{
    setXMLFile(QStringLiteral("callgraphtabui.rc"), true);
    setupActions();

    Q_ASSERT(m_graphViewer);
    setLayout(new QVBoxLayout);
    layout()->addWidget(m_graphViewerPart->widget());

    connect(m_graphViewerPart, SIGNAL(graphLoaded()),
            this, SLOT(slotGraphLoaded()));

    showDotGraph(ModelItem(0, data->peak()));
}

CallGraphTab::~CallGraphTab()
{
    if (m_dotGenerator) {
        if (m_dotGenerator->isRunning()) {
            disconnect(m_dotGenerator.data(), 0, this, 0);
            connect(m_dotGenerator.data(), SIGNAL(finished()),
                    m_dotGenerator.data(), SLOT(deleteLater()));
            m_dotGenerator->cancel();
            m_dotGenerator.take();
        }
        m_dotGenerator.reset();
    }
    if (m_graphViewer) {
        m_graphViewerPart->closeUrl();
    }
    m_lastDotItem.first = 0;
    m_lastDotItem.second = 0;
}

void CallGraphTab::setupActions()
{
    m_zoomIn = KStandardAction::zoomIn(this, SLOT(zoomIn()), actionCollection());
    actionCollection()->addAction(QStringLiteral("zoomIn"), m_zoomIn);
    m_zoomOut = KStandardAction::zoomOut(this, SLOT(zoomOut()), actionCollection());
    actionCollection()->addAction(QStringLiteral("zoomOut"), m_zoomOut);
    m_focusExpensive = new QAction(QIcon::fromTheme(QStringLiteral("flag-red")), i18n("Focus most expensive node"), actionCollection());
    connect(m_focusExpensive, SIGNAL(triggered()), this, SLOT(focusExpensiveGraphNode()));
    actionCollection()->addAction(QStringLiteral("focusExpensive"), m_focusExpensive);
}

void CallGraphTab::settingsChanged()
{

}

void CallGraphTab::focusExpensiveGraphNode()
{
    Q_ASSERT(m_graphViewer);
    Q_ASSERT(m_dotGenerator);

    m_graphViewer->centerOnNode(m_dotGenerator->mostCostIntensiveGraphvizId());
}

void CallGraphTab::showDotGraph(const ModelItem& item)
{
    m_nextDotItem = item;

    if (item == m_lastDotItem && m_graphViewerPart->url().isValid()) {
        return;
    }

    if (!isVisible()) {
        return;
    }
    m_lastDotItem = item;

    Q_ASSERT(m_graphViewer);

    qDebug() << "new dot graph requested" << item;
    if (m_dotGenerator) {
        qDebug() << "existing generator is running:" << m_dotGenerator->isRunning();
        if (m_dotGenerator->isRunning()) {
            disconnect(m_dotGenerator.data(), 0, this, 0);
            connect(m_dotGenerator.data(), SIGNAL(finished()),
                    m_dotGenerator.data(), SLOT(deleteLater()));
            m_dotGenerator->cancel();
            m_dotGenerator.take();
        }
        m_dotGenerator.reset();
    }
    if (!item.first && !item.second) {
        return;
    }
    if (item.second) {
        m_dotGenerator.reset(new DotGraphGenerator(item.second, m_data->timeUnit(), this));
    } else {
        m_dotGenerator.reset(new DotGraphGenerator(item.first, m_data->timeUnit(), this));
    }
    m_dotGenerator->start();
    connect(m_dotGenerator.data(), SIGNAL(finished()), this, SLOT(showDotGraph()));
}

void CallGraphTab::setVisible(bool visible)
{
    QWidget::setVisible(visible);

    if (visible) {
        showDotGraph(m_nextDotItem);
    }
}

void CallGraphTab::showDotGraph()
{
    if (!m_dotGenerator || !m_graphViewerPart || !isVisible()) {
        return;
    }
    qDebug() << "show dot graph in output file" << m_dotGenerator->outputFile();
    const auto url = QUrl::fromLocalFile(m_dotGenerator->outputFile());
    if (url.isValid() && m_graphViewerPart->url() != url) {
        m_graphViewerPart->openUrl(url);
    }
}

void CallGraphTab::slotGraphLoaded()
{
    Q_ASSERT(m_graphViewer);

    if (!m_dotGenerator) {
        return;
    }
    m_graphViewer->setZoomFactor(0.75);
    m_graphViewer->setPannerPosition(KGraphViewer::KGraphViewerInterface::BottomRight);
    m_graphViewer->setPannerEnabled(true);
    m_graphViewer->centerOnNode(m_dotGenerator->mostCostIntensiveGraphvizId());
}

void CallGraphTab::zoomIn()
{
    m_graphViewer->zoomIn();
}

void CallGraphTab::zoomOut()
{
    m_graphViewer->zoomOut();
}

void CallGraphTab::selectModelItem(const ModelItem& item)
{
    showDotGraph(item);
}
