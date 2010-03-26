/*
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef MASSIF_MAINWINDOW_H
#define MASSIF_MAINWINDOW_H

#include <KParts/MainWindow>

#include "ui_mainwindow.h"

namespace KDChart {
class Chart;
class HeaderFooter;
class Plotter;
class CartesianAxis;
class Legend;
class BarDiagram;
}

namespace KParts {
class ReadOnlyPart;
}

class KAction;
class KRecentFilesAction;
class KGraphViewerInterface;

namespace Massif {

class FileData;
class DetailedCostModel;
class TotalCostModel;
class DataTreeModel;
class FilteredDataTreeModel;
class DotGraphGenerator;
class SnapshotItem;
class TreeLeafItem;

class MainWindow : public KParts::MainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = 0, Qt::WindowFlags f = 0);
    virtual ~MainWindow();

    KDChart::Chart* chart();

    void setupActions();

public slots:
    /**
     * Open a dialog to pick a massif output file to display.
     */
    void openFile();

    /**
     * Opens @p file as massif output file and visualize it.
     */
    void openFile(const KUrl& file);

    /**
     * Close currently opened file.
     */
    void closeFile();

    /**
     * Depending on @p show, the total cost graph is shown or hidden.
     */
    void showTotalGraph(bool show);

    /**
     * Depending on @p show, the detailed cost graph is shown or hidden.
     */
    void showDetailedGraph(bool show);

private slots:
    void treeSelectionChanged(const QModelIndex& now, const QModelIndex& before);
    void detailedItemClicked(const QModelIndex& item);
    void totalItemClicked(const QModelIndex& item);
    void showDotGraph();
    void slotTabChanged(int index);
    void slotGraphLoaded();

private:
    void getDotGraph(QPair<TreeLeafItem*, SnapshotItem*> item);

    Ui::MainWindow ui;
    KDChart::Chart* m_chart;
    KDChart::HeaderFooter* m_header;
    KDChart::HeaderFooter* m_subheader;
    KAction* m_toggleTotal;
    KDChart::Plotter* m_totalDiagram;
    TotalCostModel* m_totalCostModel;

    KAction* m_toggleDetailed;
    KDChart::Plotter* m_detailedDiagram;
    DetailedCostModel* m_detailedCostModel;

    KDChart::Legend* m_legend;

    DataTreeModel* m_dataTreeModel;
    FilteredDataTreeModel* m_dataTreeFilterModel;
    FileData* m_data;

    KRecentFilesAction* m_recentFiles;

    bool m_changingSelections;
    KParts::ReadOnlyPart* m_graphViewerPart;
    KGraphViewerInterface* m_graphViewer;
    DotGraphGenerator* m_dotGenerator;
};

}

#endif // MASSIF_MAINWINDOW_H
