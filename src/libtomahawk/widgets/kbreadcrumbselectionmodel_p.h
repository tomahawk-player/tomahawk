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

#ifndef KBREADCRUMBSPROXYMODEL_P_H
#define KBREADCRUMBSPROXYMODEL_P_H

#include "kbreadcrumbselectionmodel.h"

#include <QtGui/QItemSelectionModel>
#include <QtCore/QAbstractItemModel>
#include <QObject>

class KBreadcrumbSelectionModelPrivate
{
  Q_DECLARE_PUBLIC(KBreadcrumbSelectionModel)
  KBreadcrumbSelectionModel * const q_ptr;
public:
  KBreadcrumbSelectionModelPrivate(KBreadcrumbSelectionModel *breadcrumbSelector, QItemSelectionModel *selectionModel, KBreadcrumbSelectionModel::BreadcrumbTarget direction)
    : q_ptr(breadcrumbSelector),
      m_includeActualSelection(true),
      m_selectionDepth(-1),
      m_showHiddenAscendantData(false),
      m_selectionModel(selectionModel),
      m_direction(direction),
      m_ignoreCurrentChanged(false)
  {

  }

  /**
    Returns a selection containing the breadcrumbs for @p index
  */
  QItemSelection getBreadcrumbSelection(const QModelIndex &index);

  /**
    Returns a selection containing the breadcrumbs for @p selection
  */
  QItemSelection getBreadcrumbSelection(const QItemSelection &selection);

  void sourceSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

  void init();
  void syncBreadcrumbs();

  bool m_includeActualSelection;
  int m_selectionDepth;
  bool m_showHiddenAscendantData;
  QItemSelectionModel *m_selectionModel;
  KBreadcrumbSelectionModel::BreadcrumbTarget m_direction;
  bool m_ignoreCurrentChanged;
};

#endif
