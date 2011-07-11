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

#include "trackmodel.h"

#include "dllmacro.h"

class TrackProxyModel;
class TrackView;

class DLLEXPORT PlaylistItemDelegate : public QStyledItemDelegate
{
Q_OBJECT

public:
    PlaylistItemDelegate( TrackView* parent = 0, TrackProxyModel* proxy = 0 );

    void updateRowSize( const QModelIndex& index );

public slots:
    void setRemovalProgress( unsigned int progress ) { m_removalProgress = progress; }

protected:
    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    QWidget* createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

private:
    void prepareStyleOption( QStyleOptionViewItemV4* option, const QModelIndex& index ) const;

    void paintDetailed( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    void paintShort( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    unsigned int m_removalProgress;
    QPixmap m_nowPlayingIcon;
    QPixmap m_arrowIcon;

    TrackView* m_view;
    TrackProxyModel* m_model;
};

#endif // PLAYLISTITEMDELEGATE_H
