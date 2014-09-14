/*
   This file is part of Massif Visualizer

   Copyright 2014 Milian Wolff <mail@milianw.de>

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

#ifndef DOCUMENTTABINTERFACE_H
#define DOCUMENTTABINTERFACE_H

#include <QWidget>

#include <KXMLGUIClient>

#include "visualizer/modelitem.h"

class QMenu;

namespace Massif {
class FileData;
}

class DocumentTabInterface : public QWidget, public KXMLGUIClient
{
    Q_OBJECT
public:
    explicit DocumentTabInterface(const Massif::FileData* data,
                                  KXMLGUIClient* guiParent, QWidget* parent = 0);
    virtual ~DocumentTabInterface();

    virtual void settingsChanged() = 0;

public Q_SLOTS:
    virtual void selectModelItem(const Massif::ModelItem& item) = 0;

Q_SIGNALS:
    void modelItemSelected(const Massif::ModelItem& item);
    void contextMenuRequested(const Massif::ModelItem& item, QMenu* menu);

protected:
    const Massif::FileData* const m_data;
};

#endif // DOCUMENTTABINTERFACE_H
