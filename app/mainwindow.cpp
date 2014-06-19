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
#include <KPluginFactory>
#include <KPluginLoader>

#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QLabel>
#include <QSpinBox>
#include <QInputDialog>
#include <QPrinter>
#include <QPrintDialog>
#include <QPixmap>

#include <KMessageBox>

#ifdef HAVE_KGRAPHVIEWER
#include <kgraphviewer_interface.h>
#endif

using namespace Massif;
using namespace KDChart;

// Helper function
static KConfigGroup allocatorConfig()
{
    return KGlobal::config()->group("Allocators");
}

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags f)
    : KParts::MainWindow(parent, f)
    , m_toggleTotal(0)
    , m_selectPeak(0)
    , m_recentFiles(0)
    , m_box(new QSpinBox(this))
    , m_zoomIn(0)
    , m_zoomOut(0)
    , m_focusExpensive(0)
    , m_close(0)
    , m_print(0)
    , m_saveAs(0)
    , m_stopParser(0)
    , m_allocatorModel(new QStringListModel(this))
    , m_newAllocator(0)
    , m_removeAllocator(0)
    , m_shortenTemplates(0)
    , m_currentDocument(0)
{
    ui.setupUi(this);

    setWindowTitle(i18n("Massif Visualizer"));

    //BEGIN KGraphViewer
    bool haveGraphViewer = false;

    // NOTE: just check if kgraphviewer is available at runtime.
    // The former logic has been moved to DocumentWidget constructor.
#ifdef HAVE_KGRAPHVIEWER
    KPluginFactory *factory = KPluginLoader("kgraphviewerpart").factory();
    if (factory) {
        KParts::ReadOnlyPart* readOnlyPart = factory->create<KParts::ReadOnlyPart>("kgraphviewerpart", this);
        if (readOnlyPart) {
            readOnlyPart->widget()->hide();
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
    m_recentFiles = KStandardAction::openRecent(this, SLOT(openFile(KUrl)), actionCollection());
    m_recentFiles->loadEntries(KGlobal::config()->group( QString() ));

    m_saveAs = KStandardAction::saveAs(this, SLOT(saveCurrentDocument()), actionCollection());
    actionCollection()->addAction("file_save_as", m_saveAs);
    m_saveAs->setEnabled(false);

    KAction* reload = KStandardAction::redisplay(this, SLOT(reloadCurrentFile()), actionCollection());
    actionCollection()->addAction("file_reload", reload);
    reload->setEnabled(false);

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
    if (toolBar("callgraphToolBar")) {
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
    m_box->setMinimum(0);
    m_box->setMaximum(50);
    m_box->setValue(10);
    connect(m_box, SIGNAL(valueChanged(int)), this, SLOT(setStackNum(int)));
    stackNumLayout->addWidget(m_box);
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

    if (m_currentDocument) {
        m_currentDocument->updateHeader();
        m_currentDocument->updatePeaks();
        m_currentDocument->updateLegendPosition();
        m_currentDocument->updateLegendFont();
    }
    ui.dataTreeView->viewport()->update();
}

void MainWindow::openFile()
{
    const KUrl::List files = KFileDialog::getOpenUrls(KUrl("kfiledialog:///massif-visualizer"),
                                                      QString("application/x-valgrind-massif"),
                                                      this, i18n("Open Massif Output File"));
    foreach (const KUrl& file, files) {
        openFile(file);
    }
}

void MainWindow::saveCurrentDocument()
{
    QString saveFilename = KFileDialog::getSaveFileName(KUrl("kfiledialog:///massif-visualizer"),
                                                        QString("image/png image/jpeg image/tiff"),
                                                        this, i18n("Save Current Visualization"));

    if (!saveFilename.isEmpty()) {

        if ( !QFile::exists(saveFilename) ||
                (KMessageBox::warningYesNo(this, QString(
                    i18n("Warning, the file(%1) exists.\n Is it OK to overwrite it?")
                        ).arg(saveFilename)) == KMessageBox::Yes)) {

            // TODO: implement a dialog to expose more options to the user.
            // for example we could expose dpi, size, and various format
            // dependent options such as compressions settings.
            // TODO: implement support for vector formats such as ps,svg etc.

            if(!QPixmap::grabWidget(m_currentDocument).save(saveFilename)) {

                KMessageBox::sorry(this, QString(
                    i18n("Error, failed to save the image to %1.")
                        ).arg(saveFilename));
            }
        }
    }
}

void MainWindow::reloadCurrentFile()
{
    if (m_currentDocument->file().isValid()) {
        openFile(KUrl(m_currentDocument->file()));
    }
}

void MainWindow::stopParser()
{
    Q_ASSERT(m_currentDocument);
    ParseWorker* parseWorker = m_documentsParseWorkers.take(m_currentDocument);

    QThread* thread = parseWorker->thread();
    parseWorker->stop();
    parseWorker->deleteLater();
    thread->quit();
    thread->wait();

    m_stopParser->setEnabled(!m_documentsParseWorkers.isEmpty());
}

void MainWindow::openFile(const KUrl& file)
{
    Q_ASSERT(file.isValid());

    // Is file already opened ?
    int indexToInsert = -1;
    for (int i = 0; i < ui.documents->count(); ++i) {
        if (qobject_cast<DocumentWidget*>(ui.documents->widget(i))->file() == file) {
            indexToInsert = i;
            break;
        }
    }

    DocumentWidget* documentWidget = new DocumentWidget(this);
    documentWidget->setLoadingMessage(i18n("loading file <i>%1</i>...", file.pathOrUrl()));

    m_changingSelections.insert(documentWidget, false);

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
        ui.documents->setCurrentIndex(indexToInsert);
    } else {
        ui.documents->addTab(documentWidget, file.fileName());
    }

    connect(parseWorker, SIGNAL(finished(KUrl, Massif::FileData*)),
            documentWidget, SLOT(parserFinished(KUrl, Massif::FileData*)));
    connect(parseWorker, SIGNAL(error(QString, QString)),
            documentWidget, SLOT(showError(QString, QString)));
    connect(parseWorker, SIGNAL(progressRange(int, int)),
            documentWidget, SLOT(setRange(int,int)));
    connect(parseWorker, SIGNAL(progress(int)),
            documentWidget, SLOT(setProgress(int)));
    connect(documentWidget, SIGNAL(loadingFinished()),
            this, SLOT(documentChanged()));

    parseWorker->parse(file, m_allocatorModel->stringList());

    m_stopParser->setEnabled(true);
    m_recentFiles->addUrl(file);
    ui.stackedWidget->setCurrentWidget(ui.displayPage);
}

void MainWindow::treeSelectionChanged(const QModelIndex& now, const QModelIndex& before)
{
    if (!m_currentDocument || currentChangingSelections()) {
        return;
    }

    if (now == before) {
        return;
    }
    setCurrentChangingSelections(true);

    const ModelItem& item = m_currentDocument->dataTreeModel()->itemForIndex(
       m_currentDocument->dataTreeFilterModel()->mapToSource(now)
    );

    if (now.parent().isValid()) {
        m_currentDocument->detailedCostModel()->setSelection(m_currentDocument->detailedCostModel()->indexForItem(item));
        m_currentDocument->totalCostModel()->setSelection(QModelIndex());
    } else {
        m_currentDocument->totalCostModel()->setSelection(m_currentDocument->totalCostModel()->indexForItem(item));
        m_currentDocument->detailedCostModel()->setSelection(QModelIndex());
    }

    m_currentDocument->chart()->update();

#ifdef HAVE_KGRAPHVIEWER
    if (m_currentDocument->graphViewer()) {
        m_currentDocument->showDotGraph(item);
    }
#endif

    setCurrentChangingSelections(false);
}

void MainWindow::detailedItemClicked(const QModelIndex& idx)
{
    if (!m_currentDocument || currentChangingSelections()) {
        return;
    }

    setCurrentChangingSelections(true);

    m_currentDocument->detailedCostModel()->setSelection(idx);
    m_currentDocument->totalCostModel()->setSelection(QModelIndex());

    // hack: the ToolTip will only be queried by KDChart and that one uses the
    // left index, but we want it to query the right one
    const QModelIndex _idx = m_currentDocument->detailedCostModel()->index(idx.row() + 1, idx.column(), idx.parent());
    ui.dataTreeView->selectionModel()->clearSelection();
    ModelItem item = m_currentDocument->detailedCostModel()->itemForIndex(_idx);
    const QModelIndex& newIndex = m_currentDocument->dataTreeFilterModel()->mapFromSource(
        m_currentDocument->dataTreeModel()->indexForItem(item)
    );
    ui.dataTreeView->selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    ui.dataTreeView->scrollTo(ui.dataTreeView->selectionModel()->currentIndex());

    m_currentDocument->chart()->update();

#ifdef HAVE_KGRAPHVIEWER
    if (m_currentDocument->graphViewer()) {
        m_currentDocument->showDotGraph(item);
    }
#endif

    setCurrentChangingSelections(false);
}

void MainWindow::totalItemClicked(const QModelIndex& idx_)
{
    if (!m_currentDocument || currentChangingSelections()) {
        return;
    }

    setCurrentChangingSelections(true);

    QModelIndex idx = idx_.model()->index(idx_.row() + 1, idx_.column(), idx_.parent());

    m_currentDocument->detailedCostModel()->setSelection(QModelIndex());
    m_currentDocument->totalCostModel()->setSelection(idx);

    ModelItem item = m_currentDocument->totalCostModel()->itemForIndex(idx);

    ui.dataTreeView->selectionModel()->clearSelection();
    const QModelIndex& newIndex = m_currentDocument->dataTreeFilterModel()->mapFromSource(
        m_currentDocument->dataTreeModel()->indexForItem(item)
    );
    ui.dataTreeView->selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    ui.dataTreeView->scrollTo(ui.dataTreeView->selectionModel()->currentIndex());

    m_currentDocument->chart()->update();

    setCurrentChangingSelections(false);
}

void MainWindow::closeCurrentFile()
{
    stopParser();

    // Tab is now removed
    int tabToCloseIndex = ui.documents->currentIndex();
    m_changingSelections.remove(m_currentDocument);
    ui.documents->widget(tabToCloseIndex)->deleteLater();
    ui.documents->removeTab(tabToCloseIndex);
}

void MainWindow::showDetailedGraph(bool show)
{
    if (!m_currentDocument) {
        return;
    }
    m_currentDocument->detailedDiagram()->setHidden(!show);
    m_toggleDetailed->setChecked(show);
    m_currentDocument->chart()->update();
}

void MainWindow::showTotalGraph(bool show)
{
    if (!m_currentDocument) {
        return;
    }
    m_currentDocument->totalDiagram()->setHidden(!show);
    m_toggleTotal->setChecked(show);
    m_currentDocument->chart()->update();
}

#ifdef HAVE_KGRAPHVIEWER

void MainWindow::zoomIn()
{
    Q_ASSERT(m_currentDocument->graphViewer());

    m_currentDocument->graphViewer()->zoomIn();
}

void MainWindow::zoomOut()
{
    Q_ASSERT(m_currentDocument->graphViewer());

    m_currentDocument->graphViewer()->zoomOut();
}

void MainWindow::focusExpensiveGraphNode()
{
    Q_ASSERT(m_currentDocument);
    m_currentDocument->focusExpensiveGraphNode();
}

#endif

void MainWindow::selectPeakSnapshot()
{
    Q_ASSERT(m_currentDocument);

    ui.dataTreeView->selectionModel()->clearSelection();
    ui.dataTreeView->selectionModel()->setCurrentIndex(
        m_currentDocument->dataTreeFilterModel()->mapFromSource(
            m_currentDocument->dataTreeModel()->indexForSnapshot(m_currentDocument->data()->peak())),
            QItemSelectionModel::Select | QItemSelectionModel::Rows
    );
}

void MainWindow::setStackNum(int num)
{
    if (!m_currentDocument || !m_currentDocument->isLoaded()) {
        return;
    }
    m_currentDocument->detailedCostModel()->setMaximumDatasetCount(num);
    m_currentDocument->updatePeaks();
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

    if (m_currentDocument) {
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
    const QModelIndex idx = m_currentDocument->dataTreeFilterModel()->mapToSource(
                                ui.dataTreeView->indexAt(pos));
    if (!idx.isValid()) {
        return;
    }

    if (!idx.parent().isValid()) {
        // snapshot item
        return;
    }

    QMenu menu;
    const TreeLeafItem* item = m_currentDocument->dataTreeModel()->itemForIndex(idx).first;
    Q_ASSERT(item);
    prepareActions(&menu, item);
    menu.exec(ui.dataTreeView->mapToGlobal(pos));
}

void MainWindow::chartContextMenuRequested(const QPoint& pos)
{
    const QPoint dPos = m_currentDocument->detailedDiagram()->mapFromGlobal(m_currentDocument->chart()->mapToGlobal(pos));

    const QModelIndex idx = m_currentDocument->detailedDiagram()->indexAt(dPos);
    if (!idx.isValid()) {
        return;
    }
    // hack: the ToolTip will only be queried by KDChart and that one uses the
    // left index, but we want it to query the right one
    const QModelIndex _idx = m_currentDocument->detailedCostModel()->index(idx.row() + 1, idx.column(), idx.parent());
    ModelItem item = m_currentDocument->detailedCostModel()->itemForIndex(_idx);

    if (!item.first) {
        return;
    }

    QMenu menu;
    menu.addAction(m_markCustomAllocator);
    prepareActions(&menu, item.first);
    menu.exec(m_currentDocument->detailedDiagram()->mapToGlobal(dPos));
}

void MainWindow::prepareActions(QMenu* menu, const TreeLeafItem* item)
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
    m_currentDocument->detailedCostModel()->hideFunction(m_hideFunction->data().value<TreeLeafItem*>());
}

void MainWindow::slotHideOtherFunctions()
{
    m_currentDocument->detailedCostModel()->hideOtherFunctions(m_hideOtherFunctions->data().value<TreeLeafItem*>());
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
    Q_ASSERT(m_currentDocument);

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
    m_currentDocument->chart()->paint(&painter, printer->pageRect());
    painter.end();
}

void MainWindow::updateWindowTitle()
{
    if (m_currentDocument && m_currentDocument->isLoaded()) {
        setWindowTitle(i18n("Massif Visualizer - evaluation of %1 (%2)", m_currentDocument->data()->cmd(), m_currentDocument->file().fileName()));
    } else {
        setWindowTitle(i18n("Massif Visualizer"));
    }
}

void MainWindow::documentChanged()
{
    if (m_currentDocument) {
        if (m_currentDocument->totalDiagram()) {
            disconnect(m_currentDocument->totalDiagram(), SIGNAL(clicked(QModelIndex)),
                       this, SLOT(totalItemClicked(QModelIndex)));
        }
        if (m_currentDocument->detailedDiagram()) {
            disconnect(m_currentDocument->detailedDiagram(), SIGNAL(clicked(QModelIndex)),
                       this, SLOT(detailedItemClicked(QModelIndex)));
        }
        if (m_currentDocument->chart()) {
            disconnect(m_currentDocument->chart(), SIGNAL(customContextMenuRequested(QPoint)),
                       this, SLOT(chartContextMenuRequested(QPoint)));
        }
        disconnect(m_currentDocument, SIGNAL(tabChanged(int)),
                   this, SLOT(documentTabChanged(int)));
        disconnect(ui.filterDataTree, SIGNAL(textChanged(QString)),
                   m_currentDocument->dataTreeFilterModel(), SLOT(setFilter(QString)));
        disconnect(ui.dataTreeView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                   this, SLOT(treeSelectionChanged(QModelIndex,QModelIndex)));
    }

    m_currentDocument = qobject_cast<DocumentWidget*>(ui.documents->currentWidget());

    updateWindowTitle();
#ifdef HAVE_KGRAPHVIEWER
    if (toolBar("callgraphToolBar")) {
        m_zoomIn->setEnabled(m_currentDocument && m_currentDocument->graphViewer());
        m_zoomOut->setEnabled(m_currentDocument && m_currentDocument->graphViewer());
        m_focusExpensive->setEnabled(m_currentDocument && m_currentDocument->graphViewer());
    }
#endif
    m_print->setEnabled(m_currentDocument && m_currentDocument->isLoaded());
    m_selectPeak->setEnabled(m_currentDocument && m_currentDocument->data());
    actionCollection()->action("file_reload")->setEnabled(m_currentDocument);
    m_toggleDetailed->setEnabled(m_currentDocument && m_currentDocument->detailedDiagram());
    m_toggleTotal->setEnabled(m_currentDocument && m_currentDocument->totalDiagram());
    m_close->setEnabled(m_currentDocument);
    m_saveAs->setEnabled(m_currentDocument && m_currentDocument->isLoaded());

    if (!m_currentDocument) {
        ui.dataTreeView->setModel(0);
        ui.stackedWidget->setCurrentWidget(ui.openPage);
        return;
    }

    ui.dataTreeView->setModel(m_currentDocument->dataTreeFilterModel());

    if (m_currentDocument->totalDiagram()) {
        connect(m_currentDocument->totalDiagram(), SIGNAL(clicked(QModelIndex)),
                this, SLOT(totalItemClicked(QModelIndex)));
    }
    if (m_currentDocument->detailedDiagram()) {
        connect(m_currentDocument->detailedDiagram(), SIGNAL(clicked(QModelIndex)),
                this, SLOT(detailedItemClicked(QModelIndex)));
    }
    if (m_currentDocument->detailedCostModel()) {
        m_box->setValue(m_currentDocument->detailedCostModel()->maximumDatasetCount());
    }
    if (m_currentDocument->chart()) {
        connect(m_currentDocument->chart(), SIGNAL(customContextMenuRequested(QPoint)),
                this, SLOT(chartContextMenuRequested(QPoint)));
    }
    connect(ui.filterDataTree, SIGNAL(textChanged(QString)),
            m_currentDocument->dataTreeFilterModel(), SLOT(setFilter(QString)));
    connect(ui.dataTreeView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(treeSelectionChanged(QModelIndex,QModelIndex)));

    connect(m_currentDocument, SIGNAL(tabChanged(int)),
            this, SLOT(documentTabChanged(int)));

    if (m_toggleDetailed->isEnabled()) {
        m_toggleDetailed->setChecked(!m_currentDocument->detailedDiagram()->isHidden());
    }
    if (m_toggleTotal->isEnabled()) {
        m_toggleTotal->setChecked(!m_currentDocument->totalDiagram()->isHidden());
    }

#ifdef HAVE_KGRAPHVIEWER
    documentTabChanged(m_currentDocument->currentIndex());
#else
    documentTabChanged(0);
#endif
}

bool MainWindow::currentChangingSelections() const
{
    return m_changingSelections[m_currentDocument];
}

void MainWindow::setCurrentChangingSelections(bool changingSelections)
{
    m_changingSelections[m_currentDocument] = changingSelections;
}

void MainWindow::documentTabChanged(int index)
{
    Q_ASSERT(m_currentDocument);
    toolBar("chartToolBar")->setVisible(index == 0);
    foreach(QAction* action, toolBar("chartToolBar")->actions()) {
        action->setEnabled(m_currentDocument->data() && index == 0);
    }
    toolBar("callgraphToolBar")->setVisible(index == 1);
    foreach(QAction* action, toolBar("callgraphToolBar")->actions()) {
        action->setEnabled(m_currentDocument->data() && index == 1);
    }
}

#include "mainwindow.moc"
