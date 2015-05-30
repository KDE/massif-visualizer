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

#include "massifdata/filedata.h"
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
#include <KRecentFilesAction>
#include <KColorScheme>
#include <KStatusBar>
#include <KToolBar>
#include <KParts/Part>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KXMLGUIFactory>
#include <KLocalizedString>

#include <QFileDialog>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QLabel>
#include <QSpinBox>
#include <QInputDialog>
#include <QIcon>

#include <KMessageBox>

#ifdef HAVE_KGRAPHVIEWER
#include <kgraphviewer_interface.h>
#endif

using namespace Massif;

// Helper function
static KConfigGroup allocatorConfig()
{
    return KSharedConfig::openConfig()->group("Allocators");
}

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags f)
    : KParts::MainWindow(parent, f)
    , m_recentFiles(0)
    , m_close(0)
    , m_allocatorModel(new QStringListModel(this))
    , m_newAllocator(0)
    , m_removeAllocator(0)
    , m_shortenTemplates(0)
    , m_selectPeak(0)
    , m_currentDocument(0)
    , m_dataTreeModel(new DataTreeModel(this))
    , m_dataTreeFilterModel(new FilteredDataTreeModel(m_dataTreeModel))
    , m_settingSelection(false)
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

    ui.documents->setMovable(true);
    ui.documents->setTabsClosable(true);
    connect(ui.documents, &QTabWidget::currentChanged,
            this, &MainWindow::documentChanged);
    connect(ui.documents, &QTabWidget::tabCloseRequested,
            this, &MainWindow::closeFileTab);

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

    connect(m_allocatorModel, &QStringListModel::modelReset,
            this, &MainWindow::allocatorsChanged);

    connect(m_allocatorModel, &QStringListModel::dataChanged,
            this, &MainWindow::allocatorsChanged);

    connect(ui.dataTreeView, &QTreeView::customContextMenuRequested,
            this, &MainWindow::dataTreeContextMenuRequested);
    ui.dataTreeView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui.allocatorView, &QTreeView::customContextMenuRequested,
            this, &MainWindow::allocatorViewContextMenuRequested);
    ui.allocatorView->setContextMenuPolicy(Qt::CustomContextMenu);
    //END custom allocators

    setupActions();
    setupGUI(StandardWindowOptions(Default ^ StatusBar));
    statusBar()->hide();

    ui.dataTreeView->setModel(m_dataTreeFilterModel);

    connect(ui.filterDataTree, &KLineEdit::textChanged,
            m_dataTreeFilterModel, &FilteredDataTreeModel::setFilter);
    connect(ui.dataTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MainWindow::treeSelectionChanged);

    // open page
    ui.stackedWidget->setCurrentWidget(ui.openPage);
}

MainWindow::~MainWindow()
{
    while (ui.documents->count()) {
        closeCurrentFile();
    }

    m_recentFiles->saveEntries(KSharedConfig::openConfig()->group( QString() ));
}

void MainWindow::setupActions()
{
    QAction* openFile = KStandardAction::open(this, SLOT(openFile()), actionCollection());
    m_recentFiles = KStandardAction::openRecent(this, SLOT(openFile(QUrl)), actionCollection());
    m_recentFiles->loadEntries(KSharedConfig::openConfig()->group( QString() ));

    QAction* reload = KStandardAction::redisplay(this, SLOT(reloadCurrentFile()), actionCollection());
    actionCollection()->addAction("file_reload", reload);
    reload->setEnabled(false);

    m_close = KStandardAction::close(this, SLOT(closeCurrentFile()), actionCollection());
    m_close->setEnabled(false);

    KStandardAction::quit(qApp, SLOT(closeAllWindows()), actionCollection());

    KStandardAction::preferences(this, SLOT(preferences()), actionCollection());

    m_shortenTemplates = new QAction(QIcon::fromTheme("shortentemplates"), i18n("Shorten Templates"), actionCollection());
    m_shortenTemplates->setCheckable(true);
    m_shortenTemplates->setChecked(Settings::shortenTemplates());
    connect(m_shortenTemplates, &QAction::toggled, this, &MainWindow::slotShortenTemplates);
    actionCollection()->addAction("shorten_templates", m_shortenTemplates);

    m_selectPeak = new QAction(QIcon::fromTheme("flag-red"), i18n("Select peak snapshot"), actionCollection());
    connect(m_selectPeak, &QAction::triggered, this, &MainWindow::selectPeakSnapshot);
    actionCollection()->addAction("selectPeak", m_selectPeak);
    m_selectPeak->setEnabled(false);

    //BEGIN custom allocators
    m_newAllocator = new QAction(QIcon::fromTheme("list-add"), i18n("add"), ui.allocatorDock);
    m_newAllocator->setToolTip(i18n("add custom allocator"));
    connect(m_newAllocator, &QAction::triggered, this, &MainWindow::slotNewAllocator);
    ui.dockMenuBar->addAction(m_newAllocator);
    m_removeAllocator = new QAction(QIcon::fromTheme("list-remove"), i18n("remove"),
                                    ui.allocatorDock);
    m_newAllocator->setToolTip(i18n("remove selected allocator"));
    connect(m_removeAllocator, &QAction::triggered, this, &MainWindow::slotRemoveAllocator);
    connect(ui.allocatorView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::allocatorSelectionChanged);
    m_removeAllocator->setEnabled(false);
    ui.dockMenuBar->addAction(m_removeAllocator);

    m_markCustomAllocator = new QAction(i18n("mark as custom allocator"), ui.allocatorDock);
    connect(m_markCustomAllocator, &QAction::triggered,
            this, &MainWindow::slotMarkCustomAllocator, Qt::QueuedConnection);
    //END custom allocators

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
    connect(dlg, &ConfigDialog::settingsChanged,
            this, &MainWindow::settingsChanged);
    dlg->show();
}

void MainWindow::settingsChanged()
{
    if (Settings::self()->shortenTemplates() != m_shortenTemplates->isChecked()) {
        m_shortenTemplates->setChecked(Settings::self()->shortenTemplates());
    }

    Settings::self()->save();

    if (m_currentDocument) {
        m_currentDocument->settingsChanged();
    }
    ui.dataTreeView->viewport()->update();
}

void MainWindow::openFile()
{
    const QList<QUrl> files = QFileDialog::getOpenFileUrls(this, i18n("Open Massif Output File"), QUrl(),
                                                           i18n("Massif data files (massif.out.*)"));
    foreach (const QUrl& file, files) {
        openFile(file);
    }
}

void MainWindow::reloadCurrentFile()
{
    if (m_currentDocument->file().isValid()) {
        openFile(QUrl(m_currentDocument->file()));
    }
}

void MainWindow::openFile(const QUrl& file)
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

    DocumentWidget* documentWidget = new DocumentWidget(file.toString(), m_allocatorModel->stringList(),
                                                        this, this);

    if (indexToInsert != -1) {
        // Remove existing instance of the file.
        ui.documents->setCurrentIndex(indexToInsert);
        closeCurrentFile();
        // Insert the new tab at the correct position.
        ui.documents->insertTab(indexToInsert, documentWidget, file.fileName());
        ui.documents->setCurrentIndex(indexToInsert);
    } else {
        const int idx = ui.documents->addTab(documentWidget, file.fileName());
        ui.documents->setCurrentIndex(idx);
    }
    connect(documentWidget, &DocumentWidget::loadingFinished,
            this, &MainWindow::documentChanged);
    connect(documentWidget, &DocumentWidget::requestClose,
            this, &MainWindow::closeRequested);

    m_recentFiles->addUrl(file);
    ui.stackedWidget->setCurrentWidget(ui.displayPage);
}

void MainWindow::treeSelectionChanged(const QModelIndex& now, const QModelIndex& before)
{
    if (!m_currentDocument || m_settingSelection || now == before) {
        return;
    }

    m_settingSelection = true;

    const ModelItem& item = now.data(DataTreeModel::ModelItemRole).value<ModelItem>();
    m_currentDocument->selectModelItem(item);

    m_settingSelection = false;
}

void MainWindow::modelItemSelected(const ModelItem& item)
{
    if (!m_currentDocument || m_settingSelection) {
        return;
    }

    m_settingSelection = true;

    const QModelIndex& newIndex = m_dataTreeFilterModel->mapFromSource(
        m_dataTreeModel->indexForItem(item)
    );
    ui.dataTreeView->selectionModel()->clearSelection();
    ui.dataTreeView->selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    ui.dataTreeView->scrollTo(ui.dataTreeView->selectionModel()->currentIndex());
    m_currentDocument->selectModelItem(item);

    m_settingSelection = false;
}

void MainWindow::selectPeakSnapshot()
{
    ui.dataTreeView->selectionModel()->clearSelection();
    ui.dataTreeView->selectionModel()->setCurrentIndex(
        m_dataTreeFilterModel->mapFromSource(
            m_dataTreeModel->indexForSnapshot(m_currentDocument->data()->peak())),
            QItemSelectionModel::Select | QItemSelectionModel::Rows
    );
}

void MainWindow::closeCurrentFile()
{
    closeFileTab(ui.documents->currentIndex());
}

void MainWindow::closeRequested()
{
    DocumentWidget* widget = qobject_cast<DocumentWidget*>(sender());
    Q_ASSERT(widget);
    closeFileTab(ui.documents->indexOf(widget));
}

void MainWindow::closeFileTab(int idx)
{
    Q_ASSERT(idx != -1);
    ui.documents->widget(idx)->deleteLater();
    ui.documents->removeTab(idx);
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
    const QModelIndex idx = ui.dataTreeView->indexAt(pos);
    const TreeLeafItem* item = idx.data(DataTreeModel::TreeItemRole).value<const TreeLeafItem*>();
    if (!item) {
        return;
    }

    QMenu menu;
    contextMenuRequested(ModelItem(item, 0), &menu);

    menu.exec(ui.dataTreeView->mapToGlobal(pos));
}

void MainWindow::contextMenuRequested(const ModelItem& item, QMenu* menu)
{
    if (!item.first) {
        // only handle context menu on tree-leaf items for now
        return;
    }

    QString func = functionInLabel(item.first->label());
    if (func.length() > 40) {
        func.resize(40);
        func.append("...");
    }
    menu->setTitle(func);

    m_markCustomAllocator->setData(item.first->label());
    menu->addAction(m_markCustomAllocator);
}

void MainWindow::slotShortenTemplates(bool shorten)
{
    if (shorten == Settings::self()->shortenTemplates()) {
        return;
    }

    Settings::self()->setShortenTemplates(shorten);
    settingsChanged();
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
    // required to prevent GUI flickering when changing documents
    // the changing actions in the toolbar are really flickering bad otherwise
    setUpdatesEnabled(false);

    if (m_currentDocument) {
        m_dataTreeModel->setSource(0);
        m_dataTreeFilterModel->setFilter(QString());
        m_currentDocument->clearGuiActions(guiFactory());
        disconnect(m_currentDocument, &DocumentWidget::modelItemSelected,
                   this, &MainWindow::modelItemSelected);
        disconnect(m_currentDocument, &DocumentWidget::contextMenuRequested,
                   this, &MainWindow::contextMenuRequested);
    }

    m_currentDocument = qobject_cast<DocumentWidget*>(ui.documents->currentWidget());

    updateWindowTitle();

    actionCollection()->action("file_reload")->setEnabled(m_currentDocument && m_currentDocument->isLoaded());
    m_close->setEnabled(m_currentDocument);
    m_selectPeak->setEnabled(m_currentDocument && m_currentDocument->isLoaded());

    if (!m_currentDocument) {
        ui.stackedWidget->setCurrentWidget(ui.openPage);
        setUpdatesEnabled(true);
        return;
    } else {
        m_dataTreeModel->setSource(m_currentDocument->data());
        m_currentDocument->addGuiActions(guiFactory());
        connect(m_currentDocument, &DocumentWidget::modelItemSelected,
                this, &MainWindow::modelItemSelected);
        connect(m_currentDocument, &DocumentWidget::contextMenuRequested,
                this, &MainWindow::contextMenuRequested);
    }

    setUpdatesEnabled(true);
}

#include "mainwindow.moc"
