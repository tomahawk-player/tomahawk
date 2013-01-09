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

#include <QStyledItemDelegate>
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

class DLLEXPORT PlaylistChartItemDelegate : public QStyledItemDelegate
{
Q_OBJECT

public:
    PlaylistChartItemDelegate( TrackView* parent = 0, PlayableProxyModel* proxy = 0 );

signals:
    void updateIndex( const QModelIndex& idx );

protected:
    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    QWidget* createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

private slots:
    void modelChanged();
    void doUpdateIndex( const QPersistentModelIndex& idx );

private:
    void prepareStyleOption( QStyleOptionViewItemV4* option, const QModelIndex& index, PlayableItem* item ) const;

    QTextOption m_topOption;
    QTextOption m_centerOption;
    QTextOption m_centerRightOption;
    QTextOption m_bottomOption;

    TrackView* m_view;
    PlayableProxyModel* m_model;

    mutable QHash< QPersistentModelIndex, QSharedPointer< Tomahawk::PixmapDelegateFader > > m_pixmaps;
};

#endif // PLAYLISTCHARTITEMDELEGATE_H
