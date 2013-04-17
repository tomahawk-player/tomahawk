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

#ifndef PLAYLISTCHARTITEMDELEGATE_H
#define PLAYLISTCHARTITEMDELEGATE_H

#include <QTextOption>

#include "PlaylistItemDelegate.h"
#include "DllMacro.h"
#include "Typedefs.h"


namespace Tomahawk {
class PixmapDelegateFader;
}

class TrackModel;
class PlayableItem;
class PlayableProxyModel;
class TrackView;

class DLLEXPORT PlaylistChartItemDelegate : public PlaylistItemDelegate
{
Q_OBJECT

public:
    PlaylistChartItemDelegate( TrackView* parent = 0, PlayableProxyModel* proxy = 0 );

    virtual QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

protected:
    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

private slots:
    void doUpdateIndex( const QPersistentModelIndex& idx );
    void modelChanged();

private:
    QTextOption m_topOption;
    QTextOption m_centerOption;
    QTextOption m_centerRightOption;
    QTextOption m_bottomOption;

    mutable QHash< QPersistentModelIndex, QSharedPointer< Tomahawk::PixmapDelegateFader > > m_pixmaps;

    TrackView* m_view;
    PlayableProxyModel* m_model;
};

#endif // PLAYLISTCHARTITEMDELEGATE_H
