/*
   This file is part of Massif Visualizer

   Copyright 2010 Milian Wolff <mail@milianw.de>

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

#ifndef MASSIF_MAINWINDOW_H
#define MASSIF_MAINWINDOW_H

#include <KParts/MainWindow>

#include <QPrintPreviewDialog>

#include "ui_mainwindow.h"

class QStringListModel;
class QLabel;

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

namespace KGraphViewer {

class KGraphViewerInterface;

}

namespace Massif {

class ParseWorker;
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
     * reload currently opened file
     */
    void reload();

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

    /**
     * Show the print preview dialog.
     */
    void showPrintPreviewDialog();

    /**
     * Create a preview of the printed file in the @p printer.
     */
    void printFile(QPrinter *printer);

private slots:
    void preferences();
    void settingsChanged();

    void treeSelectionChanged(const QModelIndex& now, const QModelIndex& before);
    void detailedItemClicked(const QModelIndex& item);
    void totalItemClicked(const QModelIndex& item);
    void selectPeakSnapshot();
    void setStackNum(int num);

#ifdef HAVE_KGRAPHVIEWER
    void showDotGraph();
    void slotTabChanged(int index);
    void slotGraphLoaded();
    void zoomIn();
    void zoomOut();
    void focusExpensiveGraphNode();
#endif

    void allocatorsChanged();
    void allocatorSelectionChanged();
    void dataTreeContextMenuRequested(const QPoint &pos);
    void slotNewAllocator();
    void slotRemoveAllocator();
    /// operates on data of @c m_markCustomAllocator
    void slotMarkCustomAllocator();
    void allocatorViewContextMenuRequested(const QPoint &pos);

    void chartContextMenuRequested(const QPoint &pos);

    void slotHideFunction();
    void slotHideOtherFunctions();

    void slotShortenTemplates(bool);

    void stopParser();
    void parserFinished(const KUrl& file, Massif::FileData* data);
    void parserError(const QString& title, const QString& error);

private:
    void showDotGraph(const QPair<TreeLeafItem*, SnapshotItem*>& item);
    void updateHeader();
    void updatePeaks();
    void updateDetailedPeaks();
    void prepareActions(QMenu* menu, TreeLeafItem* item);

    Ui::MainWindow ui;
    KDChart::Chart* m_chart;
    QLabel* m_header;
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
    KUrl m_currentFile;
    KAction* m_selectPeak;

    KRecentFilesAction* m_recentFiles;

    bool m_changingSelections;
#ifdef HAVE_KGRAPHVIEWER
    KParts::ReadOnlyPart* m_graphViewerPart;
    KGraphViewer::KGraphViewerInterface* m_graphViewer;
    QScopedPointer<DotGraphGenerator> m_dotGenerator;
    QPair<TreeLeafItem*, SnapshotItem*> m_lastDotItem;
#endif
    KAction* m_zoomIn;
    KAction* m_zoomOut;
    KAction* m_focusExpensive;
    KAction* m_close;
    KAction* m_print;
    KAction* m_stopParser;

    QStringListModel* m_allocatorModel;
    KAction* m_newAllocator;
    KAction* m_removeAllocator;
    KAction* m_markCustomAllocator;

    KAction* m_hideFunction;
    KAction* m_hideOtherFunctions;

    KAction* m_shortenTemplates;

    ParseWorker* m_parseWorker;
};

}

#endif // MASSIF_MAINWINDOW_H
