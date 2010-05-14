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

#include "mainwindow.h"

#include "KDChartChart"
#include "KDChartGridAttributes"
#include "KDChartHeaderFooter"
#include "KDChartCartesianCoordinatePlane"
#include "KDChartPlotter"
#include "KDChartLegend"
#include "KDChartDataValueAttributes"
#include "KDChartBackgroundAttributes"

#include "massifdata/filedata.h"
#include "massifdata/parser.h"
#include "massifdata/snapshotitem.h"
#include "massifdata/treeleafitem.h"

#include "visualizer/totalcostmodel.h"
#include "visualizer/detailedcostmodel.h"
#include "visualizer/datatreemodel.h"
#include "visualizer/filtereddatatreemodel.h"
#include "visualizer/dotgraphgenerator.h"
#include "visualizer/util.h"

#include <KStandardAction>
#include <KActionCollection>
#include <KAction>
#include <KFileDialog>
#include <KRecentFilesAction>

#include <KMimeType>
#include <KFilterDev>
#include <KMessageBox>

#include <KColorScheme>

#include <KStatusBar>

#include <KParts/Part>
#include <KLibFactory>
#include <KLibLoader>

#include <QSortFilterProxyModel>

#include <QLabel>
#include <QSpinBox>

#include <KDebug>
#include <KToolBar>

#include <kgraphviewer/kgraphviewer_interface.h>

using namespace Massif;
using namespace KDChart;

//BEGIN Helper Functions
void markPeak(Plotter* p, const QModelIndex& peak, uint cost, QPen foreground)
{
    DataValueAttributes dataAttributes = p->dataValueAttributes(peak);
    dataAttributes.setDataLabel(i18n("Peak of %1", prettyCost(cost)));
    dataAttributes.setVisible(true);

    MarkerAttributes a = dataAttributes.markerAttributes();
    a.setMarkerSize(QSizeF(2, 2));
    a.setPen(foreground);
    a.setMarkerStyle(KDChart::MarkerAttributes::MarkerCircle);
    a.setVisible(true);
    dataAttributes.setMarkerAttributes(a);

    TextAttributes txtAttrs = dataAttributes.textAttributes();
    txtAttrs.setPen(foreground);
    dataAttributes.setTextAttributes(txtAttrs);

    BackgroundAttributes bkgAtt = dataAttributes.backgroundAttributes();
    QBrush brush = p->model()->data(peak, DatasetBrushRole).value<QBrush>();
    QColor c = brush.color();
    c.setAlpha(127);
    brush.setColor(c);
    brush.setStyle(Qt::CrossPattern);
    bkgAtt.setBrush(brush);
    bkgAtt.setVisible(true);
    dataAttributes.setBackgroundAttributes(bkgAtt);

    p->setDataValueAttributes(peak, dataAttributes);
}
//END Helper Functions

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags f)
    : KParts::MainWindow(parent, f), m_chart(new Chart(this)), m_header(new HeaderFooter(m_chart)), m_subheader(new HeaderFooter(m_chart))
    , m_toggleTotal(0), m_totalDiagram(0), m_totalCostModel(new TotalCostModel(m_chart))
    , m_toggleDetailed(0), m_detailedDiagram(0), m_detailedCostModel(new DetailedCostModel(m_chart))
    , m_legend(new Legend(m_chart))
    , m_dataTreeModel(new DataTreeModel(m_chart)), m_dataTreeFilterModel(new FilteredDataTreeModel(m_dataTreeModel))
    , m_data(0) , m_changingSelections(false), m_graphViewerPart(0), m_dotGenerator(0)
{
    ui.setupUi(this);

    setWindowTitle(i18n("Massif Visualizer"));

    m_header->setPosition(Position(KDChartEnums::PositionNorth));
    m_header->setTextAlignment(Qt::AlignHCenter);
    m_chart->addHeaderFooter(m_header);

    m_subheader->setTextAlignment(Qt::AlignHCenter);
    TextAttributes textAttributes = m_subheader->textAttributes();
    textAttributes.setFontSize(Measure(0.5));
    m_subheader->setTextAttributes(textAttributes);
    m_chart->addHeaderFooter(m_subheader);

    // for axis labels to fit
    m_chart->setGlobalLeadingRight(10);
    m_chart->setGlobalLeadingLeft(10);

    m_legend->setPosition(Position(KDChartEnums::PositionFloating));
    m_legend->setTitleText("");
    m_legend->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_legend->setSortOrder(Qt::DescendingOrder);

    m_chart->addLegend(m_legend);

    //NOTE: this has to be set _after_ the legend was added to the chart...
    TextAttributes att = m_legend->textAttributes();
    att.setAutoShrink(true);
    att.setFontSize( Measure(12) );
    m_legend->setTextAttributes(att);
    m_legend->hide();

    ui.plotterTab->layout()->addWidget(m_chart);

    //BEGIN KGraphViewer
    KLibFactory *factory = KLibLoader::self()->factory("kgraphviewerpart");
    if (factory) {
        m_graphViewerPart = factory->create<KParts::ReadOnlyPart>(this);
        if (m_graphViewerPart) {
            m_graphViewer = qobject_cast< KGraphViewer::KGraphViewerInterface* >(m_graphViewerPart);
            ui.dotGraphTab->layout()->addWidget(m_graphViewerPart->widget());
            connect(m_graphViewerPart, SIGNAL(graphLoaded()), this, SLOT(slotGraphLoaded()));
        } else {
            ui.tabWidget->removeTab(1);
        }
    }
    //END KGraphViewer

    connect(ui.filterDataTree, SIGNAL(textChanged(QString)),
            m_dataTreeFilterModel, SLOT(setFilter(QString)));
    ui.treeView->setModel(m_dataTreeFilterModel);
    connect(ui.treeView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(treeSelectionChanged(QModelIndex,QModelIndex)));
    connect(ui.tabWidget, SIGNAL(currentChanged(int)),
            this, SLOT(slotTabChanged(int)));

    setupActions();
    setupGUI(StandardWindowOptions(Default ^ StatusBar));
    statusBar()->hide();

    slotTabChanged(ui.tabWidget->currentIndex());
}

MainWindow::~MainWindow()
{
    m_recentFiles->saveEntries(KGlobal::config()->group( QString() ));
}

void MainWindow::setupActions()
{
    KStandardAction::open(this, SLOT(openFile()), actionCollection());
    m_recentFiles = KStandardAction::openRecent(this, SLOT(openFile(KUrl)), actionCollection());
    m_recentFiles->loadEntries(KGlobal::config()->group( QString() ));

    KStandardAction::close(this, SLOT(closeFile()), actionCollection());

    KStandardAction::quit(qApp, SLOT(closeAllWindows()), actionCollection());

    m_toggleTotal = new KAction(KIcon("office-chart-area"), i18n("toggle total cost graph"), actionCollection());
    m_toggleTotal->setCheckable(true);
    m_toggleTotal->setChecked(true);
    m_toggleTotal->setEnabled(false);
    connect(m_toggleTotal, SIGNAL(toggled(bool)), SLOT(showTotalGraph(bool)));
    actionCollection()->addAction("toggle_total", m_toggleTotal);

    m_toggleDetailed = new KAction(KIcon("office-chart-area-stacked"), i18n("toggle detailed cost graph"), actionCollection());
    m_toggleDetailed->setCheckable(true);
    m_toggleDetailed->setChecked(true);
    m_toggleDetailed->setEnabled(false);
    connect(m_toggleDetailed, SIGNAL(toggled(bool)), SLOT(showDetailedGraph(bool)));
    actionCollection()->addAction("toggle_detailed", m_toggleDetailed);

    if (m_graphViewer) {
        m_zoomIn = KStandardAction::zoomIn(this, SLOT(zoomIn()), actionCollection());
        actionCollection()->addAction("zoomIn", m_zoomIn);
        m_zoomOut = KStandardAction::zoomOut(this, SLOT(zoomOut()), actionCollection());
        actionCollection()->addAction("zoomOut", m_zoomOut);
        m_focusExpensive = new KAction(KIcon("flag-red"), i18n("focus most expensive node"), actionCollection());
        m_toggleDetailed->setEnabled(false);
        connect(m_focusExpensive, SIGNAL(triggered()), this, SLOT(focusExpensiveGraphNode()));
        actionCollection()->addAction("focusExpensive", m_focusExpensive);
    }

    m_selectPeak = new KAction(KIcon("flag-red"), i18n("select peak snapshot"), ui.dataTreeDock);
    m_selectPeak->setEnabled(false);
    connect(m_selectPeak, SIGNAL(triggered()), this, SLOT(selectPeakSnapshot()));
    actionCollection()->addAction("selectPeak", m_selectPeak);

    KAction* stackNumAction = actionCollection()->addAction("stackNum");
    stackNumAction->setText(i18n("stacked diagrams"));
    QWidget *stackNumWidget = new QWidget;
    QHBoxLayout* stackNumLayout = new QHBoxLayout;
    stackNumLayout->addWidget(new QLabel(i18n("stacked diagrams:")));
    QSpinBox* box = new QSpinBox;
    box->setMinimum(0);
    box->setMaximum(50);
    box->setValue(10);
    connect(box, SIGNAL(valueChanged(int)), this, SLOT(setStackNum(int)));
    stackNumLayout->addWidget(box);
    stackNumWidget->setLayout(stackNumLayout);
    stackNumAction->setDefaultWidget(stackNumWidget);
}

void MainWindow::openFile()
{
    QString file = KFileDialog::getOpenFileName(KUrl("kfiledialog:///massif-visualizer"), QString(), this, i18n("Open Massif Output File"));
    if (!file.isEmpty()) {
        openFile(KUrl(file));
    }
}

void MainWindow::openFile(const KUrl& file)
{
    QString mimeType = KMimeType::findByPath(file.toLocalFile(), 0, false)->name ();
    QIODevice* device = KFilterDev::deviceForFile (file.toLocalFile(), mimeType, false);
    if (!device->open(QIODevice::ReadOnly)) {
        KMessageBox::error(this, i18n("Could not open file <i>%1</i> for reading.", file.toLocalFile()), i18n("Could not read file"));
        delete device;
        return;
    }
    if (m_data) {
        closeFile();
    }
    Parser p;
    m_data = p.parse(device);
    if (!m_data) {
        KMessageBox::error(this, i18n("Could not parse file <i>%1</i>.<br>"
                                      "Parse error in line %2:<br>%3", file.toLocalFile(), p.errorLine() + 1, p.errorLineString()),
                           i18n("Could not parse file"));
        return;
    }

    kDebug() << "loaded massif file:" << file;
    qDebug() << "description:" << m_data->description();
    qDebug() << "command:" << m_data->cmd();
    qDebug() << "time unit:" << m_data->timeUnit();
    qDebug() << "snapshots:" << m_data->snapshots().size();
    qDebug() << "peak: snapshot #" << m_data->peak()->number() << "after" << QString("%1%2").arg(m_data->peak()->time()).arg(m_data->timeUnit());
    qDebug() << "peak cost:" << prettyCost(m_data->peak()->memHeap()) << " heap"
                             << prettyCost(m_data->peak()->memHeapExtra()) << " heap extra"
                             << prettyCost(m_data->peak()->memStacks()) << " stacks";

    //BEGIN DotGraph
    if (m_graphViewerPart) {
        getDotGraph(QPair<TreeLeafItem*,SnapshotItem*>(0, m_data->peak()));
        m_zoomIn->setEnabled(true);
        m_zoomOut->setEnabled(true);
        m_focusExpensive->setEnabled(true);
    }

    //BEGIN KDChart
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

    //BEGIN Header
    {
        TextAttributes textAttributes = m_header->textAttributes();
        textAttributes.setPen(foreground);
        textAttributes.setFontSize(Measure(10));
        m_header->setTextAttributes(textAttributes);
        m_header->setText(i18n("memory consumption of '%1' %2", m_data->cmd(), m_data->description() != "(none)" ? m_data->description() : ""));
        m_subheader->setText(i18n("peak of %1 at snapshot %2", prettyCost(m_data->peak()->memHeap()), m_data->peak()->number()));
        textAttributes.setFontSize(Measure(8));
        m_subheader->setTextAttributes(textAttributes);
    }

    setWindowTitle(i18n("Massif Visualizer - evaluation of %1 (%2)", m_data->cmd(), file.fileName()));

    //BEGIN TotalDiagram
    m_totalDiagram = new Plotter;
    m_toggleTotal->setEnabled(true);
    m_totalDiagram->setAntiAliasing(true);

    CartesianAxis* bottomAxis = new CartesianAxis;
    TextAttributes axisTextAttributes = bottomAxis->textAttributes();
    axisTextAttributes.setPen(foreground);
    axisTextAttributes.setFontSize(Measure(8));
    bottomAxis->setTextAttributes(axisTextAttributes);
    TextAttributes axisTitleTextAttributes = bottomAxis->titleTextAttributes();
    axisTitleTextAttributes.setPen(foreground);
    axisTitleTextAttributes.setFontSize(Measure(10));
    bottomAxis->setTitleTextAttributes(axisTitleTextAttributes);
    bottomAxis->setTitleText(i18n("time in %1", m_data->timeUnit()));
    bottomAxis->setPosition ( CartesianAxis::Bottom );
    m_totalDiagram->addAxis(bottomAxis);

    CartesianAxis* leftAxis = new CartesianAxis;
    leftAxis->setTextAttributes(axisTextAttributes);
    leftAxis->setTitleTextAttributes(axisTitleTextAttributes);
    leftAxis->setTitleText(i18n("memory heap size in bytes"));
    leftAxis->setPosition ( CartesianAxis::Left );
    m_totalDiagram->addAxis(leftAxis);

    m_totalCostModel->setSource(m_data);
    m_totalDiagram->setModel(m_totalCostModel);
    if (m_data->peak()) {
        const QModelIndex peak = m_totalCostModel->peak();
        Q_ASSERT(peak.isValid());
        markPeak(m_totalDiagram, peak, m_data->peak()->memHeap(), foreground);
    }

    m_chart->coordinatePlane()->addDiagram(m_totalDiagram);
    m_legend->addDiagram(m_totalDiagram);
    connect(m_totalDiagram, SIGNAL(clicked(QModelIndex)),
            this, SLOT(totalItemClicked(QModelIndex)));

    //BEGIN DetailedDiagram
    m_detailedDiagram = new Plotter;
    m_toggleDetailed->setEnabled(true);
    m_detailedDiagram->setAntiAliasing(true);
    m_detailedDiagram->setType(KDChart::Plotter::Stacked);

    m_detailedCostModel->setSource(m_data);
    m_detailedDiagram->setModel(m_detailedCostModel);

    CartesianAxis* topAxis = new CartesianAxis;
    topAxis->setTextAttributes(axisTextAttributes);
    topAxis->setTitleTextAttributes(axisTitleTextAttributes);
    topAxis->setTitleText(i18n("time in %1", m_data->timeUnit()));
    topAxis->setPosition ( CartesianAxis::Top );
    m_detailedDiagram->addAxis(topAxis);

    CartesianAxis* rightAxis = new CartesianAxis;
    rightAxis->setTextAttributes(axisTextAttributes);
    rightAxis->setTitleTextAttributes(axisTitleTextAttributes);
    rightAxis->setTitleText(i18n("memory heap size in bytes"));
    rightAxis->setPosition ( CartesianAxis::Right );
    m_detailedDiagram->addAxis(rightAxis);

    updateDetailedPeaks();

    m_chart->coordinatePlane()->addDiagram(m_detailedDiagram);
    connect(m_detailedDiagram, SIGNAL(clicked(QModelIndex)),
            this, SLOT(detailedItemClicked(QModelIndex)));

    m_legend->addDiagram(m_detailedDiagram);

    //BEGIN Legend
    m_legend->show();

    //BEGIN TreeView
    m_dataTreeModel->setSource(m_data);
    m_selectPeak->setEnabled(true);

    //BEGIN RecentFiles
    m_recentFiles->addUrl(file);

    delete device;
}

void MainWindow::treeSelectionChanged(const QModelIndex& now, const QModelIndex& before)
{
    if (m_changingSelections || !m_data) {
        return;
    }

    if (now == before) {
        return;
    }
    m_changingSelections = true;

    const QPair< TreeLeafItem*, SnapshotItem* >& item = m_dataTreeModel->itemForIndex(
       m_dataTreeFilterModel->mapToSource(now)
    );

    if (now.parent().isValid()) {
        m_detailedCostModel->setSelection(m_detailedCostModel->indexForItem(item));
        m_totalCostModel->setSelection(QModelIndex());
    } else {
        m_totalCostModel->setSelection(m_totalCostModel->indexForItem(item));
        m_detailedCostModel->setSelection(QModelIndex());
    }

    m_chart->update();
    getDotGraph(item);

    m_changingSelections = false;
}

void MainWindow::detailedItemClicked(const QModelIndex& idx)
{
    if (m_changingSelections || !m_data) {
        return;
    }

    m_changingSelections = true;

    m_detailedCostModel->setSelection(idx);
    m_totalCostModel->setSelection(QModelIndex());

    // hack: the ToolTip will only be queried by KDChart and that one uses the
    // left index, but we want it to query the right one
    const QModelIndex _idx = m_detailedCostModel->index(idx.row() + 1, idx.column(), idx.parent());
    ui.treeView->selectionModel()->clearSelection();
    QPair< TreeLeafItem*, SnapshotItem* > item = m_detailedCostModel->itemForIndex(_idx);
    const QModelIndex& newIndex = m_dataTreeFilterModel->mapFromSource(
        m_dataTreeModel->indexForItem(item)
    );
    ui.treeView->selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    ui.treeView->scrollTo(ui.treeView->selectionModel()->currentIndex());

    m_chart->update();
    getDotGraph(item);

    m_changingSelections = false;
}

void MainWindow::totalItemClicked(const QModelIndex& idx_)
{
    if (m_changingSelections || !m_data) {
        return;
    }

    m_changingSelections = true;

    QModelIndex idx = idx_.model()->index(idx_.row() + 1, idx_.column(), idx_.parent());

    m_detailedCostModel->setSelection(QModelIndex());
    m_totalCostModel->setSelection(idx);

    QPair< TreeLeafItem*, SnapshotItem* > item = m_totalCostModel->itemForIndex(idx);

    ui.treeView->selectionModel()->clearSelection();
    const QModelIndex& newIndex = m_dataTreeFilterModel->mapFromSource(
        m_dataTreeModel->indexForItem(item)
    );
    ui.treeView->selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    ui.treeView->scrollTo(ui.treeView->selectionModel()->currentIndex());

    m_chart->update();
    getDotGraph(item);

    m_changingSelections = false;
}

void MainWindow::closeFile()
{
    if (!m_data) {
        return;
    }

    kDebug() << "closing file";

    m_chart->replaceCoordinatePlane(new CartesianCoordinatePlane);
    m_legend->removeDiagrams();
    m_legend->hide();
    m_header->setText("");
    m_subheader->setText("");

    m_toggleDetailed->setEnabled(false);
    m_toggleDetailed->setChecked(true);
    m_detailedDiagram = 0;

    m_toggleTotal->setEnabled(false);
    m_toggleTotal->setChecked(true);
    m_totalDiagram = 0;

    m_dataTreeModel->setSource(0);
    m_dataTreeFilterModel->setFilter("");
    m_detailedCostModel->setSource(0);
    m_totalCostModel->setSource(0);

    delete m_data;
    m_data = 0;

    if (m_dotGenerator) {
        if (m_dotGenerator->isRunning()) {
            disconnect(m_dotGenerator, 0, this, 0);
            connect(m_dotGenerator, SIGNAL(finished()), m_dotGenerator, SLOT(deleteLater()));
            m_dotGenerator->cancel();
        } else {
            delete m_dotGenerator;
        }
        m_dotGenerator = 0;
    }
    if (m_graphViewerPart) {
        m_graphViewerPart->closeUrl();
        m_zoomIn->setEnabled(false);
        m_zoomOut->setEnabled(false);
        m_focusExpensive->setEnabled(false);
    }

    setWindowTitle(i18n("Massif Visualizer"));
}

Chart* MainWindow::chart()
{
    return m_chart;
}

void MainWindow::showDetailedGraph(bool show)
{
    Q_ASSERT(m_data);
    Q_ASSERT(m_detailedDiagram);
    m_detailedDiagram->setHidden(!show);
    m_toggleDetailed->setChecked(show);
    m_chart->update();
}

void MainWindow::showTotalGraph(bool show)
{
    Q_ASSERT(m_data);
    Q_ASSERT(m_totalDiagram);
    m_totalDiagram->setHidden(!show);
    m_toggleTotal->setChecked(show);
    m_chart->update();
}

void MainWindow::slotTabChanged(int index)
{
    toolBar("chartToolBar")->setVisible(index == 0);
    foreach(QAction* action, toolBar("chartToolBar")->actions()) {
        action->setEnabled(index == 0);
    }
    toolBar("callgraphToolBar")->setVisible(index == 1);
    foreach(QAction* action, toolBar("callgraphToolBar")->actions()) {
        action->setEnabled(index == 1);
    }
    if (index == 1) {
        // if we parsed a dot graph we might want to show it now
        showDotGraph();
    }
}

void MainWindow::getDotGraph(QPair<TreeLeafItem*, SnapshotItem*> item)
{
    kDebug() << "new dot graph requested" << item;
    if (m_dotGenerator) {
        kDebug() << "existing generator is running:" << m_dotGenerator->isRunning();
        if (m_dotGenerator->isRunning()) {
            disconnect(m_dotGenerator, 0, this, 0);
            connect(m_dotGenerator, SIGNAL(finished()), m_dotGenerator, SLOT(deleteLater()));
            m_dotGenerator->cancel();
        } else {
            delete m_dotGenerator;
        }
        m_dotGenerator = 0;
    }
    if (!item.first && !item.second) {
        return;
    }
    if (item.second) {
        m_dotGenerator = new DotGraphGenerator(item.second, m_data->timeUnit(), this);
    } else {
        m_dotGenerator = new DotGraphGenerator(item.first, m_data->timeUnit(), this);
    }
    connect(m_dotGenerator, SIGNAL(finished()),
            this, SLOT(showDotGraph()));
    m_dotGenerator->start();
}

void MainWindow::showDotGraph()
{
    if (!m_dotGenerator || !m_graphViewerPart || !m_graphViewerPart->widget()->isVisible()) {
        return;
    }
    kDebug() << "show dot graph in output file" << m_dotGenerator->outputFile();
    if (!m_dotGenerator->outputFile().isEmpty() && m_graphViewerPart->url() != KUrl(m_dotGenerator->outputFile())) {
        m_graphViewerPart->openUrl(KUrl(m_dotGenerator->outputFile()));
    }
}

void MainWindow::slotGraphLoaded()
{
    if (!m_dotGenerator) {
        return;
    }
    m_graphViewer->setZoomFactor(0.75);
    m_graphViewer->setPannerPosition(KGraphViewer::KGraphViewerInterface::BottomRight);
    m_graphViewer->setPannerEnabled(true);
    m_graphViewer->centerOnNode(m_dotGenerator->mostCostIntensiveGraphvizId());
}

void MainWindow::zoomIn()
{
    m_graphViewer->zoomIn();
}

void MainWindow::zoomOut()
{
    m_graphViewer->zoomOut();
}

void MainWindow::focusExpensiveGraphNode()
{
    m_graphViewer->centerOnNode(m_dotGenerator->mostCostIntensiveGraphvizId());
}

void MainWindow::selectPeakSnapshot()
{
    ui.treeView->selectionModel()->clearSelection();
    ui.treeView->selectionModel()->setCurrentIndex(
        m_dataTreeFilterModel->mapFromSource(
            m_dataTreeModel->indexForSnapshot(m_data->peak())
        ), QItemSelectionModel::Select | QItemSelectionModel::Rows
    );
}

void MainWindow::setStackNum(int num)
{
    if (m_detailedCostModel) {
        m_detailedCostModel->setMaximumDatasetCount(num);
        updateDetailedPeaks();
    }
}

void MainWindow::updateDetailedPeaks()
{
    KColorScheme scheme(QPalette::Active, KColorScheme::Window);
    QPen foreground(scheme.foreground().color());

    QMap< QModelIndex, TreeLeafItem* > peaks = m_detailedCostModel->peaks();
    QMap< QModelIndex, TreeLeafItem* >::const_iterator it = peaks.constBegin();
    while (it != peaks.constEnd()) {
        const QModelIndex peak = it.key();
        Q_ASSERT(peak.isValid());
        markPeak(m_detailedDiagram, peak, it.value()->cost(), foreground);
        ++it;
    }
}

#include "mainwindow.moc"
