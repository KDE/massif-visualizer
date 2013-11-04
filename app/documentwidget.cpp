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

#include "documentwidget.h"

using namespace Massif;
using namespace KDChart;

void markPeak(Plotter* p, const QModelIndex& peak, quint64 cost, const QPen& foreground)
{
    DataValueAttributes dataAttributes = p->dataValueAttributes(peak);
    dataAttributes.setDataLabel(prettyCost(cost));
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

DocumentWidget::DocumentWidget(QWidget* parent) :
    QWidget(parent), m_chart(new Chart(this))
  , m_header(new QLabel(this))
  , m_totalDiagram(0)
  , m_totalCostModel(new TotalCostModel(m_chart))
  , m_detailedDiagram(0)
  , m_detailedCostModel(new DetailedCostModel(m_chart))
  , m_legend(new Legend(m_chart))
  , m_dataTreeModel(new DataTreeModel(m_chart))
  , m_dataTreeFilterModel(new FilteredDataTreeModel(m_dataTreeModel))
  , m_data(0)
#ifdef HAVE_KGRAPHVIEWER
  , m_graphViewerPart(0)
  , m_graphViewer(0)
  , m_dotGenerator(0)
#endif
  , m_stackedWidget(new QStackedWidget(this))
  , m_displayTabWidget(new QTabWidget(m_stackedWidget))
  , m_loadingMessage(0)
  , m_loadingProgressBar(0)
  , m_stopParserButton(0)
  , m_isLoaded(false)
{
    // for axis labels to fit
    m_chart->setGlobalLeadingRight(10);
    m_chart->setGlobalLeadingLeft(10);
    m_chart->setGlobalLeadingTop(20);
    m_chart->setContextMenuPolicy(Qt::CustomContextMenu);

    m_legend->setPosition(Position(KDChartEnums::PositionFloating));
    m_legend->setTitleText("");
    m_legend->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_legend->setSortOrder(Qt::DescendingOrder);

    m_chart->addLegend(m_legend);

    //NOTE: this has to be set _after_ the legend was added to the chart...
    TextAttributes att = m_legend->textAttributes();
    att.setAutoShrink(true);
    att.setFontSize(Measure(12));
    QFont font("monospace");
    font.setStyleHint(QFont::TypeWriter);
    att.setFont(font);
    m_legend->setTextAttributes(att);
    m_legend->setTextAlignment(Qt::AlignLeft);
    m_legend->hide();

    // Set m_stackedWidget as the main widget.
    setLayout(new QGridLayout(this));
    layout()->addWidget(m_stackedWidget);

    // First widget : displayPage
    m_displayTabWidget->setTabPosition(QTabWidget::South);
    QWidget* memoryConsumptionWidget = new QWidget(m_displayTabWidget);
    memoryConsumptionWidget->setLayout(new QVBoxLayout(memoryConsumptionWidget));
    memoryConsumptionWidget->layout()->addWidget(m_header);
    memoryConsumptionWidget->layout()->addWidget(m_chart);
    m_displayTabWidget->addTab(memoryConsumptionWidget, i18n("&Evolution of Memory Consumption"));

#ifdef HAVE_KGRAPHVIEWER
    KLibFactory *factory = KLibLoader::self()->factory("kgraphviewerpart");
    if (factory) {
        m_graphViewerPart = factory->create<KParts::ReadOnlyPart>(this);
        if (m_graphViewerPart) {
            m_graphViewer = qobject_cast< KGraphViewer::KGraphViewerInterface* >(m_graphViewerPart);
            QWidget* dotGraphWidget = new QWidget(m_displayTabWidget);
            dotGraphWidget->setLayout(new QGridLayout(dotGraphLayout));
            dotGraphWidget->layout()->addWidget(m_graphViewerPart->widget());
            m_displayTabWidget->addTab(dotGraphWidget, i18n("&Detailed Snapshot Analysis"));
            connect(m_graphViewerPart, SIGNAL(graphLoaded()), this, SLOT(slotGraphLoaded()));

            connect(m_displayTabWidget, SIGNAL(currentChanged(int)),
                    this, SLOT(slotTabChanged(int)));
            slotTabChanged(m_displayTabWidget->currentIndex());
        }
    }
#endif

    m_stackedWidget->addWidget(m_displayTabWidget);

    // Second widget : loadingPage
    QWidget* loadingPage = new QWidget(m_stackedWidget);
    QVBoxLayout* verticalLayout = new QVBoxLayout(loadingPage);
    QSpacerItem* upperSpacerItem = new QSpacerItem(20, 247, QSizePolicy::Minimum, QSizePolicy::Expanding);
    verticalLayout->addItem(upperSpacerItem);

    m_loadingMessage = new QLabel(loadingPage);
    m_loadingMessage->setText(i18n("loading..."));
    m_loadingMessage->setAlignment(Qt::AlignCenter);
    verticalLayout->addWidget(m_loadingMessage);

    m_loadingProgressBar = new QProgressBar(loadingPage);
    m_loadingProgressBar->setValue(24);
    m_loadingProgressBar->setRange(0, 0);
    verticalLayout->addWidget(m_loadingProgressBar);

    QWidget* stopParserWidget = new QWidget(loadingPage);
    stopParserWidget->setLayoutDirection(Qt::LeftToRight);
    QHBoxLayout* stopParserWidgetLayout = new QHBoxLayout(stopParserWidget);
    m_stopParserButton = new QToolButton(stopParserWidget);
    m_stopParserButton->setObjectName(QString::fromUtf8("stopParsing"));
    m_stopParserButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_stopParserButton->setIcon(KIcon("process-stop"));
    m_stopParserButton->setIconSize(QSize(48, 48));
    connect(m_stopParserButton, SIGNAL(clicked()),
            this, SLOT(slotEmitStopParserSignal()));
    stopParserWidgetLayout->addWidget(m_stopParserButton);
    verticalLayout->addWidget(stopParserWidget);

    QSpacerItem* bottomSpacerItem = new QSpacerItem(20, 230, QSizePolicy::Minimum, QSizePolicy::Expanding);
    verticalLayout->addItem(bottomSpacerItem);
    m_stackedWidget->addWidget(loadingPage);

    // By default we show the loadingPage.
    m_stackedWidget->setCurrentIndex(1);
}

DocumentWidget::~DocumentWidget()
{
    if (m_data) {

#ifdef HAVE_KGRAPHVIEWER
        if (m_dotGenerator) {
            if (m_dotGenerator->isRunning()) {
                disconnect(m_dotGenerator.data(), 0, this, 0);
                connect(m_dotGenerator.data(), SIGNAL(finished()),
                        m_dotGenerator.data(), SLOT(deleteLater()));
                m_dotGenerator->cancel();
                m_dotGenerator.take();
            }
            m_dotGenerator.reset();
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

        m_chart->replaceCoordinatePlane(new CartesianCoordinatePlane);
        m_legend->removeDiagrams();
        m_legend->hide();
        m_header->setText("");

        foreach(CartesianAxis* axis, m_detailedDiagram->axes()) {
            m_detailedDiagram->takeAxis(axis);
            delete axis;
        }
        m_detailedDiagram->deleteLater();
        m_detailedDiagram = 0;

        foreach(CartesianAxis* axis, m_totalDiagram->axes()) {
            m_totalDiagram->takeAxis(axis);
            delete axis;
        }
        m_totalDiagram->deleteLater();
        m_totalDiagram = 0;

        m_dataTreeModel->setSource(0);
        m_dataTreeFilterModel->setFilter("");
        m_detailedCostModel->setSource(0);
        m_totalCostModel->setSource(0);

        delete m_data;
        m_data = 0;
        m_file.clear();
    }
}

KUrl DocumentWidget::file() const
{
    return m_file;
}

FileData* DocumentWidget::data() const
{
    return m_data;
}

Chart* DocumentWidget::chart() const
{
    return m_chart;
}

Plotter* DocumentWidget::totalDiagram() const
{
    return m_totalDiagram;
}


TotalCostModel* DocumentWidget::totalCostModel() const
{
    return m_totalCostModel;
}

Plotter* DocumentWidget::detailedDiagram() const
{
    return m_detailedDiagram;
}

DetailedCostModel* DocumentWidget::detailedCostModel() const
{
    return m_detailedCostModel;
}

DataTreeModel* DocumentWidget::dataTreeModel() const
{
    return m_dataTreeModel;
}

FilteredDataTreeModel* DocumentWidget::dataTreeFilterModel() const
{
    return m_dataTreeFilterModel;
}

#ifdef HAVE_KGRAPHVIEWER
    KGraphViewer::KGraphViewerInterface* DocumentWidget::graphViewer()
    {
        return m_graphViewer;
    }
#endif

bool DocumentWidget::isLoaded() const
{
    return m_isLoaded;
}

void DocumentWidget::parserFinished(const KUrl& file, FileData* data)
{
    Q_ASSERT(data->peak());

    kDebug() << "loaded massif file:" << file;
    qDebug() << "description:" << data->description();
    qDebug() << "command:" << data->cmd();
    qDebug() << "time unit:" << data->timeUnit();
    qDebug() << "snapshots:" << data->snapshots().size();
    qDebug() << "peak: snapshot #" << data->peak()->number() << "after" << QString("%1%2").arg(data->peak()->time()).arg(data->timeUnit());
    qDebug() << "peak cost:" << prettyCost(data->peak()->memHeap()) << " heap"
                             << prettyCost(data->peak()->memHeapExtra()) << " heap extra"
                             << prettyCost(data->peak()->memStacks()) << " stacks";

    m_data = data;
    m_file = file;

    #ifdef HAVE_KGRAPHVIEWER
    if (m_graphViewer) {
        showDotGraph(QPair<TreeLeafItem*,SnapshotItem*>(0, m_data->peak()));
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

    m_header->setAlignment(Qt::AlignCenter);
    updateHeader();

    //BEGIN TotalDiagram
    m_totalDiagram = new Plotter;
    m_totalDiagram->setAntiAliasing(true);

    CartesianAxis* bottomAxis = new CartesianAxis(m_totalDiagram);
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

    CartesianAxis* rightAxis = new CartesianAxis(m_totalDiagram);
    rightAxis->setTextAttributes(axisTextAttributes);
    rightAxis->setTitleTextAttributes(axisTitleTextAttributes);
    rightAxis->setTitleText(i18n("memory heap size in kilobytes"));
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
    m_detailedDiagram->setType(KDChart::Plotter::Stacked);

    m_detailedCostModel->setSource(m_data);
    m_detailedDiagram->setModel(m_detailedCostModel);

    updatePeaks();

    m_chart->coordinatePlane()->addDiagram(m_detailedDiagram);

    m_legend->addDiagram(m_detailedDiagram);
    m_legend->show();

    m_dataTreeModel->setSource(m_data);
    m_isLoaded = true;

    // Switch to the display page and notify that everything is setup.
    m_stackedWidget->setCurrentIndex(0);
    emit loadingFinished();
}

void DocumentWidget::setDetailedDiagramHidden(bool hidden)
{
    m_detailedDiagram->setHidden(hidden);
}

void DocumentWidget::setDetailedDiagramVisible(bool visible)
{
    m_detailedDiagram->setVisible(visible);
}

void DocumentWidget::setTotalDiagramHidden(bool hidden)
{
    m_totalDiagram->setHidden(hidden);
}

void DocumentWidget::setTotalDiagramVisible(bool visible)
{
    m_totalDiagram->setVisible(visible);
}

void DocumentWidget::setProgress(int value)
{
    m_loadingProgressBar->setValue(value);
}

void DocumentWidget::setRange(int minimum, int maximum)
{
    m_loadingProgressBar->setRange(minimum, maximum);
}

void DocumentWidget::setLoadingMessage(const QString& message)
{
    m_loadingMessage->setText(message);
}

void DocumentWidget::slotEmitStopParserSignal()
{
    emit stopParser();
}

void DocumentWidget::updateHeader()
{
    const QString app = m_data->cmd().split(' ', QString::SkipEmptyParts).first();

    m_header->setText(QString("<b>%1</b><br /><i>%2</i>")
                        .arg(i18n("Memory consumption of %1", app))
                        .arg(i18n("Peak of %1 at snapshot #%2", prettyCost(m_data->peak()->cost()), m_data->peak()->number()))
    );
    m_header->setToolTip(i18n("Command: %1\nValgrind Options: %2", m_data->cmd(), m_data->description()));
}

void DocumentWidget::updatePeaks()
{
    KColorScheme scheme(QPalette::Active, KColorScheme::Window);
    QPen foreground(scheme.foreground().color());

    if (m_data->peak()) {
        const QModelIndex peak = m_totalCostModel->peak();
        Q_ASSERT(peak.isValid());
        markPeak(m_totalDiagram, peak, m_data->peak()->cost(), foreground);
    }
    updateDetailedPeaks();
}

void DocumentWidget::updateDetailedPeaks()
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

void MainWindow::showDotGraph(const QPair<TreeLeafItem*, SnapshotItem*>& item)
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
            disconnect(m_dotGenerator.data(), 0, this, 0);
            connect(m_dotGenerator.data(), SIGNAL(finished()),
                    m_dotGenerator.data(), SLOT(deleteLater()));
            m_dotGenerator->cancel();
            m_dotGenerator.take();
        }
        m_dotGenerator.reset();
    }
    if (!item.first && !item.second) {
        return;
    }
    if (item.second) {
        m_dotGenerator.reset(new DotGraphGenerator(item.second, m_data->timeUnit(), this));
    } else {
        m_dotGenerator.reset(new DotGraphGenerator(item.first, m_data->timeUnit(), this));
    }
    connect(m_dotGenerator.data(), SIGNAL(finished()), SLOT(showDotGraph()));
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
#endif