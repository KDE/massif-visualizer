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
#include "massifdata/parsethread.h"
#include "massifdata/snapshotitem.h"
#include "massifdata/treeleafitem.h"
#include "massifdata/util.h"

#include "visualizer/totalcostmodel.h"
#include "visualizer/detailedcostmodel.h"
#include "visualizer/datatreemodel.h"
#include "visualizer/filtereddatatreemodel.h"
#include "visualizer/dotgraphgenerator.h"

#include "massif-visualizer-settings.h"
#include "configdialog.h"

#include <KStandardAction>
#include <KActionCollection>
#include <KAction>
#include <KFileDialog>
#include <KRecentFilesAction>
#include <KColorScheme>
#include <KStatusBar>
#include <KToolBar>
#include <KParts/Part>
#include <KLibFactory>
#include <KLibLoader>

#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QLabel>
#include <QSpinBox>
#include <QInputDialog>

#include <KDebug>

#ifdef HAVE_KGRAPHVIEWER
#include <kgraphviewer_interface.h>
#endif

using namespace Massif;
using namespace KDChart;

//BEGIN Helper Functions
void markPeak(Plotter* p, const QModelIndex& peak, quint64 cost, QPen foreground)
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

KConfigGroup allocatorConfig()
{
    return KGlobal::config()->group("Allocators");
}

//END Helper Functions

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags f)
    : KParts::MainWindow(parent, f), m_chart(new Chart(this)), m_header(new QLabel(this))
    , m_toggleTotal(0)
    , m_totalDiagram(0)
    , m_totalCostModel(new TotalCostModel(m_chart))
    , m_toggleDetailed(0)
    , m_detailedDiagram(0)
    , m_detailedCostModel(new DetailedCostModel(m_chart))
    , m_legend(new Legend(m_chart))
    , m_dataTreeModel(new DataTreeModel(m_chart))
    , m_dataTreeFilterModel(new FilteredDataTreeModel(m_dataTreeModel))
    , m_data(0)
    , m_selectPeak(0)
    , m_recentFiles(0)
    , m_changingSelections(false)
#ifdef HAVE_KGRAPHVIEWER
    , m_graphViewerPart(0)
    , m_graphViewer(0)
    , m_dotGenerator(0)
#endif
    , m_zoomIn(0)
    , m_zoomOut(0)
    , m_focusExpensive(0)
    , m_close(0)
    , m_stopParser(0)
    , m_allocatorModel(new QStringListModel(this))
    , m_newAllocator(0)
    , m_removeAllocator(0)
    , m_shortenTemplates(0)
    , m_parseThread(0)
{
    ui.setupUi(this);

    setWindowTitle(i18n("Massif Visualizer"));

    // for axis labels to fit
    m_chart->setGlobalLeadingRight(10);
    m_chart->setGlobalLeadingLeft(10);
    m_chart->setGlobalLeadingTop(20);
    connect(m_chart, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(chartContextMenuRequested(QPoint)));
    m_chart->setContextMenuPolicy(Qt::CustomContextMenu);

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

    ui.plotterTab->layout()->addWidget(m_header);
    ui.plotterTab->layout()->addWidget(m_chart);

    //BEGIN KGraphViewer

    bool haveGraphViewer = false;

#ifdef HAVE_KGRAPHVIEWER
    KLibFactory *factory = KLibLoader::self()->factory("kgraphviewerpart");
    if (factory) {
        m_graphViewerPart = factory->create<KParts::ReadOnlyPart>(this);
        if (m_graphViewerPart) {
            m_graphViewer = qobject_cast< KGraphViewer::KGraphViewerInterface* >(m_graphViewerPart);
            ui.dotGraphTab->layout()->addWidget(m_graphViewerPart->widget());
            connect(m_graphViewerPart, SIGNAL(graphLoaded()), this, SLOT(slotGraphLoaded()));
            haveGraphViewer = true;
        }
    }
#endif
    //END KGraphViewer

    connect(ui.filterDataTree, SIGNAL(textChanged(QString)),
            m_dataTreeFilterModel, SLOT(setFilter(QString)));
    ui.dataTreeView->setModel(m_dataTreeFilterModel);
    connect(ui.dataTreeView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(treeSelectionChanged(QModelIndex,QModelIndex)));


    //BEGIN custom allocators
    tabifyDockWidget(ui.allocatorDock, ui.dataTreeDock);
    ui.allocatorView->setModel(m_allocatorModel);

    int iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize);
    ui.dockMenuBar->setIconSize(QSize(iconSize, iconSize));
    ui.dockMenuBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.dockMenuBar->setFloatable(false);
    ui.dockMenuBar->setMovable(false);

    KConfigGroup cfg = allocatorConfig();
    m_allocatorModel->setStringList(cfg.entryMap().values());

    connect(m_allocatorModel, SIGNAL(modelReset()),
            this, SLOT(allocatorsChanged()));

    connect(m_allocatorModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(allocatorsChanged()));

    connect(ui.dataTreeView, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(dataTreeContextMenuRequested(QPoint)));
    ui.dataTreeView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui.allocatorView, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(allocatorViewContextMenuRequested(QPoint)));
    ui.allocatorView->setContextMenuPolicy(Qt::CustomContextMenu);
    //END custom allocators

    setupActions();
    setupGUI(StandardWindowOptions(Default ^ StatusBar));
    statusBar()->hide();

    if (haveGraphViewer) {
#ifdef HAVE_KGRAPHVIEWER
        connect(ui.tabWidget, SIGNAL(currentChanged(int)),
                this, SLOT(slotTabChanged(int)));
        slotTabChanged(ui.tabWidget->currentIndex());
#endif
    } else {
        // cleanup UI when we installed with kgraphviewer but it's not available at runtime
        KToolBar* callgraphToolbar = toolBar("callgraphToolBar");
        removeToolBar(callgraphToolbar);
        delete callgraphToolbar;
        int i = ui.stackedWidget->addWidget(ui.plotterTab);
        ui.stackedWidget->setCurrentIndex(i);
        ui.stackedWidget->removeWidget(ui.displayPage);
        delete ui.displayPage;
        ui.displayPage = ui.plotterTab;
    }

    // open page
    ui.stackedWidget->setCurrentWidget(ui.openPage);
}

MainWindow::~MainWindow()
{
    closeFile();
    m_recentFiles->saveEntries(KGlobal::config()->group( QString() ));
}

void MainWindow::setupActions()
{
    KAction* openFile = KStandardAction::open(this, SLOT(openFile()), actionCollection());
    KAction* reload = KStandardAction::redisplay(this, SLOT(reload()), actionCollection());
    actionCollection()->addAction("file_reload", reload);
    m_recentFiles = KStandardAction::openRecent(this, SLOT(openFile(KUrl)), actionCollection());
    m_recentFiles->loadEntries(KGlobal::config()->group( QString() ));

    m_close = KStandardAction::close(this, SLOT(closeFile()), actionCollection());
    m_close->setEnabled(false);

    m_stopParser = new KAction(i18n("Stop Parser"), actionCollection());
    m_stopParser->setIcon(KIcon("process-stop"));
    connect(m_stopParser, SIGNAL(triggered(bool)),
            this, SLOT(stopParser()));
    m_stopParser->setEnabled(false);
    actionCollection()->addAction("file_stopparser", m_stopParser);

    KStandardAction::quit(qApp, SLOT(closeAllWindows()), actionCollection());

    KStandardAction::preferences(this, SLOT(preferences()), actionCollection());

    m_shortenTemplates = new KAction(KIcon("shortentemplates"), i18n("Shorten Templates"), actionCollection());
    m_shortenTemplates->setCheckable(true);
    m_shortenTemplates->setChecked(Settings::shortenTemplates());
    connect(m_shortenTemplates, SIGNAL(toggled(bool)), SLOT(slotShortenTemplates(bool)));
    actionCollection()->addAction("shorten_templates", m_shortenTemplates);

    m_toggleTotal = new KAction(KIcon("office-chart-area"), i18n("Toggle total cost graph"), actionCollection());
    m_toggleTotal->setCheckable(true);
    m_toggleTotal->setChecked(true);
    m_toggleTotal->setEnabled(false);
    connect(m_toggleTotal, SIGNAL(toggled(bool)), SLOT(showTotalGraph(bool)));
    actionCollection()->addAction("toggle_total", m_toggleTotal);

    m_toggleDetailed = new KAction(KIcon("office-chart-area-stacked"), i18n("Toggle detailed cost graph"), actionCollection());
    m_toggleDetailed->setCheckable(true);
    m_toggleDetailed->setChecked(true);
    m_toggleDetailed->setEnabled(false);
    connect(m_toggleDetailed, SIGNAL(toggled(bool)), SLOT(showDetailedGraph(bool)));
    actionCollection()->addAction("toggle_detailed", m_toggleDetailed);

#ifdef HAVE_KGRAPHVIEWER
    if (m_graphViewer) {
        m_zoomIn = KStandardAction::zoomIn(this, SLOT(zoomIn()), actionCollection());
        actionCollection()->addAction("zoomIn", m_zoomIn);
        m_zoomOut = KStandardAction::zoomOut(this, SLOT(zoomOut()), actionCollection());
        actionCollection()->addAction("zoomOut", m_zoomOut);
        m_focusExpensive = new KAction(KIcon("flag-red"), i18n("Focus most expensive node"), actionCollection());
        m_toggleDetailed->setEnabled(false);
        connect(m_focusExpensive, SIGNAL(triggered()), this, SLOT(focusExpensiveGraphNode()));
        actionCollection()->addAction("focusExpensive", m_focusExpensive);
    }
#endif

    m_selectPeak = new KAction(KIcon("flag-red"), i18n("Select peak snapshot"), ui.dataTreeDock);
    m_selectPeak->setEnabled(false);
    connect(m_selectPeak, SIGNAL(triggered()), this, SLOT(selectPeakSnapshot()));
    actionCollection()->addAction("selectPeak", m_selectPeak);

    KAction* stackNumAction = actionCollection()->addAction("stackNum");
    stackNumAction->setText(i18n("Stacked diagrams"));
    QWidget *stackNumWidget = new QWidget;
    QHBoxLayout* stackNumLayout = new QHBoxLayout;
    stackNumLayout->addWidget(new QLabel(i18n("Stacked diagrams:")));
    QSpinBox* box = new QSpinBox;
    box->setMinimum(0);
    box->setMaximum(50);
    box->setValue(10);
    connect(box, SIGNAL(valueChanged(int)), this, SLOT(setStackNum(int)));
    stackNumLayout->addWidget(box);
    stackNumWidget->setLayout(stackNumLayout);
    stackNumAction->setDefaultWidget(stackNumWidget);

    //BEGIN custom allocators
    m_newAllocator = new KAction(KIcon("list-add"), i18n("add"), ui.allocatorDock);
    m_newAllocator->setToolTip(i18n("add custom allocator"));
    connect(m_newAllocator, SIGNAL(triggered()), this, SLOT(slotNewAllocator()));
    ui.dockMenuBar->addAction(m_newAllocator);
    m_removeAllocator = new KAction(KIcon("list-remove"), i18n("remove"),
                                    ui.allocatorDock);
    m_newAllocator->setToolTip(i18n("remove selected allocator"));
    connect(m_removeAllocator, SIGNAL(triggered()), this, SLOT(slotRemoveAllocator()));
    connect(ui.allocatorView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(allocatorSelectionChanged()));
    m_removeAllocator->setEnabled(false);
    ui.dockMenuBar->addAction(m_removeAllocator);

    m_markCustomAllocator = new KAction(i18n("mark as custom allocator"), ui.allocatorDock);
    connect(m_markCustomAllocator, SIGNAL(triggered()),
            this, SLOT(slotMarkCustomAllocator()), Qt::QueuedConnection);
    //END custom allocators

    //BEGIN hiding functions
    m_hideFunction = new KAction(i18n("hide function"), this);
    connect(m_hideFunction, SIGNAL(triggered()),
            this, SLOT(slotHideFunction()));
    m_hideOtherFunctions = new KAction(i18n("hide other functions"), this);
    connect(m_hideOtherFunctions, SIGNAL(triggered()),
            this, SLOT(slotHideOtherFunctions()));
    //END hiding functions

    //dock actions
    actionCollection()->addAction("toggleDataTree", ui.dataTreeDock->toggleViewAction());
    actionCollection()->addAction("toggleAllocators", ui.allocatorDock->toggleViewAction());

    //open page actions
    ui.openFile->setDefaultAction(openFile);
    ui.openFile->setText(i18n("Open Massif Data File"));
    ui.openFile->setIconSize(QSize(48, 48));

    //load page actions
    ui.stopParsing->setDefaultAction(m_stopParser);
    ui.stopParsing->setIconSize(QSize(48, 48));
}

void MainWindow::preferences()
{
    if (ConfigDialog::isShown()) {
        return;
    }

    ConfigDialog* dlg = new ConfigDialog(this);
    connect(dlg, SIGNAL(settingsChanged(QString)),
            this, SLOT(settingsChanged()));
    dlg->show();
}

void MainWindow::settingsChanged()
{
    if (Settings::self()->shortenTemplates() != m_shortenTemplates->isChecked()) {
        m_shortenTemplates->setChecked(Settings::self()->shortenTemplates());
    }

    Settings::self()->writeConfig();
    updateHeader();
    updatePeaks();
    ui.dataTreeView->viewport()->update();
}

void MainWindow::openFile()
{
    QString file = KFileDialog::getOpenFileName(KUrl("kfiledialog:///massif-visualizer"),
                                                QString("application/x-valgrind-massif"),
                                                this, i18n("Open Massif Output File"));
    if (!file.isEmpty()) {
        openFile(KUrl(file));
    }
}

void MainWindow::reload()
{
    if (m_currentFile.isValid()) {
        // copy to prevent madness
        openFile(KUrl(m_currentFile));
    }
}

void MainWindow::stopParser()
{
    if (m_parseThread) {
        m_parseThread->stop();
        disconnect(m_parseThread, 0, this, 0);
        m_parseThread = 0;
    }

    m_stopParser->setEnabled(false);
    ui.stackedWidget->setCurrentWidget(ui.openPage);
}

void MainWindow::openFile(const KUrl& file)
{
    Q_ASSERT(file.isValid());
    if (m_data) {
        closeFile();
    }

    stopParser();

    m_stopParser->setEnabled(true);
    ui.stackedWidget->setCurrentWidget(ui.loadingPage);
    ui.loadingProgress->setRange(0, 0);
    ui.loadingMessage->setText(i18n("loading file <i>%1</i>...", file.pathOrUrl()));

    m_parseThread = new ParseThread(this);
    connect(m_parseThread, SIGNAL(finished(ParseThread*, FileData*)),
            this, SLOT(parserFinished(ParseThread*, FileData*)));
    connect(m_parseThread, SIGNAL(finished()),
            m_parseThread, SLOT(deleteLater()));
    connect(m_parseThread, SIGNAL(progressRange(int, int)),
            ui.loadingProgress, SLOT(setRange(int,int)));
    connect(m_parseThread, SIGNAL(progress(int)),
            ui.loadingProgress, SLOT(setValue(int)));

    m_parseThread->startParsing(file, m_allocatorModel->stringList());
}

void MainWindow::parserFinished(ParseThread* thread, FileData* data)
{
    // give the progress bar one last chance to update
    QApplication::processEvents();

    Q_ASSERT(thread == m_parseThread);

    m_parseThread = 0;
    QScopedPointer<ParseThread> threadHouseholder(thread);

    if (!data) {
        thread->showError(this);
        return;
    }

    m_data = data;
    m_currentFile = thread->file();

    Q_ASSERT(m_data->peak());

    m_close->setEnabled(true);

    kDebug() << "loaded massif file:" << m_currentFile;
    qDebug() << "description:" << m_data->description();
    qDebug() << "command:" << m_data->cmd();
    qDebug() << "time unit:" << m_data->timeUnit();
    qDebug() << "snapshots:" << m_data->snapshots().size();
    qDebug() << "peak: snapshot #" << m_data->peak()->number() << "after" << QString("%1%2").arg(m_data->peak()->time()).arg(m_data->timeUnit());
    qDebug() << "peak cost:" << prettyCost(m_data->peak()->memHeap()) << " heap"
                             << prettyCost(m_data->peak()->memHeapExtra()) << " heap extra"
                             << prettyCost(m_data->peak()->memStacks()) << " stacks";

    //BEGIN DotGraph
#ifdef HAVE_KGRAPHVIEWER
    if (m_graphViewer) {
        getDotGraph(QPair<TreeLeafItem*,SnapshotItem*>(0, m_data->peak()));
        m_zoomIn->setEnabled(true);
        m_zoomOut->setEnabled(true);
        m_focusExpensive->setEnabled(true);
    }
#endif

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
        m_header->setAlignment(Qt::AlignCenter);
        updateHeader();
    }

    setWindowTitle(i18n("Massif Visualizer - evaluation of %1 (%2)", m_data->cmd(), m_currentFile.fileName()));

    //BEGIN TotalDiagram
    m_totalDiagram = new Plotter;
    m_toggleTotal->setEnabled(true);
    m_totalDiagram->setAntiAliasing(true);

    CartesianAxis* bottomAxis = new CartesianAxis(m_totalDiagram);
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

    CartesianAxis* leftAxis = new CartesianAxis(m_totalDiagram);
    leftAxis->setTextAttributes(axisTextAttributes);
    leftAxis->setTitleTextAttributes(axisTitleTextAttributes);
    leftAxis->setTitleText(i18n("memory heap size in bytes"));
    leftAxis->setPosition ( CartesianAxis::Left );
    m_totalDiagram->addAxis(leftAxis);

    m_totalCostModel->setSource(m_data);
    m_totalDiagram->setModel(m_totalCostModel);

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

    CartesianAxis* topAxis = new CartesianAxis(m_detailedDiagram);
    topAxis->setTextAttributes(axisTextAttributes);
    topAxis->setTitleTextAttributes(axisTitleTextAttributes);
    topAxis->setTitleText(i18n("time in %1", m_data->timeUnit()));
    topAxis->setPosition ( CartesianAxis::Top );
    m_detailedDiagram->addAxis(topAxis);

    CartesianAxis* rightAxis = new CartesianAxis(m_detailedDiagram);
    rightAxis->setTextAttributes(axisTextAttributes);
    rightAxis->setTitleTextAttributes(axisTitleTextAttributes);
    rightAxis->setTitleText(i18n("memory heap size in bytes"));
    rightAxis->setPosition ( CartesianAxis::Right );
    m_detailedDiagram->addAxis(rightAxis);

    updatePeaks();

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
    m_recentFiles->addUrl(m_currentFile);

    ui.stackedWidget->setCurrentWidget(ui.displayPage);
}

void MainWindow::updateHeader()
{
    const QString app = m_data->cmd().split(' ', QString::SkipEmptyParts).first();

    m_header->setText(QString("<b>%1</b><br /><i>%2</i>")
                        .arg(i18n("Memory consumption of %1", app))
                        .arg(i18n("Peak of %1 at snapshot #%2", prettyCost(m_data->peak()->memHeap()), m_data->peak()->number()))
    );
    m_header->setToolTip(i18n("Command: %1\nValgrind Options: %2", m_data->cmd(), m_data->description()));
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
#ifdef HAVE_KGRAPHVIEWER
    if (m_graphViewer) {
        getDotGraph(item);
    }
#endif

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
    ui.dataTreeView->selectionModel()->clearSelection();
    QPair< TreeLeafItem*, SnapshotItem* > item = m_detailedCostModel->itemForIndex(_idx);
    const QModelIndex& newIndex = m_dataTreeFilterModel->mapFromSource(
        m_dataTreeModel->indexForItem(item)
    );
    ui.dataTreeView->selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    ui.dataTreeView->scrollTo(ui.dataTreeView->selectionModel()->currentIndex());

    m_chart->update();
#ifdef HAVE_KGRAPHVIEWER
    if (m_graphViewer) {
        getDotGraph(item);
    }
#endif

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

    ui.dataTreeView->selectionModel()->clearSelection();
    const QModelIndex& newIndex = m_dataTreeFilterModel->mapFromSource(
        m_dataTreeModel->indexForItem(item)
    );
    ui.dataTreeView->selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    ui.dataTreeView->scrollTo(ui.dataTreeView->selectionModel()->currentIndex());

    m_chart->update();

    m_changingSelections = false;
}

void MainWindow::closeFile()
{
    if (!m_data) {
        return;
    }

    ui.stackedWidget->setCurrentWidget(ui.openPage);

#ifdef HAVE_KGRAPHVIEWER
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
    if (m_graphViewer) {
        m_graphViewerPart->closeUrl();
        m_zoomIn->setEnabled(false);
        m_zoomOut->setEnabled(false);
        m_focusExpensive->setEnabled(false);
    }
    m_lastDotItem.first = 0;
    m_lastDotItem.second = 0;
#endif

    m_close->setEnabled(false);

    kDebug() << "closing file";

    m_chart->replaceCoordinatePlane(new CartesianCoordinatePlane);
    m_legend->removeDiagrams();
    m_legend->hide();
    m_header->setText("");

    m_toggleDetailed->setEnabled(false);
    m_toggleDetailed->setChecked(true);
    foreach(CartesianAxis* axis, m_detailedDiagram->axes()) {
        m_detailedDiagram->takeAxis(axis);
        delete axis;
    }
    delete m_detailedDiagram;
    m_detailedDiagram = 0;

    m_toggleTotal->setEnabled(false);
    m_toggleTotal->setChecked(true);
    foreach(CartesianAxis* axis, m_totalDiagram->axes()) {
        m_totalDiagram->takeAxis(axis);
        delete axis;
    }
    delete m_totalDiagram;
    m_totalDiagram = 0;

    m_dataTreeModel->setSource(0);
    m_dataTreeFilterModel->setFilter("");
    m_detailedCostModel->setSource(0);
    m_totalCostModel->setSource(0);

    m_selectPeak->setEnabled(false);

    delete m_data;
    m_data = 0;
    m_currentFile.clear();

    setWindowTitle(i18n("Massif Visualizer"));
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

#ifdef HAVE_KGRAPHVIEWER
void MainWindow::slotTabChanged(int index)
{
    toolBar("chartToolBar")->setVisible(index == 0);
    foreach(QAction* action, toolBar("chartToolBar")->actions()) {
        action->setEnabled(m_data && index == 0);
    }
    toolBar("callgraphToolBar")->setVisible(index == 1);
    foreach(QAction* action, toolBar("callgraphToolBar")->actions()) {
        action->setEnabled(m_data && index == 1);
    }
    if (index == 1) {
        // if we parsed a dot graph we might want to show it now
        showDotGraph();
    }
}

void MainWindow::getDotGraph(QPair<TreeLeafItem*, SnapshotItem*> item)
{
    if (item == m_lastDotItem) {
        return;
    }
    m_lastDotItem = item;

    Q_ASSERT(m_graphViewer);

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
    Q_ASSERT(m_graphViewer);

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
    Q_ASSERT(m_graphViewer);

    m_graphViewer->zoomIn();
}

void MainWindow::zoomOut()
{
    Q_ASSERT(m_graphViewer);

    m_graphViewer->zoomOut();
}

void MainWindow::focusExpensiveGraphNode()
{
    Q_ASSERT(m_graphViewer);

    m_graphViewer->centerOnNode(m_dotGenerator->mostCostIntensiveGraphvizId());
}

#endif

void MainWindow::selectPeakSnapshot()
{
    Q_ASSERT(m_data);

    ui.dataTreeView->selectionModel()->clearSelection();
    ui.dataTreeView->selectionModel()->setCurrentIndex(
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

void MainWindow::updatePeaks()
{
    KColorScheme scheme(QPalette::Active, KColorScheme::Window);
    QPen foreground(scheme.foreground().color());

    if (m_data->peak()) {
        const QModelIndex peak = m_totalCostModel->peak();
        Q_ASSERT(peak.isValid());
        markPeak(m_totalDiagram, peak, m_data->peak()->memHeap(), foreground);
    }
    updateDetailedPeaks();
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

void MainWindow::allocatorsChanged()
{
    KConfigGroup cfg = allocatorConfig();
    cfg.deleteGroup();
    int i = 0;
    foreach(const QString& allocator, m_allocatorModel->stringList()) {
        if (allocator.isEmpty()) {
            m_allocatorModel->removeRow(i);
            continue;
        }
        cfg.writeEntry(QString::number(i++), allocator);
    }
    cfg.sync();

    if (m_data) {
        reload();
    }
}

void MainWindow::allocatorSelectionChanged()
{
    m_removeAllocator->setEnabled(ui.allocatorView->selectionModel()->hasSelection());
}

void MainWindow::slotNewAllocator()
{
    QString allocator = QInputDialog::getText(this, i18n("Add Custom Allocator"), i18n("allocator:"));
    if (allocator.isEmpty()) {
        return;
    }
    if (!m_allocatorModel->stringList().contains(allocator)) {
        m_allocatorModel->setStringList(m_allocatorModel->stringList() << allocator);
    }
}

void MainWindow::slotRemoveAllocator()
{
    Q_ASSERT(ui.allocatorView->selectionModel()->hasSelection());
    foreach(const QModelIndex& idx, ui.allocatorView->selectionModel()->selectedRows()) {
        m_allocatorModel->removeRow(idx.row(), idx.parent());
    }
    allocatorsChanged();
}

void MainWindow::slotMarkCustomAllocator()
{
    const QString allocator = m_markCustomAllocator->data().toString();
    Q_ASSERT(!allocator.isEmpty());
    if (!m_allocatorModel->stringList().contains(allocator)) {
        m_allocatorModel->setStringList(m_allocatorModel->stringList() << allocator);
    }
}

void MainWindow::allocatorViewContextMenuRequested(const QPoint& pos)
{
    const QModelIndex idx = ui.allocatorView->indexAt(pos);

    QMenu menu;
    menu.addAction(m_newAllocator);
    if (idx.isValid()) {
        menu.addAction(m_removeAllocator);
    }
    menu.exec(ui.allocatorView->mapToGlobal(pos));
}

void MainWindow::dataTreeContextMenuRequested(const QPoint& pos)
{
    const QModelIndex idx = m_dataTreeFilterModel->mapToSource(
                                ui.dataTreeView->indexAt(pos));
    if (!idx.isValid()) {
        return;
    }

    if (!idx.parent().isValid()) {
        // snapshot item
        return;
    }

    QMenu menu;
    TreeLeafItem* item = m_dataTreeModel->itemForIndex(idx).first;
    Q_ASSERT(item);
    prepareActions(&menu, item);
    menu.exec(ui.dataTreeView->mapToGlobal(pos));
}

void MainWindow::chartContextMenuRequested(const QPoint& pos)
{
    const QPoint dPos = m_detailedDiagram->mapFromGlobal(m_chart->mapToGlobal(pos));

    const QModelIndex idx = m_detailedDiagram->indexAt(dPos);
    if (!idx.isValid()) {
        return;
    }
    // hack: the ToolTip will only be queried by KDChart and that one uses the
    // left index, but we want it to query the right one
    const QModelIndex _idx = m_detailedCostModel->index(idx.row() + 1, idx.column(), idx.parent());
    QPair< TreeLeafItem*, SnapshotItem* > item = m_detailedCostModel->itemForIndex(_idx);

    if (!item.first) {
        return;
    }

    QMenu menu;
    menu.addAction(m_markCustomAllocator);
    prepareActions(&menu, item.first);
    menu.exec(m_detailedDiagram->mapToGlobal(dPos));
}

void MainWindow::prepareActions(QMenu* menu, TreeLeafItem* item)
{
    QString func = functionInLabel(item->label());
    if (func.length() > 40) {
        func.resize(40);
        func.append("...");
    }
    menu->setTitle(func);

    m_markCustomAllocator->setData(item->label());
    menu->addAction(m_markCustomAllocator);

    m_hideFunction->setData(QVariant::fromValue(item));
    menu->addAction(m_hideFunction);

    m_hideOtherFunctions->setData(QVariant::fromValue(item));
    menu->addAction(m_hideOtherFunctions);
}

void MainWindow::slotHideFunction()
{
    m_detailedCostModel->hideFunction(m_hideFunction->data().value<TreeLeafItem*>());
}

void MainWindow::slotHideOtherFunctions()
{
    m_detailedCostModel->hideOtherFunctions(m_hideOtherFunctions->data().value<TreeLeafItem*>());
}

void MainWindow::slotShortenTemplates(bool shorten)
{
    if (shorten == Settings::self()->shortenTemplates()) {
        return;
    }

    Settings::self()->setShortenTemplates(shorten);
    settingsChanged();
}

#include "mainwindow.moc"
