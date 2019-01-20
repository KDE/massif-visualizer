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

#ifndef ALLOCATORSTAB_H
#define ALLOCATORSTAB_H

#include "documenttabinterface.h"

class QTreeView;
class QSortFilterProxyModel;
class QModelIndex;

namespace Massif {
class AllocatorsModel;
}

class AllocatorsTab : public DocumentTabInterface
{
    Q_OBJECT

public:
    explicit AllocatorsTab(const Massif::FileData* data, KXMLGUIClient* guiParent, QWidget* parent = 0);
    ~AllocatorsTab() override;

    void selectModelItem(const Massif::ModelItem& item) override;
    void settingsChanged() override;

private Q_SLOTS:
    void selectionChanged(const QModelIndex& current, const QModelIndex& previous);
    void customContextMenuRequested(const QPoint& pos);

private:
    Massif::AllocatorsModel* m_model;
    QSortFilterProxyModel* m_proxy;
    QTreeView* m_view;
};

#endif // ALLOCATORSTAB_H
