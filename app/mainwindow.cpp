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

using namespace Massif;
using namespace KDChart;

//BEGIN Helper Functions
void markPeak(Plotter* p, const QModelIndex& peak, uint cost, QPen foreground)
{
    DataValueAttributes dataAttributes = p->dataValueAttributes(peak);
    dataAttributes.setDataLabel(i18n("Peak of %1 bytes", cost));
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
    : KXmlGuiWindow(parent, f), m_chart(new Chart(this)), m_header(new HeaderFooter(m_chart)), m_subheader(new HeaderFooter(m_chart))
    , m_toggleTotal(0), m_totalDiagram(0), m_totalCostModel(new TotalCostModel(m_chart))
    , m_toggleDetailed(0), m_detailedDiagram(0), m_detailedCostModel(new DetailedCostModel(m_chart))
    , m_legend(new Legend(m_chart))
    , m_dataTreeModel(new DataTreeModel(m_chart)), m_data(0)
    , m_changingSelections(false)
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

    setCentralWidget(m_chart);

    ui.treeView->setModel(m_dataTreeModel);
    connect(ui.treeView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(treeSelectionChanged(QModelIndex,QModelIndex)));

    setupActions();
    setupGUI(StandardWindowOptions(Default ^ StatusBar));
    statusBar()->hide();
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

    qDebug() << "description:" << m_data->description();
    qDebug() << "command:" << m_data->cmd();
    qDebug() << "time unit:" << m_data->timeUnit();
    qDebug() << "snapshots:" << m_data->snapshots().size();
    qDebug() << "peak: snapshot #" << m_data->peak()->number() << "after" << QString("%1%2").arg(m_data->peak()->time()).arg(m_data->timeUnit());
    qDebug() << "peak cost:" << m_data->peak()->memHeap() << "bytes heap"
                             << m_data->peak()->memHeapExtra() << "bytes heap extra"
                             << m_data->peak()->memStacks() << "bytes stacks";

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
        m_subheader->setText(i18n("peak of %1 bytes at snapshot %2", m_data->peak()->memHeap(), m_data->peak()->number()));
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

    {
        QMap< QModelIndex, TreeLeafItem* > peaks = m_detailedCostModel->peaks();
        QMap< QModelIndex, TreeLeafItem* >::const_iterator it = peaks.constBegin();
        while (it != peaks.constEnd()) {
            const QModelIndex peak = it.key();
            Q_ASSERT(peak.isValid());
            markPeak(m_detailedDiagram, peak, it.value()->cost(), foreground);
            ++it;
        }
    }

    m_chart->coordinatePlane()->addDiagram(m_detailedDiagram);
    connect(m_detailedDiagram, SIGNAL(clicked(QModelIndex)),
            this, SLOT(detailedItemClicked(QModelIndex)));

    m_legend->addDiagram(m_detailedDiagram);

    //BEGIN Legend
    m_legend->show();

    //BEGIN TreeView
    m_dataTreeModel->setSource(m_data);

    //BEGIN RecentFiles
    m_recentFiles->addUrl(file);
}

void MainWindow::treeSelectionChanged(const QModelIndex& now, const QModelIndex& before)
{
    if (m_changingSelections) {
        return;
    }

    if (now == before) {
        return;
    }
    m_changingSelections = true;

    if (now.parent().isValid()) {
        m_detailedCostModel->setSelection(m_detailedCostModel->indexForItem(m_dataTreeModel->itemForIndex(now)));
        m_totalCostModel->setSelection(QModelIndex());
    } else {
        m_totalCostModel->setSelection(m_totalCostModel->indexForItem(m_dataTreeModel->itemForIndex(now)));
        m_detailedCostModel->setSelection(QModelIndex());
    }

    m_chart->update();

    m_changingSelections = false;
}

void MainWindow::detailedItemClicked(const QModelIndex& item)
{
    if (m_changingSelections) {
        return;
    }

    m_changingSelections = true;

    m_detailedCostModel->setSelection(item);
    m_totalCostModel->setSelection(QModelIndex());

    ui.treeView->selectionModel()->clearSelection();
    const QModelIndex& newIndex = m_dataTreeModel->indexForItem(m_detailedCostModel->itemForIndex(item));
    ui.treeView->selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    ui.treeView->scrollTo(ui.treeView->selectionModel()->currentIndex());

    m_chart->update();

    m_changingSelections = false;
}

void MainWindow::totalItemClicked(const QModelIndex& item)
{
    if (m_changingSelections) {
        return;
    }

    m_changingSelections = true;

    QModelIndex idx = item.model()->index(item.row() + 1, item.column(), item.parent());

    m_detailedCostModel->setSelection(QModelIndex());
    m_totalCostModel->setSelection(idx);

    ui.treeView->selectionModel()->clearSelection();
    const QModelIndex& newIndex = m_dataTreeModel->indexForItem(m_totalCostModel->itemForIndex(idx));
    ui.treeView->selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    ui.treeView->scrollTo(ui.treeView->selectionModel()->currentIndex());

    m_chart->update();

    m_changingSelections = false;
}

void MainWindow::closeFile()
{
    if (!m_data) {
        return;
    }

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
    m_detailedCostModel->setSource(0);
    m_totalCostModel->setSource(0);

    delete m_data;
    m_data = 0;

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
    if (show) {
        Q_ASSERT(!m_chart->coordinatePlane()->diagrams().contains(m_detailedDiagram));
        m_chart->coordinatePlane()->addDiagram(m_detailedDiagram);
    } else {
        Q_ASSERT(m_chart->coordinatePlane()->diagrams().contains(m_detailedDiagram));
        m_chart->coordinatePlane()->takeDiagram(m_detailedDiagram);
    }
    m_toggleDetailed->setChecked(show);
}

void MainWindow::showTotalGraph(bool show)
{
    Q_ASSERT(m_data);
    Q_ASSERT(m_totalDiagram);
    if (show) {
        Q_ASSERT(!m_chart->coordinatePlane()->diagrams().contains(m_totalDiagram));
        m_chart->coordinatePlane()->addDiagram(m_totalDiagram);
    } else {
        Q_ASSERT(m_chart->coordinatePlane()->diagrams().contains(m_totalDiagram));
        m_chart->coordinatePlane()->takeDiagram(m_totalDiagram);
    }
    m_toggleTotal->setChecked(show);
}

#include "mainwindow.moc"
