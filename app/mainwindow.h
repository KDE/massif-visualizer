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

#ifndef MASSIF_MAINWINDOW_H
#define MASSIF_MAINWINDOW_H

#include <KXmlGuiWindow>

#include "ui_mainwindow.h"

namespace KDChart {
class Chart;
class HeaderFooter;
}

namespace Massif {

class FileData;

class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = 0, Qt::WindowFlags f = 0);
    virtual ~MainWindow();

    KDChart::Chart* chart();

    void setupActions();

public slots:
    /**
     * Open a dialog to pick a massif output file to display.
     */
    void openFile();

    /**
     * Opens @p file as massif output file and visualize it.
     */
    void openFile(const KUrl& file);

    /**
     * Close currently opened file.
     */
    void closeFile();

private:
    Ui::MainWindow ui;
    KDChart::Chart* m_chart;
    KDChart::HeaderFooter* m_header;
    KDChart::HeaderFooter* m_subheader;
    FileData* m_data;
};

}

#endif // MASSIF_MAINWINDOW_H
