/***************************************************************************
 *   This file is part of KDevelop                                         *
 *   Copyright 2010 Milian Wolff <mail@milianw.de>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/


#include "configdialog.h"

#include "massif-visualizer-settings.h"
#include "ui_config.h"

using namespace Massif;

ConfigDialog::ConfigDialog(QWidget* parent)
: KConfigDialog(parent, "settings", Settings::self())
, m_ui(new Ui::Config)
{
    setAttribute(Qt::WA_DeleteOnClose, true);

    QWidget* settingsPage = new QWidget(this);
    m_ui->setupUi(settingsPage);

    addPage(settingsPage, Settings::self(), i18n("Settings"));
    setFaceType(KPageDialog::Plain);
}

ConfigDialog::~ConfigDialog()
{
    delete m_ui;
}

bool ConfigDialog::isShown()
{
    return KConfigDialog::showDialog("settings");
}

#include "configdialog.moc"
