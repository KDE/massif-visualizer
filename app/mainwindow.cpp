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

#include "massifdata/filedata.h"
#include "massifdata/parser.h"
#include "massifdata/snapshotitem.h"

#include "visualizer/totalcostmodel.h"

#include <KStandardAction>
#include <KActionCollection>
#include <KFileDialog>

#include <KMimeType>
#include <KFilterDev>
#include <KMessageBox>

using namespace Massif;
using namespace KDChart;

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags f)
    : KXmlGuiWindow(parent, f), m_chart(new Chart), m_header(new HeaderFooter), m_subheader(new HeaderFooter)
    , m_data(0)
{
    ui.setupUi(this);

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

    setCentralWidget(m_chart);

    setupActions();
    createGUI();
    //TODO: why the hell doesn't this work?
//     setupGUI();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupActions()
{
    KStandardAction::open(this, SLOT(openFile()), actionCollection());
    KStandardAction::openRecent(this, SLOT(openFile(KUrl)), actionCollection());
    KStandardAction::close(this, SLOT(closeFile()), actionCollection());

    KStandardAction::quit(qApp, SLOT(closeAllWindows()), actionCollection());
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

    m_header->setText(i18n("memory consumption of '%1' %2", m_data->cmd(), m_data->description() != "(none)" ? m_data->description() : ""));
    m_subheader->setText(i18n("peak of %1 bytes at snapshot %2", m_data->peak()->memHeap(), m_data->peak()->number()));

    Plotter* diagram = new Plotter;
    diagram->setAntiAliasing(true);
    LineAttributes attributes = diagram->lineAttributes();
    attributes.setDisplayArea(true);
    attributes.setTransparency(127);
    diagram->setLineAttributes(attributes);

    TotalCostModel* model = new TotalCostModel(m_chart->coordinatePlane());
    model->setSource(m_data);
    diagram->setModel(model);

    CartesianAxis *bottomAxis = new CartesianAxis( diagram );
    bottomAxis->setTitleText("time in " + m_data->timeUnit());
    bottomAxis->setPosition ( CartesianAxis::Bottom );
    diagram->addAxis(bottomAxis);

    CartesianAxis *leftAxis = new CartesianAxis ( diagram );
    leftAxis->setTitleText("memory heap size in bytes");
    leftAxis->setPosition ( CartesianAxis::Left );
    diagram->addAxis(leftAxis);

    m_chart->coordinatePlane()->addDiagram(diagram);
}

void MainWindow::closeFile()
{
    if (!m_data) {
        return;
    }
    delete m_data;
    m_data = 0;

    AbstractCoordinatePlane* plane = m_chart->coordinatePlane();
    m_chart->takeCoordinatePlane(plane);
    delete plane;
    m_chart->addCoordinatePlane(new CartesianCoordinatePlane);
}

Chart* MainWindow::chart()
{
    return m_chart;
}

#include "mainwindow.moc"
