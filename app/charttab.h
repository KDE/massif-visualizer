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

#ifndef CHARTTAB_H
#define CHARTTAB_H

#include "documenttabinterface.h"
#include <QSpinBox>
#include <QPrinter>

class QAction;
class QLabel;
class QModelIndex;

namespace KChart {
class Chart;
class HeaderFooter;
class Plotter;
class CartesianAxis;
class Legend;
class BarDiagram;
}

namespace Massif {
class FileData;
class TotalCostModel;
class DetailedCostModel;
class DataTreeModel;
class FilteredDataTreeModel;
}

class ChartTab : public DocumentTabInterface
{
    Q_OBJECT

public:
    ChartTab(const Massif::FileData* data,
             KXMLGUIClient* guiParent, QWidget* parent = nullptr);
    ~ChartTab() override;

    void settingsChanged() override;

    void selectModelItem(const Massif::ModelItem& item) override;

private:
    void setupGui();
    void setupActions();
    void updateHeader();
    void updatePeaks();
    void updateLegendPosition();
    void updateLegendFont();
    void updateDetailedPeaks();

    void printFile(QPrinter *printer);

private Q_SLOTS:
    void setDetailedDiagramHidden(bool hidden);
    void setDetailedDiagramVisible(bool visible);

    void setTotalDiagramHidden(bool hidden);
    void setTotalDiagramVisible(bool visible);

    void saveCurrentDocument();
    void showPrintPreviewDialog();

    void showDetailedGraph(bool show);
    void showTotalGraph(bool show);

    void setStackNum(int num);

    void chartContextMenuRequested(const QPoint &pos);

    void slotHideFunction();
    void slotHideOtherFunctions();

    void detailedItemClicked(const QModelIndex& item);
    void totalItemClicked(const QModelIndex& item);

private:
    KChart::Chart* m_chart;
    QLabel* m_header;
    KChart::Plotter* m_totalDiagram;
    Massif::TotalCostModel* m_totalCostModel;

    KChart::Plotter* m_detailedDiagram;
    Massif::DetailedCostModel* m_detailedCostModel;

    KChart::Legend* m_legend;

    QAction* m_print;
    QAction* m_saveAs;

    QAction* m_toggleTotal;
    QAction* m_toggleDetailed;

    QAction* m_hideFunction;
    QAction* m_hideOtherFunctions;

    QSpinBox* m_box;

    bool m_settingSelection;
};

#endif // CHARTTAB_H
