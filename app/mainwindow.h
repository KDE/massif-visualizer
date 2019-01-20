/*
   This file is part of Massif Visualizer

   Copyright 2010 Milian Wolff <mail@milianw.de>
   Copyright 2013 Arnold Dumas <arnold@dumas.at>

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
#include "documentwidget.h"

class QAction;
class QSpinBox;
class QStringListModel;

namespace KChart {
class Chart;
class HeaderFooter;
class Plotter;
class CartesianAxis;
class Legend;
class BarDiagram;
}

class KRecentFilesAction;

#ifdef HAVE_KGRAPHVIEWER
namespace KGraphViewer {
class KGraphViewerInterface;
}
#endif

namespace Massif {

class FilteredDataTreeModel;
class DataTreeModel;
class SnapshotItem;
class TreeLeafItem;

class MainWindow : public KParts::MainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr, Qt::WindowFlags f = {});
    ~MainWindow() override;

    void setupActions();

public Q_SLOTS:
    /**
     * Open a dialog to pick a massif output file(s) to display.
     */
    void openFile();

    /**
     * Opens @p file as massif output file and visualize it.
     */
    void openFile(const QUrl& file);

    /**
     * reload currently opened file
     */
    void reloadCurrentFile();

    /**
     * Close currently opened file.
     */
    void closeCurrentFile();

private Q_SLOTS:
    void closeRequested();
    void closeFileTab(int idx);

    void preferences();
    void settingsChanged();

    void treeSelectionChanged(const QModelIndex& now, const QModelIndex& before);
    void selectPeakSnapshot();
    void modelItemSelected(const Massif::ModelItem& item);
    void contextMenuRequested(const Massif::ModelItem& item, QMenu* menu);

    void documentChanged();

    void allocatorsChanged();
    void allocatorSelectionChanged();
    void dataTreeContextMenuRequested(const QPoint &pos);
    void slotNewAllocator();
    void slotRemoveAllocator();
    /// operates on data of @c m_markCustomAllocator
    void slotMarkCustomAllocator();
    void allocatorViewContextMenuRequested(const QPoint &pos);

    void slotShortenTemplates(bool);

private:
    void updateWindowTitle();

    // Helper
    Ui::MainWindow ui;

    KRecentFilesAction* m_recentFiles;

    QAction* m_close;

    QStringListModel* m_allocatorModel;
    QAction* m_newAllocator;
    QAction* m_removeAllocator;
    QAction* m_markCustomAllocator;

    QAction* m_shortenTemplates;
    QAction* m_selectPeak;

    DocumentWidget* m_currentDocument;

    Massif::DataTreeModel* m_dataTreeModel;
    Massif::FilteredDataTreeModel* m_dataTreeFilterModel;
    bool m_settingSelection;
};

}

#endif // MASSIF_MAINWINDOW_H
