/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011-2012, Leo Franchi            <lfranchi@kde.org>
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

#ifndef GRIDITEMDELEGATE_H
#define GRIDITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QTimeLine>

#include "DllMacro.h"

namespace Tomahawk {
    class PixmapDelegateFader;
}

namespace _detail {
    class Closure;
}

class QEvent;
class QTimeLine;
class PlayableProxyModel;
class ImageButton;
class HoverControls;

class DLLEXPORT GridItemDelegate : public QStyledItemDelegate
{
Q_OBJECT

public:
    GridItemDelegate( QAbstractItemView* parent, PlayableProxyModel* proxy );

    QSize itemSize() const { return m_itemSize; }
    void setItemSize( const QSize& size ) { m_itemSize = size; }

public slots:
    void resetHoverIndex();

protected:
    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    bool editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index );
    bool eventFilter( QObject* obj, QEvent* event );

signals:
    void updateIndex( const QModelIndex& idx );

    void startedPlaying( const QPersistentModelIndex& );
    void stoppedPlaying( const QPersistentModelIndex& );

private slots:
    void modelChanged();
    void doUpdateIndex( const QPersistentModelIndex& idx );
    void onCurrentIndexChanged();

    void onViewChanged();
    void onPlaybackStarted( const QPersistentModelIndex& index );

    void onPlaybackFinished();

    void onPlayClicked( const QPersistentModelIndex& index );

    void fadingFrameChanged( const QPersistentModelIndex& );
    void fadingFrameFinished( const QPersistentModelIndex& );

private:
    QTimeLine* createTimeline( QTimeLine::Direction direction, int startFrame = 0 );
    void clearButtons();

    QAbstractItemView* m_view;
    PlayableProxyModel* m_model;
    QSize m_itemSize;

    mutable QHash< QPersistentModelIndex, QRect > m_artistNameRects;
    mutable QHash< QPersistentModelIndex, QRect > m_albumNameRects;
    mutable QHash< QPersistentModelIndex, QSharedPointer< Tomahawk::PixmapDelegateFader > > m_covers;

    QPersistentModelIndex m_hoverIndex;
    QPersistentModelIndex m_hoveringOverArtist;
    QPersistentModelIndex m_hoveringOverAlbum;

    QPixmap m_shadowPixmap;
    mutable QHash< QPersistentModelIndex, QWidget* > m_spinner;
    mutable QHash< QPersistentModelIndex, HoverControls* > m_hoverControls;
    mutable QHash< QPersistentModelIndex, QTimeLine* > m_hoverFaders;
};

#endif // GRIDITEMDELEGATE_H
