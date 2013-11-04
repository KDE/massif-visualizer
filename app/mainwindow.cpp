/*
   This file is part of Massif Visualizer

   Copyright 2010 Milian Wolff <mail@milianw.de>
   Copyright 2013 Arnold Dumas <contact@arnolddumas.com>

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
#include "massifdata/parseworker.h"
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
#include <QPrinter>
#include <QPrintDialog>

#include <KMessageBox>

#ifdef HAVE_KGRAPHVIEWER
#include <kgraphviewer_interface.h>
#endif

using namespace Massif;
using namespace KDChart;

// Helper function
KConfigGroup allocatorConfig()
{
    return KGlobal::config()->group("Allocators");
}

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags f)
    : KParts::MainWindow(parent, f)
    , m_toggleTotal(0)
    , m_selectPeak(0)
    , m_recentFiles(0)
    , m_zoomIn(0)
    , m_zoomOut(0)
    , m_focusExpensive(0)
    , m_close(0)
    , m_print(0)
    , m_stopParser(0)
    , m_allocatorModel(new QStringListModel(this))
    , m_newAllocator(0)
    , m_removeAllocator(0)
    , m_shortenTemplates(0)
{
    ui.setupUi(this);

    setWindowTitle(i18n("Massif Visualizer"));

    //BEGIN KGraphViewer
    bool haveGraphViewer = false;

    // NOTE: just check if kgraphviewer is available at runtime.
    // The former logic has been moved to DocumentWidget constructor.
#ifdef HAVE_KGRAPHVIEWER
    KLibFactory *factory = KLibLoader::self()->factory("kgraphviewerpart");
    if (factory) {
        if (factory->create<KParts::ReadOnlyPart>(this)) {
            haveGraphViewer = true;
        }
    }
#endif

    if (!haveGraphViewer) {
        // cleanup UI when we installed with kgraphviewer but it's not available at runtime
        KToolBar* callgraphToolbar = toolBar("callgraphToolBar");
        removeToolBar(callgraphToolbar);
        delete callgraphToolbar;
    }
    //END KGraphViewer

    connect(ui.documents, SIGNAL(currentChanged(int)),
            this, SLOT(documentChanged()));

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

    // open page
    ui.stackedWidget->setCurrentWidget(ui.openPage);
}

MainWindow::~MainWindow()
{
    while (ui.documents->count()) {
        closeCurrentFile();
    }

    m_recentFiles->saveEntries(KGlobal::config()->group( QString() ));
}

void MainWindow::setupActions()
{
    KAction* openFile = KStandardAction::open(this, SLOT(openFile()), actionCollection());
    KAction* reload = KStandardAction::redisplay(this, SLOT(reloadCurrentFile()), actionCollection());
    actionCollection()->addAction("file_reload", reload);
    m_recentFiles = KStandardAction::openRecent(this, SLOT(openFile(KUrl)), actionCollection());
    m_recentFiles->loadEntries(KGlobal::config()->group( QString() ));

    m_close = KStandardAction::close(this, SLOT(closeCurrentFile()), actionCollection());
    m_close->setEnabled(false);

    m_print = KStandardAction::print(this, SLOT(showPrintPreviewDialog()), actionCollection());
    actionCollection()->addAction("file_print", m_print);
    m_print->setEnabled(false);

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

    // If the toolbar is still there, kgraphviewer is available at runtime.
#ifdef HAVE_KGRAPHVIEWER
    if (!toolBar("callgraphToolBar")) {
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

    if (ui.documents->count()) {
        currentDocument()->updateHeader();
        currentDocument()->updatePeaks();
    }
    ui.dataTreeView->viewport()->update();
}

void MainWindow::openFile()
{
    QStringList files = KFileDialog::getOpenFileNames(KUrl("kfiledialog:///massif-visualizer"),
                                                QString("application/x-valgrind-massif"),
                                                this, i18n("Open Massif Output File"));

    foreach (KUrl file, files) {
        openFile(file);
    }
}

void MainWindow::reloadCurrentFile()
{
    if (currentDocument()->file().isValid()) {
        openFile(KUrl(currentDocument()->file()));
    }
}

void MainWindow::stopParser()
{
    Q_ASSERT(currentDocument());
    ParseWorker* parseWorker = m_documentsParseWorkers.take(currentDocument());

    parseWorker->stop();
    parseWorker->deleteLater();
    parseWorker->thread()->quit();
    parseWorker->thread()->wait();

    m_stopParser->setEnabled(false);
}

void MainWindow::openFile(const KUrl& file)
{
    Q_ASSERT(file.isValid());

    // give the progress bar one last chance to update
    QApplication::processEvents();

    int indexToInsert = -1, i = 0;

    // Is file already opened ?
    while (indexToInsert == -1 && i < ui.documents->count()) {
        if (qobject_cast<DocumentWidget*>(ui.documents->widget(i))->file() == file) {
            indexToInsert = i;
        }

        ++i;
    }

    DocumentWidget* documentWidget = new DocumentWidget;
    documentWidget->setLoadingMessage(i18n("loading file <i>%1</i>...", file.pathOrUrl()));

    // Create dedicated thread for this document.
    ParseWorker* parseWorker = new ParseWorker;
    QThread* thread = new QThread(this);
    thread->start();
    parseWorker->moveToThread(thread);
    // Register thread in the hash map.
    m_documentsParseWorkers.insert(documentWidget, parseWorker);

    if (indexToInsert != -1) {
        // Remove existing instance of the file.
        ui.documents->setCurrentIndex(indexToInsert);
        closeCurrentFile();
        // Insert the new tab at the correct position.
        ui.documents->insertTab(indexToInsert, documentWidget, file.fileName());
    } else {
        ui.documents->addTab(documentWidget, file.fileName());
    }

    connect(parseWorker, SIGNAL(finished(KUrl, Massif::FileData*)),
            documentWidget, SLOT(parserFinished(KUrl, Massif::FileData*)));
    connect(parseWorker, SIGNAL(error(QString, QString)),
            this, SLOT(parserError(QString, QString)));
    connect(parseWorker, SIGNAL(progressRange(int, int)),
            documentWidget, SLOT(setRange(int,int)));
    connect(parseWorker, SIGNAL(progress(int)),
            documentWidget, SLOT(setProgress(int)));
    connect(documentWidget, SIGNAL(loadingFinished()),
            this, SLOT(parserFinished()));

    parseWorker->parse(file, m_allocatorModel->stringList());

    m_stopParser->setEnabled(true);
    m_close->setEnabled(true);
    m_recentFiles->addUrl(file);
    ui.stackedWidget->setCurrentWidget(ui.displayPage);
}

void MainWindow::parserFinished()
{
    DocumentWidget* documentWidget = qobject_cast<DocumentWidget*>(sender());
    m_changingSelections.insert(documentWidget, false);

#ifdef HAVE_KGRAPHVIEWER
    if (documentWidget->graphViewer()) {
        m_zoomIn->setEnabled(true);
        m_zoomOut->setEnabled(true);
        m_focusExpensive->setEnabled(true);
    }
#endif

    ui.dataTreeView->setModel(documentWidget->dataTreeFilterModel());

    connect(documentWidget->totalDiagram(), SIGNAL(clicked(QModelIndex)),
            this, SLOT(totalItemClicked(QModelIndex)));
    connect(documentWidget->detailedDiagram(), SIGNAL(clicked(QModelIndex)),
            this, SLOT(detailedItemClicked(QModelIndex)));
    connect(documentWidget->chart(), SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(chartContextMenuRequested(QPoint)));
    connect(ui.filterDataTree, SIGNAL(textChanged(QString)),
            documentWidget->dataTreeFilterModel(), SLOT(setFilter(QString)));
    connect(ui.dataTreeView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(treeSelectionChanged(QModelIndex,QModelIndex)));

    m_print->setEnabled(true);
    m_toggleDetailed->setEnabled(true);
    m_toggleTotal->setEnabled(true);
    m_selectPeak->setEnabled(true);
    actionCollection()->action("file_reload")->setEnabled(true);

    // Update the window title once the document is fully loaded.
    updateWindowTitle();
}

void MainWindow::parserError(const QString& title, const QString& error)
{
    KMessageBox::error(this, error, title);
}

void MainWindow::treeSelectionChanged(const QModelIndex& now, const QModelIndex& before)
{
    if (currentChangingSelections() || !ui.documents->count()) {
        return;
    }

    if (now == before) {
        return;
    }
    setCurrentChangingSelections(true);

    const QPair< TreeLeafItem*, SnapshotItem* >& item = currentDocument()->dataTreeModel()->itemForIndex(
       currentDocument()->dataTreeFilterModel()->mapToSource(now)
    );

    if (now.parent().isValid()) {
        currentDocument()->detailedCostModel()->setSelection(currentDocument()->detailedCostModel()->indexForItem(item));
        currentDocument()->totalCostModel()->setSelection(QModelIndex());
    } else {
        currentDocument()->totalCostModel()->setSelection(currentDocument()->totalCostModel()->indexForItem(item));
        currentDocument()->detailedCostModel()->setSelection(QModelIndex());
    }

    currentDocument()->chart()->update();

#ifdef HAVE_KGRAPHVIEWER
    if (currentDocument()->graphViewer()) {
        currendDocument()->showDotGraph(item);
    }
#endif

    setCurrentChangingSelections(false);
}

void MainWindow::detailedItemClicked(const QModelIndex& idx)
{
    if (currentChangingSelections() || !ui.documents->count()) {
        return;
    }

    setCurrentChangingSelections(true);

    currentDocument()->detailedCostModel()->setSelection(idx);
    currentDocument()->totalCostModel()->setSelection(QModelIndex());

    // hack: the ToolTip will only be queried by KDChart and that one uses the
    // left index, but we want it to query the right one
    const QModelIndex _idx = currentDocument()->detailedCostModel()->index(idx.row() + 1, idx.column(), idx.parent());
    ui.dataTreeView->selectionModel()->clearSelection();
    QPair< TreeLeafItem*, SnapshotItem* > item = currentDocument()->detailedCostModel()->itemForIndex(_idx);
    const QModelIndex& newIndex = currentDocument()->dataTreeFilterModel()->mapFromSource(
        currentDocument()->dataTreeModel()->indexForItem(item)
    );
    ui.dataTreeView->selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    ui.dataTreeView->scrollTo(ui.dataTreeView->selectionModel()->currentIndex());

    currentDocument()->chart()->update();

#ifdef HAVE_KGRAPHVIEWER
    if (currentDocument()->graphViewer()) {
        currentDocument()->showDotGraph(item);
    }
#endif

    setCurrentChangingSelections(false);
}

void MainWindow::totalItemClicked(const QModelIndex& idx_)
{
    if (currentChangingSelections() || !ui.documents->count()) {
        return;
    }

    setCurrentChangingSelections(true);

    QModelIndex idx = idx_.model()->index(idx_.row() + 1, idx_.column(), idx_.parent());

    currentDocument()->detailedCostModel()->setSelection(QModelIndex());
    currentDocument()->totalCostModel()->setSelection(idx);

    QPair< TreeLeafItem*, SnapshotItem* > item = currentDocument()->totalCostModel()->itemForIndex(idx);

    ui.dataTreeView->selectionModel()->clearSelection();
    const QModelIndex& newIndex = currentDocument()->dataTreeFilterModel()->mapFromSource(
        currentDocument()->dataTreeModel()->indexForItem(item)
    );
    ui.dataTreeView->selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    ui.dataTreeView->scrollTo(ui.dataTreeView->selectionModel()->currentIndex());

    currentDocument()->chart()->update();

    setCurrentChangingSelections(false);
}

void MainWindow::closeCurrentFile()
{
    stopParser();

    // Tab is now removed
    int tabToCloseIndex = ui.documents->currentIndex();
    m_changingSelections.remove(currentDocument());
    m_documentsParseWorkers.remove(currentDocument());
    ui.documents->widget(tabToCloseIndex)->deleteLater();
    ui.documents->removeTab(tabToCloseIndex);

    if (!ui.documents->count()) {
        actionCollection()->action("file_reload")->setEnabled(false);
        m_close->setEnabled(false);
        m_print->setEnabled(false);
        m_toggleDetailed->setEnabled(false);
        m_toggleTotal->setEnabled(false);
        m_selectPeak->setEnabled(false);
        ui.stackedWidget->setCurrentWidget(ui.openPage);
        ui.stackedWidget->setCurrentWidget(ui.openPage);
    } else {
        // Display the remaining(s) file(s).
        ui.stackedWidget->setCurrentWidget(ui.displayPage);
    }
}

void MainWindow::showDetailedGraph(bool show)
{
    if (ui.documents->count()) {
        currentDocument()->detailedDiagram()->setHidden(!show);
        m_toggleDetailed->setChecked(show);
        currentDocument()->chart()->update();
    }
}

void MainWindow::showTotalGraph(bool show)
{
    if (ui.documents->count()) {
        currentDocument()->totalDiagram()->setHidden(!show);
        m_toggleTotal->setChecked(show);
        currentDocument()->chart()->update();
    }
}

#ifdef HAVE_KGRAPHVIEWER

void MainWindow::zoomIn()
{
    Q_ASSERT(currentDocument()->graphViewer());

    currentDocument()->graphViewer()->zoomIn();
}

void MainWindow::zoomOut()
{
    Q_ASSERT(currentDocument()->graphViewer());

    currentDocument()->graphViewer()->zoomOut();
}

void MainWindow::focusExpensiveGraphNode()
{
    Q_ASSERT(currentDocument()->graphViewer());

    currentDocument()->graphViewer()->centerOnNode(m_dotGenerator->mostCostIntensiveGraphvizId());
}

#endif

void MainWindow::selectPeakSnapshot()
{
    Q_ASSERT(currentDocument());

    ui.dataTreeView->selectionModel()->clearSelection();
    ui.dataTreeView->selectionModel()->setCurrentIndex(
        currentDocument()->dataTreeFilterModel()->mapFromSource(
            currentDocument()->dataTreeModel()->indexForSnapshot(currentDocument()->data()->peak())),
            QItemSelectionModel::Select | QItemSelectionModel::Rows
    );
}

void MainWindow::setStackNum(int num)
{
    if (ui.documents->count()) {
        currentDocument()->detailedCostModel()->setMaximumDatasetCount(num);
        currentDocument()->updatePeaks();
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

    if (ui.documents->count()) {
        reloadCurrentFile();
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
    const QModelIndex idx = currentDocument()->dataTreeFilterModel()->mapToSource(
                                ui.dataTreeView->indexAt(pos));
    if (!idx.isValid()) {
        return;
    }

    if (!idx.parent().isValid()) {
        // snapshot item
        return;
    }

    QMenu menu;
    TreeLeafItem* item = currentDocument()->dataTreeModel()->itemForIndex(idx).first;
    Q_ASSERT(item);
    prepareActions(&menu, item);
    menu.exec(ui.dataTreeView->mapToGlobal(pos));
}

void MainWindow::chartContextMenuRequested(const QPoint& pos)
{
    const QPoint dPos = currentDocument()->detailedDiagram()->mapFromGlobal(currentDocument()->chart()->mapToGlobal(pos));

    const QModelIndex idx = currentDocument()->detailedDiagram()->indexAt(dPos);
    if (!idx.isValid()) {
        return;
    }
    // hack: the ToolTip will only be queried by KDChart and that one uses the
    // left index, but we want it to query the right one
    const QModelIndex _idx = currentDocument()->detailedCostModel()->index(idx.row() + 1, idx.column(), idx.parent());
    QPair< TreeLeafItem*, SnapshotItem* > item = currentDocument()->detailedCostModel()->itemForIndex(_idx);

    if (!item.first) {
        return;
    }

    QMenu menu;
    menu.addAction(m_markCustomAllocator);
    prepareActions(&menu, item.first);
    menu.exec(currentDocument()->detailedDiagram()->mapToGlobal(dPos));
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
    currentDocument()->detailedCostModel()->hideFunction(m_hideFunction->data().value<TreeLeafItem*>());
}

void MainWindow::slotHideOtherFunctions()
{
    currentDocument()->detailedCostModel()->hideOtherFunctions(m_hideOtherFunctions->data().value<TreeLeafItem*>());
}

void MainWindow::slotShortenTemplates(bool shorten)
{
    if (shorten == Settings::self()->shortenTemplates()) {
        return;
    }

    Settings::self()->setShortenTemplates(shorten);
    settingsChanged();
}

void MainWindow::showPrintPreviewDialog()
{
    Q_ASSERT(ui.documents->count());

    QPrinter printer;
    QPrintPreviewDialog *ppd = new QPrintPreviewDialog(&printer, this);
    ppd->setAttribute(Qt::WA_DeleteOnClose);
    connect(ppd, SIGNAL(paintRequested(QPrinter*)), SLOT(printFile(QPrinter*)));
    ppd->setWindowTitle(i18n("Massif Chart Print Preview"));
    ppd->resize(800, 600);
    ppd->exec();
}

void  MainWindow::printFile(QPrinter *printer)
{
    QPainter painter;
    painter.begin(printer);
    currentDocument()->chart()->paint(&painter, printer->pageRect());
    painter.end();
}

void MainWindow::updateWindowTitle()
{
    if (ui.documents->count() && currentDocument()->isLoaded()) {
        setWindowTitle(i18n("Massif Visualizer - evaluation of %1 (%2)", currentDocument()->data()->cmd(), currentDocument()->file().fileName()));
    } else {
        setWindowTitle(i18n("Massif Visualizer"));
    }
}

DocumentWidget* MainWindow::currentDocument() const
{
    return qobject_cast<DocumentWidget*>(ui.documents->currentWidget());
}

void MainWindow::documentChanged()
{
    updateWindowTitle();

    if (ui.documents->count() && currentDocument()->isLoaded()) {

        ui.dataTreeView->setModel(currentDocument()->dataTreeFilterModel());

        // FIXME (arnold): this connection shouldn't be necessary
        connect(ui.dataTreeView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                this, SLOT(treeSelectionChanged(QModelIndex,QModelIndex)));

        m_toggleDetailed->setChecked(!currentDocument()->detailedDiagram()->isHidden());
        m_toggleTotal->setChecked(!currentDocument()->totalDiagram()->isHidden());
    }
}

bool MainWindow::currentChangingSelections() const
{
    return m_changingSelections[currentDocument()];
}

void MainWindow::setCurrentChangingSelections(bool changingSelections)
{
    m_changingSelections[currentDocument()] = changingSelections;
}

#include "mainwindow.moc"
