/*
   This file is part of Massif Visualizer

   Copyright 2010 Milian Wolff <mail@milianw.de>
   Copyright 2013 Arnold Dumas <arnold@dumas.at>

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

#include <QApplication>

#include "massifdata/filedata.h"
#include "massifdata/parser.h"
#include "massifdata/parseworker.h"
#include "massifdata/snapshotitem.h"
#include "massifdata/treeleafitem.h"
#include "massifdata/util.h"

#include <KLocalizedString>
// forward include not available until later KDE versions...
#include <kmessagewidget.h>
#include <KXMLGUIFactory>

#include <QDebug>
#include <QLabel>
#include <QProgressBar>
#include <QStackedWidget>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QIcon>

#include "charttab.h"
#include "allocatorstab.h"

#ifdef HAVE_KGRAPHVIEWER
#include "callgraphtab.h"

#include <KPluginFactory>
#include <KParts/ReadOnlyPart>
#endif

using namespace Massif;

DocumentWidget::DocumentWidget(const QUrl& file, const QStringList& customAllocators,
                               KXMLGUIClient* guiParent, QWidget* parent)
    : QWidget(parent)
    , KXMLGUIClient(guiParent)
    , m_data(nullptr)
    , m_parseWorker(new ParseWorker)
    , m_file(file)
    , m_currentTab(nullptr)
    , m_stackedWidget(new QStackedWidget(this))
    , m_tabs(new QTabWidget(m_stackedWidget))
    , m_errorMessage(nullptr)
    , m_loadingMessage(nullptr)
    , m_loadingProgressBar(nullptr)
    , m_stopParserButton(nullptr)
    , m_isLoaded(false)
{
    connect(m_parseWorker, &ParseWorker::finished,
            this, &DocumentWidget::parserFinished);
    connect(m_parseWorker, &ParseWorker::error,
            this, &DocumentWidget::showError);
    connect(m_parseWorker, &ParseWorker::progressRange,
            this, &DocumentWidget::setRange);
    connect(m_parseWorker, &ParseWorker::progress,
            this, &DocumentWidget::setProgress);

    // Create dedicated thread for this document.
    // TODO: use ThreadWeaver
    QThread* thread = new QThread(this);
    thread->start();
    m_parseWorker->moveToThread(thread);
    m_parseWorker->parse(file, customAllocators);

    setXMLFile(QStringLiteral("documentwidgetui.rc"), true);

    // Set m_stackedWidget as the main widget.
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->addWidget(m_stackedWidget);

    m_tabs->setDocumentMode(true);
    m_tabs->setTabPosition(QTabWidget::South);
    m_stackedWidget->addWidget(m_tabs);

    // Second widget : loadingPage
    QWidget* loadingPage = new QWidget(m_stackedWidget);
    QVBoxLayout* verticalLayout = new QVBoxLayout(loadingPage);
    QSpacerItem* upperSpacerItem = new QSpacerItem(20, 247, QSizePolicy::Minimum, QSizePolicy::Expanding);
    verticalLayout->addItem(upperSpacerItem);

    m_loadingMessage = new QLabel(loadingPage);
    m_loadingMessage->setText(xi18nc("@info:status", "Loading file <filename>%1</filename>…", file.toString()));
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
    m_stopParserButton->setObjectName(QStringLiteral("stopParsing"));
    m_stopParserButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_stopParserButton->setIcon(QIcon::fromTheme(QStringLiteral("process-stop")));
    m_stopParserButton->setIconSize(QSize(48, 48));
    connect(m_stopParserButton, &QToolButton::clicked,
            this, &DocumentWidget::requestClose);
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
    stopParser();
    if (m_data) {
        delete m_data;
        m_data = nullptr;
        m_file.clear();
    }
}

FileData* DocumentWidget::data() const
{
    return m_data;
}

QUrl DocumentWidget::file() const
{
    return m_file;
}

bool DocumentWidget::isLoaded() const
{
    return m_isLoaded;
}

void DocumentWidget::settingsChanged()
{
    for (int i = 0; i < m_tabs->count(); ++i) {
        static_cast<DocumentTabInterface*>(m_tabs->widget(i))->settingsChanged();
    }
}

void DocumentWidget::parserFinished(const QUrl& file, FileData* data)
{
    Q_ASSERT(data->peak());

    qDebug() << "loaded massif file:" << file;
    qDebug() << "description:" << data->description();
    qDebug() << "command:" << data->cmd();
    qDebug() << "time unit:" << data->timeUnit();
    qDebug() << "snapshots:" << data->snapshots().size();
    qDebug() << "peak: snapshot #" << data->peak()->number() << "after" << data->peak()->time() << data->timeUnit();
    qDebug() << "peak cost:" << prettyCost(data->peak()->memHeap()) << "heap"
                             << prettyCost(data->peak()->memHeapExtra()) << "heap extra"
                             << prettyCost(data->peak()->memStacks()) << "stacks";

    m_data = data;
    m_file = file;

    m_tabs->addTab(new ChartTab(m_data, this, this), QIcon::fromTheme(QStringLiteral("office-chart-area-stacked")),
                   i18nc("@title:tab", "Memory Chart"));

#ifdef HAVE_KGRAPHVIEWER
    static KPluginFactory *factory = KPluginFactory::loadFactory(KPluginMetaData(QStringLiteral("kf6/parts/kgraphviewerpart"))).plugin;
    if (factory) {
        KParts::ReadOnlyPart* part = factory->create<KParts::ReadOnlyPart>(this);
        if (part) {
            m_tabs->addTab(new CallGraphTab(m_data, part, this, this), QIcon::fromTheme(QStringLiteral("kgraphviewer")),
                           i18nc("@title:tab", "Callgraph"));
        }
    }
#endif

    m_tabs->addTab(new AllocatorsTab(m_data, this, this), QIcon::fromTheme(QStringLiteral("view-list-text")),
                   i18nc("@title:tab", "Allocators"));

    for (int i = 0; i < m_tabs->count(); ++i) {
        DocumentTabInterface* tab = static_cast<DocumentTabInterface*>(m_tabs->widget(i));
        connect(tab, &DocumentTabInterface::modelItemSelected,
                this, &DocumentWidget::modelItemSelected);
        connect(tab, &DocumentTabInterface::contextMenuRequested,
                this, &DocumentWidget::contextMenuRequested);
    }

    m_tabs->setCurrentIndex(0);
    connect(m_tabs, &QTabWidget::currentChanged,
            this, &DocumentWidget::slotTabChanged);
    slotTabChanged(0);

    m_isLoaded = true;

    // Switch to the display page and notify that everything is setup.
    m_stackedWidget->setCurrentIndex(0);
    emit loadingFinished();
}

void DocumentWidget::addGuiActions(KXMLGUIFactory* factory)
{
    factory->addClient(this);

    // ensure only the current tab's client is in the factory
    // otherwise the actions from the other tabs are visible
    const int current = m_tabs->currentIndex();
    for (int i = 0; i < m_tabs->count(); ++i) {
        if (i != current) {
            factory->removeClient(static_cast<DocumentTabInterface*>(m_tabs->widget(i)));
        }
    }
}

void DocumentWidget::clearGuiActions(KXMLGUIFactory* factory)
{
    factory->removeClient(this);
}

void DocumentWidget::slotTabChanged(int tab)
{
    if (!factory()) {
        return;
    }

    if (m_currentTab) {
        factory()->removeClient(m_currentTab);
    }

    if (tab >= 0 && tab < m_tabs->count()) {
        m_currentTab = static_cast<DocumentTabInterface*>(m_tabs->widget(tab));
        factory()->addClient(m_currentTab);
    }
}

void DocumentWidget::selectModelItem(const ModelItem& item)
{
    for (int i = 0; i < m_tabs->count(); ++i) {
        static_cast<DocumentTabInterface*>(m_tabs->widget(i))->selectModelItem(item);
    }
}

void DocumentWidget::setProgress(int value)
{
    m_loadingProgressBar->setValue(value);
}

void DocumentWidget::setRange(int minimum, int maximum)
{
    m_loadingProgressBar->setRange(minimum, maximum);
}

void DocumentWidget::showError(const QString& title, const QString& error)
{
    if (!m_errorMessage) {
        m_errorMessage = new KMessageWidget(m_stackedWidget);
        m_stackedWidget->addWidget(m_errorMessage);
        m_errorMessage->setWordWrap(true);
        m_errorMessage->setMessageType(KMessageWidget::Error);
        m_errorMessage->setCloseButtonVisible(false);
    }
    m_errorMessage->setText(QString::fromLatin1("<b>%1</b><p style=\"text-align:left\">%2</p>").arg(title).arg(error));
    m_stackedWidget->setCurrentWidget(m_errorMessage);
}

void DocumentWidget::stopParser()
{
    if (!m_parseWorker) {
        return;
    }

    QThread* thread = m_parseWorker->thread();
    m_parseWorker->stop();
    m_parseWorker->deleteLater();
    m_parseWorker = nullptr;
    thread->quit();
    thread->wait();
}

#include "moc_documentwidget.cpp"
