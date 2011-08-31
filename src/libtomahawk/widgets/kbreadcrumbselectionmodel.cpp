/*
    Copyright (C) 2010 KlarÃ¤lvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Stephen Kelly <stephen@kdab.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/


#include "kbreadcrumbselectionmodel.h"
#include "kbreadcrumbselectionmodel_p.h"

#include <QDebug>

KBreadcrumbSelectionModel::KBreadcrumbSelectionModel(QItemSelectionModel *selectionModel, QObject* parent)
  : QItemSelectionModel(const_cast<QAbstractItemModel *>(selectionModel->model()), parent),
    d_ptr(new KBreadcrumbSelectionModelPrivate(this, selectionModel, MakeBreadcrumbSelectionInSelf))
{
  d_ptr->init();
}

KBreadcrumbSelectionModel::KBreadcrumbSelectionModel(QItemSelectionModel *selectionModel, BreadcrumbTarget direction, QObject* parent)
  : QItemSelectionModel(const_cast<QAbstractItemModel *>(selectionModel->model()), parent),
    d_ptr(new KBreadcrumbSelectionModelPrivate(this, selectionModel, direction))
{
  if ( direction != MakeBreadcrumbSelectionInSelf)
    connect(selectionModel, SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            this, SLOT(sourceSelectionChanged(const QItemSelection&,const QItemSelection&)));

  d_ptr->init();
}

KBreadcrumbSelectionModel::~KBreadcrumbSelectionModel()
{
  delete d_ptr;
}

bool KBreadcrumbSelectionModel::isActualSelectionIncluded() const
{
  Q_D(const KBreadcrumbSelectionModel);
  return d->m_includeActualSelection;
}

void KBreadcrumbSelectionModel::setActualSelectionIncluded(bool includeActualSelection)
{
  Q_D(KBreadcrumbSelectionModel);
  d->m_includeActualSelection = includeActualSelection;
}

int KBreadcrumbSelectionModel::breadcrumbLength() const
{
  Q_D(const KBreadcrumbSelectionModel);
  return d->m_selectionDepth;
}

void KBreadcrumbSelectionModel::setBreadcrumbLength(int breadcrumbLength)
{
  Q_D(KBreadcrumbSelectionModel);
  d->m_selectionDepth = breadcrumbLength;
}

QItemSelection KBreadcrumbSelectionModelPrivate::getBreadcrumbSelection(const QModelIndex& index)
{
  QItemSelection breadcrumbSelection;

  if (m_includeActualSelection)
    breadcrumbSelection.append(QItemSelectionRange(index));

  QModelIndex parent = index.parent();
  int sumBreadcrumbs = 0;
  bool includeAll = m_selectionDepth < 0;
  while (parent.isValid() && (includeAll || sumBreadcrumbs < m_selectionDepth)) {
    breadcrumbSelection.append(QItemSelectionRange(parent));
    parent = parent.parent();
  }
  return breadcrumbSelection;
}

QItemSelection KBreadcrumbSelectionModelPrivate::getBreadcrumbSelection(const QItemSelection& selection)
{
  QItemSelection breadcrumbSelection;

  if (m_includeActualSelection)
    breadcrumbSelection = selection;

  QItemSelection::const_iterator it = selection.constBegin();
  const QItemSelection::const_iterator end = selection.constEnd();

  for ( ; it != end; ++it)
  {
    QModelIndex parent = it->parent();

    if (breadcrumbSelection.contains(parent))
      continue;

    int sumBreadcrumbs = 0;
    bool includeAll = m_selectionDepth < 0;

    while (parent.isValid() && (includeAll || sumBreadcrumbs < m_selectionDepth))
    {
      breadcrumbSelection.append(QItemSelectionRange(parent));
      parent = parent.parent();

      if (breadcrumbSelection.contains(parent))
        break;

      ++sumBreadcrumbs;
    }
  }
  return breadcrumbSelection;
}

void KBreadcrumbSelectionModelPrivate::sourceSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  Q_Q(KBreadcrumbSelectionModel);
  QItemSelection deselectedCrumbs = getBreadcrumbSelection(deselected);
  QItemSelection selectedCrumbs = getBreadcrumbSelection(selected);

  QItemSelection removed = deselectedCrumbs;
  foreach(const QItemSelectionRange &range, selectedCrumbs)
  {
    removed.removeAll(range);
  }

  QItemSelection added = selectedCrumbs;
  foreach(const QItemSelectionRange &range, deselectedCrumbs)
  {
    added.removeAll(range);
  }

  if (!removed.isEmpty())
  {
    q->QItemSelectionModel::select(removed, QItemSelectionModel::Deselect);
  }
  if (!added.isEmpty())
  {
    q->QItemSelectionModel::select(added, QItemSelectionModel::Select);
  }
}

void KBreadcrumbSelectionModel::select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command)
{
  Q_D(KBreadcrumbSelectionModel);
  // When an item is removed, the current index is set to the top index in the model.
  // That causes a selectionChanged signal with a selection which we do not want.
  if ( d->m_ignoreCurrentChanged )
  {
    d->m_ignoreCurrentChanged = false;
    return;
  }
  if ( d->m_direction == MakeBreadcrumbSelectionInOther )
  {
    d->m_selectionModel->select(d->getBreadcrumbSelection(index), command);
    QItemSelectionModel::select(index, command);
  } else {
    d->m_selectionModel->select(index, command);
    QItemSelectionModel::select(d->getBreadcrumbSelection(index), command);
  }
}

void KBreadcrumbSelectionModel::select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command)
{
  Q_D(KBreadcrumbSelectionModel);
  QItemSelection bcc = d->getBreadcrumbSelection(selection);
  if ( d->m_direction == MakeBreadcrumbSelectionInOther )
  {
    d->m_selectionModel->select(selection, command);
    QItemSelectionModel::select(bcc, command);
  } else {
    d->m_selectionModel->select(bcc, command);
    QItemSelectionModel::select(selection, command);
  }
}

void KBreadcrumbSelectionModelPrivate::init()
{
  Q_Q(KBreadcrumbSelectionModel);
  q->connect(m_selectionModel->model(), SIGNAL(layoutChanged()), SLOT(syncBreadcrumbs()));
  q->connect(m_selectionModel->model(), SIGNAL(modelReset()), SLOT(syncBreadcrumbs()));
  q->connect(m_selectionModel->model(), SIGNAL(rowsMoved(QModelIndex, int, int, QModelIndex, int)), SLOT(syncBreadcrumbs()));
  // Don't need to handle insert & remove because they can't change the breadcrumbs on their own.
}

void KBreadcrumbSelectionModelPrivate::syncBreadcrumbs()
{
  Q_Q(KBreadcrumbSelectionModel);
  q->select(m_selectionModel->selection(), QItemSelectionModel::ClearAndSelect);
}

