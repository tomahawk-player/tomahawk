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

    QSize itemSize() const;
    void setItemWidth( int width ) { m_itemWidth = width; }
    void setShowBuyButtons( bool enabled ) { m_showBuyButtons = enabled; }

public slots:
    void resetHoverIndex();
    void setShowPosition( bool enabled );
    void setWordWrapping( bool enabled );

protected:
    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    bool editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index );
    bool eventFilter( QObject* obj, QEvent* event );

    QWidget* createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    void updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    void setModelData( QWidget* editor, QAbstractItemModel* model, const QModelIndex& index ) const;

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

    void closeEditor( const QModelIndex& index, QWidget* editor );
    void addDownloadJob( const QModelIndex& index, QWidget* editor );

private:
    QTimeLine* createTimeline( QTimeLine::Direction direction, int startFrame = 0 );
    void clearButtons();

    QAbstractItemView* m_view;
    PlayableProxyModel* m_model;
    int m_itemWidth;
    bool m_showPosition;
    bool m_showBuyButtons;
    bool m_wordWrapping;

    mutable QHash< QPersistentModelIndex, QRect > m_artistNameRects;
    mutable QHash< QPersistentModelIndex, QRect > m_albumNameRects;
    mutable QHash< QPersistentModelIndex, QRect > m_buyButtonRects;
    mutable QHash< QPersistentModelIndex, QRect > m_downloadDropDownRects;
    mutable QHash< QPersistentModelIndex, QSharedPointer< Tomahawk::PixmapDelegateFader > > m_covers;

    QPersistentModelIndex m_hoverIndex;
    QPersistentModelIndex m_hoveringOverArtist;
    QPersistentModelIndex m_hoveringOverAlbum;
    QPersistentModelIndex m_hoveringOverBuyButton;

    mutable QHash< QPersistentModelIndex, QWidget* > m_spinner;
    mutable QHash< QPersistentModelIndex, HoverControls* > m_hoverControls;
    mutable QHash< QPersistentModelIndex, QTimeLine* > m_hoverFaders;

    QFont m_font;
    QFont m_smallFont;
    const int m_margin;
};

#endif // GRIDITEMDELEGATE_H
