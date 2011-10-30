/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ALBUMITEMDELEGATE_H
#define ALBUMITEMDELEGATE_H

#include <QStyledItemDelegate>

#include "dllmacro.h"

class QEvent;
class AlbumProxyModel;

class DLLEXPORT AlbumItemDelegate : public QStyledItemDelegate
{
Q_OBJECT

public:
    AlbumItemDelegate( QAbstractItemView* parent = 0, AlbumProxyModel* proxy = 0 );

protected:
    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    bool editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index );
//    QWidget* createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

signals:
    void updateIndex( const QModelIndex& idx );

private:
    QAbstractItemView* m_view;
    AlbumProxyModel* m_model;

    mutable QHash< qint64, QPixmap > m_cache;
    mutable QHash< QPersistentModelIndex, QRect > m_artistNameRects;
    QPersistentModelIndex m_hoveringOver;

    QPixmap m_shadowPixmap;
    QPixmap m_defaultCover;
};

#endif // ALBUMITEMDELEGATE_H
