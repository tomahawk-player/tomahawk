/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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
#include <QTextDocument>

#include "DllMacro.h"
#include "Typedefs.h"

class TrackModel;
class PlayableItem;
class PlayableProxyModel;
class TrackView;

class QComboBox;

namespace Tomahawk {
    class PixmapDelegateFader;
}

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

private slots:
    void doUpdateIndex( const QPersistentModelIndex& index );
    void closeEditor( const QModelIndex& index, QWidget* editor );
    void addDownloadJob( const QModelIndex& index, QWidget* editor );

protected:
    void prepareStyleOption( QStyleOptionViewItemV4* option, const QModelIndex& index, PlayableItem* item ) const;

    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    QWidget* createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    bool editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index );
    void updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    void setModelData( QWidget* editor, QAbstractItemModel* model, const QModelIndex& index ) const;

    QPersistentModelIndex hoveringOver() const { return m_hoveringOver; }

    QRect drawInfoButton( QPainter* painter, const QRect& rect, const QModelIndex& index, float height ) const;
    QRect drawSourceIcon( QPainter* painter, const QRect& rect, PlayableItem* item, float height ) const;
    QRect drawCover( QPainter* painter, const QRect& rect, PlayableItem* item, const QModelIndex& index ) const;
    QRect drawLoveBox( QPainter* painter,
                       const QRect& rect,
                       PlayableItem* item,
                       const QModelIndex& index ) const;
    QRect drawGenericBox( QPainter* painter,
                          const QStyleOptionViewItem& option,
                          const QRect& rect,
                          const QString& text,
                          const QList< Tomahawk::source_ptr >& sources,
                          const QModelIndex& index ) const;
    void drawRectForBox( QPainter* painter, const QRect& rect ) const;
    void drawAvatarsForBox( QPainter* painter,
                            const QRect& avatarsRect,
                            int avatarSize,
                            int avatarMargin,
                            int count,
                            const QList< Tomahawk::source_ptr >& sources,
                            const QModelIndex& index ) const;
    void drawRichText( QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, int flags, QTextDocument& text ) const;

    QRect drawSource( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, const QRect& rect, PlayableItem* item ) const;
    QRect drawTrack( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, const QRect& rect, PlayableItem* item ) const;

    void paintDetailed( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    QTextOption m_topOption;
    QTextOption m_bottomOption;
    QTextOption m_centerOption;
    QTextOption m_centerRightOption;

    QFont m_demiBoldFont;
    QFont m_normalFont;

protected slots:
    virtual void modelChanged();
    virtual void onAudioEngineTick( qint64 ms );
    virtual void onPlaybackChange();

private:
    mutable QHash< QPersistentModelIndex, QSharedPointer< Tomahawk::PixmapDelegateFader > > m_pixmaps;
    mutable QHash< QPersistentModelIndex, QRect > m_infoButtonRects;
    mutable QHash< QPersistentModelIndex, QRect > m_loveButtonRects;
    mutable QHash< QPersistentModelIndex, QRect > m_buyButtonRects;
    mutable QHash< QPersistentModelIndex, QRect > m_downloadDropDownRects;
    mutable QHash< QPersistentModelIndex, QRect > m_artistNameRects;
    mutable QHash< QPersistentModelIndex, QHash< Tomahawk::source_ptr, QRect > > m_avatarBoxRects;
    QPersistentModelIndex m_hoveringOver;
    QPersistentModelIndex m_hoveringOverArtist;
    QPersistentModelIndex m_hoveringOverBuyButton;
    mutable QPersistentModelIndex m_nowPlaying;

    TrackView* m_view;
    PlayableProxyModel* m_model;
};

#endif // PLAYLISTITEMDELEGATE_H
