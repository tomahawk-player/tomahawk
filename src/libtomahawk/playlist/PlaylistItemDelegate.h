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

#ifndef PLAYLISTITEMDELEGATE_H
#define PLAYLISTITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QTextOption>

#include "DllMacro.h"

class TrackModel;
class PlayableItem;
class PlayableProxyModel;
class TrackView;

class DLLEXPORT PlaylistItemDelegate : public QStyledItemDelegate
{
Q_OBJECT

public:
    PlaylistItemDelegate( TrackView* parent, PlayableProxyModel* proxy );

    void updateRowSize( const QModelIndex& index );

    virtual QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

public slots:
    void resetHoverIndex();

signals:
    void updateIndex( const QModelIndex& idx );

protected:
    void prepareStyleOption( QStyleOptionViewItemV4* option, const QModelIndex& index, PlayableItem* item ) const;

    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    bool editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index );

    QPersistentModelIndex hoveringOver() const { return m_hoveringOver; }
    void setInfoButtonRect( const QPersistentModelIndex& index, const QRect& rect ) const { m_infoButtonRects[ index ] = rect; }

private:
    void paintDetailed( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    void paintShort( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, bool useAvatars = false ) const;

    QTextOption m_topOption;
    QTextOption m_bottomOption;

    mutable QHash< QPersistentModelIndex, QRect > m_infoButtonRects;
    QPersistentModelIndex m_hoveringOver;

    TrackView* m_view;
    PlayableProxyModel* m_model;
};

#endif // PLAYLISTITEMDELEGATE_H
