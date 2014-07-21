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

#ifndef DOCUMENTWIDGET_H
#define DOCUMENTWIDGET_H

#include <QWidget>

#include <KUrl>
#include <KXMLGUIClient>

#include "visualizer/modelitem.h"

class QMenu;
class DocumentTabInterface;
class QLabel;
class QProgressBar;
class QToolButton;
class QStackedWidget;
class QTabWidget;

class KMessageWidget;

namespace Massif {
class FileData;
}

class DocumentWidget : public QWidget, public KXMLGUIClient
{
    Q_OBJECT

public:
    explicit DocumentWidget(KXMLGUIClient* guiParent, QWidget* parent = 0);
    ~DocumentWidget();

    Massif::FileData* data() const;
    KUrl file() const;

    bool isLoaded() const;

    void settingsChanged();

    void addGuiActions(KXMLGUIFactory* factory);
    void clearGuiActions(KXMLGUIFactory* factory);

    void selectModelItem(const Massif::ModelItem& item);

signals:
    void stopParser();
    void loadingFinished();
    void modelItemSelected(const Massif::ModelItem& item);
    void contextMenuRequested(const Massif::ModelItem& item, QMenu* menu);

public slots:
    void parserFinished(const KUrl& file, Massif::FileData* data);

    void setProgress(int value);
    void setRange(int minimum, int maximum);
    void setLoadingMessage(const QString& message);

    void showError(const QString& title, const QString& error);

private slots:
    void slotTabChanged(int tab);

private:
    Massif::FileData* m_data;
    KUrl m_file;

    DocumentTabInterface* m_currentTab;

    QStackedWidget* m_stackedWidget;
    QTabWidget* m_tabs;
    KMessageWidget* m_errorMessage;
    QLabel* m_loadingMessage;
    QProgressBar* m_loadingProgressBar;
    QToolButton* m_stopParserButton;
    bool m_isLoaded;
};

#endif // DOCUMENTWIDGET_H
