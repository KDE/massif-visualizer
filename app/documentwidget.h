/*
   This file is part of Massif Visualizer

   Copyright 2010 Milian Wolff <mail@milianw.de>
   Copyright 2013 Arnold Dumas <contact@arnolddumas.fr>

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

#ifndef DOCUMENTWIDGET_H
#define DOCUMENTWIDGET_H

#include <QWidget>
#include <KUrl>


namespace KDChart {
class Chart;
class HeaderFooter;
class CartesianAxis;
class Legend;
class BarDiagram;
class Plotter;
}

class QLabel;
class QProgressBar;
class QToolButton;
class QStackedWidget;
class QTabWidget;

class KMessageWidget;

namespace KParts {
class ReadOnlyPart;
}

namespace Massif {
class FileData;
class TotalCostModel;
class DetailedCostModel;
class DataTreeModel;
class FilteredDataTreeModel;
class TreeLeafItem;
class SnapshotItem;
class DotGraphGenerator;
class FlameGraph;
}

#ifdef HAVE_KGRAPHVIEWER
namespace KGraphViewer {
class KGraphViewerInterface;
}
#endif

class DocumentWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DocumentWidget(QWidget* parent = 0);
    ~DocumentWidget();

    void updateHeader();
    void updatePeaks();

    KUrl file() const;
    Massif::FileData* data() const;
    KDChart::Chart* chart() const;
    KDChart::Plotter* totalDiagram() const;
    Massif::TotalCostModel* totalCostModel() const;
    KDChart::Plotter* detailedDiagram() const;
    Massif::DetailedCostModel* detailedCostModel() const;
    Massif::DataTreeModel* dataTreeModel() const;
    Massif::FilteredDataTreeModel* dataTreeFilterModel() const;

#ifdef HAVE_KGRAPHVIEWER
    KGraphViewer::KGraphViewerInterface* graphViewer();
    void showDotGraph(const QPair<Massif::TreeLeafItem*, Massif::SnapshotItem*>& item);
    void focusExpensiveGraphNode();
    int currentIndex();
#endif

    bool isLoaded() const;

signals:
    void stopParser();
    void loadingFinished();
    void tabChanged(int);

public slots:
    void parserFinished(const KUrl& file, Massif::FileData* data);

    void setDetailedDiagramHidden(bool hidden);
    void setDetailedDiagramVisible(bool visible);

    void setTotalDiagramHidden(bool hidden);
    void setTotalDiagramVisible(bool visible);

    void setProgress(int value);
    void setRange(int minimum, int maximum);
    void setLoadingMessage(const QString& message);

    void showError(const QString& title, const QString& error);

#ifdef HAVE_KGRAPHVIEWER
    void showDotGraph();
#endif

private slots:
#ifdef HAVE_KGRAPHVIEWER
    void slotTabChanged(int index);
    void slotGraphLoaded();
#endif

private:
    void updateDetailedPeaks();

    KDChart::Chart* m_chart;
    QLabel* m_header;
    KDChart::Plotter* m_totalDiagram;
    Massif::TotalCostModel* m_totalCostModel;

    KDChart::Plotter* m_detailedDiagram;
    Massif::DetailedCostModel* m_detailedCostModel;

    KDChart::Legend* m_legend;

    Massif::FlameGraph* m_flameGraph;

    Massif::DataTreeModel* m_dataTreeModel;
    Massif::FilteredDataTreeModel* m_dataTreeFilterModel;
    Massif::FileData* m_data;
    KUrl m_file;

    QStackedWidget* m_stackedWidget;
    KMessageWidget* m_errorMessage;
    QLabel* m_loadingMessage;
    QProgressBar* m_loadingProgressBar;
    QToolButton* m_stopParserButton;
    bool m_isLoaded;

#ifdef HAVE_KGRAPHVIEWER
    KParts::ReadOnlyPart* m_graphViewerPart;
    KGraphViewer::KGraphViewerInterface* m_graphViewer;
    QScopedPointer<Massif::DotGraphGenerator> m_dotGenerator;
    QPair<Massif::TreeLeafItem*, Massif::SnapshotItem*> m_lastDotItem;
    QTabWidget* m_displayTabWidget;
#endif
};

#endif // DOCUMENTWIDGET_H
