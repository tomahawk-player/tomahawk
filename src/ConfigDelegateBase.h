/*
    Copyright (C) 2011  Leo Franchi <leo.franchi@kdab.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef CONFIGDELEGATEBASE_H
#define CONFIGDELEGATEBASE_H

#include "DllMacro.h"

#include <QStyledItemDelegate>

#define PADDING 4

class QPainter;
class Q_DECL_EXPORT ConfigDelegateBase : public QStyledItemDelegate
{
    Q_OBJECT
public:
  ConfigDelegateBase( QObject* parent = 0 );
  virtual QSize sizeHint ( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

  virtual bool editorEvent ( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index );

  // if you want to use a checkbox, you need to have this say where to paint it
  virtual QRect checkRectForIndex( const QStyleOptionViewItem& option, const QModelIndex& idx, int role ) const = 0;
  // if you want to use a config wrench, you need to have this say where to paint it
  virtual QRect configRectForIndex( const QStyleOptionViewItem& option, const QModelIndex& idx ) const = 0;

  virtual QList<int> extraCheckRoles() const;
signals:
    void configPressed( const QModelIndex& idx );

protected:

private:
    QModelIndex m_configPressed;
};

#endif // CONFIGDELEGATEBASE_H
