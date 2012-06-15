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

#ifndef PLAYLISTLARGEITEMDELEGATE_H
#define PLAYLISTLARGEITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QTextOption>

#include "DllMacro.h"
#include "Typedefs.h"

namespace Tomahawk {
class PixmapDelegateFader;
}

class TrackModel;
class PlayableItem;
class PlayableProxyModel;
class TrackView;

class DLLEXPORT PlaylistLargeItemDelegate : public QStyledItemDelegate
{
Q_OBJECT

public:
    enum DisplayMode
    { LovedTracks, RecentlyPlayed, LatestAdditions };

    PlaylistLargeItemDelegate( DisplayMode mode, TrackView* parent = 0, PlayableProxyModel* proxy = 0 );

protected:
    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    QWidget* createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

signals:
    void updateIndex( const QModelIndex& idx );

private slots:
    void modelChanged();
    void doUpdateIndex( const QPersistentModelIndex& idx );

private:
    void prepareStyleOption( QStyleOptionViewItemV4* option, const QModelIndex& index, PlayableItem* item ) const;
    void drawRichText( QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, int flags, QTextDocument& text ) const;

    QTextOption m_topOption;
    QTextOption m_centerRightOption;
    QTextOption m_bottomOption;

    mutable QHash< QPersistentModelIndex, QSharedPointer< Tomahawk::PixmapDelegateFader > > m_pixmaps;

    TrackView* m_view;
    PlayableProxyModel* m_model;
    DisplayMode m_mode;
};

#endif // PLAYLISTLARGEITEMDELEGATE_H
