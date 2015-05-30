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

#include "charttab.h"

#include "KChartChart"
#include "KChartGridAttributes"
#include "KChartHeaderFooter"
#include "KChartCartesianCoordinatePlane"
#include "KChartPlotter"
#include "KChartLegend"
#include "KChartDataValueAttributes"
#include "KChartBackgroundAttributes"
#include <KChartFrameAttributes.h>

#include "visualizer/totalcostmodel.h"
#include "visualizer/detailedcostmodel.h"
#include "visualizer/datatreemodel.h"
#include "visualizer/filtereddatatreemodel.h"

#include "massifdata/util.h"
#include "massifdata/snapshotitem.h"
#include "massifdata/treeleafitem.h"
#include "massifdata/filedata.h"

#include "massif-visualizer-settings.h"

#include <QDebug>
#include <QVBoxLayout>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QLabel>
#include <QMenu>
#include <QApplication>
#include <QDesktopWidget>
#include <QSvgGenerator>
#include <QWidgetAction>
#include <QFileDialog>

#include <KColorScheme>
#include <KLocalizedString>
#include <KStandardAction>
#include <KActionCollection>
#include <KMessageBox>
#include <KFormat>

using namespace KChart;
using namespace Massif;

namespace {

class TimeAxis : public CartesianAxis
{
    Q_OBJECT
public:
    explicit TimeAxis(AbstractCartesianDiagram* diagram = 0)
        : CartesianAxis(diagram)
    {}

    virtual const QString customizedLabel(const QString& label) const
    {
        // squeeze large numbers here
        // TODO: when the unit is 'b' also use prettyCost() here
        return QString::number(label.toDouble());
    }
};

class SizeAxis : public CartesianAxis
{
    Q_OBJECT
public:
    explicit SizeAxis(AbstractCartesianDiagram* diagram = 0)
        : CartesianAxis(diagram)
    {}

    virtual const QString customizedLabel(const QString& label) const
    {
        // TODO: change distance between labels to 1024 and simply use prettyCost() here
        KFormat format(QLocale::system());
        return format.formatByteSize(label.toDouble(), 1, KFormat::MetricBinaryDialect);
    }
};

void markPeak(Plotter* p, const QModelIndex& peak, quint64 cost, const KColorScheme& scheme)
{
    QBrush brush = p->model()->data(peak, DatasetBrushRole).value<QBrush>();

    QColor outline = brush.color();
    QColor foreground = scheme.foreground().color();
    QBrush background = scheme.background();

    DataValueAttributes dataAttributes = p->dataValueAttributes(peak);
    dataAttributes.setDataLabel(prettyCost(cost));
    dataAttributes.setVisible(true);
    dataAttributes.setShowRepetitiveDataLabels(true);
    dataAttributes.setShowOverlappingDataLabels(false);

    FrameAttributes frameAttrs = dataAttributes.frameAttributes();
    QPen framePen(outline);
    framePen.setWidth(2);
    frameAttrs.setPen(framePen);
    frameAttrs.setVisible(true);
    dataAttributes.setFrameAttributes(frameAttrs);

    MarkerAttributes a = dataAttributes.markerAttributes();
    a.setMarkerSize(QSizeF(7, 7));
    a.setPen(outline);
    a.setMarkerStyle(KChart::MarkerAttributes::MarkerDiamond);
    a.setVisible(true);
    dataAttributes.setMarkerAttributes(a);

    TextAttributes txtAttrs = dataAttributes.textAttributes();
    txtAttrs.setPen(foreground);
    txtAttrs.setFontSize(Measure(12));
    dataAttributes.setTextAttributes(txtAttrs);

    BackgroundAttributes bkgAtt = dataAttributes.backgroundAttributes();

    bkgAtt.setBrush(background);
    bkgAtt.setVisible(true);
    dataAttributes.setBackgroundAttributes(bkgAtt);

    p->setDataValueAttributes(peak, dataAttributes);
}

}

ChartTab::ChartTab(const FileData* data,
                   KXMLGUIClient* guiParent, QWidget* parent)
  : DocumentTabInterface(data, guiParent, parent)
    , m_chart(new Chart(this))
    , m_header(new QLabel(this))
    , m_totalDiagram(0)
    , m_totalCostModel(new TotalCostModel(m_chart))
    , m_detailedDiagram(0)
    , m_detailedCostModel(new DetailedCostModel(m_chart))
    , m_legend(new Legend(m_chart))
    , m_print(0)
    , m_saveAs(0)
    , m_toggleTotal(0)
    , m_toggleDetailed(0)
    , m_hideFunction(0)
    , m_hideOtherFunctions(0)
    , m_box(new QSpinBox(this))
    , m_settingSelection(false)
{
    setXMLFile("charttabui.rc", true);
    setupActions();

    setLayout(new QVBoxLayout(this));

    setupGui();
}

ChartTab::~ChartTab()
{
}

void ChartTab::setupActions()
{
    m_print = KStandardAction::print(this, SLOT(showPrintPreviewDialog()), actionCollection());
    actionCollection()->addAction("file_print", m_print);

    m_saveAs = KStandardAction::saveAs(this, SLOT(saveCurrentDocument()), actionCollection());
    actionCollection()->addAction("file_save_as", m_saveAs);

    m_toggleTotal = new QAction(QIcon::fromTheme("office-chart-area"), i18n("Toggle total cost graph"), actionCollection());
    m_toggleTotal->setCheckable(true);
    m_toggleTotal->setChecked(true);
    connect(m_toggleTotal, &QAction::toggled, this, &ChartTab::showTotalGraph);
    actionCollection()->addAction("toggle_total", m_toggleTotal);

    m_toggleDetailed = new QAction(QIcon::fromTheme("office-chart-area-stacked"), i18n("Toggle detailed cost graph"), actionCollection());
    m_toggleDetailed->setCheckable(true);
    m_toggleDetailed->setChecked(true);
    connect(m_toggleDetailed, &QAction::toggled, this, &ChartTab::showDetailedGraph);
    actionCollection()->addAction("toggle_detailed", m_toggleDetailed);

    QWidgetAction* stackNumAction = new QWidgetAction(actionCollection());
    actionCollection()->addAction("stackNum", stackNumAction);
    stackNumAction->setText(i18n("Stacked diagrams"));
    QWidget *stackNumWidget = new QWidget;
    QHBoxLayout* stackNumLayout = new QHBoxLayout;
    stackNumLayout->addWidget(new QLabel(i18n("Stacked diagrams:")));
    m_box->setMinimum(0);
    m_box->setMaximum(50);
    m_box->setValue(10);
    connect(m_box, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ChartTab::setStackNum);
    stackNumLayout->addWidget(m_box);
    stackNumWidget->setLayout(stackNumLayout);
    stackNumAction->setDefaultWidget(stackNumWidget);

    m_hideFunction = new QAction(i18n("hide function"), this);
    connect(m_hideFunction, &QAction::triggered,
            this, &ChartTab::slotHideFunction);
    m_hideOtherFunctions = new QAction(i18n("hide other functions"), this);
    connect(m_hideOtherFunctions, &QAction::triggered,
            this, &ChartTab::slotHideOtherFunctions);
}

void ChartTab::setupGui()
{
    layout()->addWidget(m_header);
    layout()->addWidget(m_chart);

    // HACK: otherwise the legend becomes _really_ large and might even crash X...
    // to visualize the issue, try: m_chart->setMaximumSize(QSize(10000, 10000));
    m_chart->setMaximumSize(qApp->desktop()->size());

    // for axis labels to fit
    m_chart->setGlobalLeadingRight(10);
    m_chart->setGlobalLeadingLeft(10);
    m_chart->setGlobalLeadingTop(20);
    m_chart->setContextMenuPolicy(Qt::CustomContextMenu);

    updateLegendPosition();
    m_legend->setTitleText(QString());
    m_legend->setSortOrder(Qt::DescendingOrder);

    m_chart->addLegend(m_legend);

    //NOTE: this has to be set _after_ the legend was added to the chart...
    updateLegendFont();
    m_legend->setTextAlignment(Qt::AlignLeft);
    m_legend->hide();

    connect(m_chart, &Chart::customContextMenuRequested,
            this, &ChartTab::chartContextMenuRequested);

    //BEGIN KChart
    KColorScheme scheme(QPalette::Active, KColorScheme::Window);
    QPen foreground(scheme.foreground().color());

    //Begin Legend
    BackgroundAttributes bkgAtt = m_legend->backgroundAttributes();
    QColor background = scheme.background(KColorScheme::AlternateBackground).color();
    background.setAlpha(200);
    bkgAtt.setBrush(QBrush(background));
    bkgAtt.setVisible(true);
    m_legend->setBackgroundAttributes(bkgAtt);
    TextAttributes txtAttrs = m_legend->textAttributes();
    txtAttrs.setPen(foreground);
    m_legend->setTextAttributes(txtAttrs);

    m_header->setAlignment(Qt::AlignCenter);
    updateHeader();

    //BEGIN TotalDiagram
    m_totalDiagram = new Plotter(this);
    m_totalDiagram->setAntiAliasing(true);

    CartesianAxis* bottomAxis = new TimeAxis(m_totalDiagram);
    TextAttributes axisTextAttributes = bottomAxis->textAttributes();
    axisTextAttributes.setPen(foreground);
    axisTextAttributes.setFontSize(Measure(10));
    bottomAxis->setTextAttributes(axisTextAttributes);
    TextAttributes axisTitleTextAttributes = bottomAxis->titleTextAttributes();
    axisTitleTextAttributes.setPen(foreground);
    axisTitleTextAttributes.setFontSize(Measure(12));
    bottomAxis->setTitleTextAttributes(axisTitleTextAttributes);
    bottomAxis->setTitleText(i18n("time in %1", m_data->timeUnit()));
    bottomAxis->setPosition ( CartesianAxis::Bottom );
    m_totalDiagram->addAxis(bottomAxis);

    CartesianAxis* rightAxis = new SizeAxis(m_totalDiagram);
    rightAxis->setTextAttributes(axisTextAttributes);
    rightAxis->setTitleTextAttributes(axisTitleTextAttributes);
    rightAxis->setTitleText(i18n("memory heap size"));
    rightAxis->setPosition ( CartesianAxis::Right );
    m_totalDiagram->addAxis(rightAxis);

    m_totalCostModel->setSource(m_data);
    m_totalDiagram->setModel(m_totalCostModel);

    CartesianCoordinatePlane* coordinatePlane = dynamic_cast<CartesianCoordinatePlane*>(m_chart->coordinatePlane());
    Q_ASSERT(coordinatePlane);
    coordinatePlane->addDiagram(m_totalDiagram);

    GridAttributes gridAttributes = coordinatePlane->gridAttributes(Qt::Horizontal);
    gridAttributes.setAdjustBoundsToGrid(false, false);
    coordinatePlane->setGridAttributes(Qt::Horizontal, gridAttributes);

    m_legend->addDiagram(m_totalDiagram);

    m_detailedDiagram = new Plotter;
    m_detailedDiagram->setAntiAliasing(true);
    m_detailedDiagram->setType(KChart::Plotter::Stacked);

    m_detailedCostModel->setSource(m_data);
    m_detailedDiagram->setModel(m_detailedCostModel);

    updatePeaks();

    m_chart->coordinatePlane()->addDiagram(m_detailedDiagram);

    m_legend->addDiagram(m_detailedDiagram);
    m_legend->show();

    m_box->setValue(m_detailedCostModel->maximumDatasetCount());

    connect(m_totalDiagram, &Plotter::clicked,
            this, &ChartTab::totalItemClicked);
    connect(m_detailedDiagram, &Plotter::clicked,
            this, &ChartTab::detailedItemClicked);
}

void ChartTab::settingsChanged()
{
    updateHeader();
    updatePeaks();
    updateLegendPosition();
    updateLegendFont();
}

void ChartTab::setDetailedDiagramHidden(bool hidden)
{
    m_detailedDiagram->setHidden(hidden);
}

void ChartTab::setDetailedDiagramVisible(bool visible)
{
    m_detailedDiagram->setVisible(visible);
}

void ChartTab::setTotalDiagramHidden(bool hidden)
{
    m_totalDiagram->setHidden(hidden);
}

void ChartTab::setTotalDiagramVisible(bool visible)
{
    m_totalDiagram->setVisible(visible);
}

void ChartTab::updatePeaks()
{
    KColorScheme scheme(QPalette::Active, KColorScheme::Window);

    if (m_data->peak()) {
        const QModelIndex peak = m_totalCostModel->peak();
        Q_ASSERT(peak.isValid());
        markPeak(m_totalDiagram, peak, m_data->peak()->cost(), scheme);
    }
    updateDetailedPeaks();
}

void ChartTab::updateLegendPosition()
{
    KChartEnums::PositionValue pos;
    switch (Settings::self()->legendPosition()) {
        case Settings::EnumLegendPosition::North:
            pos = KChartEnums::PositionNorth;
            break;
        case Settings::EnumLegendPosition::South:
            pos = KChartEnums::PositionSouth;
            break;
        case Settings::EnumLegendPosition::East:
            pos = KChartEnums::PositionEast;
            break;
        case Settings::EnumLegendPosition::West:
            pos = KChartEnums::PositionWest;
            break;
        case Settings::EnumLegendPosition::Floating:
            pos = KChartEnums::PositionFloating;
            break;
        default:
            pos = KChartEnums::PositionFloating;
            qDebug() << "invalid legend position";
    }
    m_legend->setPosition(Position(pos));

    Qt::Alignment align;
    switch (Settings::self()->legendAlignment()) {
        case Settings::EnumLegendAlignment::Left:
            align = Qt::AlignLeft;
            break;
        case Settings::EnumLegendAlignment::Center:
            align = Qt::AlignHCenter | Qt::AlignVCenter;
            break;
        case Settings::EnumLegendAlignment::Right:
            align = Qt::AlignRight;
            break;
        case Settings::EnumLegendAlignment::Top:
            align = Qt::AlignTop;
            break;
        case Settings::EnumLegendAlignment::Bottom:
            align = Qt::AlignBottom;
            break;
        default:
            align = Qt::AlignHCenter | Qt::AlignVCenter;
            qDebug() << "invalid legend alignmemnt";
    }

    // do something reasonable since top,bottom have no effect
    // when used with north,south, same for left,right used with
    // east,west
    if ((((pos == KChartEnums::PositionNorth) || (pos == KChartEnums::PositionSouth))
         && ((align == Qt::AlignTop) || (align == Qt::AlignBottom)))
         || (((pos == KChartEnums::PositionEast) || (pos == KChartEnums::PositionWest))
         && ((align == Qt::AlignLeft) || (align == Qt::AlignRight)))) {

         align = Qt::AlignHCenter | Qt::AlignVCenter;
    }

    m_legend->setAlignment(align);
}

void ChartTab::updateLegendFont()
{
    TextAttributes att = m_legend->textAttributes();
    att.setAutoShrink(true);
    att.setFontSize(Measure(Settings::self()->legendFontSize()));
    QFont font("monospace");
    font.setStyleHint(QFont::TypeWriter);
    att.setFont(font);
    m_legend->setTextAttributes(att);
}

void ChartTab::updateDetailedPeaks()
{
    KColorScheme scheme(QPalette::Active, KColorScheme::Window);

    const DetailedCostModel::Peaks& peaks = m_detailedCostModel->peaks();
    DetailedCostModel::Peaks::const_iterator it = peaks.constBegin();
    while (it != peaks.constEnd()) {
        const QModelIndex peak = it.key();
        Q_ASSERT(peak.isValid());
        markPeak(m_detailedDiagram, peak, it.value()->cost(), scheme);
        ++it;
    }
}

void ChartTab::updateHeader()
{
    const QString app = m_data->cmd().split(' ', QString::SkipEmptyParts).first();

    m_header->setText(QString("<b>%1</b><br /><i>%2</i>")
                        .arg(i18n("Memory consumption of %1", app))
                        .arg(i18n("Peak of %1 at snapshot #%2", prettyCost(m_data->peak()->cost()), m_data->peak()->number()))
    );
    m_header->setToolTip(i18n("Command: %1\nValgrind Options: %2", m_data->cmd(), m_data->description()));
}

void ChartTab::saveCurrentDocument()
{
    const auto saveFilename = QFileDialog::getSaveFileName(this, i18n("Save Current Visualization"), QString(),
                                                           i18n("Images (*.png *.jpg *.tiff *.svg)"));

    if (!saveFilename.isEmpty()) {

        // TODO: implement a dialog to expose more options to the user.
        // for example we could expose dpi, size, and various format
        // dependent options such as compressions settings.

        // Vector graphic format
        if (QFileInfo(saveFilename).suffix().compare(QLatin1String("svg")) == 0) {
            QSvgGenerator generator;
            generator.setFileName(saveFilename);
            generator.setSize(m_chart->size());
            generator.setViewBox(m_chart->rect());

            QPainter painter;
            painter.begin(&generator);
            m_chart->paint(&painter, m_chart->rect());
            painter.end();
        }

        // Other format
        else if (!QPixmap::grabWidget(m_chart).save(saveFilename)) {

            KMessageBox::sorry(this, QString(
                i18n("Error, failed to save the image to %1.")
                    ).arg(saveFilename));
        }
    }
}

void ChartTab::showPrintPreviewDialog()
{
    QPrinter printer;
    QPrintPreviewDialog *ppd = new QPrintPreviewDialog(&printer, this);
    ppd->setAttribute(Qt::WA_DeleteOnClose);
    connect(ppd, &QPrintPreviewDialog::paintRequested, this, &ChartTab::printFile);
    ppd->setWindowTitle(i18n("Massif Chart Print Preview"));
    ppd->resize(800, 600);
    ppd->exec();
}

void ChartTab::printFile(QPrinter *printer)
{
    QPainter painter;
    painter.begin(printer);
    m_chart->paint(&painter, printer->pageRect());
    painter.end();
}

void ChartTab::showDetailedGraph(bool show)
{
    m_detailedDiagram->setHidden(!show);
    m_toggleDetailed->setChecked(show);
    m_chart->update();
}

void ChartTab::showTotalGraph(bool show)
{
    m_totalDiagram->setHidden(!show);
    m_toggleTotal->setChecked(show);
    m_chart->update();
}

void ChartTab::slotHideFunction()
{
    m_detailedCostModel->hideFunction(m_hideFunction->data().value<const TreeLeafItem*>());
}

void ChartTab::slotHideOtherFunctions()
{
    m_detailedCostModel->hideOtherFunctions(m_hideOtherFunctions->data().value<const TreeLeafItem*>());
}

void ChartTab::chartContextMenuRequested(const QPoint& pos)
{
    const QPoint dPos = m_detailedDiagram->mapFromGlobal(m_chart->mapToGlobal(pos));

    const QModelIndex idx = m_detailedDiagram->indexAt(dPos);
    if (!idx.isValid()) {
        return;
    }
    // hack: the ToolTip will only be queried by KChart and that one uses the
    // left index, but we want it to query the right one
    const QModelIndex _idx = m_detailedCostModel->index(idx.row() + 1, idx.column(), idx.parent());
    ModelItem item = m_detailedCostModel->itemForIndex(_idx);

    if (!item.first) {
        return;
    }

    QMenu menu;

    m_hideFunction->setData(QVariant::fromValue(item.first));
    menu.addAction(m_hideFunction);

    m_hideOtherFunctions->setData(QVariant::fromValue(item.first));
    menu.addAction(m_hideOtherFunctions);

    menu.addSeparator();

    emit contextMenuRequested(item, &menu);

    menu.exec(m_detailedDiagram->mapToGlobal(dPos));
}

void ChartTab::setStackNum(int num)
{
    m_detailedCostModel->setMaximumDatasetCount(num);
    updatePeaks();
}

void ChartTab::detailedItemClicked(const QModelIndex& idx)
{
    m_detailedCostModel->setSelection(idx);
    m_totalCostModel->setSelection(QModelIndex());
    m_chart->update();

    // hack: the ToolTip will only be queried by KChart and that one uses the
    // left index, but we want it to query the right one
    m_settingSelection = true;
    const QModelIndex _idx = m_detailedCostModel->index(idx.row() + 1, idx.column(), idx.parent());
    emit modelItemSelected(m_detailedCostModel->itemForIndex(_idx));
    m_settingSelection = false;
}

void ChartTab::totalItemClicked(const QModelIndex& idx)
{
    const QModelIndex _idx = m_totalCostModel->index(idx.row() + 1, idx.column(), idx.parent());
    m_totalCostModel->setSelection(_idx);
    m_detailedCostModel->setSelection(QModelIndex());
    m_chart->update();

    m_settingSelection = true;
    emit modelItemSelected(m_totalCostModel->itemForIndex(_idx));
    m_settingSelection = false;
}

void ChartTab::selectModelItem(const ModelItem& item)
{
    if (m_settingSelection) {
        return;
    }

    if (item.first) {
        m_detailedCostModel->setSelection(m_detailedCostModel->indexForItem(item));
        m_totalCostModel->setSelection(QModelIndex());
    } else {
        m_totalCostModel->setSelection(m_totalCostModel->indexForItem(item));
        m_detailedCostModel->setSelection(QModelIndex());
    }

    m_chart->update();
}

#include "charttab.moc"
#include "moc_charttab.cpp"
