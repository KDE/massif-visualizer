/***************************************************************************
 *   This file is part of KDevelop                                         *
 *   Copyright 2011 Milian Wolff <mail@milianw.de>                         *
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


#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <KConfigDialog>

namespace Ui {
class Config;
}

namespace Massif {

/**
 * Config dialog, gets deleted when closed.
 */
class ConfigDialog : public KConfigDialog
{
    Q_OBJECT

public:
    ConfigDialog(QWidget* parent);
    virtual ~ConfigDialog();

    static bool isShown();

private:
    Ui::Config* m_ui;
};

}

#endif // CONFIGDIALOG_H
