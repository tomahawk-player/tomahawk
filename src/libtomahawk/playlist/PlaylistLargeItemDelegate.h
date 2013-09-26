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

#include "PlaylistItemDelegate.h"
#include "DllMacro.h"
#include "Typedefs.h"

class PlayableItem;
class PlayableProxyModel;
class TrackView;

class DLLEXPORT PlaylistLargeItemDelegate : public PlaylistItemDelegate
{
Q_OBJECT

public:
    enum DisplayMode
    { LovedTracks, RecentlyPlayed, LatestAdditions, Inbox };

    PlaylistLargeItemDelegate( DisplayMode mode, TrackView* parent, PlayableProxyModel* proxy );

    virtual QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

protected:
    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

protected slots:
    virtual void modelChanged();

private:
    TrackView* m_view;
    PlayableProxyModel* m_model;
    DisplayMode m_mode;
};

#endif // PLAYLISTLARGEITEMDELEGATE_H
