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

#include "allocatorstab.h"

#include "visualizer/allocatorsmodel.h"

#include <QVBoxLayout>
#include <QTreeView>
#include <QSortFilterProxyModel>
#include <QMenu>

using namespace Massif;

AllocatorsTab::AllocatorsTab(const FileData* data,
                             KXMLGUIClient* guiParent, QWidget* parent)
    : DocumentTabInterface(data, guiParent, parent)
    , m_model(new AllocatorsModel(data, this))
    , m_proxy(new QSortFilterProxyModel(this))
    , m_view(new QTreeView(this))
{
    m_proxy->setSourceModel(m_model);
    m_proxy->setSortRole(AllocatorsModel::SortRole);

    m_view->setRootIsDecorated(false);
    m_view->setModel(m_proxy);
    m_view->setSortingEnabled(true);
    m_view->sortByColumn(AllocatorsModel::Peak, Qt::DescendingOrder);
    m_view->resizeColumnToContents(AllocatorsModel::Function);
    m_view->resizeColumnToContents(AllocatorsModel::Peak);
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_view->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &AllocatorsTab::selectionChanged);
    connect(m_view, &QTreeView::customContextMenuRequested,
            this, &AllocatorsTab::customContextMenuRequested);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->addWidget(m_view);
}

AllocatorsTab::~AllocatorsTab()
{

}

void AllocatorsTab::selectModelItem(const ModelItem& item)
{
    const QModelIndex idx = m_model->indexForItem(item);
    if (!idx.isValid()) {
        return;
    }

    m_view->selectionModel()->clearSelection();
    m_view->selectionModel()->setCurrentIndex(m_proxy->mapFromSource(idx),
                                              QItemSelectionModel::Select | QItemSelectionModel::Rows);
    m_view->scrollTo(m_view->selectionModel()->currentIndex());
}

void AllocatorsTab::settingsChanged()
{
    m_model->settingsChanged();
}

void AllocatorsTab::selectionChanged(const QModelIndex& current, const QModelIndex& /*previous*/)
{
    const ModelItem item = current.data(AllocatorsModel::ItemRole).value<ModelItem>();
    if (item.first) {
        emit modelItemSelected(item);
    }
}

void AllocatorsTab::customContextMenuRequested(const QPoint& pos)
{
    const QModelIndex idx = m_view->indexAt(pos);
    const ModelItem item = idx.data(AllocatorsModel::ItemRole).value<ModelItem>();

    QMenu menu;
    emit contextMenuRequested(item, &menu);
    if (!menu.actions().isEmpty()) {
        menu.exec(m_view->mapToGlobal(pos));
    }
}

#include "moc_allocatorstab.cpp"
